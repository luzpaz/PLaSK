#include "geometry.h"
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <plask/geometry/primitives.h>

namespace plask { namespace python {

// Default constructor wrap
static shared_ptr<Box2d> Box2d_constructor_default() {
    return shared_ptr<Box2d> { new Box2d() };
}
// Init constructor wrap
static shared_ptr<Box2d> Box2d_constructor_2vec(const Vec<2,double>& lower, const Vec<2,double>& upper) {
    shared_ptr<Box2d> R { new Box2d(lower, upper) };
    R->fix();
    return R;
}
// Init constructor wrap
static shared_ptr<Box2d> Box2d_constructor_4numbers(double x1, double y1, double x2, double y2) {
    shared_ptr<Box2d> R { new Box2d(Vec<2,double>(x1,y1), Vec<2,double>(x2,y2)) };
    R->fix();
    return R;
}
// __str__(v)
static std::string Box2d__str__(const Box2d& to_print) {
    std::stringstream out;
    out << to_print;
    return out.str();
}
// __repr__(v)
static std::string Box2d__repr__(const Box2d& to_print) {
    std::stringstream out;
    out << "Box2D(" << to_print.lower.c0 << ", " << to_print.lower.c1 << ", "
                    << to_print.upper.c0 << ", " << to_print.upper.c1 << ")";
    return out.str();
}


// Default constructor wrap
static shared_ptr<Box3d> Box3d_constructor_default() {
    return shared_ptr<Box3d> { new Box3d() };
}
// Init constructor wrap
static shared_ptr<Box3d> Box3d_constructor_2vec(const Vec<3,double>& lower, const Vec<3,double>& upper) {
    shared_ptr<Box3d> R { new Box3d(lower, upper) };
    R->fix();
    return R;
}
// Init constructor wrap
static shared_ptr<Box3d> Box3d_constructor_4numbers(double x1, double y1, double z1, double x2, double y2, double z2) {
    shared_ptr<Box3d> R { new Box3d(Vec<3,double>(x1,y1,z1), Vec<3,double>(x2,y2,z2)) };
    R->fix();
    return R;
}
// __str__(v)
static std::string Box3d__str__(const Box3d& self) {
    std::stringstream out;
    out << self;
    return out.str();
}
// __repr__(v)
static std::string Box3d__repr__(const Box3d& self) {
    std::stringstream out;
    out << "Box3D(" << self.lower.c0 << ", " << self.lower.c1 << ", " << self.lower.c2 << ", "
                    << self.upper.c0 << ", " << self.upper.c1 << ", " << self.upper.c2 << ")";
    return out.str();
}

static std::string Box2d_list_str(const std::vector<Box2d>& self) {
    std::string result = "[";
    int i = self.size()-1;
    for (auto v: self) {
        result += Box2d__repr__(v) + ((i)?", ":"");
        --i;
    }
    return result + "]";
}

static std::string Box3d_list_str(const std::vector<Box3d>& self) {
    std::string result = "[";
    int i = self.size()-1;
    for (auto v: self) {
        result += Box3d__repr__(v) + ((i)?", ":"");
        --i;
    }
    return result + "]";
}

/// Register primitives to Python
void register_geometry_primitive()
{
    void (Box2d::*includeR2p)(const Vec<2,double>&) = &Box2d::include;
    void (Box2d::*includeR2R)(const Box2d&)       = &Box2d::include;

    py::class_<Box2d, shared_ptr<Box2d>>("Box2D",
        "Rectangular two-dimensional box. Provides some basic operation on boxes.\n\n"
        "Box2D()\n    create empty box\n\n"
        "Box2D(lower, upper)\n    create box with opposite corners described by 2D vectors\n\n"
        "Box2D(l1, l2, u1, u2)\n    create box with opposite corners described by two coordinates\n"
        )
        .def("__init__", py::make_constructor(&Box2d_constructor_default))
        .def("__init__", py::make_constructor(&Box2d_constructor_2vec, py::default_call_policies(), (py::arg("lower"), py::arg("upper"))))
        .def("__init__", py::make_constructor(&Box2d_constructor_4numbers, py::default_call_policies(), (py::arg("l0"), py::arg("l1"), py::arg("u0"), py::arg("u1"))))
        .def_readwrite("lower", &Box2d::lower, "Lower left corner of the box")
        .def_readwrite("upper", &Box2d::upper, "Upper right corner of the box")
        .def("fix", &Box2d::fix, "Ensure that lower[0] <= upper[0] and lower[1] <= upper[1]. Exchange components of lower and upper if necessary.")
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("inside", &Box2d::inside, py::args("point"), "Check if the point is inside the box.")
        .def("intersect", &Box2d::intersect, py::args("other"), "Check if this and the other box have common points.")
        .def("include", includeR2p, py::args("point"), "Make this box, the minimal one which include this and given point")
        .def("include", includeR2R, py::args("other"), "Make this box, the minimal one which include this and the other box.")
        .def("translated", &Box2d::translated, py::args("trans"), "Get translated copy of this box")
        .def("translate", &Box2d::translate, py::args("trans"), "Translate this box")
        .def("__str__", &Box2d__str__)
        .def("__repr__", &Box2d__repr__)
    ;


    void (Box3d::*includeR3p)(const Vec<3,double>&) = &Box3d::include;
    void (Box3d::*includeR3R)(const Box3d&)       = &Box3d::include;

    py::class_<Box3d, shared_ptr<Box3d>>("Box3D",
        "Cuboidal three-dimensional box. Provides some basic operation on boxes.\n\n"
        "Box3D()\n    create empty box\n\n"
        "Box3D(lower, upper)\n    create box with opposite corners described by 3D vectors\n\n"
        "Box3D(l1, l2, l3, u1, u2, u3)\n    create box with opposite corners described by three coordinates\n"
        )
        .def("__init__", py::make_constructor(&Box3d_constructor_default))
        .def("__init__", py::make_constructor(&Box3d_constructor_2vec, py::default_call_policies(), (py::arg("lower"), py::arg("upper"))))
        .def("__init__", py::make_constructor(&Box3d_constructor_4numbers, py::default_call_policies(), (py::arg("l0"), py::arg("l1"), py::arg("l2"), py::arg("u0"), py::arg("u1"), py::arg("u2"))))
        .def_readwrite("lower", &Box3d::lower, "Closer lower left corner of the box")
        .def_readwrite("upper", &Box3d::upper, "Farther upper right corner of the box")
        .def("fix", &Box3d::fix, "Ensure that lower[0] <= upper.c0, lower[1] <= upper[1], and lower[2] <= upper[3].  Exchange components of lower and upper if necessary.")
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("inside", &Box3d::inside, "Check if the point is inside the box.")
        .def("intersect", &Box3d::intersect, "Check if this and the other box have common points.")
        .def("include", includeR3p, (py::arg("point")), "Make this box, the minimal one which include this and given point")
        .def("include", includeR3R, (py::arg("other")), "Make this box, the minimal one which include this and the other box.")
        .def("translated", &Box2d::translated, py::args("trans"), "Get translated copy of this box")
        .def("translate", &Box2d::translate, py::args("trans"), "Translate this box")
        .def("__str__", &Box3d__str__)
        .def("__repr__", &Box3d__repr__)
    ;

    py::class_< std::vector<Box2d>, shared_ptr<std::vector<Box2d>> >("Box2D_list")
        .def(py::vector_indexing_suite<std::vector<Box2d>>())
        .def("__str__", &Box2d_list_str)
        .def("__repr__", &Box2d_list_str)
    ;

    py::class_< std::vector<Box3d>, shared_ptr<std::vector<Box3d>> >("Box3D_list")
        .def(py::vector_indexing_suite<std::vector<Box3d>>())
        .def("__str__", &Box3d_list_str)
        .def("__repr__", &Box3d_list_str)
    ;
}

}} // namespace plask::python
