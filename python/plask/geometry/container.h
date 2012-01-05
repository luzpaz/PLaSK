#ifndef PLASK__PYTHON_GEOMETRY_CONTAINER_H
#define PLASK__PYTHON_GEOMETRY_CONTAINER_H

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
namespace py = boost::python;

#include <plask/geometry/container.h>

#include "geometry.h"

namespace plask { namespace python {



/// Wrapper for PathHints::getChild.
/// Throws exception if there is no such element
GeometryElement* PathHints_getChild(const PathHints& self, GeometryElement* container) {
    GeometryElement* value = self.getChild(container);
    if (value == nullptr) {
        PyErr_SetString(PyExc_KeyError, "No such container in hints");
        throw py::error_already_set();
    }
    return value;
}

// Some other wrappers:

size_t PathHints__len__(const PathHints& self) { return self.hintFor.size(); }

void PathHints__delitem__(PathHints& self, const GeometryElement* key) { self.hintFor.erase(const_cast< GeometryElement*>(key)); }

bool PathHints__contains__(const PathHints& self, const GeometryElement* key) { return self.hintFor.find(const_cast< GeometryElement*>(key)) != self.hintFor.end(); }




void register_geometry_container_h()
{
    py::class_<PathHints>("PathHints", "Hints are used to to find unique path for all GeometryElement pairs, "
                                            "even if one of the pair element is inserted to geometry graph in more than one place.")
        .def("__len__", &PathHints__len__)
        .def("__getitem__", &PathHints_getChild, py::return_internal_reference<1>())
        .def("__setitem__", (void (PathHints::*)(const PathHints::Hint&))(&PathHints::addHint))
        .def("__delitem__", &PathHints__delitem__)
        .def("__contains__", &PathHints__contains__)
        .def("__iter__", py::iterator<PathHints::HintMap, py::return_internal_reference<>>())
        .def("addHint", (void (PathHints::*)(const PathHints::Hint&))(&PathHints::addHint), "Add hint to hints map. Overwrite if hint for given container already exists.")
        .def("addHint", (void (PathHints::*)(GeometryElement*,GeometryElement*))(&PathHints::addHint), "Add hint to hints map. Overwrite if hint for given container already exists.")
        .def("getChild", &PathHints_getChild, "Get child for given container.", py::return_internal_reference<1>())
    ;
}



}} // namespace plask::python
#endif // PLASK__PYTHON_GEOMETRY_CONTAINER_H
