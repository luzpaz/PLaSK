#include <cmath>
#include <plask/python.hpp>
using namespace plask;
using namespace plask::python;

#include "../femT.h"
using namespace plask::solvers::thermal;

/**
 * Initialization of your solver class to Python
 *
 * The \a solver_name should be changed to match the name of the directory with our solver
 * (the one where you have put CMakeLists.txt). It will be visible from user interface under this name.
 */
BOOST_PYTHON_MODULE(fem2d)
{
    {CLASS(FiniteElementMethodThermalCartesian2DSolver, "CartesianFEM", "Finite Element thermal solver for 2D Cartesian Geometry.")
        METHOD(runCalc, "Run thermal calculations");
        RECEIVER(inHeats, "Heats"); // receiver in the solver
        PROVIDER(outTemperature, "Temperatures"); // provider in the solver
/*
        METHOD(method_name, "Short documentation", "name_or_argument_1", arg("name_of_argument_2")=default_value_of_arg_2, ...);
        RO_FIELD(field_name, "Short documentation"); // read-only field
        RW_FIELD(field_name, "Short documentation"); // read-write field
        RO_PROPERTY(python_property_name, get_method_name, "Short documentation"); // read-only property
        RW_PROPERTY(python_property_name, get_method_name, set_method_name, "Short documentation"); // read-write property
        RECEIVER(inReceiver, "Short documentation"); // receiver in the solver
        PROVIDER(outProvider, "Short documentation"); // provider in the solver
*/
    }

}

