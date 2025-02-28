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
#ifndef PLASK__GEOMETRY_TRANSFORM_H
#define PLASK__GEOMETRY_TRANSFORM_H

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/bind.hpp>
#include "object.hpp"
//#include <functional>

namespace plask {

/**
 * Template of base class for all transform nodes.
 * Transform node has exactly one child node and represent object which is equal to child after transform.
 * @tparam dim number of dimensions of this object
 * @tparam Child_Type type of child, can be in space with different number of dimensions than this is (in such case see
 * @ref GeometryObjectTransformSpace).
 * @ingroup GEOMETRY_OBJ
 */
template <int dim, typename Child_Type = GeometryObjectD<dim>>
struct GeometryObjectTransform : public GeometryObjectD<dim> {
    typedef typename GeometryObjectD<dim>::DVec DVec;
    typedef typename GeometryObjectD<dim>::Box Box;
    typedef Child_Type ChildType;

    explicit GeometryObjectTransform(shared_ptr<ChildType> child = nullptr) : _child(child) { connectOnChildChanged(); }

    explicit GeometryObjectTransform(ChildType& child)
        : _child(static_pointer_cast<ChildType>(child.shared_from_this())) {
        connectOnChildChanged();
    }

    virtual ~GeometryObjectTransform() { disconnectOnChildChanged(); }

    GeometryObject::Type getType() const override { return GeometryObject::TYPE_TRANSFORM; }

    /*virtual void getLeafsToVec(std::vector< shared_ptr<const GeometryObject> >& dest) const {
        getChild()->getLeafsToVec(dest);
    }*/

    void getObjectsToVec(const GeometryObject::Predicate& predicate,
                         std::vector<shared_ptr<const GeometryObject>>& dest,
                         const PathHints* path = 0) const override {
        if (predicate(*this)) {
            dest.push_back(this->shared_from_this());
        } else {
            if (hasChild()) _child->getObjectsToVec(predicate, dest, path);
        }
    }

    /// Called by child.change signal, call this change
    virtual void onChildChanged(const GeometryObject::Event& evt) {
        this->fireChanged(evt.originalSource(), evt.flagsForParent());
    }

    /// Connect onChildChanged to current child change signal
    void connectOnChildChanged() {
        if (hasChild())
            _child->changed.connect(boost::bind(&GeometryObjectTransform<dim, Child_Type>::onChildChanged, this, _1));
    }

    /// Disconnect onChildChanged from current child change signal
    void disconnectOnChildChanged() {
        if (hasChild())
            _child->changed.disconnect(
                boost::bind(&GeometryObjectTransform<dim, Child_Type>::onChildChanged, this, _1));
    }

    /**
     * Get child.
     * @return child
     */
    inline shared_ptr<ChildType> getChild() const { return _child; }

    /**
     * Set new child.
     * This method is fast but also unsafe because it doesn't ensure that there will be no cycle in geometry graph after
     * setting the new child. It also doesn't call change signal.
     * @param child new child
     */
    void setChildUnsafe(const shared_ptr<ChildType>& child) {
        if (child == _child) return;
        disconnectOnChildChanged();
        _child = child;
        connectOnChildChanged();
    }

    /**
     * Set new child. Call change signal to inform observer about it.
     * @param child new child
     * @throw CyclicReferenceException if set new child cause inception of cycle in geometry graph
     * TODO @throw NoChildException if child is an empty pointer
     */
    void setChild(const shared_ptr<ChildType>& child) {
        // if (!child) throw NoChildException();
        if (child == _child) return;
        if (child) this->ensureCanHaveAsChild(*child);
        setChildUnsafe(child);
        this->fireChildrenChanged();
    }

    /**
     * @return @c true only if child is set (is not @c nullptr)
     */
    bool hasChild() const { return _child != nullptr; }

    /**
     * Throws NoChildException if child is not set.
     */
    void validate() const override {
        if (!hasChild()) throw NoChildException();
    }

    bool hasInSubtree(const GeometryObject& el) const override {
        return &el == this || (hasChild() && _child->hasInSubtree(el));
    }

    GeometryObject::Subtree getPathsTo(const GeometryObject& el, const PathHints* path = 0) const override {
        if (this == &el) return GeometryObject::Subtree(this->shared_from_this());
        if (!hasChild()) GeometryObject::Subtree();
        GeometryObject::Subtree e = _child->getPathsTo(el, path);
        if (e.empty()) return GeometryObject::Subtree();
        GeometryObject::Subtree result(this->shared_from_this());
        result.children.push_back(std::move(e));
        return result;
    }

    std::size_t getChildrenCount() const override { return hasChild() ? 1 : 0; }

    shared_ptr<GeometryObject> getChildNo(std::size_t child_no) const override {
        if (!hasChild() || child_no > 0) throw OutOfBoundsException("geometryObjectTransform::getChildNo", "child_no");
        return _child;
    }

    /**
     * Get copy of this, and change child in the copy,
     * @param child child for the copy
     * @return the copy of this
     */
    shared_ptr<GeometryObjectTransform<dim, Child_Type>> shallowCopyWithChild(
        const shared_ptr<ChildType>& child) const {
        shared_ptr<GeometryObjectTransform<dim, Child_Type>> result =
            static_pointer_cast<GeometryObjectTransform<dim, Child_Type>>(this->shallowCopy());
        result->setChild(child);
        result->roles = this->roles;
        return result;
    }

    shared_ptr<GeometryObject> deepCopy(
        std::map<const GeometryObject*, shared_ptr<GeometryObject>>& copied) const override {
        auto found = copied.find(this);
        if (found != copied.end()) return found->second;
        shared_ptr<GeometryObjectTransform<dim, Child_Type>> result =
            static_pointer_cast<GeometryObjectTransform<dim, Child_Type>>(this->shallowCopy());
        copied[this] = result;
        if (hasChild()) result->setChild(dynamic_pointer_cast<ChildType>(_child->deepCopy(copied)));
        return result;
    }

    shared_ptr<const GeometryObject> changedVersion(const GeometryObject::Changer& changer,
                                                    Vec<3, double>* translation = 0) const override {
        shared_ptr<GeometryObject> result(const_pointer_cast<GeometryObject>(this->shared_from_this()));
        if (changer.apply(result, translation) || !hasChild()) return result;
        shared_ptr<const GeometryObject> new_child = _child->changedVersion(changer, translation);
        if (!new_child) return shared_ptr<const GeometryObject>();  // child was deleted, so we also should be
        return new_child == _child ? result
                                   : shallowCopyWithChild(const_pointer_cast<ChildType>(
                                         dynamic_pointer_cast<const ChildType>(new_child)));
    }

    void removeAtUnsafe(std::size_t) override { _child.reset(); }

    /**
     * Conver bounding box from child's to this's coordinates.
     * @param child_bbox bouding box of child
     * @return @p child_bbox converted to this's coordinates
     */
    virtual Box fromChildCoords(const typename ChildType::Box& child_bbox) const = 0;

    Box getBoundingBox() const override {
        return this->hasChild() ? this->fromChildCoords(this->_child->getBoundingBox())
                                : Box(Primitive<dim>::ZERO_VEC, Primitive<dim>::ZERO_VEC);
    }

    void getBoundingBoxesToVec(const GeometryObject::Predicate& predicate,
                               std::vector<Box>& dest,
                               const PathHints* path) const override {
        if (predicate(*this)) {
            dest.push_back(this->getBoundingBox());
            return;
        }
        if (!hasChild()) return;
        auto child_boxes = this->_child->getBoundingBoxes(predicate, path);
        dest.reserve(dest.size() + child_boxes.size());
        for (auto& r : child_boxes) dest.push_back(this->fromChildCoords(r));
    }

    /**
     * Check if child of this has given type.
     * @param type required type
     * @return @c true only if this has a child of required @c type
     */
    inline bool childHasType(GeometryObject::Type type) const { return hasChild() && (_child->getType() == type); }

  protected:
    shared_ptr<ChildType> _child;

    /**
     * Possible implementation for getPositionsToVec, which does not change children position and is used by many
     * subclasses. This is a tamplate since this code is not proper for space changer.
     */
    template <typename ThisType>
    inline static void _getNotChangedPositionsToVec(ThisType _this,
                                                    const GeometryObject::Predicate& predicate,
                                                    std::vector<DVec>& dest,
                                                    const PathHints* path) {
        if (predicate(*_this)) {
            dest.push_back(Primitive<dim>::ZERO_VEC);
            return;
        }
        if (_this->hasChild()) _this->_child->getPositionsToVec(predicate, dest, path);
    }
};

/**
 * Template of base class for all transformations which change the space between its parent and child.
 * @tparam this_dim number of dimensions of this object
 * @tparam child_dim number of dimensions of child object
 * @tparam ChildType type of child, should be in space with @a child_dim number of dimensions
 * @ingroup GEOMETRY_OBJ
 */
template <int this_dim, int child_dim = 5 - this_dim, typename ChildType = GeometryObjectD<child_dim>>
struct GeometryObjectTransformSpace : public GeometryObjectTransform<this_dim, ChildType> {
    typedef typename ChildType::Box ChildBox;
    typedef typename ChildType::DVec ChildVec;
    typedef typename GeometryObjectTransform<this_dim, ChildType>::DVec DVec;
    using GeometryObjectTransform<this_dim, ChildType>::getChild;

    explicit GeometryObjectTransformSpace(shared_ptr<ChildType> child = shared_ptr<ChildType>())
        : GeometryObjectTransform<this_dim, ChildType>(child) {}

    /// @return TYPE_SPACE_CHANGER
    GeometryObject::Type getType() const override { return GeometryObject::TYPE_SPACE_CHANGER; }

    /*virtual std::vector< std::tuple<shared_ptr<const GeometryObject>, DVec> > getLeafsWithTranslations() const {
        std::vector< shared_ptr<const GeometryObject> > v = getChild()->getLeafs();
        std::vector< std::tuple<shared_ptr<const GeometryObject>, DVec> > result(v.size());
        std::transform(v.begin(), v.end(), result.begin(), [](shared_ptr<const GeometryObject> e) {
            return std::make_pair(e, Primitive<this_dim>::NAN_VEC);
        });
        return result;
    }*/
};

/**
 * Represent geometry object equal to its child translated by vector.
 * @ingroup GEOMETRY_OBJ
 */
template <int dim> struct PLASK_API Translation : public GeometryObjectTransform<dim> {
    typedef GeometryObjectTransform<dim> BaseClass;

    static const char* NAME;

    std::string getTypeName() const override;

    typedef typename BaseClass::ChildType ChildType;

    /// Vector of doubles type in space on this, vector in space with dim number of dimensions.
    typedef typename BaseClass::DVec DVec;

    /// Box type in space on this, rectangle in space with dim number of dimensions.
    typedef typename BaseClass::Box Box;

    using BaseClass::getChild;

    /**
     * Translation vector.
     */
    DVec translation;

    // Translation(const Translation<dim>& translation) = default;

    /**
     * @param child child geometry object, object to translate
     * @param translation translation
     */
    explicit Translation(shared_ptr<GeometryObjectD<dim>> child = shared_ptr<GeometryObjectD<dim>>(),
                         const DVec& translation = Primitive<dim>::ZERO_VEC)
        : BaseClass(child), translation(translation) {}

    explicit Translation(GeometryObjectD<dim>& child, const DVec& translation = Primitive<dim>::ZERO_VEC)
        : BaseClass(child), translation(translation) {}

    /**
     * Construct new translation which:
     * - if child is translation it is equal to Translation(child_or_translation->getChild(),
     * child_or_translation->translation + translation)
     * - in other it is equal to Translation(child_or_translation, translation)
     * @param child_or_translation child, potential Translation
     * @param translation extra translation
     * @return constructed translation
     */
    static shared_ptr<Translation<dim>> compress(
        shared_ptr<GeometryObjectD<dim>> child_or_translation = shared_ptr<GeometryObjectD<dim>>(),
        const DVec& translation = Primitive<dim>::ZERO_VEC);

    shared_ptr<Material> getMaterial(const DVec& p) const override;

    bool contains(const DVec& p) const override;

    // TODO to use (impl. is good) or remove
    /*virtual bool intersects(const Box& area) const {
        return getChild()->intersects(area.translated(-translation));
    }*/

    using GeometryObjectTransform<dim>::getPathsTo;

    GeometryObject::Subtree getPathsAt(const DVec& point, bool all = false) const override;

    /*virtual void getLeafsInfoToVec(std::vector< std::tuple<shared_ptr<const GeometryObject>, Box, DVec> >& dest, const
    PathHints* path = 0) const { const std::size_t old_size = dest.size(); getChild()->getLeafsInfoToVec(dest, path);
        for (auto i = dest.begin() + old_size; i != dest.end(); ++i) {
            std::get<1>(*i).translate(translation);
            std::get<2>(*i) += translation;
        }
    }*/

    Box fromChildCoords(const typename ChildType::Box& child_bbox) const override;

    /*virtual std::vector< std::tuple<shared_ptr<const GeometryObject>, DVec> > getLeafsWithTranslations() const {
        std::vector< std::tuple<shared_ptr<const GeometryObject>, DVec> > result =
    getChild()->getLeafsWithTranslations(); for (std::tuple<shared_ptr<const GeometryObject>, DVec>& r: result)
    std::get<1>(r) += translation; return result;
    }*/

    virtual void getPositionsToVec(const GeometryObject::Predicate& predicate,
                                   std::vector<DVec>& dest,
                                   const PathHints* path = 0) const override;

    /**
     * Get shallow copy of this.
     * @return shallow copy of this
     */
    shared_ptr<Translation<dim>> copyShallow() const {
        return shared_ptr<Translation<dim>>(new Translation<dim>(getChild(), translation));
    }

    shared_ptr<GeometryObject> shallowCopy() const override;

    virtual shared_ptr<const GeometryObject> changedVersion(const GeometryObject::Changer& changer,
                                                            Vec<3, double>* translation = 0) const override;

    /**
     * Get shallow copy of this with diffrent translation.
     * @param new_translation translation vector of copy
     */
    shared_ptr<Translation<dim>> copyShallow(const DVec& new_translation) const {
        return shared_ptr<Translation<dim>>(new Translation<dim>(getChild(), new_translation));
    }

    void writeXMLAttr(XMLWriter::Element& dest_xml_object, const AxisNames& axes) const override;

    // void extractToVec(const GeometryObject::Predicate &predicate, std::vector< shared_ptr<const GeometryObjectD<dim>
    // > >& dest, const PathHints *path) const;

    void addPointsAlongToSet(std::set<double>& points,
                        Primitive<3>::Direction direction,
                        unsigned max_steps,
                        double min_step_size) const override;

    void addLineSegmentsToSet(std::set<typename GeometryObjectD<dim>::LineSegment>& segments,
                              unsigned max_steps,
                              double min_step_size) const override;
};

template <> void Translation<2>::writeXMLAttr(XMLWriter::Element& dest_xml_object, const AxisNames& axes) const;
template <> void Translation<3>::writeXMLAttr(XMLWriter::Element& dest_xml_object, const AxisNames& axes) const;

PLASK_API_EXTERN_TEMPLATE_STRUCT(Translation<2>)
PLASK_API_EXTERN_TEMPLATE_STRUCT(Translation<3>)

}  // namespace plask

#endif  // PLASK__GEOMETRY_TRANSFORM_H
