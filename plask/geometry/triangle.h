#ifndef TRIANGLE_H
#define TRIANGLE_H

/** @file
This file contains triangle (geometry object) class.
*/

#include "leaf.h"

namespace plask {

/**
 * Represents triangle with one vertex at point (0, 0).
 * @ingroup GEOMETRY_OBJ
 */
struct PLASK_API Triangle: public GeometryObjectLeaf<2> {

    ///Vector of doubles type in space on this, vector in space with dim number of dimensions.
    typedef typename GeometryObjectLeaf<2>::DVec DVec;

    ///Rectangle type in space on this, rectangle in space with dim number of dimensions.
    typedef typename GeometryObjectLeaf<2>::Box Box;

    static constexpr const char* NAME = "triangle";

    virtual std::string getTypeName() const override;

    /**
     * Contruct a solid triangle with vertexes at points: (0, 0), @p p0, @p p1
     * @param p0, p1 coordinates of the triangle vertexes
     * @param material material inside the whole triangle
     */
    explicit Triangle(const DVec& p0 = Primitive<2>::ZERO_VEC, const DVec& p1 = Primitive<2>::ZERO_VEC, const shared_ptr<Material>& material = shared_ptr<Material>());

    /**
     * Contruct a triangle with vertexes at points: (0, 0), @p p0, @p p1
     * @param p0, p1 coordinates of the triangle vertexes
     * @param materialTopBottom describes materials inside the triangle
     */
    explicit Triangle(const DVec& p0, const DVec& p1, shared_ptr<MaterialsDB::MixedCompositionFactory> materialTopBottom);

    virtual Box2D getBoundingBox() const override;

    virtual bool contains(const DVec& p) const override;

    virtual void writeXMLAttr(XMLWriter::Element& dest_xml_object, const AxisNames& axes) const override;

    DVec p0, p1;

    /**
     * Set coordinates of first vertex and inform observers about changes.
     * @param new_p0 new coordinates for p0
     */
    void setP0(const DVec& new_p0) {
        p0 = new_p0;
        this->fireChanged(GeometryObject::Event::EVENT_RESIZE);
    }

    /**
     * Set coordinates of first vertex and inform observers about changes.
     * @param vecCtrArg new coordinates to set (parameters to Vec<2> constructor)
     */
    template <typename ...VecCtrArg>
    void setP0(VecCtrArg&&... vecCtrArg) {
        this->setP0(DVec(std::forward<VecCtrArg>(vecCtrArg)...));
    }

    /**
     * Set coordinates of second vertex and inform observers about changes.
     * @param new_p0 new coordinates for p1
     */
    void setP1(const DVec& new_p1) {
        p1 = new_p1;
        this->fireChanged(GeometryObject::Event::EVENT_RESIZE);
    }

    /**
     * Set coordinates of second vertex and inform observers about changes.
     * @param vecCtrArg new coordinates to set (parameters to Vec<2> constructor)
     */
    template <typename ...VecCtrArg>
    void setP1(VecCtrArg&&... vecCtrArg) {
        this->setP1(DVec(std::forward<VecCtrArg>(vecCtrArg)...));
    }
};

}   // namespace plask

#endif // TRIANGLE_H
