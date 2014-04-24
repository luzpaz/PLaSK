#ifndef PLASK__PYTHON_MESH_H
#define PLASK__PYTHON_MESH_H

#include <cmath>

// Important contains
#include "python_globals.h"
#include <plask/mesh/mesh.h>
#include <plask/mesh/boundary.h>

namespace plask { namespace python {

namespace py = boost::python;

/// Generic declaration of mesh generator
template <int mesh_dim>
py::class_<MeshGeneratorD<mesh_dim>, shared_ptr<MeshGeneratorD<mesh_dim>>, py::bases<MeshGenerator>, boost::noncopyable>
ExportMeshGenerator(py::object parent) {
    py::scope scope = parent;
    //std::string name = py::extract<std::string>(parent.attr("__name__"));
    py::class_<MeshGeneratorD<mesh_dim>, shared_ptr<MeshGeneratorD<mesh_dim>>, py::bases<MeshGenerator>, boost::noncopyable>
    pyclass("Generator", ("Base class for all " + boost::lexical_cast<std::string>(mesh_dim) +  "D mesh generators.").c_str(), py::no_init);
    pyclass.def("__call__", &MeshGeneratorD<mesh_dim>::operator(), "Generate mesh for given geometry or load it from the cache", py::arg("geometry"));
    pyclass.def("generate", &MeshGeneratorD<mesh_dim>::generate, "Generate mesh for given geometry omitting the cache", py::arg("geometry"));
    pyclass.def("clear_cache", &MeshGeneratorD<mesh_dim>::clearCache, "Clear cache of generated meshes");
    return pyclass;
}

/// Generic declaration of boundary class for a specific mesh type
template <typename MeshType>
struct ExportBoundary {

    struct PythonPredicate {

        py::object pyfun;

        PythonPredicate(PyObject* obj) : pyfun(py::object(py::handle<>(py::incref(obj)))) {}

        bool operator()(const MeshType& mesh, std::size_t indx) const {
            py::object pyresult = pyfun(mesh, indx);
            bool result;
            try {
                result = py::extract<bool>(pyresult);
            } catch (py::error_already_set) {
                throw TypeError("Boundary predicate did not return Boolean value");
            }
            return result;
        }

        static void* convertible(PyObject* obj) {
            if (PyCallable_Check(obj)) return obj;
            return nullptr;
        }
        static void construct(PyObject* obj, boost::python::converter::rvalue_from_python_stage1_data* data) {
            void* storage = ((boost::python::converter::rvalue_from_python_storage<typename MeshType::Boundary>*)data)->storage.bytes;
            PythonPredicate predicate(obj);
            new (storage) typename MeshType::Boundary { makePredicateBoundary<MeshType>(predicate) };
            data->convertible = storage;
        }

    };

    static typename MeshType::Boundary::WithMesh Boundary__call__(
        const typename MeshType::Boundary& self, const MeshType& mesh, shared_ptr<const GeometryD<MeshType::DIM>> geometry
    ) {
        return self(mesh, geometry);
    }

    ExportBoundary(py::object mesh_class) {

        py::scope scope = mesh_class;

        std::string name = py::extract<std::string>(mesh_class.attr("__name__"));

        if (py::converter::registry::lookup(py::type_id<typename MeshType::Boundary::WithMesh>()).m_class_object == nullptr) {
            py::class_<typename MeshType::Boundary::WithMesh, shared_ptr<typename MeshType::Boundary::WithMesh>>("BoundaryInstance",
                ("Boundary specification for particular "+name+" mesh object").c_str(), py::no_init)
                .def("__contains__", &MeshType::Boundary::WithMesh::contains)
                .def("__iter__", py::range(&MeshType::Boundary::WithMesh::begin, &MeshType::Boundary::WithMesh::end))
                .def("__len__", &MeshType::Boundary::WithMesh::size)
            ;
            py::delattr(scope, "BoundaryInstance");
        }

        py::class_<typename MeshType::Boundary, shared_ptr<typename MeshType::Boundary>>("Boundary",
            ("Generic boundary specification for "+name+" mesh").c_str(), py::no_init)
            .def("__call__", &Boundary__call__, (py::arg("mesh"), "geometry"), "Get boundary instance for particular mesh",
                 py::with_custodian_and_ward_postcall<0,1,
                 py::with_custodian_and_ward_postcall<0,2>>())
        ;

        boost::python::converter::registry::push_back(&PythonPredicate::convertible, &PythonPredicate::construct, boost::python::type_id<typename MeshType::Boundary>());
    }
};

}} // namespace plask::python

#endif // PLASK__PYTHON_MESH_H
