#include "geometry.h"
#include <boost/python/raw_function.hpp>
#include "../../util/raw_constructor.h"

#include <plask/geometry/stack.h>


namespace plask { namespace python {

template <typename StackT>
PathHints::Hint Stack_push_back(py::tuple args, py::dict kwargs) {
    parseKwargs("append", args, kwargs, "self", "item");
    StackT* self = py::extract<StackT*>(args[0]);
    shared_ptr<typename StackT::ChildType> child = py::extract<shared_ptr<typename StackT::ChildType>>(args[1]);
    if (py::len(kwargs) == 0)
        return self->push_back(child);
    else
        return self->push_back(child, py::extract<typename StackT::ChildAligner>(kwargs));
}

template <typename StackT>
PathHints::Hint Stack_push_front(py::tuple args, py::dict kwargs) {
    parseKwargs("prepend", args, kwargs, "self", "item");
    StackT* self = py::extract<StackT*>(args[0]);
    shared_ptr<typename StackT::ChildType> child = py::extract<shared_ptr<typename StackT::ChildType>>(args[1]);
    if (py::len(kwargs) == 0)
        return self->push_front(child);
    else
        return self->push_front(child, py::extract<typename StackT::ChildAligner>(kwargs));
}

template <typename StackT>
PathHints::Hint Stack_insert(py::tuple args, py::dict kwargs) {
    parseKwargs("insert", args, kwargs, "self", "item", "index");
    StackT* self = py::extract<StackT*>(args[0]);
    shared_ptr<typename StackT::ChildType> child = py::extract<shared_ptr<typename StackT::ChildType>>(args[1]);
    size_t pos = py::extract<size_t>(args[2]);
    if (py::len(kwargs) == 0)
        return self->insert(child, pos);
    else
        return self->insert(child, pos, py::extract<typename StackT::ChildAligner>(kwargs));
}

template <int dim>
shared_ptr<StackContainer<dim>> Stack__init__(const py::tuple& args, py::dict kwargs) {
    kwargs = kwargs.copy();
    double shift = 0.;
    if (py::len(args) > 1) {
        if (kwargs.has_key("shift"))
            throw TypeError("__init__() got multiple values for keyword argument 'shift'");
        shift = py::extract<double>(args[1]);
        if (py::len(args) > 2)
            throw TypeError("__init__() takes at most 2 non-keyword arguments ({0} given)", py::len(args));
    } else if (kwargs.has_key("shift")) {
        shift = py::extract<double>(kwargs["shift"]);
        py::delitem(kwargs, py::str("shift"));
    }
    if (py::len(kwargs) == 0)
        return plask::make_shared<StackContainer<dim>>(shift);
    else
        return plask::make_shared<StackContainer<dim>>(shift, py::extract<typename StackContainer<dim>::ChildAligner>(kwargs));
}

template <int dim>
shared_ptr<MultiStackContainer<plask::StackContainer<dim>>> MultiStack__init__(const py::tuple& args, py::dict kwargs) {
    kwargs = kwargs.copy();
    double shift;
    size_t repeat;
    if (py::len(args) > 1) {
        if (kwargs.has_key("repeat"))
            throw TypeError("__init__() got multiple values for keyword argument 'repeat'");
        repeat = py::extract<size_t>(args[1]);
    } else if (kwargs.has_key("repeat")) {
        repeat = py::extract<size_t>(kwargs["repeat"]);
        py::delitem(kwargs, py::str("repeat"));
    } else
        throw TypeError("__init__() takes at least 2 arguments ({0} given)", py::len(args));
    if (py::len(args) > 2) {
        if (kwargs.has_key("shift"))
            throw TypeError("__init__() got multiple values for keyword argument 'shift'");
        shift = py::extract<double>(args[2]);
        if (py::len(args) > 3)
            throw TypeError("__init__() takes at most 3 non-keyword arguments ({0} given)", py::len(args));
    } else if (kwargs.has_key("shift")) {
        shift = py::extract<double>(kwargs["shift"]);
        py::delitem(kwargs, py::str("shift"));
    }
    if (py::len(kwargs) == 0)
        return plask::make_shared<MultiStackContainer<plask::StackContainer<dim>>>(repeat, shift);
    else
        return plask::make_shared<MultiStackContainer<plask::StackContainer<dim>>>(repeat, shift, py::extract<typename StackContainer<dim>::ChildAligner>(kwargs));
}

void register_geometry_container_stack()
{
    // Stack container

    py::class_<StackContainer<2>, shared_ptr<StackContainer<2>>, py::bases<GeometryObjectContainer<2>>, boost::noncopyable>("SingleStack2D",
        "SingleStack2D(shift=0, **alignment)\n\n"
        "Container that organizes its items in a vertical stack (2D version).\n\n"
        "The bottom side of the first object is located at the `shift` position in\n"
        "container local coordinates. Each consecutive object is placed on the top of\n"
        "the previous one.\n\n"
        "Args:\n"
        "    shift (float): Position in the local coordinates of the bottom of the stack.\n"
        "    alignment (dict): Horizontal alignment specifications. This dictionary can\n"
        "                      contain only one item. Its key can be ``left``, ``right``,\n"
        "                      ``#center``, and ``#`` where `#` is the horizontal axis\n"
        "                      name. The corresponding value is the position of the given\n"
        "                      edge/center/origin of the item. This alignment can be\n"
        "                      overriden while adding the objects to the stack.\n"
        "                      By default the alignment is ``{'left': 0}``.\n"
        "See also:\n"
        "    Function :func:`plask.geometry.Stack2D`.\n", py::no_init)
        .def("__init__", raw_constructor(Stack__init__<2>))
        .def("append", py::raw_function(&Stack_push_back<StackContainer<2>>),
             "append(item, **alignment)\n\n"
             "Append a new object to the stack.\n\n"
             "This method adds a new item to the stack and places it at its top.\n\n"
             "Args:\n"
             "    item (GeometryObject2D): Object to append to the stack.\n"
             "    alignment (dict): Horizontal alignment specifications. This dictionary can\n"
             "                      contain only one item. Its key can be ``left``, ``right``,\n"
             "                      ``#center``, and ``#`` where `#` is the horizontal axis\n"
             "                      name. By default the object is aligned according to the\n"
             "                      specification in the stack constructor.\n")
        .def("prepend", py::raw_function(&Stack_push_front<StackContainer<2>>),
             "prepend(item, **alignment)\n\n"
             "Prepend a new object to the stack.\n\n"
             "This method adds a new item to the stack and places it at its bottom.\n\n"
             "Args:\n"
             "    item (GeometryObject2D): Object to prepend to the stack.\n"
             "    alignment (dict): Horizontal alignment specifications. This dictionary can\n"
             "                      contain only one item. Its key can be ``left``, ``right``,\n"
             "                      ``#center``, and ``#`` where `#` is the horizontal axis\n"
             "                      name. By default the object is aligned according to the\n"
             "                      specification in the stack constructor.\n")
        .def("insert", py::raw_function(&Stack_insert<StackContainer<2>>),
             "insert(item, **alignment)\n\n"
             "Insert a new object to the stack.\n\n"
             "This method adds a new item to the stack and places it at the position\n"
             "specified by `index`.\n\n"
             "Args:\n"
             "    item (GeometryObject2D): Object to insert to the stack.\n"
             "    index (int): Index of the inserted item in the stack.\n"
             "    alignment (dict): Horizontal alignment specifications. This dictionary can\n"
             "                      contain only one item. Its key can be ``left``, ``right``,\n"
             "                      ``#center``, and ``#`` where `#` is the horizontal axis\n"
             "                      name. By default the object is aligned according to the\n"
             "                      specification in the stack constructor.\n")
        .def("set_zero_below", &StackContainer<2>::setZeroHeightBefore, py::arg("index"),
             "Set zero below the item with the given index.\n\n"
             "This method shifts the local coordinates of the stack vertically. The vertical\n"
             "coordinate of the stack origin is placed at the bootom egde of the item with\n"
             "the specified index.\n\n"
             "Args:\n"
             "    index (int): Index of the item to align the zero with.\n")
        .def("move_item", py::raw_function(&Container_move<StackContainer<2>>),
             "move_item(path, **alignment)\n\n"
             "Move horizontally item existing in the stack, setting its position according\n"
             "to the new aligner.\n\n"
             "Args:\n"
             "    path (Path): Path returned by :meth:`~plask.geometry.Align2D.append`\n"
             "                 specifying the object to move.\n"
             "    alignment (dict): Alignment specifications. The only key in this dictionary\n"
             "                      are can be ``left``, ``right``, ``#center``, and `#``\n"
             "                      where `#` is an axis name. The corresponding values is\n"
             "                      the positions of a given edge/center/origin of the item.\n"
             "                      Exactly one alignment for horizontal axis must be given.\n")
        .add_property("default_aligner", py::make_getter(&StackContainer<2>::default_aligner, py::return_value_policy<py::return_by_value>()),
                      py::make_setter(&StackContainer<2>::default_aligner),
                      "Default alignment for new stack items.")
    ;

    py::class_<StackContainer<3>, shared_ptr<StackContainer<3>>, py::bases<GeometryObjectContainer<3>>, boost::noncopyable>("SingleStack3D",
        "SingleStack3D(shift=0, **alignments)\n\n"
        "Container that organizes its items in a vertical stack (3D version).\n\n"
        "The bottom side of the first object is located at the `shift` position in\n"
        "container local coordinates. Each consecutive object is placed on the top of\n"
        "the previous one.\n\n"
        "Args:\n"
        "    shift (float): Position in the local coordinates of the bottom of the stack.\n"
        "    alignments (dict): Horizontal alignments specifications. Keys in this dict\n"
        "                       can be ``left``, ``right``, ``back``, ``front``,\n"
        "                       ``#center``, and ``#`` where `#` are the horizontal axis\n"
        "                       names. The corresponding value is the position of the\n"
        "                       given edge/center/origin of the item. This alignment can\n"
        "                       be overriden while adding the objects to the stack.\n"
        "                       By default the alignment is ``{'left': 0, 'back': 0}``.\n"
        "See also:\n"
        "    Function :func:`plask.geometry.Stack3D`.\n", py::no_init)
        .def("__init__", raw_constructor(Stack__init__<3>))
        .def("append", py::raw_function(&Stack_push_back<StackContainer<3>>),
             "append(item, **alignments)\n\n"
             "Append a new object to the stack.\n\n"
             "This method adds a new item to the stack and places it at its top.\n\n"
             "Args:\n"
             "    item (GeometryObject3D): Object to append to the stack.\n"
             "    alignments (dict): Horizontal alignments specifications. Keys in this dict\n"
             "                       can be ``left``, ``right``, ``back``, ``front``,\n"
             "                       ``#center``, and ``#`` where `#` are the horizontal axis\n"
             "                       names. The corresponding value is the position of the\n"
             "                       given edge/center/origin of the item. By default the\n"
             "                       object is aligned according to the specification in the\n"
             "                       stack constructor.\n")
        .def("prepend", py::raw_function(&Stack_push_front<StackContainer<3>>),
             "prepend(item, **alignments)\n\n"
             "Prepend a new object to the stack.\n\n"
             "This method adds a new item to the stack and places it at its bottom.\n\n"
             "Args:\n"
             "    item (GeometryObject3D): Object to prepend to the stack.\n"
             "    alignments (dict): Horizontal alignments specifications. Keys in this dict\n"
             "                       can be ``left``, ``right``, ``back``, ``front``,\n"
             "                       ``#center``, and ``#`` where `#` are the horizontal axis\n"
             "                       names. The corresponding value is the position of the\n"
             "                       given edge/center/origin of the item. By default the\n"
             "                       object is aligned according to the specification in the\n"
             "                       stack constructor.\n")
        .def("insert", py::raw_function(&Stack_insert<StackContainer<3>>),
             "insert(item, **alignments)\n\n"
             "Insert a new object to the stack.\n\n"
             "This method adds a new item to the stack and places it at the position\n"
             "specified by `index`.\n\n"
             "Args:\n"
             "    item (GeometryObject3D): Object to insert to the stack.\n"
             "    index (int): Index of the inserted item in the stack.\n"
             "    alignments (dict): Horizontal alignments specifications. Keys in this dict\n"
             "                       can be ``left``, ``right``, ``back``, ``front``,\n"
             "                       ``#center``, and ``#`` where `#` are the horizontal axis\n"
             "                       names. The corresponding value is the position of the\n"
             "                       given edge/center/origin of the item. By default the\n"
             "                       object is aligned according to the specification in the\n"
             "                       stack constructor.\n")
        .def("set_zero_below", &StackContainer<3>::setZeroHeightBefore, py::arg("index"),
             "Set zero below the item with the given index.\n\n"
             "This method shifts the local coordinates of the stack vertically. The vertical\n"
             "coordinate of the stack origin is placed at the bootom egde of the item with\n"
             "the specified index.\n\n"
             "Args:\n"
             "    index (int): Index of the item to align the zero with.\n")
        .def("move_item", py::raw_function(&Container_move<StackContainer<3>>),
             "move_item(path, **alignments)\n\n"
             "Move horizontally item existing in the stack, setting its position according\n"
             "to the new aligner.\n\n"
             "Args:\n"
             "    path (Path): Path returned by :meth:`~plask.geometry.Align2D.append`\n"
             "                 specifying the object to move.\n"
             "    alignments (dict): Horizontal alignments specifications. Keys in this dict\n"
             "                       can be ``left``, ``right``, ``back``, ``front``,\n"
             "                       ``#center``, and ``#`` where `#` are the horizontal axis\n"
             "                       names. The corresponding value is the position of the\n"
             "                       given edge/center/origin of the item. Exactly one\n"
             "                       alignment for each horizontal axis must be given.\n")
        .add_property("default_aligner", py::make_getter(&StackContainer<3>::default_aligner, py::return_value_policy<py::return_by_value>()),
                      py::make_setter(&StackContainer<3>::default_aligner), "Default alignment for new stack items")
    ;

    // Multi-stack constainer

    py::class_<MultiStackContainer<plask::StackContainer<2>>, shared_ptr<MultiStackContainer<plask::StackContainer<2>>>, py::bases<StackContainer<2>>, boost::noncopyable>("MultiStack2D",
        "MultiStack2D(repeat=1, shift=0, **alignment)\n\n"
        "Stack container that repeats it contents (2D version).\n\n"
        "The bottom side of the first object is located at the `shift` position in\n"
        "container local coordinates. Each consecutive object is placed on the top of\n"
        "the previous one. Then the whole stack is repeated *repeat* times.\n\n"
        "Args:\n"
        "    repeat (int): Number of the stack contents repetitions.\n"
        "    shift (float): Position in the local coordinates of the bottom of the stack.\n"
        "    alignment (dict): Horizontal alignment specifications. This dictionary can\n"
        "                      contain only one item. Its key can be ``left``, ``right``,\n"
        "                      ``#center``, and ``#`` where `#` is the horizontal axis\n"
        "                      name. The corresponding value is the position of the given\n"
        "                      edge/center/origin of the item. This alignment can be\n"
        "                      overriden while adding the objects to the stack.\n"
        "                      By default the alignment is ``{'left': 0}``.\n"
        "See also:\n"
        "   Function :func:`plask.geometry.Stack2D`.\n", py::no_init)
        .def("__init__", raw_constructor(MultiStack__init__<2>))
        .add_property("repeat", &MultiStackContainer<plask::StackContainer<2>>::getRepeatCount, &MultiStackContainer<plask::StackContainer<2>>::setRepeatCount,
                      "Number of repeats of the stack contents.")
    ;

    py::class_<MultiStackContainer<plask::StackContainer<3>>, shared_ptr<MultiStackContainer<StackContainer<3>>>, py::bases<StackContainer<3>>, boost::noncopyable>("MultiStack3D",
        "MultiStack3D(repeat=1, shift=0, **alignments)\n\n"
        "Stack container that repeats it contents (3D version).\n\n"
        "The bottom side of the first object is located at the `shift` position in\n"
        "container local coordinates. Each consecutive object is placed on the top of\n"
        "the previous one. Then the whole stack is repeated *repeat* times.\n\n"
        "Args:\n"
        "    repeat (int): Number of the stack contents repetitions.\n"
        "    shift (float): Position in the local coordinates of the bottom of the stack.\n"
        "    alignments (dict): Horizontal alignments specifications. Keys in this dict\n"
        "                       can be ``left``, ``right``, ``back``, ``front``,\n"
        "                       ``#center``, and ``#`` where `#` are the horizontal axis\n"
        "                       names. The corresponding value is the position of the\n"
        "                       given edge/center/origin of the item. This alignment can\n"
        "                       be overriden while adding the objects to the stack.\n"
        "                       By default the alignment is ``{'left': 0, 'back': 0}``.\n"
        "See also:\n"
        "   Function :func:`plask.geometry.Stack3D`.\n", py::no_init)
        .def("__init__", raw_constructor(MultiStack__init__<3>))
        .add_property("repeat", &MultiStackContainer<plask::StackContainer<3>>::getRepeatCount, &MultiStackContainer<StackContainer<3>>::setRepeatCount,
                      "Number of repeats of the stack contents.")
    ;

    // Shelf (horizontal stack)

    py::class_<ShelfContainer2D, shared_ptr<ShelfContainer2D>, py::bases<GeometryObjectContainer<2>>, boost::noncopyable>("Shelf2D",
        "Shelf2D(shift=0)\n\n"
        "2D container that organizes its items one next to another.\n\n"
        "The objects are placed in this container like books on a bookshelf.\n"
        "The left side of the first object is located at the `shift` position in\n"
        "container local coordinates. Each consecutive object is placed to the right of\n"
        "the previous one. All the items are vertically aligned according to its bottom\n"
        "edge.\n\n"
        "Args:\n"
        "    shift (float): Position in the local coordinates of the left side of the\n"
        "                   shelf.\n\n"
        "See also:\n"
        "   Function :func:`plask.geometry.Shelf`.\n",
        py::init<double>((py::arg("shift")=0.)))
        .def("append", &ShelfContainer2D::push_back, (py::arg("item")),
             "Append a new object to the shelf.\n\n"
             "This method adds a new item to the shelf and places it at its right.\n\n"
             "Args:\n"
             "    item (GeometryObject2D): Object to append to the stack.\n")
        .def("prepend", &ShelfContainer2D::push_front, (py::arg("item")),
             "Prepend a new object to the shelf.\n\n"
             "This method adds a new item to the shelf and places it at its left.\n\n"
             "Args:\n"
             "    item (GeometryObject2D): Object to append to the stack.\n")
        .def("insert", &ShelfContainer2D::insert, (py::arg("item"), "pos"),
             "Insert a new object to the shelf.\n\n"
             "This method adds a new item to the shelf and places it at the position\n"
             "specified by `index`.\n\n"
             "Args:\n"
             "    item (GeometryObject2D): Object to insert to the shelf.\n"
             "    index (int): Index of the inserted item in the stack.\n")
        .def("set_zero_before", &StackContainer<3>::setZeroHeightBefore, py::arg("index"),
             "Set zero to the left of the item with the given index.\n\n"
             "This method shifts the local coordinates of the shelf horizontally.\n"
             "The horizontal coordinate of the shelf origin is placed at the left egde\n"
             "of the item with the specified index.\n\n"
             "Args:\n"
             "    index (int): Index of the item to align the zero with.\n")
        .def("append_gap", &ShelfContainer2D::addGap, py::arg("size"),
             "Add a gap to the end of the shelf.\n\n"
             "This method adds a gap to the end of the shelf. All consecutive items will be\n"
             "separated by the specified width from the previous ones.\n\n"
             "Args:\n"
             "    size (float): Size of the gap [µm].\n")
        .add_property("flat", &ShelfContainer2D::isFlat,
            "True if all items has the same height (the shelf top edge is flat).")
    ;
    py::scope().attr("Shelf") = py::scope().attr("Shelf2D");

    py::class_<MultiStackContainer<plask::ShelfContainer2D>, shared_ptr<MultiStackContainer<ShelfContainer2D>>, py::bases<ShelfContainer2D>, boost::noncopyable>("MultiShelf2D",
        "MultiShelf2D(repeat=1, shift=0)\n\n"
        "Shelf container that repeats its contents.\n\n"
        "The objects are placed in this container like books on a bookshelf.\n"
        "The left side of the first object is located at the `shift` position in\n"
        "container local coordinates. Each consecutive object is placed to the right\n"
        "of the previous one. Then the whole shelf is repeated *repeat* times. All the"
        "items\n are vertically aligned according to its bottom edge.\n\n"
        "Args:\n"
        "    repeat (int): Number of the shelf contents repetitions.\n"
        "    shift (float): Position in the local coordinates of the left side of the\n"
        "                   shelf.\n\n"
        "See also:\n"
        "   Function :func:`plask.geometry.Shelf`.\n",
        py::init<size_t, double>((py::arg("repeat")=1, py::arg("shift")=0.)))
        .add_property("repeat", &MultiStackContainer<plask::ShelfContainer2D>::getRepeatCount, &MultiStackContainer<ShelfContainer2D>::setRepeatCount,
                      "Number of repeats of the shelf contents.")
    ;


}



}} // namespace plask::python
