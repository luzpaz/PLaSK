#ifndef PLASK__GEOMETRY_CONTAINER_H
#define PLASK__GEOMETRY_CONTAINER_H

/** @file
This file includes containers of geometries elements.
*/

#include <map>
#include <vector>
#include <algorithm>
#include "element.h"
#include "transform.h"

namespace plask {

/**
Represent hints for path finder.

Hints are used to to find unique path for all GeometryElement pairs,
even if one of the pair element is inserted to geometry graph in more than one place.

Each hint allow to choose one child for geometry element container and it is a pair:
geometry element container -> element in container.

Typically, hints are returned by methods which adds new elements to containers.
*/
struct PathHints {

    ///Type for map: geometry element container -> element in container
    typedef std::map<GeometryElement*, GeometryElement*> HintMap;
    //TODO co z wieloma ojcami, może powinno być GeometryElement* -> set<GeometryElement*> (bo kontenery też mogą mieć wiele ojców) albo mapy w obie strony
    //TODO może GeometryElement* -> int

    ///Pair type: geometry element container -> element in container
    typedef HintMap::value_type Hint;

    ///Hints map.
    HintMap hintFor;

    /**
     * Add hint to hints map. Overwrite if hint for given container already exists.
     * @param hint hint to add
     */
    void addHint(const Hint& hint);

    /**
     * Add hint to hints map. Overwrite if hint for given container already exists.
     * @param hint hint to add
     */
    PathHints& operator+=(const Hint& hint) {
        addHint(hint);
        return *this;
    }

    /**
     * Add hint to hints map. Overwrite if hint for given container already exists.
     * @param container, child hint to add
     */
    void addHint(GeometryElement* container, GeometryElement* child);

    /**
     * Get child for given container.
     * @return child for given container or @c nullptr if there is no hint for given container
     */
    GeometryElement* getChild(GeometryElement* container) const;

};

/**
 * Template which implment container using stl container of pointers to some geometry elements objects (Translation by default).
 * @tparam dim GeometryElementContainer dimension
 * @tparam container_type container of pointers to children
 */
template <int dim, typename container_type = std::vector<Translation<dim>*> >
struct GeometryElementContainerImpl: public GeometryElementContainer<dim> {

    typedef typename GeometryElementContainer<dim>::Vec Vec;
    typedef typename GeometryElementContainer<dim>::Rect Rect;

protected:
    container_type children;

public:
    ~GeometryElementContainerImpl() {
        for (auto child: children) delete child;
    }

    virtual bool inside(const Vec& p) const {
        for (auto child: children) if (child->inside(p)) return true;
        return false;
    }

    virtual bool intersect(const Rect& area) const {
        for (auto child: children) if (child->intersect(area)) return true;
        return false;
    }

    virtual Rect getBoundingBox() const {
        //if (childs.empty()) throw?
        Rect result = children[0]->getBoundingBox();
        for (std::size_t i = 1; i < children.size(); ++i)
            result.include(children[i]->getBoundingBox());
        return result;
    }

    /**
     * Check children in reverse order and check if any returns material.
     * @return material or nullptr
     */
    virtual shared_ptr<Material> getMaterial(const Vec& p) const {
        for (auto child_it = children.rbegin(); child_it != children.rend(); ++child_it) {
            shared_ptr<Material> r = (*child_it)->getMaterial(p);
            if (r != nullptr) return r;
        }
        return shared_ptr<Material>();
    }

    virtual std::vector<Rect> getLeafsBoundingBoxes() const {
        std::vector<Rect> result;
        for (auto child: children) {
            std::vector<Rect> child_leafs_boxes = child->getLeafsBoundingBoxes();
            result.insert(result.end(), child_leafs_boxes.begin(), child_leafs_boxes.end());
        }
        return result;
    }

};

/**
 * Geometry elements container in which all child is connected with translation vector.
 */
//TODO some implementation are naive, and can be done faster with some caches
template < int dim >
struct TranslationContainer: public GeometryElementContainerImpl<dim> {

    typedef typename GeometryElementContainer<dim>::Vec Vec;
    typedef typename GeometryElementContainer<dim>::Rect Rect;
    typedef GeometryElementD<dim> ChildType;
    typedef Translation<dim> TranslationT;

    using GeometryElementContainerImpl<dim>::children;

    PathHints::Hint add(ChildType* el, const Vec& translation = Primitive<dim>::ZERO_VEC) {
        TranslationT* trans_geom = new TranslationT(el, translation);
        children.push_back(trans_geom);
        return PathHints::Hint(this, trans_geom);
    }

};

/**
 * Container which have children in stack/layers.
 */
struct StackContainer2d: public GeometryElementContainerImpl<2> {

    typedef typename GeometryElementContainer<2>::Vec Vec;
    typedef typename GeometryElementContainer<2>::Rect Rect;
    typedef GeometryElementD<2> ChildType;
    typedef Translation<2> TranslationT;

    using GeometryElementContainerImpl<2>::children;

    /**
     * @param baseHeight height where should start first element
     */
    explicit StackContainer2d(const double baseHeight = 0.0);

    /**
     * Add children to stack top.
     * @param el element to add
     * @param x_translation horizontal translation of element
     * @return path hint
     */
    PathHints::Hint push_back(ChildType* el, const double x_translation = 0.0);
    
    /**
     * Add children to stack top.
     * @param el element to add
     * @param x_translation horizontal translation of element
     * @return path hint
     */
    PathHints::Hint add(ChildType* el, const double x_translation = 0.0) { return push_back(el, x_translation); }

    /**
     * @param height
     * @return child which are on given @a height or @c nullptr
     */
    const TranslationT* getChildForHeight(double height) const;

    virtual bool inside(const Vec& p) const;

    virtual shared_ptr<Material> getMaterial(const Vec& p) const;

private:

    ///stackHeights[x] is current stack heights with x first elements in it (sums of heights of first x elements)
    std::vector<double> stackHeights;

};


}	// namespace plask

#endif // PLASK__GEOMETRY_CONTAINER_H
