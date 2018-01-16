#include "python_filters.h"

namespace plask { namespace python {

namespace detail {

    void filterin_parse_key(const py::object& key, shared_ptr<GeometryObject>& geom, PathHints*& path, int& points) {
        py::object object;
        path = nullptr;
        points = 10;
        if (PyTuple_Check(key.ptr())) {
            if (py::len(key) < 2 || py::len(key) > 3) throw KeyError(py::extract<std::string>(py::str(key)));
            object = key[0];
            if (py::len(key) == 3) {
                path = py::extract<PathHints*>(key[1]);
                points = py::extract<int>(key[2]);
            } else {
                try {
                    path = py::extract<PathHints*>(key[1]);
                } catch (py::error_already_set) {
                    PyErr_Clear();
                    try {
                        points = py::extract<int>(key[1]);
                    } catch (py::error_already_set) {
                        throw KeyError(py::extract<std::string>(py::str(key)));
                    }
                }
            }
            if (points < 0) throw KeyError(py::extract<std::string>(py::str(key)));
        } else {
            object = key;
        }
        geom = py::extract<shared_ptr<GeometryObject>>(object);
    }
}   // namespace detail

} } // namespace plask::python
