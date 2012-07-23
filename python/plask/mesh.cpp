#include "python_globals.h"
#include <algorithm>
#include <boost/python/stl_iterator.hpp>

#include <plask/mesh/mesh.h>
#include <plask/mesh/interpolation.h>
#include <plask/mesh/rectilinear1d.h>

namespace plask { namespace python {

void register_mesh_rectangular();

void register_mesh()
{
    py::enum_<InterpolationMethod> pyInterpolationMethod("interpolation", "Available interpolation methods.");
    for (int i = (int)DEFAULT_INTERPOLATION; i != (int)__ILLEGAL_INTERPOLATION_METHOD__; ++i) {
        pyInterpolationMethod.value(interpolationMethodNames[i], (InterpolationMethod)i);
    }

    py::object mesh_module { py::handle<>(py::borrowed(PyImport_AddModule("plask.mesh"))) };
    py::scope().attr("mesh") = mesh_module;
    py::scope scope = mesh_module;

    py::class_<Mesh, shared_ptr<Mesh>, boost::noncopyable>("Mesh2D", "Base class for all meshes", py::no_init)
        .def("__len__", &Mesh::size)
    ;

    py::class_<MeshD<2>, shared_ptr<MeshD<2>>, py::bases<Mesh>, boost::noncopyable>("Mesh2D", "Base class for every two-dimensional mesh", py::no_init)
    ;

    py::class_<MeshD<3>, shared_ptr<MeshD<3>>, py::bases<Mesh>, boost::noncopyable>("Mesh3D", "Base class for every two-dimensional mesh", py::no_init)
    ;

    py::class_<MeshGenerator, shared_ptr<MeshGenerator>, boost::noncopyable>("MeshGenerator", "Base class for all mesh generators", py::no_init)
    ;

    register_mesh_rectangular();

}

}} // namespace plask::python