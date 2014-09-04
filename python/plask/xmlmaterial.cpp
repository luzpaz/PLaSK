#include "python_globals.h"

#include <plask/utils/string.h>
#include <plask/utils/xml/reader.h>
#include <plask/material/db.h>

#include "python_material.h"
#include "python_ptr.h"

namespace plask { namespace python {

/**
 * Wrapper for Material read from XML of type eval
 * For all virtual functions it calls Python derivatives
 */
class PythonEvalMaterial;

extern py::dict xml_globals;

struct PythonEvalMaterialConstructor: public MaterialsDB::MaterialConstructor {

    shared_ptr<Material> base_obj;  // base object (nullptr if base_ctr is not nullptr)
    shared_ptr<const MaterialsDB::MaterialConstructor> base_ctr;  // base constructor (nullptr if base_obj is not nullptr)
    Material::Composition baseMaterialComposition;  // composition passed to base_ctr, empty if base_ctr is nullptr, can be not completed

    weak_ptr<PythonEvalMaterialConstructor> self;
    bool base_has_defined_dopant;

    MaterialCache cache;

    std::string base;
    Material::Kind kind;
    Material::ConductivityType condtype;

    PyHandle<PyCodeObject>
        lattC, Eg, CB, VB, Dso, Mso, Me, Mhh, Mlh, Mh, ac, av, b, d, c11, c12, c44, eps, chi,
        Nc, Nv, Ni, Nf, EactD, EactA, mob, cond, A, B, C, D,
        thermk, dens, cp, nr, absp, Nr, NR;

    PythonEvalMaterialConstructor(MaterialsDB& db, const std::string& name, const std::string& base) :
        MaterialsDB::MaterialConstructor(name), base_has_defined_dopant(base.find("=") != std::string::npos),
        kind(Material::NONE), condtype(Material::CONDUCTIVITY_UNDETERMINED)
    {
        // fill base_obj or base_ctr:
        if (base != "") {
            if (base.find("=") != std::string::npos)    //base material has defined dopant
                base_obj = db.get(base);
            else {  //doping amount is not given
                if (base.find(":") != std::string::npos) {  //but dopant type is given
                    if (base.find("(") != std::string::npos) {  //material is complex
                        std::string name, dopant_name;
                        std::tie(name, dopant_name) = splitString2(base, ':');
                        baseMaterialComposition = Material::parseComposition(name);
                        base_ctr = db.getConstructor(baseMaterialComposition, dopant_name);
                    } else {    //material is simple
                        base_ctr = db.getConstructor(base);
                    }
                } else  //dopant type is not given
                    base_obj = db.get(base);
            }
        } else {
            base_obj = make_shared<EmptyMaterial>();
        }
    }

    inline shared_ptr<Material> operator()(const Material::Composition& composition, Material::DopingAmountType doping_amount_type, double doping_amount) const;

    bool isSimple() const override { return true; }
};

class PythonEvalMaterial : public Material
{
    shared_ptr<PythonEvalMaterialConstructor> cls;
    shared_ptr<Material> base;

    Material::DopingAmountType doping_amount_type;
    double doping_amount;

    py::object self;

    static inline PyObject* py_eval(PyCodeObject *fun, const py::dict& locals) {
        return
#if PY_VERSION_HEX >= 0x03000000
            PyEval_EvalCode((PyObject*)fun, xml_globals.ptr(), locals.ptr());
#else
            PyEval_EvalCode(fun, xml_globals.ptr(), locals.ptr());
#endif
    }

    template <typename RETURN>
    inline RETURN call(PyCodeObject *fun, const py::dict& locals, const char* funname) const {
        try {
            return py::extract<RETURN>(py::handle<>(py_eval(fun, locals)).get());
        } catch (py::error_already_set) {
            PyObject *ptype, *pvalue, *ptraceback;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            Py_XDECREF(ptraceback);
            std::string type, value;
            if (ptype) { type = py::extract<std::string>(py::object(py::handle<>(ptype)).attr("__name__")); type = ": " + type; }
            if (pvalue) { value = py::extract<std::string>(py::str(py::handle<>(pvalue))); value = ": " + value; }
            throw ValueError("Error in the custom material function <%2%> of '%1%'%3%%4%", this->name(), funname, type, value);
        }
    }

  public:

    PythonEvalMaterial(const shared_ptr<PythonEvalMaterialConstructor>& constructor, const shared_ptr<Material>& base,
                       const Material::Composition& composition, Material::DopingAmountType doping_amount_type, double doping_amount) :
        cls(constructor), base(base), doping_amount_type(doping_amount_type), doping_amount(doping_amount) {
        self = py::object(base);
        if (doping_amount_type == Material::DOPANT_CONCENTRATION) self.attr("dc") = doping_amount;
        else if (doping_amount_type == Material::CARRIER_CONCENTRATION) self.attr("cc") = doping_amount;
    }

    // Here there are overridden methods from Material class

    OmpLockGuard<OmpNestLock> lock() const override {
        return OmpLockGuard<OmpNestLock>(python_omp_lock);
    }

    virtual bool isEqual(const Material& other) const override {
        auto theother = static_cast<const PythonEvalMaterial&>(other);
        return
            cls == theother.cls &&
            doping_amount_type == theother.doping_amount_type &&
            doping_amount == theother.doping_amount;
    }

    virtual std::string name() const override { return cls->materialName; }
    virtual Material::Kind kind() const override { return (cls->kind == Material::NONE)? base->kind() : cls->kind; }
    virtual Material::ConductivityType condtype() const override { return (cls->condtype == Material::CONDUCTIVITY_UNDETERMINED)? base->condtype() : cls->condtype; }

#   define PYTHON_EVAL_CALL_1(rtype, fun, arg1) \
        if (cls->cache.fun) return *cls->cache.fun;\
        if (cls->fun == NULL) return base->fun(arg1); \
        OmpLockGuard<OmpNestLock> lock(python_omp_lock); \
        py::dict locals; locals["self"] = self; locals[BOOST_PP_STRINGIZE(arg1)] = arg1; \
        return call<rtype>(cls->fun, locals, BOOST_PP_STRINGIZE(fun));

#   define PYTHON_EVAL_CALL_2(rtype, fun, arg1, arg2) \
        if (cls->cache.fun) return *cls->cache.fun;\
        if (cls->fun == NULL) return base->fun(arg1, arg2); \
        OmpLockGuard<OmpNestLock> lock(python_omp_lock); \
        py::dict locals; locals["self"] = self; locals[BOOST_PP_STRINGIZE(arg1)] = arg1; locals[BOOST_PP_STRINGIZE(arg2)] = arg2; \
        return call<rtype>(cls->fun, locals, BOOST_PP_STRINGIZE(fun));

#   define PYTHON_EVAL_CALL_3(rtype, fun, arg1, arg2, arg3) \
        if (cls->cache.fun) return *cls->cache.fun; \
        if (cls->fun == NULL) return base->fun(arg1, arg2, arg3); \
        OmpLockGuard<OmpNestLock> lock(python_omp_lock); \
        py::dict locals; locals["self"] = self; locals[BOOST_PP_STRINGIZE(arg1)] = arg1; locals[BOOST_PP_STRINGIZE(arg2)] = arg2; \
        locals[BOOST_PP_STRINGIZE(arg3)] = arg3; \
        return call<rtype>(cls->fun, locals, BOOST_PP_STRINGIZE(fun));

#   define PYTHON_EVAL_CALL_4(rtype, fun, arg1, arg2, arg3, arg4) \
        if (cls->cache.fun) return *cls->cache.fun;\
        if (cls->fun == NULL) return base->fun(arg1, arg2, arg3, arg4); \
        OmpLockGuard<OmpNestLock> lock(python_omp_lock); \
        py::dict locals; locals["self"] = self; locals[BOOST_PP_STRINGIZE(arg1)] = arg1; locals[BOOST_PP_STRINGIZE(arg2)] = arg2; \
        locals[BOOST_PP_STRINGIZE(arg3)] = arg3; locals[BOOST_PP_STRINGIZE(arg4)] = arg4; \
        return call<rtype>(cls->fun, locals, BOOST_PP_STRINGIZE(fun));

    virtual double lattC(double T, char x) const override { PYTHON_EVAL_CALL_2(double, lattC, T, x) }
    virtual double Eg(double T, double e, char point) const override { PYTHON_EVAL_CALL_3(double, Eg, T, e, point) }
    virtual double CB(double T, double e, char point) const override {
        try { PYTHON_EVAL_CALL_3(double, CB, T, e, point) }
        catch (NotImplemented) { return VB(T, e, point, 'H') + Eg(T, e, point); }
    }
    virtual double VB(double T, double e, char point, char hole) const override { PYTHON_EVAL_CALL_4(double, VB, T, e, point, hole) }
    virtual double Dso(double T, double e) const override { PYTHON_EVAL_CALL_2(double, Dso, T, e) }
    virtual double Mso(double T, double e) const override { PYTHON_EVAL_CALL_2(double, Mso, T, e) }
    virtual Tensor2<double> Me(double T, double e, char point) const override { PYTHON_EVAL_CALL_3(Tensor2<double>, Me, T, e, point) }
    virtual Tensor2<double> Mhh(double T, double e) const override { PYTHON_EVAL_CALL_2(Tensor2<double>, Mhh, T, e) }
    virtual Tensor2<double> Mlh(double T, double e) const override { PYTHON_EVAL_CALL_2(Tensor2<double>, Mlh, T, e) }
    virtual Tensor2<double> Mh(double T, double e) const override { PYTHON_EVAL_CALL_2(Tensor2<double>, Mh, T, e) }
    virtual double ac(double T) const override { PYTHON_EVAL_CALL_1(double, ac, T) }
    virtual double av(double T) const override { PYTHON_EVAL_CALL_1(double, av, T) }
    virtual double b(double T) const override { PYTHON_EVAL_CALL_1(double, b, T) }
    virtual double d(double T) const override { PYTHON_EVAL_CALL_1(double, d, T) }
    virtual double c11(double T) const override { PYTHON_EVAL_CALL_1(double, c11, T) }
    virtual double c12(double T) const override { PYTHON_EVAL_CALL_1(double, c12, T) }
    virtual double c44(double T) const override { PYTHON_EVAL_CALL_1(double, c44, T) }
    virtual double eps(double T) const override { PYTHON_EVAL_CALL_1(double, eps, T) }
    virtual double chi(double T, double e, char point) const override { PYTHON_EVAL_CALL_3(double, chi, T, e, point) }
    virtual double Nc(double T, double e, char point) const override { PYTHON_EVAL_CALL_3(double, Nc, T, e, point) }
    virtual double Nv(double T, double e, char point) const override { PYTHON_EVAL_CALL_3(double, Nv, T, e, point) }
    virtual double Ni(double T) const override { PYTHON_EVAL_CALL_1(double, Ni, T) }
    virtual double Nf(double T) const override { PYTHON_EVAL_CALL_1(double, Nf, T) }
    virtual double EactD(double T) const override { PYTHON_EVAL_CALL_1(double, EactD, T) }
    virtual double EactA(double T) const override { PYTHON_EVAL_CALL_1(double, EactA, T) }
    virtual Tensor2<double> mob(double T) const override { PYTHON_EVAL_CALL_1(Tensor2<double>, mob, T) }
    virtual Tensor2<double> cond(double T) const override { PYTHON_EVAL_CALL_1(Tensor2<double>, cond, T) }
    virtual double A(double T) const override { PYTHON_EVAL_CALL_1(double, A, T) }
    virtual double B(double T) const override { PYTHON_EVAL_CALL_1(double, B, T) }
    virtual double C(double T) const override { PYTHON_EVAL_CALL_1(double, C, T) }
    virtual double D(double T) const override {
        try { PYTHON_EVAL_CALL_1(double, D, T) }
        catch (NotImplemented) { return mob(T).c00 * T * 8.6173423e-5; } // D = µ kB T / e
    }
    virtual Tensor2<double> thermk(double T, double h)  const { PYTHON_EVAL_CALL_2(Tensor2<double>, thermk, T, h) }
    virtual double dens(double T) const override { PYTHON_EVAL_CALL_1(double, dens, T) }
    virtual double cp(double T) const override { PYTHON_EVAL_CALL_1(double, cp, T) }
    virtual double nr(double wl, double T, double n = .0) const override { PYTHON_EVAL_CALL_3(double, nr, wl, T, n) }
    virtual double absp(double wl, double T) const override { PYTHON_EVAL_CALL_2(double, absp, wl, T) }
    virtual dcomplex Nr(double wl, double T, double n = .0) const override {
        if (cls->Nr != NULL) {
            OmpLockGuard<OmpNestLock> lock(python_omp_lock);
            py::dict locals; locals["self"] = self; locals["wl"] = wl; locals["T"] = T; locals["n"] = n;
            return py::extract<dcomplex>(py::handle<>(py_eval(cls->Nr, locals)).get());
        }
        if (cls->nr != NULL || cls->absp != NULL)
            return dcomplex(nr(wl, T, n), -7.95774715459e-09 * absp(wl, T)*wl);
        return base->Nr(wl, T, n);
    }
    virtual Tensor3<dcomplex> NR(double wl, double T, double n = .0) const override {
        if (cls->NR != NULL) {
            OmpLockGuard<OmpNestLock> lock(python_omp_lock);
            py::dict locals; locals["self"] = self; locals["wl"] = wl; locals["T"] = T; locals["n"] = n;
            return py::extract<Tensor3<dcomplex>>(py::handle<>(py_eval(cls->NR, locals)).get());
        }
        if (cls->Nr != NULL) {
            dcomplex nc = Nr(wl, T, n);
            return Tensor3<dcomplex>(nc, nc, nc, 0.);
        }
        if (cls->nr != NULL || cls->absp != NULL) {
            dcomplex nc(nr(wl, T, n), -7.95774715459e-09 * absp(wl, T)*wl);
            return Tensor3<dcomplex>(nc, nc, nc, 0.);
        }
        return base->NR(wl, T, n);
    }
    // End of overridden methods
};

inline shared_ptr<Material> PythonEvalMaterialConstructor::operator()(const Material::Composition& composition, Material::DopingAmountType doping_amount_type, double doping_amount) const {
    if (base_ctr)
        return make_shared<PythonEvalMaterial>(self.lock(), (*base_ctr)(baseMaterialComposition, doping_amount_type, doping_amount), composition, doping_amount_type, doping_amount);
    else
        return make_shared<PythonEvalMaterial>(self.lock(), base_obj, composition, doping_amount_type, doping_amount);
}

void PythonEvalMaterialLoadFromXML(XMLReader& reader, MaterialsDB& materialsDB) {
    std::string material_name = reader.requireAttribute("name");
    std::string base_name = reader.requireAttribute("base");
    shared_ptr<PythonEvalMaterialConstructor> constructor = make_shared<PythonEvalMaterialConstructor>(materialsDB, material_name, base_name);

    constructor->self = constructor;

    auto trim = [](const char* s) -> const char* {
        for(; *s != 0 && std::isspace(*s); ++s)
        ;
        return s;
    };

#   if PY_VERSION_HEX >= 0x03000000
#       define COMPILE_PYTHON_MATERIAL_FUNCTION2(funcname, func) \
        else if (reader.getNodeName() == funcname) { \
            constructor->func = (PyCodeObject*)Py_CompileString(trim(reader.requireTextInCurrentTag().c_str()), funcname, Py_eval_input); \
            if (constructor->func == nullptr) \
            throw XMLException(format("XML line %1% in <" funcname ">", reader.getLineNr()), "Material parameter syntax error"); \
            try { \
                py::dict locals; \
                constructor->cache.func.reset( \
                    py::extract<typename std::remove_reference<decltype(*constructor->cache.func)>::type>( \
                        py::handle<>(PyEval_EvalCode(constructor->func.ptr_cast<PyObject>(), xml_globals.ptr(), locals.ptr())).get() \
                    ) \
                ); \
                writelog(LOG_DEBUG, "Cached parameter '" funcname "' in material '%1%'", material_name); \
            } catch (py::error_already_set) { \
                PyErr_Clear(); \
            } \
        }
#   else
        PyCompilerFlags flags { CO_FUTURE_DIVISION };
#       define COMPILE_PYTHON_MATERIAL_FUNCTION2(funcname, func) \
        else if (reader.getNodeName() == funcname) { \
            constructor->func = (PyCodeObject*)Py_CompileStringFlags(trim(reader.requireTextInCurrentTag().c_str()), funcname, Py_eval_input, &flags); \
            if (constructor->func == nullptr) \
            throw XMLException(format("XML line %1% in <" funcname ">", reader.getLineNr()), "Material parameter syntax error"); \
            try { \
                py::dict locals; \
                constructor->cache.func.reset( \
                    py::extract<typename std::remove_reference<decltype(*constructor->cache.func)>::type>( \
                        py::handle<>(PyEval_EvalCode(constructor->func, xml_globals.ptr(), locals.ptr())).get() \
                    ) \
                ); \
                writelog(LOG_DEBUG, "Cached parameter '" funcname "' in material '%1%'", material_name); \
            } catch (py::error_already_set) { \
                PyErr_Clear(); \
            } \
        }
#   endif

#   define COMPILE_PYTHON_MATERIAL_FUNCTION(func) COMPILE_PYTHON_MATERIAL_FUNCTION2(BOOST_PP_STRINGIZE(func), func)

    while (reader.requireTagOrEnd()) {
        if (reader.getNodeName() == "condtype") {
            auto condname = reader.requireTextInCurrentTag();
            Material::ConductivityType condtype = (condname == "n" || condname == "N")? Material::CONDUCTIVITY_N :
                            (condname == "i" || condname == "I")? Material::CONDUCTIVITY_I :
                            (condname == "p" || condname == "P")? Material::CONDUCTIVITY_P :
                            (condname == "other" || condname == "OTHER")? Material::CONDUCTIVITY_OTHER :
                             Material::CONDUCTIVITY_UNDETERMINED;
            if (condtype == Material::CONDUCTIVITY_UNDETERMINED)
                throw XMLException(format("XML line %1% in <%2%>", reader.getLineNr(), "condtype"), "Material parameter syntax error, condtype must be given as one of: n, i, p, other (or: N, I, P, OTHER)");
            constructor->condtype = condtype;
        } //else if
        COMPILE_PYTHON_MATERIAL_FUNCTION(lattC)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Eg)
        COMPILE_PYTHON_MATERIAL_FUNCTION(CB)
        COMPILE_PYTHON_MATERIAL_FUNCTION(VB)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Dso)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Mso)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Me)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Mhh)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Mlh)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Mh)
        COMPILE_PYTHON_MATERIAL_FUNCTION(ac)
        COMPILE_PYTHON_MATERIAL_FUNCTION(av)
        COMPILE_PYTHON_MATERIAL_FUNCTION(b)
        COMPILE_PYTHON_MATERIAL_FUNCTION(d)
        COMPILE_PYTHON_MATERIAL_FUNCTION(c11)
        COMPILE_PYTHON_MATERIAL_FUNCTION(c12)
        COMPILE_PYTHON_MATERIAL_FUNCTION(c44)
        COMPILE_PYTHON_MATERIAL_FUNCTION(eps)
        COMPILE_PYTHON_MATERIAL_FUNCTION(chi)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Nc)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Nv)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Ni)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Nf)
        COMPILE_PYTHON_MATERIAL_FUNCTION(EactD)
        COMPILE_PYTHON_MATERIAL_FUNCTION(EactA)
        COMPILE_PYTHON_MATERIAL_FUNCTION(mob)
        COMPILE_PYTHON_MATERIAL_FUNCTION(cond)
        COMPILE_PYTHON_MATERIAL_FUNCTION(A)
        COMPILE_PYTHON_MATERIAL_FUNCTION(B)
        COMPILE_PYTHON_MATERIAL_FUNCTION(C)
        COMPILE_PYTHON_MATERIAL_FUNCTION(D)
        COMPILE_PYTHON_MATERIAL_FUNCTION(thermk)
        COMPILE_PYTHON_MATERIAL_FUNCTION(dens)
        COMPILE_PYTHON_MATERIAL_FUNCTION(cp)
        COMPILE_PYTHON_MATERIAL_FUNCTION(nr)
        COMPILE_PYTHON_MATERIAL_FUNCTION(absp)
        COMPILE_PYTHON_MATERIAL_FUNCTION(Nr)
        COMPILE_PYTHON_MATERIAL_FUNCTION(NR)
        else throw XMLUnexpectedElementException(reader, "material parameter tag");
    }

    materialsDB.addSimple(constructor);
}

}} // namespace plask::python
