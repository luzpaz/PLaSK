#include <complex>

#include "python_globals.h"
#include <numpy/arrayobject.h>

#include <plask/version.h>
#include <plask/exceptions.h>
#include <plask/mesh/interpolation.h>
#include <plask/module.h>

using namespace plask::python;

// Declare some initialization functions
namespace plask { namespace python {

void initMaterials();
void initGeometry();

void register_manager();

void register_vectors();
void register_mesh();
void register_providers();

void register_data_vectors();

// Hack necessary as macro import_array wants to return some value
static inline bool plask_import_array() {
    import_array1(false);
    return true;
}

// Config
Config config;
AxisNames Config::axes = AxisNames::axisNamesRegister.get("xyz");

inline static void register_config()
{
    py::class_<Config>("config", "Global PLaSK configuration.", py::no_init)
        .def("__str__", &Config::__str__)
        .def("__repr__", &Config::__repr__)
        .add_property("axes", &Config::axes_name, &Config::set_axes,
                      "String representing axis names")
    ;
    py::scope().attr("config") = config;
}


}} // namespace plask::python

BOOST_PYTHON_MODULE(plaskcore)
{
    // Initialize numpy
    if (!plask_import_array()) throw(py::error_already_set());

    py::scope scope; // Default scope

    // Config
    register_config();

    // Vectors
    register_vectors();

    register_vector_of<double>("float");
    register_vector_of<std::complex<double>>("complex");

    // Materials
    initMaterials();

    // Geometry
    initGeometry();

    // Meshes
    register_mesh();

    // Data vector
    register_data_vectors();

    // Modules
    py::class_<plask::Module, plask::shared_ptr<plask::Module>, boost::noncopyable>("Module", "Base class for all modules", py::no_init)
        .add_property("name", &plask::Module::getName, "Full name of the module")
        .add_property("description", &plask::Module::getDescription, "Short description of the module")
        .add_property("initialized", &plask::Module::isInitialized, "True if the module has been initialized")
        .def("invalidate", &plask::Module::invalidate, "Set module back to uninitialized state")
    ;

    // Exceptions
    register_exception<plask::Exception>(PyExc_RuntimeError);

    register_exception<plask::NotImplemented>(PyExc_NotImplementedError);
    register_exception<plask::NoSuchMaterial>(PyExc_ValueError);
    register_exception<plask::BadInput>(PyExc_ValueError);
    register_exception<plask::NoValue>(PyExc_ValueError);
    register_exception<plask::NoProvider>(PyExc_TypeError);

    register_exception<plask::python::ValueError>(PyExc_ValueError);
    register_exception<plask::python::TypeError>(PyExc_TypeError);
    register_exception<plask::python::IndexError>(PyExc_IndexError);
    register_exception<plask::python::KeyError>(PyExc_KeyError);
    register_exception<plask::python::AttributeError>(PyExc_AttributeError);
    register_exception<plask::python::StopIteration>(PyExc_StopIteration);

    // PLaSK version
    scope.attr("version") = PLASK_VERSION;
    scope.attr("version_major") = PLASK_VERSION_MAJOR;
    scope.attr("version_minor") = PLASK_VERSION_MINOR;
}
