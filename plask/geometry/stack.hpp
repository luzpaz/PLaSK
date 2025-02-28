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
#ifndef PLASK__GEOMETRY_STACK_H
#define PLASK__GEOMETRY_STACK_H

#include "primitives.hpp"
#include "container.hpp"
#include <deque>

namespace plask {

/**
 * Common code for stack containers (which have children in stack/layers).
 * @tparam dim number of space dimentions
 * @tparam growingDirection direction in which stack growing
 * @ingroup GEOMETRY_OBJ
 */
template <int dim, typename Primitive<dim>::Direction growingDirection = Primitive<dim>::DIRECTION_VERT>
struct PLASK_API StackContainerBaseImpl: public GeometryObjectContainer<dim> {

    typedef GeometryObjectContainer<dim> BaseClass;

    static constexpr typename Primitive<dim>::Direction GROWING_DIR = growingDirection;

    /// Vector of doubles type in space on this, vector in space with dim number of dimensions.
    typedef typename BaseClass::DVec DVec;

    /// Rectangle type in space on this, rectangle in space with dim number of dimensions.
    typedef typename BaseClass::Box Box;

    /// Type of this child.
    typedef typename BaseClass::ChildType ChildType;

    /// Type of translation geometry element in space of this.
    typedef typename BaseClass::TranslationT TranslationT;

    /// Type of the vector holding container children
    typedef typename BaseClass::TranslationVector TranslationVector;

    using BaseClass::children;

    /**
     * @param baseHeight height where should start first object
     */
    explicit StackContainerBaseImpl(const double baseHeight = 0.0) {
        stackHeights.push_back(baseHeight);
    }

    /**
     * Get component of position in growing direction where stack starts.
     * @return component of position in growing direction where stack starts
     */
    double getBaseHeight() const { return stackHeights.front(); }

    /**
     * Set height where should start first object. Call changed.
     * @param newBaseHeight where lower bound of the lowest object should be
     */
    void setBaseHeight(double newBaseHeight);

    /**
     * Set zero coordinate at the bottom of the specified object
     * @param index index of object which lower bound should lie at height 0
     */
    void setZeroBefore(std::size_t index);

    /**
     * Set zero coordinate at the bottom of the specified object
     * \param object object which lower bound should lie at height 0
     * \param path path to object which lower bound should lie at height 0
     */
    void setZeroBefore(const shared_ptr<ChildType>& object, const PathHints* path = 0) {
        setZeroBefore(this->getChildIndex(object, path));
    }

    /**
     * Set zero coordinate at the bottom of the specified object
     * \param object object which lower bound should lie at height 0
     * \param path path to object which lower bound should lie at height 0
     */
    void setZeroBefore(const shared_ptr<ChildType>& object, const PathHints& path) {
        setZeroBefore(this->getChildIndex(object, path));
    }

    /**
     * Align zero coordinate with the specified position of the object of specified index
     * \param index index of object which should be aligned
     * \param pos local object position to align stack zero
     */
    void alignZeroOn(std::size_t index, double pos = 0.);

    /**
     * Align zero coordinate with the zero of the specified object
     * \param object object which should be aligned
     * \param path path to object which should be aligned
     * \param pos local object position to align stack zero
     */
    void alignZeroOn(const shared_ptr<ChildType>& object, const PathHints* path = 0, double pos = 0.) {
        alignZeroOn(this->getChildIndex(object, path), pos);
    }

    /**
     * Align zero coordinate with the zero of the specified object
     * \param object object which should be aligned
     * \param path path to object which should be aligned
     * \param pos local object position to align stack zero
     */
    void alignZeroOn(const shared_ptr<ChildType>& object, const PathHints& path, double pos = 0.) {
        alignZeroOn(this->getChildIndex(object, path), pos);
    }

    std::size_t getInsertionIndexForHeight(double height) const;

    /**
     * @param height
     * @param sec_candidate[out] child which can be at given @a height
     * @return child which can be at given @a height or @c nullptr (than sec_candidate is also not set)
     */
    const shared_ptr<TranslationT> getChildForHeight(double height, shared_ptr<TranslationT>& sec_candidate) const;

    const shared_ptr<TranslationT> getChildForHeight(double height) const {
        shared_ptr<TranslationT> to_ignore;
        return getChildForHeight(height, to_ignore);
    }

    bool contains(const DVec& p) const override;

    shared_ptr<Material> getMaterial(const DVec& p) const override;

    GeometryObject::Subtree getPathsAt(const DVec& point, bool all=false) const override;

    bool removeIfTUnsafe(const std::function<bool(const shared_ptr<TranslationT>& c)>& predicate) override;

    void removeAtUnsafe(std::size_t index) override;

    /// Called by child.change signal, update heights call this change
    void onChildChanged(const GeometryObject::Event& evt) override;

    /**
     * Get height of stack.
     * @return height of stack, size of stack in growing direction
     */
    double getHeight() const {
        return stackHeights.back() - stackHeights.front();
    }

  protected:

    /**
     * stackHeights[x] is current stack heights with x first objects in it (sums of heights of first x objects),
     * stackHeights.size() = children.size() + 1 and stackHeights[0] is a base height (typically 0.0)
     */
    std::vector<double> stackHeights;

    /**
     * Calculate object vertical translation and height of stack with object @a el.
     * @param[in] elBoudingBox bounding box of geometry object (typically: for object which is or will be in stack)
     * @param[in] prev_height height of stack under an @a el
     * @param[out] el_translation up translation which should object @a el have
     * @param[out] next_height height of stack with an @a el on top (up to @a el)
     */
    void calcHeight(const Box& elBoudingBox, double prev_height, double& el_translation, double& next_height) {
        el_translation = prev_height - elBoudingBox.lower[growingDirection];
        next_height = elBoudingBox.upper[growingDirection] + el_translation;
    }

    /**
     * Calculate object vertical translation and height of stack with object @a el.
     * @param[in] el geometry object (typically: which is or will be in stack)
     * @param[in] prev_height height of stack under an @a el
     * @param[out] el_translation up translation which should object @a el have
     * @param[out] next_height height of stack with an @a el on top (up to @a el)
     */
    void calcHeight(const shared_ptr<ChildType>& el, double prev_height, double& el_translation, double& next_height) {
        calcHeight(el->getBoundingBox(), prev_height, el_translation, next_height);
    }

    /**
     * Update stack height (fragment with pointed child on top) and pointed child up translation.
     * @param child_index index of child
     */
    void updateHeight(std::size_t child_index) {
        calcHeight(children[child_index]->getChild(),
                   stackHeights[child_index],
                   children[child_index]->translation[growingDirection],
                   stackHeights[child_index+1]);
    }

    /**
     * Update stack heights and translation in stack growing direction of all children, with indexes from @a first_child_index.
     * @param first_child_index index of first child for which stackHeights should be update
     */
    void updateAllHeights(std::size_t first_child_index);

    void updateAllHeights();

    /**
     * Resize stackHeights (to be compatibile with children vector) and refresh its value from given index.
     * @param first_child_index index of first child for which stackHeights should be update
     */
    void rebuildStackHeights(std::size_t first_child_index = 0);

    void writeXMLAttr(XMLWriter::Element &dest_xml_object, const AxisNames &axes) const override;
};

PLASK_API_EXTERN_TEMPLATE_STRUCT(StackContainerBaseImpl<2, Primitive<2>::DIRECTION_VERT>)
PLASK_API_EXTERN_TEMPLATE_STRUCT(StackContainerBaseImpl<3, Primitive<3>::DIRECTION_VERT>)
PLASK_API_EXTERN_TEMPLATE_STRUCT(StackContainerBaseImpl<2, Primitive<2>::DIRECTION_TRAN>)

/**
 * Horizontal stack.
 * @ingroup GEOMETRY_OBJ
 */
struct PLASK_API ShelfContainer2D: public StackContainerBaseImpl<2, Primitive<2>::DIRECTION_TRAN> {

    typedef StackContainerBaseImpl<2, Primitive<2>::DIRECTION_TRAN> BaseClass;

    /// Type of this child.
    typedef typename BaseClass::ChildType ChildType;

    /// Type of translation geometry elment in space of this.
    typedef typename BaseClass::TranslationT TranslationT;

private:
    /// Gap which is update to make all shelf to have given, total width
    //TODO use it, support in remove
    shared_ptr<TranslationT> resizableGap;

public:

    ShelfContainer2D(double baseH = 0.0): BaseClass(baseH) {}

    static const char* NAME;

    std::string getTypeName() const override { return NAME; }

    /**
     * Check if all children have the same heights.
     * @return @c true only if all children have the same heights
     */
    bool isFlat() const;

    /**
     * Check if all children have the same heights and throw exception it's not true.
     * @throw Exception if not all children have the same heights
     */
    void ensureFlat() const {
        if (!isFlat()) throw Exception("not all items in the shelf have the same height "
                                       "(correct heights or set 'flat' to false)");
    }

    /**
     * Add children to shelf top.
     * @param el object to add
     * @return path hint, see @ref geometry_paths
     * @throw CyclicReferenceException if adding the new child cause inception of cycle in geometry graph
     */
    PathHints::Hint add(const shared_ptr<ChildType> &el) {
        if (el) this->ensureCanHaveAsChild(*el);
        return addUnsafe(el);
    }

    /**
     * Add gap to shelf top.
     * @param size size of gap
     * @return path hint, see @ref geometry_paths
     */
    PathHints::Hint addGap(double size);

    /*
     * Try to resize gap which have given index.
     * @param gap_index
     */
    //void resizeGap(std::size_t gap_index, double new_size);

    /**
     * Add child to shelf top.
     * @param el object to add
     * @return path hint, see @ref geometry_paths
     * @throw CyclicReferenceException if adding the new child cause inception of cycle in geometry graph
     */
    PathHints::Hint push_back(const shared_ptr<ChildType> &el) { return add(el); }

    /**
     * Add children to shelf top.
     * This method is fast but also unsafe because it doesn't ensure that there will be no cycle in geometry graph after adding the new child.
     * @param el object to add
     * @return path hint, see @ref geometry_paths
     */
    PathHints::Hint addUnsafe(const shared_ptr<ChildType>& el);

    /**
     * Insert children to shelf at given position.
     * This method is fast but also unsafe because it doesn't ensure that there will be no cycle in geometry graph after adding the new child.
     * @param el object to insert
     * @param pos position where (before which) child should be inserted
     * @return path hint, see @ref geometry_paths
     */
    PathHints::Hint insertUnsafe(const shared_ptr<ChildType>& el, const std::size_t pos);

    /**
     * Insert children to shelf at given position.
     * @param el object to insert
     * @param pos position where (before which) child should be inserted
     * @return path hint, see @ref geometry_paths
     * @throw CyclicReferenceException if adding the new child cause inception of cycle in geometry graph
     */
    PathHints::Hint insert(const shared_ptr<ChildType>& el, const std::size_t pos) {
        if (el) this->ensureCanHaveAsChild(*el);
        return insertUnsafe(el, pos);
    }

    /**
     * Add children to shelf begin, move all other children right.
     * This method is fast but also unsafe because it doesn't ensure that there will be no cycle in geometry graph after adding the new child.
     * @param el object to add
     * @return path hint, see @ref geometry_paths
     * @throw CyclicReferenceException if adding the new child cause inception of cycle in geometry graph
     */
    PathHints::Hint push_front(const shared_ptr<ChildType>& el) {
        if (el) this->ensureCanHaveAsChild(*el);
        return insertUnsafe(el, 0);
    }

    shared_ptr<GeometryObject> changedVersionForChildren(std::vector<std::pair<shared_ptr<ChildType>, Vec<3, double>>>& children_after_change, Vec<3, double>* recomended_translation) const override;

    void writeXMLAttr(XMLWriter::Element &dest_xml_object, const AxisNames &) const override;

    shared_ptr<GeometryObject> shallowCopy() const override;

    shared_ptr<GeometryObject> deepCopy(std::map<const GeometryObject*, shared_ptr<GeometryObject>>& copied) const override;

};

template <int dim>
using StackContainerChildAligner =
    typename chooseType<dim-2, align::Aligner<Primitive<3>::DIRECTION_TRAN>, align::Aligner<Primitive<3>::DIRECTION_LONG, Primitive<3>::DIRECTION_TRAN> >::type;

/**
 * Container which have children in stack/layers.
 * @ingroup GEOMETRY_OBJ
 */
//TODO copy constructor
//TODO remove some redundant code and use one from WithAligners
template <int dim>
struct PLASK_API StackContainer: public WithAligners< StackContainerBaseImpl<dim>, StackContainerChildAligner<dim> > {

    typedef StackContainerChildAligner<dim> ChildAligner;
    static const ChildAligner& DefaultAligner();

    typedef typename StackContainerBaseImpl<dim>::ChildType ChildType;
    typedef typename StackContainerBaseImpl<dim>::TranslationT TranslationT;

    /// Rectangle type in space on this, rectangle in space with dim number of dimensions.
    typedef typename StackContainerBaseImpl<dim>::Box Box;

    using StackContainerBaseImpl<dim>::shared_from_this;
    using StackContainerBaseImpl<dim>::children;
    using StackContainerBaseImpl<dim>::stackHeights;

    static const char* NAME;

    std::string getTypeName() const override { return NAME; }

    ChildAligner default_aligner;

  private:

    typedef WithAligners< StackContainerBaseImpl<dim>, StackContainerChildAligner<dim> > ParentClass;

    /**
     * Get translation object over given object @p el.
     * @param el object to wrap
     * @param aligner aligner for object
     * @param up_trans translation in growing direction
     * @param elBB bouding box of @p el
     * @return translation over @p el
     */
    shared_ptr<TranslationT> newTranslation(const shared_ptr<ChildType>& el, const ChildAligner& aligner, double up_trans, const Box& elBB) const;

    /**
     * Get translation object over given object @p el.
     * @param el object to wrap
     * @param aligner aligner for object
     * @param up_trans translation in growing direction
     * @return translation over @p el
     */
    shared_ptr<TranslationT> newTranslation(const shared_ptr<ChildType>& el, const ChildAligner& aligner, double up_trans) const;

  public:

    /**
     * @param baseHeight height where the first object should start
     * @param aligner default stack aligner
     */
    explicit StackContainer(const double baseHeight = 0.0, const ChildAligner& aligner=DefaultAligner()):
        ParentClass(baseHeight), default_aligner(aligner) {}

    void onChildChanged(const GeometryObject::Event& evt) override;

    /**
     * Insert children to stack at given position.
     * This method is fast but also unsafe because it doesn't ensure that there will be no cycle in geometry graph after adding the new child.
     * @param el object to insert
     * @param pos position where (before which) child should be inserted
     * @param aligner aligner which will be used to calculate horizontal translation of inserted object
     * @return path hint, see @ref geometry_paths
     */
    PathHints::Hint insertUnsafe(const shared_ptr<ChildType>& el, const std::size_t pos, const ChildAligner& aligner);

    PathHints::Hint insertUnsafe(const shared_ptr<ChildType>& el, const std::size_t pos) {
        return insertUnsafe(el, pos, default_aligner);
    }

    /**
     * Insert children to stack at given position.
     * @param el object to insert
     * @param pos position where (before which) child should be inserted
     * @param aligner aligner which will be used to calculate horizontal translation of inserted object
     * @return path hint, see @ref geometry_paths
     * @throw CyclicReferenceException if adding the new child cause inception of cycle in geometry graph
     */
    PathHints::Hint insert(const shared_ptr<ChildType>& el, const std::size_t pos, const ChildAligner& aligner) {
        if (el) this->ensureCanHaveAsChild(*el);
        return insertUnsafe(el, pos, aligner);
    }

    PathHints::Hint insert(const shared_ptr<ChildType>& el, const std::size_t pos) {
        return insert(el, pos, default_aligner);
    }

    /**
     * Add children to stack top.
     * This method is fast but also unsafe because it doesn't ensure that there will be no cycle in geometry graph after adding the new child.
     * @param el object to add
     * @param aligner aligner which will be used to calculate horizontal translation of inserted object
     * @return path hint, see @ref geometry_paths
     */
    PathHints::Hint addUnsafe(const shared_ptr<ChildType>& el, const ChildAligner& aligner);

    PathHints::Hint addUnsafe(const shared_ptr<ChildType>& el) {
        return addUnsafe(el, default_aligner);
    }

    /**
     * Add children to stack top.
     * @param el object to add
     * @param aligner aligner which will be used to calculate horizontal translation of inserted object
     * @return path hint, see @ref geometry_paths
     * @throw CyclicReferenceException if adding the new child cause inception of cycle in geometry graph
     */
    PathHints::Hint add(const shared_ptr<ChildType> &el, const ChildAligner& aligner) {
        if (el) this->ensureCanHaveAsChild(*el);
        return addUnsafe(el, aligner);
    }

    PathHints::Hint add(const shared_ptr<ChildType>& el) {
        return add(el, default_aligner);
    }

    /**
     * Add child to stack top.
     * @param el object to add
     * @param aligner aligner for horizontal translation of object
     * @return path hint, see @ref geometry_paths
     * @throw CyclicReferenceException if adding the new child cause inception of cycle in geometry graph
     */
    PathHints::Hint push_back(const shared_ptr<ChildType> &el, const ChildAligner& aligner) {
        return add(el, aligner);
    }

    PathHints::Hint push_back(const shared_ptr<ChildType> &el) {
        return push_back(el, default_aligner);
    }

    /**
     * Add child to stack bottom, move all other children up.
     * @param el object to add
     * @param aligner aligner which describe horizontal translation of object
     * @return path hint, see @ref geometry_paths
     * @throw CyclicReferenceException if adding the new child cause inception of cycle in geometry graph
     */
    PathHints::Hint push_front(const shared_ptr<ChildType>& el, const ChildAligner& aligner) {
        if (el) this->ensureCanHaveAsChild(*el);
        return insertUnsafe(el, 0, aligner);
    }

    PathHints::Hint push_front(const shared_ptr<ChildType>& el) {
        return push_front(el, default_aligner);
    }

     const ChildAligner& getAlignerAt(std::size_t child_no) const {
        this->ensureIsValidChildNr(child_no, "getAlignerAt");
        return this->aligners[child_no];
    }

    /*
     * Set new aligner.
     * @param child_no (real) child number for which aligner will be set
     * @param aligner new aligner for given child, this pointer will be delete by this stack and it can be used only in one stack, for one child
     */
    /*void setAlignerAtMove(std::size_t child_no, Aligner* aligner) {
        this->ensureIsValidChildNr(child_no, "setAlignerAtMove");
        if (aligners[child_no] == aligner) return; //protected for self assign
        aligners[child_no] = aligner;
        aligners[child_no]->align(*children[child_no]);
        this->fireChanged(GeometryObject::Event::EVENT_RESIZE);
    }*/

    bool removeIfTUnsafe(const std::function<bool(const shared_ptr<TranslationT>& c)>& predicate) override;

    void removeAtUnsafe(std::size_t index) override;

    //add in reverse order
    void writeXML(XMLWriter::Element& parent_xml_object, GeometryObject::WriteXMLCallback& write_cb, AxisNames parent_axes) const override;

    shared_ptr<GeometryObject> shallowCopy() const override;

    shared_ptr<GeometryObject> deepCopy(std::map<const GeometryObject*, shared_ptr<GeometryObject>>& copied) const override;

protected:
    void writeXMLChildAttr(XMLWriter::Element &dest_xml_child_tag, std::size_t child_index, const AxisNames &axes) const override;

    shared_ptr<GeometryObject> changedVersionForChildren(std::vector<std::pair<shared_ptr<ChildType>, Vec<3, double>>>& children_after_change, Vec<3, double>* recomended_translation) const override;

};

template <> const StackContainer<2>::ChildAligner& StackContainer<2>::DefaultAligner();
template <> const StackContainer<3>::ChildAligner& StackContainer<3>::DefaultAligner();

PLASK_API_EXTERN_TEMPLATE_STRUCT(StackContainer<2>)
PLASK_API_EXTERN_TEMPLATE_STRUCT(StackContainer<3>)

/**
 * N-stacks
 * @ingroup GEOMETRY_OBJ
 */
template <typename UpperClass>
class PLASK_API MultiStackContainer: public UpperClass {

    static const int DIM = UpperClass::DIM;

    /*
     * @param a, divider
     * @return \f$a - \floor{a / divider} * divider\f$
     */
    /*static double modulo(double a, double divider) {
        return a - static_cast<double>( static_cast<int>( a / divider ) ) * divider;
    }*/

  public:
    using UpperClass::getChildForHeight;
    using UpperClass::stackHeights;
    using UpperClass::children;

    typedef typename StackContainerBaseImpl<DIM>::ChildType ChildType;
    typedef typename StackContainerBaseImpl<DIM>::TranslationT TranslationT;

    /// Vector of doubles type in space on this, vector in space with dim number of dimensions.
    typedef typename UpperClass::DVec DVec;

    /// Rectangle type in space on this, rectangle in space with dim number of dimensions.
    typedef typename UpperClass::Box Box;

  protected:

    /*
     * Get number of all children.
     * @return number of all children
     */
    //std::size_t size() const { return children.size() * repeat_count; }

    /*
     * Get child with translation.
     * @param index index of child
     * @return child with given index
     */
    //typename UpperClass::TranslationT& operator[] (std::size_t index) { return children[index % children.size()]; }

    /**
     * Reduce @a height to the first repetition.
     * @param height to reduce
     * @return @c true only if height is inside this stack (only in such case @a height is reduced)
     */
    bool reduceHeight(double& height) const;

  public:

    /// How many times all stack is repeated.
    unsigned repeat_count;

    /**
     * Constructor.
     * @param repeat_count how many times stack should be repeated, must be 1 or more
     * @param baseHeight height where the first object should start
     * @param aligner default stack aligner
     */
    template <typename... UpperCtrArgs>
    explicit MultiStackContainer(unsigned repeat_count, UpperCtrArgs&&... upperCtrArgs):
        UpperClass(std::forward<UpperCtrArgs>(upperCtrArgs)...), repeat_count(repeat_count) {}

    //this is not used but, just for case redefine UpperClass::getChildForHeight
    /*const shared_ptr<TranslationT> getChildForHeight(double height) const {
        if (!reduceHeight(height)) return nullptr;
        return UpperClass::getChildForHeight(height);
    }*/

    //TODO good but unused
    //virtual bool intersects(const Box& area) const;

    Box getBoundingBox() const override;

    Box getRealBoundingBox() const override;

    void getBoundingBoxesToVec(const GeometryObject::Predicate& predicate, std::vector<Box>& dest, const PathHints* path = 0) const override;

    void getObjectsToVec(const GeometryObject::Predicate& predicate, std::vector< shared_ptr<const GeometryObject> >& dest, const PathHints* path = 0) const override;

    void getPositionsToVec(const GeometryObject::Predicate& predicate, std::vector<DVec>& dest, const PathHints* path = 0) const override;

    // void extractToVec(const GeometryObject::Predicate &predicate, std::vector< shared_ptr<const GeometryObjectD<dim> > >& dest, const PathHints *path = 0) const;

    GeometryObject::Subtree getPathsTo(const GeometryObject& el, const PathHints* path = 0) const override;

    GeometryObject::Subtree getPathsAt(const DVec& point, bool all=false) const override;

    bool contains(const DVec& p) const override;

    shared_ptr<Material> getMaterial(const DVec& p) const override;

    std::size_t getChildrenCount() const override { return children.size() * repeat_count; }

    shared_ptr<GeometryObject> getChildNo(std::size_t child_no) const override;

    std::size_t getRealChildrenCount() const override;

    shared_ptr<GeometryObject> getRealChildNo(std::size_t child_no) const override;

    unsigned getRepeatCount() const { return repeat_count; }

    void setRepeatCount(unsigned new_repeat_count) {
        if (repeat_count == new_repeat_count) return;
        repeat_count = new_repeat_count;
        this->fireChildrenChanged();    //TODO should this be called? or simple change?
    }

    shared_ptr<GeometryObject> shallowCopy() const override;

    shared_ptr<GeometryObject> deepCopy(std::map<const GeometryObject*, shared_ptr<GeometryObject>>& copied) const override;

    void addPointsAlongToSet(std::set<double>& points,
                        Primitive<3>::Direction direction,
                        unsigned max_steps,
                        double min_step_size) const override;

    void addLineSegmentsToSet(std::set<typename GeometryObjectD<UpperClass::DIM>::LineSegment>& segments,
                              unsigned max_steps,
                              double min_step_size) const override;

protected:
    void writeXMLAttr(XMLWriter::Element &dest_xml_object, const AxisNames &axes) const override;

    shared_ptr<GeometryObject> changedVersionForChildren(std::vector<std::pair<shared_ptr<ChildType>, Vec<3, double>>>& children_after_change, Vec<3, double>* recomended_translation) const override;

};


}   // namespace plask


#endif // STACK_H
