#include "python_globals.h"

#include <plask/utils/string.h>
#include <plask/utils/xml/reader.h>
#include <plask/material/db.h>

namespace plask { namespace python {

/**
 * Wrapper for Material read from XML of type eval
 * For all virtual functions it calls Python derivatives
 */
class PythonEvalMaterial;

extern py::dict xml_globals;

struct PythonEvalMaterialConstructor: public MaterialsDB::MaterialConstructor {

    weak_ptr<PythonEvalMaterialConstructor> self;
    MaterialsDB* db;

    std::string base;
    Material::Kind kind;
    Material::ConductivityType condtype;

    PyCodeObject
        *lattC, *Eg, *CB, *VB, *Dso, *Mso, *Me, *Mhh, *Mlh, *Mh, *ac, *av, *b, *d, *c11, *c12, *c44, *eps, *chi,
        *Nc, *Nv, *Ni, *Nf, *EactD, *EactA, *mob, *cond, *A, *B, *C, *D,
        *thermk, *dens, *cp, *nr, *absp, *Nr, *NR;

    PythonEvalMaterialConstructor(const std::string& name) :
        MaterialsDB::MaterialConstructor(name), base(""), kind(Material::NONE), condtype(Material::CONDUCTIVITY_UNDETERMINED),
        lattC(NULL), Eg(NULL), CB(NULL), VB(NULL), Dso(NULL), Mso(NULL), Me(NULL),
        Mhh(NULL), Mlh(NULL), Mh(NULL), ac(NULL), av(NULL), b(NULL), d(NULL), c11(NULL), c12(NULL), c44(NULL), eps(NULL), chi(NULL), Nc(NULL), Nv(NULL), Ni(NULL), Nf(NULL),
        EactD(NULL), EactA(NULL), mob(NULL), cond(NULL), A(NULL), B(NULL), C(NULL), D(NULL),
        thermk(NULL), dens(NULL), cp(NULL), nr(NULL), absp(NULL), Nr(NULL), NR(NULL) {}

    PythonEvalMaterialConstructor(const std::string& name, const std::string& base) :
        MaterialsDB::MaterialConstructor(name), base(base), kind(Material::NONE), condtype(Material::CONDUCTIVITY_UNDETERMINED),
        lattC(NULL), Eg(NULL), CB(NULL), VB(NULL), Dso(NULL), Mso(NULL), Me(NULL),
        Mhh(NULL), Mlh(NULL), Mh(NULL), ac(NULL), av(NULL), b(NULL), d(NULL), c11(NULL), c12(NULL), c44(NULL), eps(NULL), chi(NULL), Nc(NULL), Nv(NULL), Ni(NULL), Nf(NULL),
        EactD(NULL), EactA(NULL), mob(NULL), cond(NULL), A(NULL), B(NULL), C(NULL), D(NULL),
        thermk(NULL), dens(NULL), cp(NULL), nr(NULL), absp(NULL), Nr(NULL), NR(NULL) {}

    virtual ~PythonEvalMaterialConstructor() {
        Py_XDECREF(lattC); Py_XDECREF(Eg); Py_XDECREF(CB); Py_XDECREF(VB); Py_XDECREF(Dso); Py_XDECREF(Mso); Py_XDECREF(Me);
        Py_XDECREF(Mhh); Py_XDECREF(Mlh); Py_XDECREF(Mh); Py_XDECREF(ac); Py_XDECREF(av); Py_XDECREF(b); Py_XDECREF(d); Py_XDECREF(c11); Py_XDECREF(c12); Py_XDECREF(c44); Py_XDECREF(eps); Py_XDECREF(chi);
        Py_XDECREF(Nc); Py_XDECREF(Nv); Py_XDECREF(Ni); Py_XDECREF(Nf); Py_XDECREF(EactD); Py_XDECREF(EactA);
        Py_XDECREF(mob); Py_XDECREF(cond); Py_XDECREF(A); Py_XDECREF(B); Py_XDECREF(C); Py_XDECREF(D);
        Py_XDECREF(thermk); Py_XDECREF(dens); Py_XDECREF(cp);
        Py_XDECREF(nr); Py_XDECREF(absp); Py_XDECREF(Nr); Py_XDECREF(NR);
    }

    inline shared_ptr<Material> operator()(const Material::Composition& composition, Material::DopingAmountType doping_amount_type, double doping_amount) const;
};

class PythonEvalMaterial : public Material
{
    shared_ptr<PythonEvalMaterialConstructor> cls;
    shared_ptr<Material> base;

    Material::DopingAmountType doping_amount_type;
    double doping_amount;

    py::object self;

    static inline PyObject* PY_EVAL(PyCodeObject *fun, const py::dict& locals) {
        return
#if PY_VERSION_HEX >= 0x03000000
            PyEval_EvalCode((PyObject*)fun, xml_globals.ptr(), locals.ptr());
#else
            PyEval_EvalCode(fun, xml_globals.ptr(), locals.ptr());
#endif
    }

    template <typename RETURN>
    inline RETURN call(PyCodeObject *fun, const py::dict& locals) const {
        return py::extract<RETURN>(py::handle<>(PY_EVAL(fun, locals)).get());
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

    virtual bool isEqual(const Material& other) const {
        auto theother = static_cast<const PythonEvalMaterial&>(other);
        return
            cls == theother.cls &&
            doping_amount_type == theother.doping_amount_type &&
            doping_amount == theother.doping_amount;
    }

    virtual std::string name() const { return cls->materialName; }
    virtual Material::Kind kind() const { return (cls->kind == Material::NONE)? base->kind() : cls->kind; }
    virtual Material::ConductivityType condtype() const { return (cls->condtype == Material::CONDUCTIVITY_UNDETERMINED)? base->condtype() : cls->condtype; }

#   define PYTHON_EVAL_CALL_1(rtype, fun, arg1) \
        if (cls->fun == NULL) return base->fun(arg1); \
        py::dict locals; locals["self"] = self; locals[BOOST_PP_STRINGIZE(arg1)] = arg1; \
        return call<rtype>(cls->fun, locals);

#   define PYTHON_EVAL_CALL_2(rtype, fun, arg1, arg2) \
        if (cls->fun == NULL) return base->fun(arg1, arg2); \
        py::dict locals; locals["self"] = self; locals[BOOST_PP_STRINGIZE(arg1)] = arg1; locals[BOOST_PP_STRINGIZE(arg2)] = arg2; \
        return call<rtype>(cls->fun, locals);

#   define PYTHON_EVAL_CALL_3(rtype, fun, arg1, arg2, arg3) \
        if (cls->fun == NULL) return base->fun(arg1, arg2); \
        py::dict locals; locals["self"] = self; locals[BOOST_PP_STRINGIZE(arg1)] = arg1; locals[BOOST_PP_STRINGIZE(arg2)] = arg2; \
        locals[BOOST_PP_STRINGIZE(arg3)] = arg3; \
        return call<rtype>(cls->fun, locals);

#   define PYTHON_EVAL_CALL_4(rtype, fun, arg1, arg2, arg3, arg4) \
        if (cls->fun == NULL) return base->fun(arg1, arg2); \
        py::dict locals; locals["self"] = self; locals[BOOST_PP_STRINGIZE(arg1)] = arg1; locals[BOOST_PP_STRINGIZE(arg2)] = arg2; \
        locals[BOOST_PP_STRINGIZE(arg3)] = arg3; locals[BOOST_PP_STRINGIZE(arg4)] = arg4; \
        return call<rtype>(cls->fun, locals);

    virtual double lattC(double T, char x) const { PYTHON_EVAL_CALL_2(double, lattC, T, x) }
    virtual double Eg(double T, double e, char point) const { PYTHON_EVAL_CALL_3(double, Eg, T, e, point) }
    virtual double CB(double T, double e, char point) const {
        try { PYTHON_EVAL_CALL_3(double, CB, T, e, point) }
        catch (NotImplemented) { return VB(T, e, point, 'H') + Eg(T, e, point); }
    }
    virtual double VB(double T, double e, char point, char hole) const { PYTHON_EVAL_CALL_4(double, VB, T, e, point, hole) }
    virtual double Dso(double T, double e) const { PYTHON_EVAL_CALL_2(double, Dso, T, e) }
    virtual double Mso(double T, double e) const { PYTHON_EVAL_CALL_2(double, Mso, T, e) }
    virtual Tensor2<double> Me(double T, double e, char point) const { PYTHON_EVAL_CALL_3(Tensor2<double>, Me, T, e, point) }
    virtual Tensor2<double> Mhh(double T, double e) const { PYTHON_EVAL_CALL_2(Tensor2<double>, Mhh, T, e) }
    virtual Tensor2<double> Mlh(double T, double e) const { PYTHON_EVAL_CALL_2(Tensor2<double>, Mlh, T, e) }
    virtual Tensor2<double> Mh(double T, double e) const { PYTHON_EVAL_CALL_2(Tensor2<double>, Mh, T, e) }
    virtual double ac(double T) const { PYTHON_EVAL_CALL_1(double, ac, T) }
    virtual double av(double T) const { PYTHON_EVAL_CALL_1(double, av, T) }
    virtual double b(double T) const { PYTHON_EVAL_CALL_1(double, b, T) }
    virtual double d(double T) const { PYTHON_EVAL_CALL_1(double, d, T) }
    virtual double c11(double T) const { PYTHON_EVAL_CALL_1(double, c11, T) }
    virtual double c12(double T) const { PYTHON_EVAL_CALL_1(double, c12, T) }
    virtual double c44(double T) const { PYTHON_EVAL_CALL_1(double, c44, T) }
    virtual double eps(double T) const { PYTHON_EVAL_CALL_1(double, eps, T) }
    virtual double chi(double T, double e, char point) const { PYTHON_EVAL_CALL_3(double, chi, T, e, point) }
    virtual double Nc(double T, double e, char point) const { PYTHON_EVAL_CALL_3(double, Nc, T, e, point) }
    virtual double Nv(double T, double e, char point) const { PYTHON_EVAL_CALL_3(double, Nv, T, e, point) }
    virtual double Ni(double T) const { PYTHON_EVAL_CALL_1(double, Ni, T) }
    virtual double Nf(double T) const { PYTHON_EVAL_CALL_1(double, Nf, T) }
    virtual double EactD(double T) const { PYTHON_EVAL_CALL_1(double, EactD, T) }
    virtual double EactA(double T) const { PYTHON_EVAL_CALL_1(double, EactA, T) }
    virtual Tensor2<double> mob(double T) const { PYTHON_EVAL_CALL_1(Tensor2<double>, mob, T) }
    virtual Tensor2<double> cond(double T) const { PYTHON_EVAL_CALL_1(Tensor2<double>, cond, T) }
    virtual double A(double T) const { PYTHON_EVAL_CALL_1(double, A, T) }
    virtual double B(double T) const { PYTHON_EVAL_CALL_1(double, B, T) }
    virtual double C(double T) const { PYTHON_EVAL_CALL_1(double, C, T) }
    virtual double D(double T) const {
        try { PYTHON_EVAL_CALL_1(double, D, T) }
        catch (NotImplemented) { return mob(T).c00 * T * 8.6173423e-5; } // D = µ kB T / e
    }
    virtual Tensor2<double> thermk(double T, double h)  const { PYTHON_EVAL_CALL_2(Tensor2<double>, thermk, T, h) }
    virtual double dens(double T) const { PYTHON_EVAL_CALL_1(double, dens, T) }
    virtual double cp(double T) const { PYTHON_EVAL_CALL_1(double, cp, T) }
    virtual double nr(double wl, double T) const { PYTHON_EVAL_CALL_2(double, nr, wl, T) }
    virtual double absp(double wl, double T) const { PYTHON_EVAL_CALL_2(double, absp, wl, T) }
    virtual dcomplex Nr(double wl, double T) const {
        py::dict locals; locals["self"] = self; locals["wl"] = wl; locals["T"] = T;
        if (cls->Nr != NULL) {
            return py::extract<dcomplex>(py::handle<>(PY_EVAL(cls->Nr, locals)).get());
        }
        if (cls->nr != NULL || cls->absp != NULL)
            return dcomplex(nr(wl, T), -7.95774715459e-09 * absp(wl, T)*wl);
        return base->Nr(wl, T);
    }
    virtual Tensor3<dcomplex> NR(double wl, double T) const {
        py::dict locals; locals["self"] = self; locals["wl"] = wl; locals["T"] = T;
        if (cls->NR != NULL) {
            return py::extract<Tensor3<dcomplex>>(py::handle<>(PY_EVAL(cls->NR, locals)).get());
        }
        if (cls->Nr != NULL) {
            dcomplex n = Nr(wl, T);
            return Tensor3<dcomplex>(n, n, n, 0., 0.);
        }
        if (cls->nr != NULL || cls->absp != NULL) {
            dcomplex n(nr(wl, T), -7.95774715459e-09 * absp(wl, T)*wl);
            return Tensor3<dcomplex>(n, n, n, 0., 0.);
        }
        return base->NR(wl, T);
    }
    // End of overridden methods
};

inline shared_ptr<Material> PythonEvalMaterialConstructor::operator()(const Material::Composition& composition, Material::DopingAmountType doping_amount_type, double doping_amount) const {
    shared_ptr<Material> base_obj;
    if (base != "") {
        if (base.find("=") != std::string::npos)
            base_obj = this->db->get(base);
        else
            base_obj = this->db->get(base, doping_amount_type, doping_amount);
    }
    else base_obj = make_shared<EmptyMaterial>();
    return make_shared<PythonEvalMaterial>(self.lock(), base_obj, composition, doping_amount_type, doping_amount);
}



void PythonEvalMaterialLoadFromXML(XMLReader& reader, MaterialsDB& materialsDB) {
    shared_ptr<PythonEvalMaterialConstructor> constructor;
    std::string name = reader.requireAttribute("name");
    auto base = reader.getAttribute("base");
    if (base)
        constructor = make_shared<PythonEvalMaterialConstructor>(name, *base);
    else {
        std::string kindname = reader.requireAttribute("kind");
        Material::Kind kind =  (kindname == "semiconductor" || kindname == "SEMICONDUCTOR")? Material::SEMICONDUCTOR :
                               (kindname == "oxide" || kindname == "OXIDE")? Material::OXIDE :
                               (kindname == "dielectric" || kindname == "DIELECTRIC")? Material::DIELECTRIC :
                               (kindname == "metal" || kindname == "METAL")? Material::METAL :
                               (kindname == "liquid crystal" || kindname == "liquid_crystal" || kindname == "LIQUID_CRYSTAL" || kindname == "LC")? Material::LIQUID_CRYSTAL :
                                Material::NONE;
        if (kind == Material::NONE) throw XMLBadAttrException(reader, "kind", kindname);

        Material::ConductivityType condtype = Material::CONDUCTIVITY_UNDETERMINED;
        auto condname = reader.getAttribute("condtype");
        if (condname) {
            condtype = (*condname == "n" || *condname == "N")? Material::CONDUCTIVITY_N :
                        (*condname == "i" || *condname == "I")? Material::CONDUCTIVITY_I :
                        (*condname == "p" || *condname == "P")? Material::CONDUCTIVITY_P :
                        (*condname == "other" || *condname == "OTHER")? Material::CONDUCTIVITY_OTHER :
                         Material::CONDUCTIVITY_UNDETERMINED;
            if (condtype == Material::CONDUCTIVITY_UNDETERMINED) throw XMLBadAttrException(reader, "condtype", *condname);
        }

        constructor = make_shared<PythonEvalMaterialConstructor>(name);
        constructor->kind = kind;
        constructor->condtype = condtype;
    }
    constructor->self = constructor;
    constructor->db = &materialsDB;

    auto trim = [](const char* s) -> const char* {
        for(; *s != 0 && std::isspace(*s); ++s)
        ;
        return s;
    };

#   if PY_VERSION_HEX >= 0x03000000

#       define COMPILE_PYTHON_MATERIAL_FUNCTION(func) \
        else if (reader.getNodeName() == BOOST_PP_STRINGIZE(func)) \
            constructor->func = (PyCodeObject*)Py_CompileString(trim(reader.requireTextInCurrentTag().c_str()), BOOST_PP_STRINGIZE(func), Py_eval_input);

#       define COMPILE_PYTHON_MATERIAL_FUNCTION2(name, func) \
        else if (reader.getNodeName() == name) \
            constructor->func = (PyCodeObject*)Py_CompileString(trim(reader.requireTextInCurrentTag().c_str()), BOOST_PP_STRINGIZE(func), Py_eval_input);

#   else
        PyCompilerFlags flags { CO_FUTURE_DIVISION };

#       define COMPILE_PYTHON_MATERIAL_FUNCTION(func) \
        else if (reader.getNodeName() == BOOST_PP_STRINGIZE(func)) { \
            constructor->func = (PyCodeObject*)Py_CompileStringFlags(trim(reader.requireTextInCurrentTag().c_str()), BOOST_PP_STRINGIZE(func), Py_eval_input, &flags); \
            if (constructor->func == nullptr)  throw XMLException(format("XML line %1% in <%2%>", reader.getLineNr(), BOOST_PP_STRINGIZE(func)), "Material parameter syntax error"); \
        }

#       define COMPILE_PYTHON_MATERIAL_FUNCTION2(name, func) \
        else if (reader.getNodeName() == name) { \
            constructor->func = (PyCodeObject*)Py_CompileStringFlags(trim(reader.requireTextInCurrentTag().c_str()), BOOST_PP_STRINGIZE(func), Py_eval_input, &flags); \
            if (constructor->func == nullptr)  throw XMLException(format("XML line %1% in <%2%>", reader.getLineNr(), name), "Material parameter syntax error"); \
        }

#   endif
    while (reader.requireTagOrEnd()) {
        if (false);
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
