#include <cmath>
#include <plask/python.hpp>
using namespace plask;
using namespace plask::python;

#include "../therm3d.h"
using namespace plask::solvers::thermal3d;

namespace plask { namespace solvers { namespace thermal3d {

    std::string Convection__repr__(const Convection& self) {
        return "Convection(" + str(self.coeff) + "," + str(self.ambient) + ")";
    }

    std::string Radiation__repr__(const Radiation& self) {
        return "Radiation(" + str(self.emissivity) + "," + str(self.ambient) + ")";
    }

}}}


//TODO remove after 1.06.2014
py::object inHeatDensity_get(py::object self) {
    writelog(LOG_WARNING, "'inHeatDensity' is obsolete. Use 'inHeat' instead!");
    return self.attr("inHeat");
}
//TODO remove after 1.06.2014
void inHeatDensity_set(py::object self, py::object value) {
    writelog(LOG_WARNING, "'inHeatDensity' is obsolete. Use 'inHeat' instead!");
    self.attr("__setattr__")("inHeat", value);
}


BOOST_PYTHON_MODULE(fem3d)
{
    py_enum<Algorithm>()
        .value("CHOLESKY", ALGORITHM_CHOLESKY)
        .value("GAUSS", ALGORITHM_GAUSS)
        .value("ITERATIVE", ALGORITHM_ITERATIVE)
    ;

    py::class_<Convection>("Convection", "Convective boundary condition value", py::init<double,double>())
        .def_readwrite("coeff", &Convection::coeff)
        .def_readwrite("ambient", &Convection::ambient)
        .def("__repr__", &Convection__repr__)
    ;

    py::class_<Radiation>("Radiation", "Radiative boundary condition value", py::init<double,double>())
        .def_readwrite("emissivity", &Radiation::emissivity)
        .def_readwrite("ambient", &Radiation::ambient)
        .def("__repr__", &Radiation__repr__)
    ;

    {CLASS(FiniteElementMethodThermal3DSolver, "Static3D", "Finite element thermal solver for 3D Geometry.")
        METHOD(compute, compute, "Run thermal calculations", py::arg("loops")=0);
        RO_PROPERTY(err, getErr, "Maximum estimated error");
        RECEIVER(inHeat, "");
        solver.setattr("inHeatDensity", solver.attr("inHeat"));
        PROVIDER(outTemperature, "");
        PROVIDER(outHeatFlux, "");
        PROVIDER(outThermalConductivity, "");
        BOUNDARY_CONDITIONS(temperature_boundary, "Boundary conditions for the constant temperature");
        BOUNDARY_CONDITIONS(heatflux_boundary, "Boundary conditions for the constant heat flux");
        BOUNDARY_CONDITIONS(convection_boundary, "Convective boundary conditions");
        BOUNDARY_CONDITIONS(radiation_boundary, "Radiative boundary conditions");
        RW_FIELD(inittemp, "Initial temperature");
        RW_FIELD(maxerr, "Limit for the temperature updates");
        RW_PROPERTY(algorithm, getAlgorithm, setAlgorithm, "Chosen matrix factorization algorithm");
        RW_FIELD(itererr, "Allowed residual iteration for iterative method");
        RW_FIELD(iterlim, "Maximum number of iterations for iterative method");
        RW_FIELD(logfreq, "Frequency of iteration progress reporting");
    }

}
