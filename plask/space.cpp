#include "space.h"

#include <memory>

namespace plask {

//void CalculationSpace::setBorders(const std::function<boost::optional<std::unique_ptr<border::Strategy> >(const std::string& s)>& borderValuesGetter, const AxisNames& axesNames);

void CalculationSpace::setBorders(const std::function<boost::optional<std::string>(const std::string& s)>& borderValuesGetter, const AxisNames& axesNames) {
    const char* directions[3][2] = { {"back", "front"}, {"left", "right"}, {"bottom", "top"} };
    boost::optional<std::string> v, v_lo, v_hi;
    v = borderValuesGetter("borders");
    if (v) setAllBorders(*border::Strategy::fromStrUnique(*v));
    v = borderValuesGetter("planar");
    if (v) setPlanarBorders(*border::Strategy::fromStrUnique(*v));
    for (int dir_nr = 0; dir_nr < 3; ++dir_nr) {
        std::string axis_name = axesNames[dir_nr];
        v = borderValuesGetter(axis_name);
        if (v) setBorders(plask::Primitive<3>::DIRECTION(dir_nr), *border::Strategy::fromStrUnique(*v));
        v_lo = borderValuesGetter(axis_name + "-lo");
        if (v = borderValuesGetter(directions[dir_nr][0])) {
            if (v_lo) throw BadInput("setBorders", "border specified by both '%1%-lo' and '%2%'", axis_name, directions[dir_nr][0]);
            else v_lo = v;
        }
        v_hi = borderValuesGetter(axis_name + "-hi");
        if (v = borderValuesGetter(directions[dir_nr][1])) {
            if (v_hi) throw BadInput("setBorders", "border specified by both '%1%-hi' and '%2%'", axis_name, directions[dir_nr][1]);
            else v_hi = v;
        }
        try {
            if (v_lo && v_hi) {
                setBorders(plask::Primitive<3>::DIRECTION(dir_nr),  *border::Strategy::fromStrUnique(*v_lo), *border::Strategy::fromStrUnique(*v_hi));
            } else {
                if (v_lo) setBorder(plask::Primitive<3>::DIRECTION(dir_nr), false, *border::Strategy::fromStrUnique(*v_lo));
                if (v_hi) setBorder(plask::Primitive<3>::DIRECTION(dir_nr), true, *border::Strategy::fromStrUnique(*v_hi));
            }
        } catch (DimensionError) {
            throw BadInput("setBorders", "axis '%1%' is not allowed for this space", axis_name);
        }
    }
}

template <>
void CalculationSpaceD<2>::setPlanarBorders(const border::Strategy& border_to_set) {
    setBorders(Primitive<3>::DIRECTION_TRAN, border_to_set);
}

template <>
void CalculationSpaceD<3>::setPlanarBorders(const border::Strategy& border_to_set) {
    setBorders(Primitive<3>::DIRECTION_LON, border_to_set);
    setBorders(Primitive<3>::DIRECTION_TRAN, border_to_set);
}

Space2dCartesian::Space2dCartesian(const shared_ptr<Extrusion>& extrusion)
    : extrusion(extrusion)
{
    init();
}

Space2dCartesian::Space2dCartesian(const shared_ptr<GeometryElementD<2>>& childGeometry, double length)
    : extrusion(make_shared<Extrusion>(childGeometry, length))
{
   init();
}

shared_ptr< GeometryElementD<2> > Space2dCartesian::getChild() const {
    return extrusion->getChild();
}

shared_ptr<Material> Space2dCartesian::getMaterial(const Vec<2, double> &p) const {
    Vec<2, double> r = p;
    shared_ptr<Material> material;

    bottomup.apply(cachedBoundingBox, r, material);
    if (material) return material;

    leftright.apply(cachedBoundingBox, r, material);
    if (material) return material;

    return getMaterialOrDefault(r);
}

Space2dCartesian* Space2dCartesian::getSubspace(const shared_ptr< GeometryElementD<2> >& element, const PathHints* path, bool copyBorders) const {
    auto new_child = getChild()->getElementInThisCordinates(element, path);
    if (copyBorders) {
        std::unique_ptr<Space2dCartesian> result(new Space2dCartesian(*this));
        result->extrusion = make_shared<Extrusion>(new_child, getExtrusion()->length);
        return result.release();
    } else
        return new Space2dCartesian(new_child, getExtrusion()->length);

}

void Space2dCartesian::setBorders(Primitive<3>::DIRECTION direction, const border::Strategy& border_lo, const border::Strategy& border_hi) {
    Primitive<3>::ensureIsValid2dDirection(direction);
    if (direction == Primitive<3>::DIRECTION_TRAN)
        leftright.setStrategies(border_lo, border_hi);
    else
        bottomup.setStrategies(border_lo, border_hi);
}

void Space2dCartesian::setBorder(Primitive<3>::DIRECTION direction, bool higher, const border::Strategy& border_to_set) {
    Primitive<3>::ensureIsValid2dDirection(direction);
    if (direction == Primitive<3>::DIRECTION_TRAN)
        leftright.set(higher, border_to_set);
    else
        bottomup.set(higher, border_to_set);
}

const border::Strategy& Space2dCartesian::getBorder(Primitive<3>::DIRECTION direction, bool higher) const {
    Primitive<3>::ensureIsValid2dDirection(direction);
    return (direction == Primitive<3>::DIRECTION_TRAN) ? leftright.get(higher) : bottomup.get(higher);
}

Space2dCylindrical::Space2dCylindrical(const shared_ptr<Revolution>& revolution)
    : revolution(revolution)
{
    init();
}

Space2dCylindrical::Space2dCylindrical(const shared_ptr<GeometryElementD<2>>& childGeometry)
    : revolution(make_shared<Revolution>(childGeometry))
{
   init();
}

shared_ptr< GeometryElementD<2> > Space2dCylindrical::getChild() const {
    return revolution->getChild();
}

shared_ptr<Material> Space2dCylindrical::getMaterial(const Vec<2, double> &p) const {
    Vec<2, double> r = p;
    shared_ptr<Material> material;

    bottomup.apply(cachedBoundingBox, r, material);
    if (material) return material;

    outer.applyIfHi(cachedBoundingBox, r, material);
    if (material) return material;

    return getMaterialOrDefault(r);
}

Space2dCylindrical* Space2dCylindrical::getSubspace(const shared_ptr< GeometryElementD<2> >& element, const PathHints* path, bool copyBorders) const {
    auto new_child = getChild()->getElementInThisCordinates(element, path);
    if (copyBorders) {
        std::unique_ptr<Space2dCylindrical> result(new Space2dCylindrical(*this));
        result->revolution = make_shared<Revolution>(new_child);
        return result.release();
    } else
        return new Space2dCylindrical(new_child);
}

void Space2dCylindrical::setBorders(Primitive<3>::DIRECTION direction, const border::Strategy& border_to_set) {
    Primitive<3>::ensureIsValid2dDirection(direction);
    if (direction == Primitive<3>::DIRECTION_TRAN)
        outer = castBorder<border::UniversalStrategy>(border_to_set);
    else
        bottomup.setBoth(border_to_set);
}

void Space2dCylindrical::setBorders(Primitive<3>::DIRECTION direction, const border::Strategy& border_lo, const border::Strategy& border_hi) {
    ensureBoundDirIsProper(direction, false);
    ensureBoundDirIsProper(direction, true);
    bottomup.setStrategies(border_lo, border_hi);   //bottomup is only one valid proper bound for lo and hi
}

void Space2dCylindrical::setBorder(Primitive<3>::DIRECTION direction, bool higher, const border::Strategy& border_to_set) {
    ensureBoundDirIsProper(direction, higher);
    if (direction == Primitive<3>::DIRECTION_TRAN) {
        outer = castBorder<border::UniversalStrategy>(border_to_set);
    } else
        bottomup.set(higher, border_to_set);
}

const border::Strategy& Space2dCylindrical::getBorder(Primitive<3>::DIRECTION direction, bool higher) const {
    ensureBoundDirIsProper(direction, higher);
    return (direction == Primitive<3>::DIRECTION_TRAN) ? outer.getStrategy() : bottomup.get(higher);
}

}   // namespace plask
