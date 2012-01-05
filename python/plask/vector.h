#ifndef PLASK__PYTHON_VECTOR_H
#define PLASK__PYTHON_VECTOR_H

#include <sstream>

#include <boost/python.hpp>
namespace py = boost::python;

#include <config.h>
#include <plask/vec.h>

namespace plask { namespace python {

// // v = vector[i]
// template <int dim, typename T>
// static T __getitem__(Vec<dim,T>& self, int i) {
//     if (i < 0) i = dim + i;
//     if (i >= dim || i < 0) {
//         const char message[] = "vector index out of range";
//         PyErr_SetString(PyExc_IndexError, message);
//         throw py::error_already_set();
//     }
//     return self[i];
// }
//
// // vector[i] = v
// template <int dim, typename T>
// static T __setitem__(Vec<dim,T>& self, int i, T v) {
//     if (i < 0) i = dim + i;
//     if (i >= dim || i < 0) {
//         const char message[] = "vector index out of range";
//         PyErr_SetString(PyExc_IndexError, message);
//         throw py::error_already_set();
//     }
//     self.component[i] = v;
// }
//
// // __str__(v)
// template <int dim, typename T>
// static std::string __str__(const Vec<dim,T>& to_print) {
//     std::stringstream out;
//     out << to_print;
//     return out.str();
// }
//
// // __repr__(v)
// template <int dim, typename T>
// static std::string __repr__(const Vec<dim,T>& to_print) {
//     std::stringstream out;
//     out << "vector(" << to_print << ")";
//     return out.str();
// }
//
// // v.__iter__
// template <int dim, typename T>
// struct Vec_iterator
// {
//     Vec<dim,T>* vec;
//     int i;
//
//     static Vec_iterator<dim, T> new_iterator(Vec<dim,T>& v) {
//         return Vec_iterator<dim, T>(v);
//     }
//
//     Vec_iterator(Vec<dim,T>& v) : vec(&v), i(0) {}
//
//     Vec_iterator<dim, T>* __iter__() { return this; }
//
//     T next() {
//         if (i >= dim) {
//             PyErr_SetString(PyExc_StopIteration, "No more components.");
//             boost::python::throw_error_already_set();
//         }
//         return vec->components[i++];
//     }
// };
//
// // copy v
// template <int dim, typename T>
// static Vec<dim,T> copy_vec(const Vec<dim,T>& v) {
//     return v;
// }
//
// // Register vector class to python
// template <int dim, typename T, typename Init>
// inline static py::class_<Vec<dim,T>>  py_vector_class_common(const char* name, const char* doc)
// {
//     typedef Vec<dim,T> V;
//     typedef Vec<dim,double> VR;
//     typedef Vec<dim,complex<double>> VC;
//
//     T (*dr)(const V&, const VR&) = &dot<T,double>;
//     complex<double> (*dc)(const V&, const VC&) = &dot<T,complex<double>>;
//
//     V (*c)(const V&) = &plask::conj<T>;
//
//     py::class_<V> vec_class = py::class_<V>(name, doc, Init())
//         .def_readwrite("x", &V::x)
//         .def_readwrite("y", &V::y)
//         .def_readwrite("r", &V::r)
//         .def_readwrite("z", &V::z)
//         .def("__getitem__", __getitem__<dim,T>)
//         .def("__setitem__", __getitem__<dim,T>)
//         .def("__iter__", &Vec_iterator<dim,T>::new_iterator, py::with_custodian_and_ward_postcall<0,1>())
//         .def(py::self == py::other<VR>())
//         .def(py::self == py::other<VC>())
//         .def(py::self != py::other<VR>())
//         .def(py::self != py::other<VC>())
//         .def("abs2", &abs2<dim,V>, "Squared vector magnitude (little faster than self.magnitude)")
//         .def("copy", &copy_vec<dim,T>)
//         .def("__abs__", &V::magnitude, "Vector magnitue")
//         .def(py::self + py::other<VR>())
//         .def(py::self + py::other<VC>())
//         .def(py::self - py::other<VR>())
//         .def(py::self - py::other<VC>())
//         .def( - py::self)
//         .def(py::self * T())
//         .def(T() * py::self)
//         .def(py::self += py::other<V>())
//         .def(py::self -= py::other<V>())
//         .def(py::self *= T())
//         .def("__str__", __str__<dim,T>)
//         .def("__repr__", __repr__<dim,T>)
//         .def("dot", dr, "Dot product with another vector")
//         .def("dot", dc, "Dot product with another vector")
//         .def("__mul__", dr)
//         .def("__mul__", dc)
//         .def("conjugate", c)
//         .def("conj", c)
//     ;
//
//     py::scope vec_scope = vec_class;
//
//     py::class_<Vec_iterator<dim,T>>("_Iterator", py::no_init)
//         .def("__iter__", &Vec_iterator<dim,T>::__iter__, py::return_self<>())
//         .def("next", &Vec_iterator<dim,T>::next)
//     ;
//
//     return vec_class;
// }

inline static void register_vector_h()
{
//     py_vector_class_common<2,double,py::init<double,double>>("vector2d_float",
//         "Float vector in 2D space. Create new instance as v = vector(x,y, float).")
//     ;
//
//     py_vector_class_common<2,complex<double>,py::init<complex<double>,complex<double>>>("vector2d_complex",
//         "Complex vector in 2D space. Create new instance as v = vector(x,y, complex).")
//     ;
//
//     py_vector_class_common<3,double,py::init<double,double,double>>("vector3d_float",
//         "Float vector in 3D space. Create new instance as v = vector(x,y,z, float).")
//         //.def_readwrite("phi", &Vec<3,double>::phi)
//     ;
//
//     py_vector_class_common<3,complex<double>,py::init<complex<double>,complex<double>,complex<double>>>("vector3d_complex",
//         "Complex vector in 3D space. Create new instance as v = vector(x,y,z, complex).")
//         //.def_readwrite("phi", &Vec<3,complex<double>>::phi)
//     ;
}

}} // namespace plask::python
#endif // PLASK__PYTHON_VECTOR_H
