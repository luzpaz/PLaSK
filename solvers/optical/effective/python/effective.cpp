/** \file
 * Python wrapper for optical/effective solvers.
 */
#include <cmath>
#include <plask/python.hpp>
#include <util/ufunc.h>
using namespace plask;
using namespace plask::python;

#include "../eim.h"
#include "../efm.h"
using namespace plask::solvers::effective;

static py::object EffectiveIndex2DSolver_getSymmetry(const EffectiveIndex2DSolver& self) {
    switch (self.getSymmetry()) {
        case EffectiveIndex2DSolver::SYMMETRY_POSITIVE: return py::object("positive");
        case EffectiveIndex2DSolver::SYMMETRY_NEGATIVE: return py::object("negative");
        case EffectiveIndex2DSolver::NO_SYMMETRY: return py::object();
    }
    return py::object();
}

static void EffectiveIndex2DSolver_setSymmetry(EffectiveIndex2DSolver& self, py::object symmetry) {
    if (symmetry == py::object()) { self.setSymmetry(EffectiveIndex2DSolver::NO_SYMMETRY); return; }
    try {
        std::string sym = py::extract<std::string>(symmetry);
        if (sym == "0" || sym == "none" ) {
            self.setSymmetry(EffectiveIndex2DSolver::NO_SYMMETRY); return;
        }
        else if (sym == "positive" || sym == "pos" || sym == "symmeric" || sym == "+" || sym == "+1") {
            self.setSymmetry(EffectiveIndex2DSolver::SYMMETRY_POSITIVE); return;
        }
        else if (sym == "negative" || sym == "neg" || sym == "anti-symmeric" || sym == "antisymmeric" || sym == "-" || sym == "-1") {
            self.setSymmetry(EffectiveIndex2DSolver::SYMMETRY_NEGATIVE); return;
        }
        throw py::error_already_set();
    } catch (py::error_already_set) {
        PyErr_Clear();
        try {
            int sym = py::extract<int>(symmetry);
            if (sym ==  0) { self.setSymmetry(EffectiveIndex2DSolver::NO_SYMMETRY); return; }
            else if (sym == +1) { self.setSymmetry(EffectiveIndex2DSolver::SYMMETRY_POSITIVE); return; }
            else if (sym == -1) { self.setSymmetry(EffectiveIndex2DSolver::SYMMETRY_NEGATIVE); return; }
            throw py::error_already_set();
        } catch (py::error_already_set) {
            throw ValueError("Wrong symmetry specification.");
        }
    }
}

static std::string EffectiveIndex2DSolver_getPolarization(const EffectiveIndex2DSolver& self) {
    return self.getPolarization() == EffectiveIndex2DSolver::TE ? "TE" : "TM";
}

static void EffectiveIndex2DSolver_setPolarization(EffectiveIndex2DSolver& self, std::string polarization) {
    if (polarization == "TE" || polarization == "s" ) {
        self.setPolarization(EffectiveIndex2DSolver::TE); return;
    }
    if (polarization == "TM" || polarization == "p") {
        self.setPolarization(EffectiveIndex2DSolver::TM); return;
    }
}

py::object EffectiveIndex2DSolver_getStripeDeterminant(EffectiveIndex2DSolver& self, int stripe, py::object val)
{
    if (!self.getMesh()) self.setSimpleMesh();
    if (stripe < 0) stripe = self.getMesh()->tran().size() + 1 + stripe;
    if (stripe < 0 || size_t(stripe) >= self.getMesh()->tran().size() + 1) throw IndexError("wrong stripe number");

    return UFUNC<dcomplex>([&](dcomplex x){return self.getStripeDeterminant(stripe, x);}, val);
}

static py::object EffectiveIndex2DSolver_getDeterminant(EffectiveIndex2DSolver& self, py::object val)
{
   return UFUNC<dcomplex>([&](dcomplex x){return self.getDeterminant(x);}, val);
}

static py::object EffectiveFrequencyCylSolver_getDeterminant(EffectiveFrequencyCylSolver& self, py::object val)
{
   return UFUNC<dcomplex>([&](dcomplex x){return self.getDeterminant(x);}, val);
}

static py::object EffectiveFrequencyCylSolver_getDeterminantV(EffectiveFrequencyCylSolver& self, py::object val)
{
   return UFUNC<dcomplex>([&](dcomplex x){return self.getDeterminantV(x);}, val);
}

dcomplex EffectiveFrequencyCylSolver_getLambda0(const EffectiveFrequencyCylSolver& self) {
    return 2e3*M_PI / self.k0;
}

void EffectiveFrequencyCylSolver_setLambda0(EffectiveFrequencyCylSolver& self, dcomplex lambda0) {
    self.k0 = 2e3*M_PI / lambda0;
}

py::object EffectiveIndex2DSolver_getMirrors(const EffectiveIndex2DSolver& self) {
    if (!self.mirrors) return py::object();
    return py::make_tuple(self.mirrors->first, self.mirrors->second);
}

void EffectiveIndex2DSolver_setMirrors(EffectiveIndex2DSolver& self, py::object value) {
    if (value == py::object())
        self.mirrors.reset();
    else {
        try {
            double v = py::extract<double>(value);
            self.mirrors.reset(std::make_pair(v,v));
        } catch (py::error_already_set) {
            PyErr_Clear();
            try {
                if (py::len(value) != 2) throw py::error_already_set();
                self.mirrors.reset(std::make_pair<double,double>(py::extract<double>(value[0]),py::extract<double>(value[1])));
            } catch (py::error_already_set) {
                throw ValueError("None, float, or tuple of two floats required");
            }
        }
    }
}

/**
    * Return mode wavelength
    * \param n mode number
    */
double EffectiveFrequencyCylSolver_Mode_Wavelength(const EffectiveFrequencyCylSolver::Mode& mode) {
    return real(2e3*M_PI / (mode.solver->k0 * (1. - mode.freqv/2.)));
}

/**
    * Return mode modal loss
    * \param n mode number
    */
double EffectiveFrequencyCylSolver_Mode_ModalLoss(const EffectiveFrequencyCylSolver::Mode& mode) {
    return imag(1e7 * mode.solver->k0 * (1. - mode.freqv/2.));
}


/**
 * Initialization of your solver to Python
 *
 * The \a solver_name should be changed to match the name of the directory with our solver
 * (the one where you have put CMakeLists.txt). It will be visible from user interface under this name.
 */
BOOST_PYTHON_MODULE(effective)
{
    if (!plask_import_array()) throw(py::error_already_set());

    {CLASS(EffectiveIndex2DSolver, "EffectiveIndex2D",
        "Calculate optical modes and optical field distribution using the effective index\n"
        "method in two-dimensional Cartesian space.")
        solver.add_property("symmetry", &EffectiveIndex2DSolver_getSymmetry, &EffectiveIndex2DSolver_setSymmetry, "Symmetry of the searched modes");
        solver.add_property("polarization", &EffectiveIndex2DSolver_getPolarization, &EffectiveIndex2DSolver_setPolarization, "Polarization of the searched modes");
        RW_FIELD(outdist, "Distance outside outer borders where material is sampled");
        RO_FIELD(root, "Configuration of the global rootdigger");
        RO_FIELD(stripe_root, "Configuration of the rootdigger for a single stripe");
        RW_PROPERTY(emission, getEmission, setEmission, "Emission direction");
        METHOD(set_simple_mesh, setSimpleMesh, "Set simple mesh based on the geometry objects bounding boxes");
        METHOD(set_horizontal_mesh, setHorizontalMesh, "Set custom mesh in horizontal direction, vertical one is based on the geometry objects bounding boxes", "points");
        METHOD(find_vneffs, findVeffs, "Find the effective index in the vertical direction within the specified range using global method",
               arg("start")=0., arg("end")=0., arg("resteps")=256, arg("imsteps")=64, arg("eps")=dcomplex(1e-6, 1e-9));
        METHOD(compute, computeMode, "Compute the mode near the specified effective index", "neff");
        METHOD(find_modes, findModes, "Find the modes within the specified range using global method",
               arg("start")=0., arg("end")=0., arg("resteps")=256, arg("imsteps")=64, arg("eps")=dcomplex(1e-6, 1e-9));
        METHOD(set_mode, setMode, "Set the current mode the specified effective index.\nneff can be a value returned e.g. by 'find_modes'.", "neff");
        RW_PROPERTY(stripex, getStripeX, setStripeX, "Horizontal position of the main stripe (with dominat mode)");
        RW_FIELD(vneff, "Effective index in the vertical direction");
        solver.add_property("mirrors", EffectiveIndex2DSolver_getMirrors, EffectiveIndex2DSolver_setMirrors,
                    "Mirror reflectivities. If None then they are automatically estimated from Fresenel equations");
        solver.def("get_stripe_determinant", EffectiveIndex2DSolver_getStripeDeterminant, "Get single stripe modal determinant for debugging purposes",
                       (py::arg("stripe"), "neff"));
        solver.def("get_determinant", &EffectiveIndex2DSolver_getDeterminant, "Get modal determinant", (py::arg("neff")));
        RECEIVER(inWavelength, "Wavelength of the light");
        RECEIVER(inTemperature, "Temperature distribution in the structure");
        RECEIVER(inGain, "Optical gain in the active region");
        PROVIDER(outNeff, "Effective index of the last computed mode");
        PROVIDER(outIntensity, "Light intensity of the last computed mode");

        py::scope scope = solver;
        py_enum<EffectiveIndex2DSolver::Emission>("Emission", "Emission direction for Cartesian structure")
            .value("FRONT", EffectiveIndex2DSolver::FRONT)
            .value("BACK", EffectiveIndex2DSolver::BACK)
        ;
    }

    {CLASS(EffectiveFrequencyCylSolver, "EffectiveFrequencyCyl",
        "Calculate optical modes and optical field distribution using the effective frequency\n"
        "method in two-dimensional cylindrical space.")
        RW_FIELD(k0, "Reference normalized frequency");
        solver.add_property("lam0", &EffectiveFrequencyCylSolver_getLambda0, &EffectiveFrequencyCylSolver_setLambda0, "Reference wavelength");
        RW_FIELD(outdist, "Distance outside outer borders where material is sampled");
        RO_FIELD(root, "Configuration of the global rootdigger");
        RO_FIELD(stripe_root, "Configuration of the rootdigger for a single stripe");
        RW_PROPERTY(emission, getEmission, setEmission, "Emission direction");
        METHOD(set_simple_mesh, setSimpleMesh, "Set simple mesh based on the geometry objects bounding boxes");
        METHOD(set_horizontal_mesh, setHorizontalMesh, "Set custom mesh in horizontal direction, vertical one is based on the geometry objects bounding boxes", "points");
        METHOD(find_mode, findMode, "Compute the mode near the specified wavelength", "wavelength", arg("m")=0);
        METHOD(find_modes, findModes, "Find the modes within the specified range using global method",
               arg("start")=0., arg("end")=0., arg("m")=0, arg("resteps")=256, arg("imsteps")=64, arg("eps")=dcomplex(1e-6, 1e-9));
        solver.def("get_determinant_v", &EffectiveFrequencyCylSolver_getDeterminantV, "Get modal determinant for frequency parameter v for debugging purposes",
                   py::arg("v"), arg("m")=0);
        solver.def("get_determinant", &EffectiveFrequencyCylSolver_getDeterminant, "Get modal determinant", arg("lam"), arg("m")=0);
        RECEIVER(inTemperature, "Temperature distribution in the structure");
        RECEIVER(inGain, "Optical gain distribution in the active region");
        PROVIDER(outWavelength, "Wavelength of the computed mode [nm]");
        PROVIDER(outLoss, "Modal loss of the computed mode [1/cm]");
        PROVIDER(outIntensity, "Light intensity of the last computed mode");
        RO_FIELD(modes, "Computed modes");
        
        py::scope scope = solver;
        
        register_vector_of<EffectiveFrequencyCylSolver::Mode>("Modes");
        
        py::class_<EffectiveFrequencyCylSolver::Mode>("Mode", "Detailed information about the mode", py::no_init)
            .def_readonly("m", &EffectiveFrequencyCylSolver::Mode::m, "LP_mn mode parameter describing angular dependence")
            .add_property("wavelength", &EffectiveFrequencyCylSolver_Mode_Wavelength, "Mode wavelength [nm]")
            .add_property("loss", &EffectiveFrequencyCylSolver_Mode_ModalLoss, "Mode loss [1/cm]")
            .def_readwrite("power", &EffectiveFrequencyCylSolver::Mode::power, "Total power emitted into the mode")
        ;
        
        py_enum<EffectiveFrequencyCylSolver::Emission>("Emission", "Emission direction for cylindrical structure")
            .value("TOP", EffectiveFrequencyCylSolver::TOP)
            .value("BOTTOM", EffectiveFrequencyCylSolver::BOTTOM)
        ;
    }

    py::class_<RootDigger::Params, boost::noncopyable>("RootdiggerParams", py::no_init)
        .def_readwrite("tolx", &RootDigger::Params::tolx, "Absolute tolerance on the argument")
        .def_readwrite("tolf_min", &RootDigger::Params::tolf_min, "Sufficient tolerance on the function value")
        .def_readwrite("tolf_max", &RootDigger::Params::tolf_max, "Required tolerance on the function value")
        .def_readwrite("maxstep", &RootDigger::Params::maxstep, "Maximum step in one iteration")
        .def_readwrite("maxiter", &RootDigger::Params::maxiter, "Maximum number of iterations")
    ;
}
