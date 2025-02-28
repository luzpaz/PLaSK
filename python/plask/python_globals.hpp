/*
 * This file is part of PLaSK (https://plask.app) by Photonics Group at TUL
 * Copyright (c) 2022 Lodz University of Technology
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef PLASK__PYTHON_GLOBALS_H
#define PLASK__PYTHON_GLOBALS_H

#include <plask/vec.hpp>
#include <cmath>
#include <vector>

// ----------------------------------------------------------------------------------------------------------------------
// Shared pointer
#include <plask/memory.hpp>

#ifdef PLASK_SHARED_PTR_STD
namespace boost { namespace python {
    template<class T> inline T* get_pointer(std::shared_ptr<T> const& p) { return p.get(); }
}}
#endif

// ----------------------------------------------------------------------------------------------------------------------
// Important contains
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <frameobject.h>

#if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32)) && defined(hypot)
#   undef hypot
#endif


#include <plask/exceptions.hpp>
#include <plask/math.hpp>
#include <plask/axes.hpp>
#include <plask/geometry/space.hpp>
#include <plask/log/log.hpp>
#include <plask/parallel.hpp>
#include <plask/manager.hpp>

#include "python_enum.hpp"

namespace plask { namespace python {

namespace py = boost::python;

PLASK_PYTHON_API py::object py_eval(std::string string, py::object global=py::object(), py::object local=py::object());

// ----------------------------------------------------------------------------------------------------------------------
// Exceptions

template <typename ExcType>
void register_exception(PyObject* py_exc) {
    py::register_exception_translator<ExcType>( [=](const ExcType& err) { PyErr_SetString(py_exc, err.what()); } );
}

struct ValueError: public Exception {
    template <typename... T>
    ValueError(const std::string& msg, const T&... args) : Exception(msg, args...) {}
};

struct TypeError: public Exception {
    template <typename... T>
    TypeError(const std::string& msg, const T&... args) : Exception(msg, args...) {}
};

struct IndexError: public Exception {
    template <typename... T>
    IndexError(const std::string& msg, const T&... args) : Exception(msg, args...) {}
};

struct KeyError: public Exception {
    template <typename... T>
    KeyError(const std::string& msg, const T&... args) : Exception(msg, args...) {}
};

struct AttributeError: public Exception {
    template <typename... T>
    AttributeError(const std::string& msg, const T&... args) : Exception(msg, args...) {}
};

struct StopIteration: public Exception {
    template <typename... T>
    StopIteration(const std::string& msg, const T&... args) : Exception(msg, args...) {}
};

struct IOError: public Exception {
    template <typename... T>
    IOError(const std::string& msg, const T&... args) : Exception(msg, args...) {}
};

struct RecursionError: public Exception {
    template <typename... T>
    RecursionError(const std::string& msg, const T&... args) : Exception(msg, args...) {}
};

PLASK_PYTHON_API std::string getPythonExceptionMessage();

PLASK_PYTHON_API int printPythonException(PyObject* otype, PyObject* value, PyObject* otraceback,
                                          const char* scriptname=nullptr, const char* top_frame=nullptr, int scriptline=0);

inline int printPythonException(PyObject* value, const char* scriptname=nullptr, const char* top_frame=nullptr, int scriptline=0) {
    PyObject* type = PyObject_Type(value);
    PyObject* traceback = PyException_GetTraceback(value);
    py::handle<> type_h(type), traceback_h(py::allow_null(traceback));
    return printPythonException(type, value, traceback, scriptname, top_frame, scriptline);
}


// ----------------------------------------------------------------------------------------------------------------------
// Compare shared pointers
template <typename T>
bool __is__(const shared_ptr<T>& a, const shared_ptr<T>& b) {
    return a == b;
}

template <typename T>
long __hash__(const shared_ptr<T>& a) {
    auto p = a.get();
    return *reinterpret_cast<long*>(&p);
}


// ----------------------------------------------------------------------------------------------------------------------
inline py::object pass_through(const py::object& o) { return o; }


// ----------------------------------------------------------------------------------------------------------------------
struct PredicatePythonCallable {
    py::object callable;
    PredicatePythonCallable(const py::object& callable): callable(callable) {};
    bool operator()(const GeometryObject& obj) const {
        return py::extract<bool>(callable(const_pointer_cast<GeometryObject>(obj.shared_from_this())));
    }
};


// ----------------------------------------------------------------------------------------------------------------------
// Format complex numbers in Python way
template <typename T>
inline std::string pyformat(const T& v) { std::stringstream s; s << v; return s.str(); }

template <>
inline std::string pyformat<dcomplex>(const dcomplex& v) { return format("({:g}{:+g}j)", real(v), imag(v)); }


// ----------------------------------------------------------------------------------------------------------------------
// PLaSK str function for Python objects
inline std::string str(py::object obj) {
    return py::extract<std::string>(py::str(obj));
}


// ----------------------------------------------------------------------------------------------------------------------
// Get dtype for data
namespace detail {
    template <typename T> inline py::handle<> dtype() { return py::handle<>(py::borrowed<>(reinterpret_cast<PyObject*>(py::converter::registry::lookup(py::type_id<T>()).get_class_object()))); }
    template<> inline py::handle<> dtype<double>() { return py::handle<>(py::borrowed<>(reinterpret_cast<PyObject*>(&PyFloat_Type))); }
    template<> inline py::handle<> dtype<dcomplex>() { return py::handle<>(py::borrowed<>(reinterpret_cast<PyObject*>(&PyComplex_Type))); }
    // template<> inline py::handle<> dtype<Tensor2<double>>() { return py::handle<>(py::borrowed<>(reinterpret_cast<PyObject*>(&PyTuple_Type))); }
    // template<> inline py::handle<> dtype<Tensor2<dcomplex>>() { return py::handle<>(py::borrowed<>(reinterpret_cast<PyObject*>(&PyTuple_Type))); }
    // template<> inline py::handle<> dtype<Tensor3<double>>() { return py::handle<>(py::borrowed<>(reinterpret_cast<PyObject*>(&PyTuple_Type))); }
    // template<> inline py::handle<> dtype<Tensor3<dcomplex>>() { return py::handle<>(py::borrowed<>(reinterpret_cast<PyObject*>(&PyTuple_Type))); }
}

// ----------------------------------------------------------------------------------------------------------------------
// Geometry suffix

template <typename GeometryT> inline std::string format_geometry_suffix(const char*);
template <> inline std::string format_geometry_suffix<Geometry2DCartesian>(const char* fmt) { return format(fmt, "2D"); }
template <> inline std::string format_geometry_suffix<Geometry2DCylindrical>(const char* fmt) { return format(fmt, "Cyl"); }
template <> inline std::string format_geometry_suffix<Geometry3D>(const char* fmt) { return format(fmt, "3D"); }


// ----------------------------------------------------------------------------------------------------------------------
// Register vectors of something

template <typename T>
struct VectorFromSequence {
    VectorFromSequence() {
        boost::python::converter::registry::push_back(&convertible, &construct, boost::python::type_id<std::vector<T>>());
    }
    static void* convertible(PyObject* obj) {
        if (!PySequence_Check(obj)) return NULL;
        return obj;
    }
    static void construct(PyObject* obj, boost::python::converter::rvalue_from_python_stage1_data* data) {
        void* storage = ((boost::python::converter::rvalue_from_python_storage<std::vector<T>>*)data)->storage.bytes;
        auto seq = py::object(py::handle<>(py::borrowed(obj)));
        py::stl_input_iterator<T> begin(seq), end;
        std::vector<T>* result = new(storage) std::vector<T>();
        result->reserve(py::len(seq)); for (auto iter = begin; iter != end; ++iter) result->push_back(*iter);
        data->convertible = storage;
    }
};

template <typename T>
static std::string str__vector_of(const std::vector<T>& self) {
    std::string result = "[";
    int i = int(self.size())-1;
    for (auto v: self) {
        result += py::extract<std::string>(py::object(v).attr("__repr__")());
        result += (i)? ", " : "";
        --i;
    }
    return result + "]";
}

template <typename T>
static inline py::class_<std::vector<T>, shared_ptr<std::vector<T>>> register_vector_of(const std::string& name) {
    VectorFromSequence<T>();
    py::class_<std::vector<T>, shared_ptr<std::vector<T>>> cls((name+"_list").c_str(), py::no_init); cls
        .def(py::vector_indexing_suite<std::vector<T>>())
        .def("__repr__", &str__vector_of<T>)
        .def("__str__", &str__vector_of<T>)
    ;
    py::scope scope;
    py::delattr(scope, py::str(name+"_list"));
    return cls;
}


// ----------------------------------------------------------------------------------------------------------------------
// Get space names
template <typename SpaceT> static inline std::string spaceName();
template <> inline std::string spaceName<Geometry2DCartesian>() { return "Cartesian2D"; }
template <> inline std::string spaceName<Geometry2DCylindrical>() { return "Cylindrical"; }
template <> inline std::string spaceName<Geometry3D>() { return "Cartesian3D"; }
template <> inline std::string spaceName<void>() { return ""; }

template <typename SpaceT> static inline std::string spaceSuffix();
template <> inline std::string spaceSuffix<Geometry2DCartesian>() { return "2D"; }
template <> inline std::string spaceSuffix<Geometry2DCylindrical>() { return "Cyl"; }
template <> inline std::string spaceSuffix<Geometry3D>() { return "3D"; }
template <> inline std::string spaceSuffix<void>() { return ""; }


// ----------------------------------------------------------------------------------------------------------------------
/// Class for setting logging configuration
struct LoggingConfig
{
    py::object getLoggingColor() const;
    void setLoggingColor(std::string color);

    py::object getLoggingDest() const;
    void setLoggingDest(py::object dest);

    LogLevel getLogLevel() const { return maxLoglevel; }
    void setLogLevel(LogLevel level) { if (!forcedLoglevel) maxLoglevel = level; }

    void forceLogLevel(LogLevel level) { maxLoglevel = level; }

    std::string __str__() const;

    std::string __repr__() const;
};


/// Config class
struct Config
{
    std::string axes_name() const;

    void set_axes(std::string axis);

    bool getUfuncIgnoreError() const;
    void setUfuncIgnoreError(bool value);

    std::string __str__() const;

    std::string __repr__() const;
};

extern PLASK_PYTHON_API AxisNames current_axes;

inline AxisNames* getCurrentAxes() {
    return &current_axes;
}

// ----------------------------------------------------------------------------------------------------------------------
// Helpers for parsing kwargs

namespace detail
{
    template <size_t i>
    static inline void _parse_kwargs(py::list& PLASK_UNUSED(arglist), py::dict& PLASK_UNUSED(kwargs)) {}

    template <size_t i, typename... Names>
    static inline void _parse_kwargs(py::list& arglist, py::dict& kwargs, const std::string& name, const Names&... names) {
        py::object oname(name);
        if (kwargs.has_key(oname))
        {
            if (i < std::size_t(py::len(arglist))) {
                throw name;
            } else {
                arglist.append(kwargs[oname]);
                py::delitem(kwargs, oname);
            }
        }
        _parse_kwargs<i+1>(arglist, kwargs, names...);
    }
}

/// Helper for parsing arguments in raw_function
template <typename... Names>
static inline void parseKwargs(const std::string& fname, py::tuple& args, py::dict& kwargs, const Names&... names) {
    kwargs = kwargs.copy();
    py::list arglist(args);
    try {
        detail::_parse_kwargs<0>(arglist, kwargs, names...);
    } catch (const std::string& name) {
        throw TypeError(u8"{0}() got multiple values for keyword argument '{1}'", fname, name);
    }
    if (py::len(arglist) != sizeof...(names))
        throw TypeError(u8"{0}() takes exactly {1} non-keyword arguments ({2} given)", fname, sizeof...(names), py::len(arglist));
    args = py::tuple(arglist);
}

/// Convert Python dict to std::map
template <typename TK, typename TV>
std::map<TK,TV> dict_to_map(PyObject* obj) {
    std::map<TK,TV> map;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(obj, &pos, &key, &value)) {
        map[py::extract<TK>(key)] = py::extract<TV>(value);
    }
    return map;
}

template <typename TK, typename TV>
std::map<TK,TV> dict_to_map(const py::object& obj) {
    return dict_to_map<TK,TV>(obj.ptr());
}

// ----------------------------------------------------------------------------------------------------------------------
// Parallel locking

extern PLASK_PYTHON_API OmpNestLock python_omp_lock;

// ----------------------------------------------------------------------------------------------------------------------
// Virtual functions overriding

/**
 * Base class for methods that can be overriden form Python
 */
template <typename T>
struct Overriden
{
    PyObject* self;

    Overriden() {}

    Overriden(PyObject* self): self(self) {}

    bool overriden(char const* name) const {
        py::converter::registration const& r = py::converter::registered<T>::converters;
        PyTypeObject* class_object = r.get_class_object();
        if (self) {
            py::handle<> mh(PyObject_GetAttrString(self, const_cast<char*>(name)));
            if (mh && PyMethod_Check(mh.get())) {
                PyMethodObject* mo = (PyMethodObject*)mh.get();
                PyObject* borrowed_f = nullptr;
                if(mo->im_self == self && class_object->tp_dict != 0)
                    borrowed_f = PyDict_GetItemString(class_object->tp_dict, const_cast<char*>(name));
                if (borrowed_f != mo->im_func) return true;
            }
        }
        return false;
    }

    bool overriden_no_recursion(char const* name) const {
        py::converter::registration const& r = py::converter::registered<T>::converters;
        PyTypeObject* class_object = r.get_class_object();
        if (self) {
            py::handle<> mh(PyObject_GetAttrString(self, const_cast<char*>(name)));
            if (mh && PyMethod_Check(mh.get())) {
                PyMethodObject* mo = (PyMethodObject*)mh.get();
                PyObject* borrowed_f = nullptr;
                if(mo->im_self == self && class_object->tp_dict != 0)
                    borrowed_f = PyDict_GetItemString(class_object->tp_dict, const_cast<char*>(name));
                if (borrowed_f != mo->im_func) {
                    PyFrameObject* frame = PyEval_GetFrame();
                    if (frame == nullptr) return true;
                    bool result = true;
                    PyCodeObject* f_code =
                        #if PY_VERSION_HEX >= 0x030900B1
                            PyFrame_GetCode(frame);
                        #else
                            frame->f_code;
                        #endif
                    PyCodeObject* method_code = (PyCodeObject*)((PyFunctionObject*)mo->im_func)->func_code;
                    #if PY_VERSION_HEX >= 0x030b0000
                        if (f_code == method_code && f_code->co_argcount > 0) {
                            PyObject* f_locals = PyFrame_GetLocals(frame);
                            PyObject* co_varnames = PyCode_GetVarnames(f_code);
                            if (PyDict_GetItem(f_locals, PyTuple_GetItem(co_varnames, 0)) == self)
                                result = false;
                            Py_XDECREF(co_varnames);
                            Py_XDECREF(f_locals);
                        }
                    #else
                        if (f_code == method_code && frame->f_localsplus[0] == self)
                            result = false;
                        #if PY_VERSION_HEX >= 0x030900B1
                            Py_XDECREF(f_code);
                        #endif
                    #endif
                    return result;
                }
            }
        }
        return false;
    }

    template <typename R, typename... Args>
    inline R call_python(const char* name, Args... args) const {
        OmpLockGuard<OmpNestLock> lock(python_omp_lock);
        if (overriden(name)) {
            return py::call_method<R>(self, name, args...);
        }
        py::handle<> __class__(PyObject_GetAttrString(self, "__class__"));
        py::handle<> __name__(PyObject_GetAttrString(__class__.get(), "__name__"));
        throw AttributeError("'{}' object has not attribute '{}'",
                             std::string(py::extract<std::string>(py::object(__name__))),
                             name);
    }

};

// ----------------------------------------------------------------------------------------------------------------------
// Helper for XML reads

/// Evalueate common Python types
inline static py::object eval_common_type(const std::string& value) {
    if (value == "" || value == "None") return py::object();
    if (value == "yes" || value == "true" || value == "True") return py::object(true);
    if (value == "no" || value == "false" || value == "False") return py::object(false);
    try {
        py::object val = py::eval(value.c_str());
        if (PyLong_Check(val.ptr()) || PyFloat_Check(val.ptr()) || PyComplex_Check(val.ptr()) ||
            PyTuple_Check(val.ptr()) || PyList_Check(val.ptr()))
            return val;
        else
            return py::str(value);
    } catch (py::error_already_set&) {
        PyErr_Clear();
        return py::str(value);
    }
}

/**
 * Remove indentation of the Python part (based on the indentation of the first line)
 * \param text text to fix
 * \param xmlline line number for error messages
 * \param tag tag name for error messages
 */
void removeIndent(std::string& text, unsigned xmlline, const char* tag = nullptr);


/**
 * Read Python code from reader (either for eval or exec).
 * \param reader XML reader
 * \param manager XPL manager
 * \param exec if \c false the code is compiled only for eval
 * \return compiled PyCodeObject
 */
PyCodeObject* compilePythonFromXml(XMLReader& reader, Manager& manager, bool exec = true);

}} // namespace plask::python

#endif // PLASK__PYTHON_GLOBALS_H
