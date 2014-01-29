/** \file
 * Sample Python wrapper for your solver.
 */
#include <cmath>
#include <plask/python.hpp>
#include <util/ufunc.h>
#include <boost/python/raw_function.hpp>
using namespace plask;
using namespace plask::python;

#if defined(_WIN32) && defined(interface)
#   undef interface
#endif

#include "../fourier_reflection_2d.h"
using namespace plask::solvers::slab;

template <typename SolverT>
static const std::vector<std::size_t>& SlabSolver_getStack(const SolverT& self) { return self.getStack(); }

template <typename SolverT>
static const std::vector<RectilinearAxis>& SlabSolver_getLayerSets(const SolverT& self) { return self.getLayersPoints(); }

template <typename SolverT>
struct PmlWrapper {

    SolverT* solver;
    typename SolverT::PML* pml;

    PmlWrapper(SolverT* solver, typename SolverT::PML* pml): solver(solver), pml(pml) {}

    dcomplex get_factor() const { return pml->factor; }
    void set_factor(dcomplex val) {
        pml->factor = val;
        solver->invalidate();
    }

    double get_size() const { return pml->size; }
    void set_size(double val) {
        pml->size = val;
        solver->invalidate();
    }

    double get_shift() const { return pml->shift; }
    void set_shift(double val) {
        pml->shift = val;
        solver->invalidate();
    }

    double get_order() const { return pml->order; }
    void set_order(double val) {
        pml->order = val;
        solver->invalidate();
    }

    std::string __str__() const {
        return format("<factor: %1%, size: %2%, shift: %3%, order: %4%>", pml->factor, pml->size, pml->shift, pml->order);
    }

    std::string __repr__() const {
        return format("PML(factor=%1%, size=%2%, shift=%3%, order=%4%)", pml->factor, pml->size, pml->shift, pml->order);
    }
};

template <typename Class>
inline void export_base(Class solver) {
    typedef typename Class::wrapped_type Solver;
    solver.def_readwrite("outdist", &Solver::outdist, "Distance outside outer borders where material is sampled");
    solver.add_property("interface", &Solver::getInterface, &Solver::setInterface, "Matching interface position");
    solver.def("set_interface", (void(Solver::*)(const shared_ptr<GeometryObject>&, const PathHints&))&Solver::setInterfaceOn,
               "Set interface at the bottom of the object pointed by path", (py::arg("object"), py::arg("path")=py::object()));
    solver.def("set_interface", &Solver::setInterfaceAt, "Set interface around position pos", py::arg("pos"));
    solver.def_readwrite("smooth", &Solver::smooth, "Smoothing parameter");
    solver.add_property("stack", py::make_function<>(&SlabSolver_getStack<Solver>, py::return_internal_reference<>()), "Stack of distinct layers");
    solver.add_property("layer_sets", py::make_function<>(&SlabSolver_getLayerSets<Solver>, py::return_internal_reference<>()), "Vertical positions of layers in each layer set");
    solver.add_receiver("inTemperature", &Solver::inTemperature, "");
    solver.add_receiver("inGain", &Solver::inGain, "");
    solver.add_provider("outLightIntensity", &Solver::outLightIntensity, "");
    solver.add_provider("outElectricField", &Solver::outElectricField, "");
    solver.add_provider("outMagneticField", &Solver::outMagneticField, "");

    py::scope scope = solver;

    py::class_<PmlWrapper<Solver>>("PML", "Perfectly matched layer details", py::no_init)
        .add_property("factor", &PmlWrapper<Solver>::get_factor, &PmlWrapper<Solver>::set_factor, "PML scaling factor")
        .add_property("size", &PmlWrapper<Solver>::get_size, &PmlWrapper<Solver>::set_size, "PML size")
        .add_property("shift", &PmlWrapper<Solver>::get_shift, &PmlWrapper<Solver>::set_shift, "PML shift from the structure")
        .add_property("order", &PmlWrapper<Solver>::get_order, &PmlWrapper<Solver>::set_order, "PML shape order")
        .def("__str__", &PmlWrapper<Solver>::__str__)
        .def("__repr__", &PmlWrapper<Solver>::__repr__)
    ;
}

template <typename Class>
inline void export_reflection_base(Class solver) {
    export_base(solver);
    typedef typename Class::wrapped_type Solver;
    solver.add_property("emitting", &Solver::getEmitting, &Solver::setEmitting, "Should emitted field be computed?");
    py::scope scope = solver;
    py_enum<typename Solver::IncidentDirection>("Incindent", "Direction of incident light for reflection calculations.")
        .value("TOP", Solver::INCIDENCE_TOP)
        .value("BOTTOM", Solver::INCIDENCE_BOTTOM)
    ;
}


struct PythonComponentConventer2D {

    // Determine if obj can be converted into an Aligner
    static void* convertible(PyObject* obj) {
        if (PyString_Check(obj)) return obj;
        return nullptr;
    }

    static void construct(PyObject* obj, boost::python::converter::rvalue_from_python_stage1_data* data) {
        void* storage = ((boost::python::converter::rvalue_from_python_storage<ExpansionPW2D::Component>*)data)->storage.bytes;
        AxisNames* axes = getCurrentAxes();
        ExpansionPW2D::Component val;
        if (obj == Py_None) {
            val = ExpansionPW2D::E_UNSPECIFIED;
        } else {
            try {
                std::string repr = py::extract<std::string>(obj);
                if (repr == "Etran" || repr == "Et" || repr == "E"+axes->getNameForTran() ||
                    repr == "Hlong" || repr == "Hl" || repr == "H"+axes->getNameForLong())
                    val = ExpansionPW2D::E_TRAN;
                else if (repr == "Elong" || repr == "El" || repr == "E"+axes->getNameForLong() ||
                         repr == "Htran" || repr == "Ht" || repr == "H"+axes->getNameForTran())
                    val = ExpansionPW2D::E_LONG;
                else
                    throw py::error_already_set();
            } catch (py::error_already_set) {
                throw ValueError("Wrong component specification.");
            }
        }
        new(storage) ExpansionPW2D::Component(val);
        data->convertible = storage;
    }

    static PyObject* convert(ExpansionPW2D::Component val) {
        AxisNames* axes = getCurrentAxes();
        switch (val) {
            case ExpansionPW2D::E_TRAN: return py::incref(py::object("E"+axes->getNameForTran()).ptr());
            case ExpansionPW2D::E_LONG: return py::incref(py::object("E"+axes->getNameForLong()).ptr());
            default: return py::incref(Py_None);
        }
    }
};

void FourierReflection2D_parseKeywords(const char* name, FourierReflection2D* self, const py::dict& kwargs) {
    AxisNames* axes = getCurrentAxes();
    boost::optional<dcomplex> lambda, neff, ktran;
    py::stl_input_iterator<std::string> begin(kwargs), end;
    for (auto i = begin; i != end; ++i) {
        if (*i == "lam")
            lambda.reset(py::extract<dcomplex>(kwargs[*i]));
        else if (*i == "neff")
            neff.reset(py::extract<dcomplex>(kwargs[*i]));
        else if (*i == "ktran" || *i == "kt" || *i == "k"+axes->getNameForTran())
            ktran.reset(py::extract<dcomplex>(kwargs[*i]));
        else
            throw TypeError("%2%() got unexpected keyword argument '%1%'", *i, name);
    }
    if (lambda) self->setWavelength(*lambda);
    if (neff) self->setKlong(*neff * self->getK0());
    if (ktran) self->setKtran(*ktran);
}


DataVectorWrap<const Tensor3<dcomplex>,2> FourierReflection2D_getRefractiveIndexProfile(FourierReflection2D& self,
                const shared_ptr<RectilinearMesh2D>& dst_mesh, InterpolationMethod interp=INTERPOLATION_DEFAULT) {
    return DataVectorWrap<const Tensor3<dcomplex>,2>(self.getRefractiveIndexProfile(*dst_mesh, interp), dst_mesh);
}

dcomplex FourierReflection2D_getDeterminant(py::tuple args, py::dict kwargs) {
    if (py::len(args) != 1)
        throw TypeError("determinant() takes exactly one non-keyword argument (%1% given)", py::len(args));
    FourierReflection2D* self = py::extract<FourierReflection2D*>(args[0]);
    FourierReflection2D_parseKeywords("determinant", self, kwargs);
    return self->getDeterminant();
}

py::object FourierReflection2D_computeReflectivity(FourierReflection2D* self,
                                                   py::object wavelength,
                                                   ExpansionPW2D::Component polarization,
                                                   FourierReflection2D::IncidentDirection incidence
                                                  )
{
    return UFUNC<double>([=](double lam)->double {
        self->setWavelength(lam);
        return 100. * self->getReflection(polarization, incidence);
    }, wavelength);
}

PmlWrapper<FourierReflection2D> FourierReflection2D_getPML(FourierReflection2D* self) {
    return PmlWrapper<FourierReflection2D>(self, &self->pml);
}

double FourierReflection2D_Mode_Wavelength(const FourierReflection2D::Mode& mode) {
    return real(2e3 / mode.k0);
}
double FourierReflection2D_Mode_ModalLoss(const FourierReflection2D::Mode& mode) {
    return imag(2e4 * mode.k0);
}
double FourierReflection2D_Mode_Beta(const FourierReflection2D::Mode& mode) {
    return real(mode.beta);
}
double FourierReflection2D_Mode_Neff(const FourierReflection2D::Mode& mode) {
    return real(mode.beta / mode.k0);
}
double FourierReflection2D_Mode_KTran(const FourierReflection2D::Mode& mode) {
    return real(mode.ktran);
}

shared_ptr<FourierReflection2D::Reflected> FourierReflection2D_getReflected(FourierReflection2D* parent,
                                                                double wavelength,
                                                                ExpansionPW2D::Component polarization,
                                                                FourierReflection2D::IncidentDirection side)
{
    return make_shared<FourierReflection2D::Reflected>(parent, wavelength, polarization, side);
}


BOOST_PYTHON_MODULE(slab)
{
    plask_import_array();

    py::to_python_converter<ExpansionPW2D::Component, PythonComponentConventer2D>();
    py::converter::registry::push_back(&PythonComponentConventer2D::convertible, &PythonComponentConventer2D::construct,
                                       py::type_id<ExpansionPW2D::Component>());

    {CLASS(FourierReflection2D, "FourierReflection2D",
        "Optical Solver using Fourier expansion in 2D.\n\n"
        "It calculates optical modes and optical field distribution using Fourier slab method\n"
        "and reflection transfer in two-dimensional Cartesian space.")
        export_reflection_base(solver);
        PROVIDER(outNeff, "Effective index of the last computed mode.");
        METHOD(find_mode, findMode,
               "Compute the mode near the specified effective index.\n\n"
               "Args:\n"
               "    neff: starting effective index.\n"
               , "neff");
        RW_PROPERTY(wavelength, getWavelength, setWavelength, "Wavelength of the light.");
        RW_PROPERTY(beta, getKlong, setKlong, "Longitudinal propagation constant of the light.");
        RW_PROPERTY(ktran, getKtran, setKtran, "Transverse propagation constant of the light.");
        RW_PROPERTY(size, getSize, setSize, "Orthogonal expansion size.");
        RW_PROPERTY(symmetry, getSymmetry, setSymmetry, "Mode symmetry.");
        RW_PROPERTY(polarization, getPolarization, setPolarization, "Mode polarization.");
        RW_FIELD(refine, "Number of refinement points for refractive index averaging.");
        solver.def("determinant", py::raw_function(FourierReflection2D_getDeterminant),
                   "Compute discontinuity matrix determinant.");
        solver.def("compute_reflectivity", &FourierReflection2D_computeReflectivity,
                   "Compute reflectivity on the perpendicular incidence [%].\n\n"
                   "Args:\n"
                   "    lam (float or array of floats): Incident light wavelength.\n"
                   "    polarization: Specification of the incident light polarization.\n"
                   "        It should be a string of the form 'E\\ *#*\\ ', where *#* is the axis name\n"
                   "        of the non-vanishing electric field component.\n"
                   "    side (`top` or `bottom`): Side of the structure at which the reflectivity\n"
                   "        is computed.\n"
                   , (py::arg("lam"), "polarization", "side"));
        solver.def("get_refractive_index_profile", &FourierReflection2D_getRefractiveIndexProfile,
                   "Get profile of the expanded refractive index.\n\n"
                   "Args:\n"
                   "    mesh: Target mesh.\n"
                   "    interp: Interpolation method\n"
                   , (py::arg("mesh"), py::arg("interp")=INTERPOLATION_DEFAULT));
        solver.add_property("pml", py::make_function(&FourierReflection2D_getPML, py::with_custodian_and_ward_postcall<0,1>()), "Side PMLs.");
        RO_PROPERTY(period, getPeriod, "Period for the periodic structures.");
        RO_FIELD(modes, "Computed modes.");
        solver.def("reflected", &FourierReflection2D_getReflected, py::with_custodian_and_ward_postcall<0,1>(),
                   "Access to the reflected field.\n\n"
                   "Args:\n"
                   "    lam (float): Incident light wavelength.\n"
                   "    polarization: Specification of the incident light polarization.\n"
                   "        It should be a string of the form 'E\\ *#*\\ ', where *#* is the axis name\n"
                   "        of the non-vanishing electric field component.\n"
                   "    side (`top` or `bottom`): Side of the structure at which the reflectivity\n"
                   "        is computed.\n"
                   , (py::arg("lam"), "polarization", "side"));
        py::scope scope = solver;

        register_vector_of<FourierReflection2D::Mode>("Modes");
        py::class_<FourierReflection2D::Mode>("Mode", "Detailed information about the mode.", py::no_init)
            .def_readonly("symmetry", &FourierReflection2D::Mode::symmetry, "Mode horizontal symmetry.")
            .def_readonly("polarization", &FourierReflection2D::Mode::polarization, "Mode polarization.")
            .add_property("lam", &FourierReflection2D_Mode_Wavelength, "Mode wavelength [nm].")
            .add_property("wavelength", &FourierReflection2D_Mode_Wavelength, "Mode wavelength [nm].")
            .add_property("loss", &FourierReflection2D_Mode_ModalLoss, "Mode loss [1/cm].")
            .add_property("beta", &FourierReflection2D_Mode_Beta, "Mode longitudinal wavevector.")
            .add_property("neff", &FourierReflection2D_Mode_Neff, "Mode longitudinal wavevector.")
            .add_property("ktran", &FourierReflection2D_Mode_KTran, "Mode transverse wavevector.")
            .def_readwrite("power", &FourierReflection2D::Mode::power, "Total power emitted into the mode.")
        ;
        
        py::class_<FourierReflection2D::Reflected, shared_ptr<FourierReflection2D::Reflected>, boost::noncopyable>("Reflected",
            "Reflected mode proxy.\n\n"
            "This class contains providers for the optical field for a reflected field"
            "under the normal incidence.\n"
            , py::no_init)
            .def_readonly("outElectricField", reinterpret_cast<ProviderFor<OpticalElectricField,Geometry2DCartesian> FourierReflection2D::Reflected::*>
                                              (&FourierReflection2D::Reflected::outElectricField),
                format(docstring_attr_provider<FIELD_PROPERTY>(), "OpticalElectricField", "2D", "electric field", "V/m", "", "", "", "outElectricField").c_str()
            )
            .def_readonly("outMagneticField", reinterpret_cast<ProviderFor<OpticalMagneticField,Geometry2DCartesian> FourierReflection2D::Reflected::*>
                                              (&FourierReflection2D::Reflected::outMagneticField),
                format(docstring_attr_provider<FIELD_PROPERTY>(), "OpticalMagneticField", "2D", "magnetic field", "A/m", "", "", "", "outMagneticField").c_str()
            )
            .def_readonly("outLightIntensity", reinterpret_cast<ProviderFor<LightIntensity,Geometry2DCartesian> FourierReflection2D::Reflected::*>
                                              (&FourierReflection2D::Reflected::outLightIntensity),
                format(docstring_attr_provider<FIELD_PROPERTY>(), "LightIntensity", "2D", "light intensity", "W/m²", "", "", "", "outLightIntensity").c_str()
            )
        ;
    }
}

