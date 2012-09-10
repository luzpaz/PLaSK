#include "geometry.h"
#include <plask/geometry/transform.h>
#include <plask/geometry/leaf.h>
#include <plask/geometry/container.h>
#include <plask/geometry/path.h>

namespace plask { namespace python {

// Some helpful wrappers
template <int dim> struct GeometryElementD_includes {};
template <> struct GeometryElementD_includes<2> {
    static inline bool call(const GeometryElementD<2>& self, double c0, double c1) {
        return self.includes(Vec<2,double>(c0, c1));
    }
};
template <> struct GeometryElementD_includes<3> {
    static inline bool call(const GeometryElementD<3>& self, double c0, double c1, double c2) {
        return self.includes(Vec<3,double>(c0, c1, c2));
    }
};

template <int dim> struct GeometryElementD_getMaterial {};
template <> struct GeometryElementD_getMaterial<2> {
    static inline shared_ptr<Material> call(const GeometryElementD<2>& self, double c0, double c1) {
        return self.getMaterial(Vec<2,double>(c0, c1));
    }
};
template <> struct GeometryElementD_getMaterial<3> {
    static inline shared_ptr<Material> call(const GeometryElementD<3>& self, double c0, double c1, double c2) {
        return self.getMaterial(Vec<3,double>(c0, c1, c2));
    }
};

template <int dim> struct GeometryElementD_getPathsTo {};
template <> struct GeometryElementD_getPathsTo<2> {
    static inline GeometryElement::Subtree call(const GeometryElementD<2>& self, double c0, double c1) {
        return self.getPathsTo(Vec<2,double>(c0, c1));
    }
};
template <> struct GeometryElementD_getPathsTo<3> {
    static inline GeometryElement::Subtree call(const GeometryElementD<3>& self, double c0, double c1, double c2) {
        return self.getPathsTo(Vec<3,double>(c0, c1, c2));
    }
};

template <int dim>
static py::list GeometryElementD_getLeafsAsTranslations(const GeometryElementD<dim>& self, const PathHints& path) {
    py::list result;
    auto leafs = self.getLeafs(&path);
    auto translations = self.getLeafsPositions(&path);
    auto l = leafs.begin();
    auto t = translations.begin();
    for (; l != leafs.end(); ++l, ++t) {
        result.append(make_shared<Translation<dim>>(const_pointer_cast<GeometryElementD<dim>>(static_pointer_cast<const GeometryElementD<dim>>(*l)), *t));
    }
    return result;
}

template <int dim>
static py::list GeometryElementD_getElementAsTranslations(const shared_ptr<GeometryElementD<dim>>& self, const shared_ptr<GeometryElementD<dim>>& element, const PathHints& path) {
    auto translations = self->getElementInThisCoordinates(element, path);
    py::list result;
    for (auto i: translations) result.append(const_pointer_cast<Translation<dim>>(i));
    return result;
}

static py::list GeometryElement_getLeafs(const shared_ptr<GeometryElement>& self, const PathHints& path) {
    std::vector<shared_ptr<const GeometryElement>> leafs = self->getLeafs(&path);
    py::list result;
    for (auto i: leafs) result.append(const_pointer_cast<GeometryElement>(i));
    return result;
}

std::string GeometryElement__repr__(const shared_ptr<GeometryElement>& self) {
    std::stringstream out;
    try {
        py::object obj(self);
        py::object cls = obj.attr("__class__");
        std::string solver = py::extract<std::string>(cls.attr("__module__"));
        std::string name = py::extract<std::string>(cls.attr("__name__"));
        out << "<" << solver << "." << name << " object at (" << self << ")>";
    } catch (py::error_already_set) {
        PyErr_Clear();
        out << "<Unrecognized plask.geometry.GeometryElement subclass object at (" << self << ")>";
    }
    return out.str();
}

/// Initialize class GeometryElementD for Python
template <int dim> struct GeometryElementD_vector_args { static const py::detail::keywords<dim> args; };
template<> const py::detail::keywords<2> GeometryElementD_vector_args<2>::args = (py::arg("c0"), py::arg("c1"));
template<> const py::detail::keywords<3> GeometryElementD_vector_args<3>::args = (py::arg("c0"), py::arg("c1"), py::arg("c2"));
DECLARE_GEOMETRY_ELEMENT_23D(GeometryElementD, "GeometryElement", "Base class for "," geometry elements") {
    ABSTRACT_GEOMETRY_ELEMENT_23D(GeometryElementD, GeometryElement)
        .def("includes", &GeometryElementD<dim>::includes, (py::arg("point")),
             "Return True if the geometry element includes a point (in local coordinates)")
        .def("includes", &GeometryElementD_includes<dim>::call, GeometryElementD_vector_args<dim>::args,
             "Return True if the geometry element includes a point (in local coordinates)")
        .def("intersects", &GeometryElementD<dim>::intersects, (py::arg("area")),
             "Return True if the geometry element has common points (in local coordinates) with an area")
        .def("getMaterial", &GeometryElementD<dim>::getMaterial, (py::arg("point")),
             "Return material at given point, provided that it is inside the bounding box (in local coordinates) and None otherwise")
        .def("getMaterial", &GeometryElementD_getMaterial<dim>::call, GeometryElementD_vector_args<dim>::args,
             "Return material at given point, provided that it is inside the bounding box (in local coordinates) and None otherwise")
        .add_property("bbox", &GeometryElementD<dim>::getBoundingBox,
                      "Minimal rectangle which includes all points of the geometry element (in local coordinates)")
        .add_property("bbox_size", &GeometryElementD<dim>::getBoundingBoxSize,
                      "Size of the bounding box")
        .def("getLeafsPositions", (std::vector<typename Primitive<dim>::DVec>(GeometryElementD<dim>::*)(const PathHints&)const) &GeometryElementD<dim>::getLeafsPositions,
             (py::arg("path")=py::object()), "Calculate positions of all leafs (in local coordinates)")
        .def("getLeafsBBoxes", (std::vector<typename Primitive<dim>::Box>(GeometryElementD<dim>::*)(const PathHints&)const) &GeometryElementD<dim>::getLeafsBoundingBoxes,
             (py::arg("path")=py::object()), "Calculate bounding boxes of all leafs (in local coordinates)")
        .def("getLeafsAsTranslations", &GeometryElementD_getLeafsAsTranslations<dim>, (py::arg("path")=py::object()),
             "Return list of Translation objects holding all leafs")
        .def("getLeafs", &GeometryElement_getLeafs, (py::arg("path")=py::object()),
             "Return list of all leafs in the subtree originating from this element")
        .def("getElementPositions", (std::vector<typename Primitive<dim>::DVec>(GeometryElementD<dim>::*)(const GeometryElement&, const PathHints&)const) &GeometryElementD<dim>::getElementPositions,
             (py::arg("element"), py::arg("path")=py::object()), "Calculate positions of all all instances of specified element (in local coordinates)")
        .def("getElementBBoxes", (std::vector<typename Primitive<dim>::Box>(GeometryElementD<dim>::*)(const GeometryElement&, const PathHints&)const) &GeometryElementD<dim>::getElementBoundingBoxes,
             (py::arg("element"), py::arg("path")=py::object()), "Calculate bounding boxes of all instances of specified element (in local coordinates)")
        .def("getElementAsTranslations", &GeometryElementD_getElementAsTranslations<dim>,
             (py::arg("element"), py::arg("path")=py::object()), "Return Translations holding specified element")
        .def("getPathsTo", (GeometryElement::Subtree(GeometryElementD<dim>::*)(const typename GeometryElementD<dim>::DVec&)const) &GeometryElementD<dim>::getPathsTo, py::arg("point"),
             "Return subtree containing paths to all leafs covering specified point")
        .def("getPathsTo", &GeometryElementD_getPathsTo<dim>::call, GeometryElementD_vector_args<dim>::args,
             "Return subtree containing paths to all leafs covering specified point")
    ;
}

void register_geometry_element()
{
    py::enum_<GeometryElement::Type>("ElementType")
        .value("LEAF", GeometryElement::TYPE_LEAF)
        .value("TRANSFORM", GeometryElement::TYPE_TRANSFORM)
        .value("SPACE_CHANGER", GeometryElement::TYPE_SPACE_CHANGER)
        .value("CONTAINER", GeometryElement::TYPE_CONTAINER)
    ;

    py::class_<GeometryElement, shared_ptr<GeometryElement>, boost::noncopyable>("GeometryElement",
        "Base class for all geometry elements.", py::no_init)
        .add_property("type", &GeometryElement::getType)
        .def("validate", &GeometryElement::validate, "Check if the element is compete and ready for calculations")
        .def("__repr__", &GeometryElement__repr__)
        .def("__eq__", __is__<GeometryElement>)
    ;

    register_vector_of<shared_ptr<GeometryElement>>("GeometryElement");

    init_GeometryElementD<2>();
    init_GeometryElementD<3>();

}


}} // namespace plask::python
