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
#ifndef PLASK__TRIANGULAR2D_H
#define PLASK__TRIANGULAR2D_H

#include "mesh.hpp"
#include "interpolation.hpp"
#include "boundary.hpp"
#include "../geometry/path.hpp"
#include <array>
#include <unordered_map>

#include <boost/geometry/index/rtree.hpp>
#include "../vector/boost_geometry.hpp"
#include <boost/geometry/geometries/box.hpp>

#include <boost/functional/hash.hpp>

namespace plask {

struct PLASK_API TriangularMesh2D: public MeshD<2> {

    friend struct ExtrudedTriangularMesh3D;

    /// Boundary type.
    typedef plask::Boundary<TriangularMesh2D> Boundary;

    using MeshD<2>::LocalCoords;
    typedef std::vector<LocalCoords> LocalCoordsVec;
    typedef LocalCoordsVec::const_iterator const_iterator;
    typedef const_iterator iterator;

    LocalCoordsVec nodes;

    typedef std::array<std::size_t, 3> TriangleNodeIndexes;

    std::vector< TriangleNodeIndexes > elementNodes;

    /**
     * Represent FEM-like element (triangle) in TriangularMesh2D.
     */
    struct PLASK_API Element {
        TriangleNodeIndexes triangleNodes;
        const TriangularMesh2D& mesh;   // for getting access to the nodes

        Element(const TriangularMesh2D& mesh, TriangleNodeIndexes triangleNodes)
            : triangleNodes(triangleNodes), mesh(mesh) {}

        /**
         * Get index of the triangle vertex in mesh (nodes vector).
         * @param index index of vertex in the triangle corresponded to this element; equals to 0, 1 or 2
         * @return index of the triangle vertex in mesh (nodes vector)
         */
        std::size_t getNodeIndex(std::size_t index) const noexcept {
            assert(index < 3);
            return triangleNodes[index];
        }

        /**
         * Get coordinates of the triangle vertex.
         * @param index index of vertex in the triangle corresponded to this element; equals to 0, 1 or 2
         * @return coordinates of the triangle vertex
         */
        const LocalCoords& getNode(std::size_t index) const noexcept {
            return mesh.nodes[getNodeIndex(index)];
        }

        /**
         * Get coordinates of the triangle vertices.
         * @return coordinates of the triangle vertices
         */
        std::array<LocalCoords, 3> getNodes() const {
            return { getNode(0), getNode(1), getNode(2) };
        }

        /**
         * Get centroid of the triangle corresponded to this element.
         * @return centroid of the triangle corresponded to this element
         */
        LocalCoords getMidpoint() const {
            return (getNode(0)+getNode(1)+getNode(2)) / 3.0;
        }

        /**
         * Get area of the triangle represented by this element.
         * @return the area of the triangle
         */
        double getArea() const noexcept {
            // formula comes from http://www.mathguru.com/level2/application-of-coordinate-geometry-2007101600011139.aspx
            const LocalCoords A = getNode(0);
            const LocalCoords B = getNode(1);
            const LocalCoords C = getNode(2);
            return abs( (A.c0 - C.c0) * (B.c1 - A.c1)
                      - (A.c0 - B.c0) * (C.c1 - A.c1) ) / 2.0;
        }

        /**
         * Calculate barycentric (area) coordinates of the point @p p with respect to the triangle represented by this.
         * @param p point
         * @return the barycentric (area) coordinates of @c p
         */
        Vec<3, double> barycentric(Vec<2, double> p) const;

        /**
         * Check if point @p p is included in triangle represented by @c this element.
         * @param p point to check
         * @return @c true only if @p p is included in @c this
         */
        bool contains(Vec<2, double> p) const {
            auto b = barycentric(p);
            return b.c0 >= 0 && b.c1 >= 0 && b.c2 >= 0;
        }

        /**
         * Calculate minimal rectangle which contains the triangle represented by the element.
         * @return calculated rectangle
         */
        Box2D getBoundingBox() const;
    };

    /**
     * Wrapper to TriangularMesh2D which allows for accessing FEM-like elements.
     *
     * It works like read-only, random access container of @ref Element objects.
     */
    struct PLASK_API Elements {
        const TriangularMesh2D& mesh;

        explicit Elements(const TriangularMesh2D& mesh): mesh(mesh) {}

        Element at(std::size_t index) const {
            if (index >= mesh.elementNodes.size())
                throw OutOfBoundsException("triangularMesh2D::Elements::at", "index", index, 0, mesh.elementNodes.size()-1);
            return Element(mesh, mesh.elementNodes[index]);
        }

        Element operator[](std::size_t index) const {
            return Element(mesh, mesh.elementNodes[index]);
        }

        /**
         * Get number of elements (triangles) in the mesh.
         * @return number of elements
         */
        std::size_t size() const { return mesh.getElementsCount(); }

        bool empty() const { return mesh.getElementsCount() == 0; }

        typedef IndexedIterator<const Elements, Element> const_iterator;
        typedef const_iterator iterator;

        /// @return iterator referring to the first element (triangle) in the mesh
        const_iterator begin() const { return const_iterator(this, 0); }

        /// @return iterator referring to the past-the-end element (triangle) in the mesh
        const_iterator end() const { return const_iterator(this, this->size()); }
    };

    Elements getElements() const { return Elements(*this); }
    Elements elements() const { return Elements(*this); }

    Element getElement(std::size_t elementIndex) const {
        return Element(*this, elementNodes[elementIndex]);
    };

    Element element(std::size_t elementIndex) const {
        return Element(*this, elementNodes[elementIndex]);
    };

    /**
     * Get number of elements (triangles) in this mesh.
     * @return number of elements
     */
    std::size_t getElementsCount() const {
        return elementNodes.size();
    }

    /**
     * Instance of this class allows for adding triangles to the mesh effectively.
     */
    struct PLASK_API Builder {
        std::map<LocalCoords, std::size_t> indexOfNode; ///< map nodes to their indexes in mesh.nodes vector
        TriangularMesh2D& mesh; ///< destination mesh

        /**
         * Construct builder which will add triangles to the given @p mesh.
         * @param mesh triangles destination
         */
        explicit Builder(TriangularMesh2D& mesh);

        /**
         * Construct builder which will add triangles to the given @p mesh.
         *
         * This constructor preallocate extra space for elements and nodes in @p mesh,
         * which usually improves performance.
         * @param mesh triangles destination
         * @param predicted_number_of_elements predicted (maximal) number of elements (triangles) to be added
         * @param predicted_number_of_elements predicted (maximal) number of nodes to be added
         */
        explicit Builder(TriangularMesh2D& mesh, std::size_t predicted_number_of_elements, std::size_t predicted_number_of_nodes);

        /**
         * Construct builder which will add triangles to the given @p mesh.
         *
         * This constructor preallocate extra space for elements and nodes (3*predicted_number_of_elements) in @p mesh,
         * which usually improves performance.
         * @param mesh triangles destination
         * @param predicted_number_of_elements predicted (maximal) number of elements to be added
         */
        explicit Builder(TriangularMesh2D& mesh, std::size_t predicted_number_of_elements)
            : Builder(mesh, predicted_number_of_elements, predicted_number_of_elements*3) {}

        /// Shrink to fit both: elements and nodes vectors of destination mesh.
        ~Builder();

        /**
         * Add a triangle to the mesh.
         * @param p1, p2, p3 coordinates of vertices of the triangle to add
         * @return <code>*this</code>
         */
        Builder& add(LocalCoords p1, LocalCoords p2, LocalCoords p3);

        /**
         * Add a triangle to the mesh.
         * @param e a trianglular element (from another mesh) which defines the triangle to add
         * @return <code>*this</code>
         */
        Builder& add(const Element& e) { return add(e.getNode(0), e.getNode(1), e.getNode(2)); }

        /**
         * Add a triangle to the mesh.
         * @param points a coordinates of vertices of the triangle to add
         * @return <code>*this</code>
         */
        Builder& add(const std::array<LocalCoords, 3>& points) { return add(points[0], points[1], points[2]); }

    private:

        /**
         * Add a @p node to nodes vector of destination mesh if it is not already there.
         * @param node coordinates of node to (conditionally) add
         * @return an index of the node (added or found) in the vector
         */
        std::size_t addNode(LocalCoords node);
    };

    class ElementMesh;

    /**
     * Return a mesh that enables iterating over middle points of the rectangles.
     * \return the mesh
     */
    shared_ptr<ElementMesh> getElementMesh() const { return make_shared<ElementMesh>(this); }

    /**
     * Index which allows for fast finding elements which includes particular points.
     */
    struct PLASK_API ElementIndex {
        typedef boost::geometry::model::box<Vec<2>> Box;

        typedef boost::geometry::index::rtree<
                std::pair<Box, std::size_t>,
                boost::geometry::index::quadratic<16> //??
                > Rtree;

        const TriangularMesh2D& mesh;

        Rtree rtree;

        ElementIndex(const TriangularMesh2D& mesh);

        static constexpr std::size_t INDEX_NOT_FOUND = std::numeric_limits<std::size_t>::max();

        /**
         * Get index of element which incldes point @p p.
         * @param p point to find
         * @return index of element which includes @p p or INDEX_NOT_FOUND if there is no element includes @p p.
         */
        std::size_t getIndex(Vec<2, double> p) const;

        /**
         * Get element which incldes point @p p.
         * @param p point to find
         * @return element which includes @p p or empty optional if no element includes @p p.
         */
        optional<Element> getElement(Vec<2, double> p) const;
    };

    // ------------------ Mesh and MeshD<2> interfaces: ------------------
    LocalCoords at(std::size_t index) const override {
        assert(index < nodes.size());
        return nodes[index];
    }

    std::size_t size() const override {
        return nodes.size();
    }

    bool empty() const override {
        return nodes.empty();
    }

    // ---- Faster iterators used when exact type of mesh is known; they hide polimorphic iterators of parent class ----
    const_iterator begin() const { return nodes.begin(); }
    const_iterator end() const { return nodes.end(); }

    // ----------------------- Masked meshes -----------------------

    /// Type of predicate function which returns bool for given element of a mesh.
    typedef std::function<bool(const Element&)> Predicate;

    /**
     * Construct masked mesh with elements of @c this chosen by a @p predicate.
     * Preserve order of elements of @p this.
     * @param predicate predicate which returns either @c true for accepting element or @c false for rejecting it
     * @return the masked mesh constructed
     */
    TriangularMesh2D masked(const Predicate& predicate) const;

    /**
     * Construct masked mesh with all elements of @c this which have required materials in the midpoints.
     * Preserve order of elements of @c this.
     * @param geom geometry to get materials from
     * @param materialPredicate predicate which returns either @c true for accepting material or @c false for rejecting it
     * @return the masked mesh constructed
     */
    TriangularMesh2D masked(const GeometryD<2>& geom, const std::function<bool(shared_ptr<const Material>)> materialPredicate) const {
        return masked([&](const Element& el) { return materialPredicate(geom.getMaterial(el.getMidpoint())); });
    }

    /**
     * Construct masked mesh with all elements of @c this which have required kinds of materials (in the midpoints).
     * Preserve order of elements of @p this.
     * @param geom geometry to get materials from
     * @param materialKinds one or more kinds of material encoded with bit @c or operation,
     *        e.g. @c DIELECTRIC|METAL for selecting all dielectrics and metals,
     *        or @c ~(DIELECTRIC|METAL) for selecting everything else
     * @return the masked mesh constructed
     */
    TriangularMesh2D masked(const GeometryD<2>& geom, unsigned materialKinds) const {
        return masked([&](const Element& el) { return (geom.getMaterial(el.getMidpoint())->kind() & materialKinds) != 0; });
    }

    /**
     * Write mesh to XML
     * \param object XML object to write to
     */
    void writeXML(XMLElement& object) const override;

    /**
     * Construct new mesh with data from XPL file.
     * @param reader
     * @return the mesh constructed
     */
    static TriangularMesh2D read(XMLReader& reader);


    // ------------------ Boundaries: -----------------------

    template <typename Predicate>
    static Boundary getBoundary(Predicate predicate) {
        return Boundary(new PredicateBoundaryImpl<TriangularMesh2D, Predicate>(predicate));
    }

    /// Segment (two-element set of node indices) represented by a pair of indices such that first < second.
    typedef std::pair<std::size_t, std::size_t> Segment;

    /// Map segments to their counts.
    typedef std::unordered_map<TriangularMesh2D::Segment, std::size_t, boost::hash<Segment>> SegmentsCounts;

    /**
     * Calculate numbers of segments (sides of triangles).
     * @return the numbers of segments
     */
    SegmentsCounts countSegments() const;

    /**
     * Calculate numbers of segments (sides of triangles) inside a @p box.
     * @param box a region in which segments should be counted
     * @return the numbers of segments
     */
    SegmentsCounts countSegmentsIn(const Box2D& box) const;

    /**
     * Calculate numbers of segments (sides of triangles) inside any of @p boxes member.
     * @param box vector of boxes that describes a region in which segments should be counted
     * @return the numbers of segments
     */
    SegmentsCounts countSegmentsIn(const std::vector<Box2D>& boxes) const;

    /**
     * Calculate numbers of segments (sides of triangles) inside of @p object.
     * @param geometry geometry (of the mesh) which contains an object
     * @param object object to test
     * @param path path hints specifying the object
     * @return the numbers of segments
     */
    SegmentsCounts countSegmentsIn(const GeometryD<2>& geometry, const GeometryObject& object, const PathHints* path = nullptr) const;

private:

    /// Add all edges of el to counter.
    static void countSegmentsOf(SegmentsCounts& counter, const Element& el);

    /**
     * Calculate a set of indices of all boundary nodes (in a whole mesh or a certain region).
     * @param segmentsCount numbers of segments in a whole mesh or requested region
     *          (usually a result of countSegments or countSegmentsIn method)
     * @return the set of indices of boundary nodes
     */
    static std::set<std::size_t> allBoundaryNodes(const SegmentsCounts& segmentsCount);

    template <typename T>   // we can't use std::grater as it use >, and we have < only
    struct greater: public std::function<bool(T, T)> {
      bool operator()(const T& first, const T& second) const { return second < first; }
    };

    /**
     * Calculate a set of indices of boundary nodes (in a whole mesh or a certain region).
     * @param segmentsCount numbers of segments in a whole mesh or requested region
     *         (usually a result of countSegments or countSegmentsIn method)
     * @return the set of indices of boundary nodes
     * @tparam SEG_DIR 0 for right or left boundary, 1 for top or bottom boundary
     * @tparam Compare std::less for right ot top boundary, greater for left or bottom boundary
     */
    template<int SEG_DIR, template<class> class Compare>
    std::set<std::size_t> dirBoundaryNodes(const SegmentsCounts& segmentsCount) const;

    enum class BoundaryDir { LEFT, RIGHT, BOTTOM, TOP, ALL };

    template<BoundaryDir boundaryDir, typename std::enable_if<boundaryDir != BoundaryDir::ALL && int(boundaryDir)%2==0, int>::type = 0>
    std::set<std::size_t> boundaryNodes(const SegmentsCounts& segmentsCount) const {
        return dirBoundaryNodes<int(boundaryDir) / 2, greater>(segmentsCount);
    }

    template<BoundaryDir boundaryDir, typename std::enable_if<boundaryDir != BoundaryDir::ALL && int(boundaryDir)%2==1, int>::type = 0>
    std::set<std::size_t> boundaryNodes(const SegmentsCounts& segmentsCount) const {
        return dirBoundaryNodes<int(boundaryDir) / 2, std::less>(segmentsCount);
    }

    template<BoundaryDir boundaryDir, typename std::enable_if<boundaryDir == BoundaryDir::ALL, int>::type = 0>
    std::set<std::size_t> boundaryNodes(const SegmentsCounts& segmentsCount) const {
        return allBoundaryNodes(segmentsCount);
    }

public:

    /**
     * Get boundary which describes all nodes which lies on all (outer and inner) boundaries of the whole mesh.
     * @return the boundary
     */
    static Boundary getAllBoundary() {
        return Boundary( [](const TriangularMesh2D& mesh, const shared_ptr<const GeometryD<2>>&) {
            return BoundaryNodeSet(new StdSetBoundaryImpl(allBoundaryNodes(mesh.countSegments())));
        } );
    }

    /**
     * Get boundary which describes all nodes which lies on outer right boundary of the whole mesh.
     * @return the boundary
     */
    static Boundary getRightBoundary();

    /**
     * Get boundary which describes all nodes which lies on outer top boundary of the whole mesh.
     * @return the boundary
     */
    static Boundary getTopBoundary();

    /**
     * Get boundary which describes all nodes which lies on outer left boundary of the whole mesh.
     * @return the boundary
     */
    static Boundary getLeftBoundary();

    /**
     * Get boundary which describes all nodes which lies on outer bottom boundary of the whole mesh.
     * @return the boundary
     */
    static Boundary getBottomBoundary();

    /**
     * Get boundary which describes all nodes which lies on all (outer and inner) boundaries of a given @p box.
     * @param box box which describes a region
     * @return the boundary
     */
    static Boundary getAllBoundaryIn(const Box2D& box) {
        return Boundary( [box](const TriangularMesh2D& mesh, const shared_ptr<const GeometryD<2>>&) {
            return BoundaryNodeSet(new StdSetBoundaryImpl(allBoundaryNodes(mesh.countSegmentsIn(box))));
        } );
    }

    /**
     * Get boundary which describes all nodes which lies on right boundary of a given @p box.
     * @param box box which describes a region
     * @return the boundary
     */
    static Boundary getRightOfBoundary(const Box2D& box);

    /**
     * Get boundary which describes all nodes which lies on left boundary of a given @p box.
     * @param box box which describes a region
     * @return the boundary
     */
    static Boundary getLeftOfBoundary(const Box2D& box);

    /**
     * Get boundary which describes all nodes which lies on top boundary of a given @p box.
     * @param box box which describes a region
     * @return the boundary
     */
    static Boundary getTopOfBoundary(const Box2D& box);

    /**
     * Get boundary which describes all nodes which lies on bottom boundary of a given @p box.
     * @param box box which describes a region
     * @return the boundary
     */
    static Boundary getBottomOfBoundary(const Box2D& box);

    /**
     * Get boundary which describes all nodes which lies on all (outer and inner) boundaries of a given @p boxes.
     * @param boxes vector of boxes that describes a region
     * @return the boundary
     */
    static Boundary getAllBoundaryIn(const std::vector<Box2D>& boxes) {
        return Boundary( [boxes](const TriangularMesh2D& mesh, const shared_ptr<const GeometryD<2>>&) {
            return BoundaryNodeSet(new StdSetBoundaryImpl(allBoundaryNodes(mesh.countSegmentsIn(boxes))));
        } );
    }

    /**
     * Get boundary which describes all nodes which lies on right boundaries of a given @p boxes.
     * @param boxes vector of boxes that describes a region
     * @return the boundary
     */
    static Boundary getRightOfBoundary(const std::vector<Box2D>& boxes);

    /**
     * Get boundary which describes all nodes which lies on left boundaries of a given @p boxes.
     * @param boxes vector of boxes that describes a region
     * @return the boundary
     */
    static Boundary getLeftOfBoundary(const std::vector<Box2D>& boxes);

    /**
     * Get boundary which describes all nodes which lies on top boundaries of a given @p boxes.
     * @param boxes vector of boxes that describes a region
     * @return the boundary
     */
    static Boundary getTopOfBoundary(const std::vector<Box2D>& boxes);

    /**
     * Get boundary which describes all nodes which lies on bottom boundaries of a given @p boxes.
     * @param boxes vector of boxes that describes a region
     * @return the boundary
     */
    static Boundary getBottomOfBoundary(const std::vector<Box2D>& boxes);

    /**
     * Get boundary which describes all nodes which lies on all (outer and inner) boundaries of a given @p object.
     * @param object object to test
     * @return the boundary
     */
    static Boundary getAllBoundaryIn(shared_ptr<const GeometryObject> object) {
        return Boundary( [=](const TriangularMesh2D& mesh, const shared_ptr<const GeometryD<2>>& geom) {
            return BoundaryNodeSet(new StdSetBoundaryImpl(allBoundaryNodes(mesh.countSegmentsIn(*geom, *object))));
        } );
    }

    /**
     * Get boundary which describes all nodes which lies on outer right boundary of a given @p object.
     * @param object object to test
     * @return the boundary
     */
    static Boundary getRightOfBoundary(shared_ptr<const GeometryObject> object);

    /**
     * Get boundary which describes all nodes which lies on outer left boundary of a given @p object.
     * @param object object to test
     * @param path path hints specifying the object
     * @return the boundary
     */
    static Boundary getLeftOfBoundary(shared_ptr<const GeometryObject> object);

    /**
     * Get boundary which describes all nodes which lies on outer top boundary of a given @p object.
     * @param object object to test
     * @return the boundary
     */
    static Boundary getTopOfBoundary(shared_ptr<const GeometryObject> object);

    /**
     * Get boundary which describes all nodes which lies on outer bottom boundary of a given @p object.
     * @param object object to test
     * @return the boundary
     */
    static Boundary getBottomOfBoundary(shared_ptr<const GeometryObject> object);

    /**
     * Get boundary which describes all nodes which lies on all (outer and inner) boundaries of a given @p object.
     * @param object object to test
     * @param path path hints specifying the object
     * @return the boundary
     */
    static Boundary getAllBoundaryIn(shared_ptr<const GeometryObject> object, const PathHints& path) {
        return Boundary( [=](const TriangularMesh2D& mesh, const shared_ptr<const GeometryD<2>>& geom) {
            return BoundaryNodeSet(new StdSetBoundaryImpl(allBoundaryNodes(mesh.countSegmentsIn(*geom, *object, &path))));
        } );
    }

    /**
     * Get boundary which describes all nodes which lies on all (outer and inner) boundaries of a given @p object.
     * @param object object to test
     * @param path (optional) path hints specifying the object
     * @return the boundary
     */
    static Boundary getAllBoundaryIn(shared_ptr<const GeometryObject> object, const PathHints* path) {
        return path ? getAllBoundaryIn(object, *path) : getAllBoundaryIn(object);
    }

    /**
     * Get boundary which describes all nodes which lies on outer right boundary of a given @p object.
     * @param object object to test
     * @param path path hints specifying the object
     * @return the boundary
     */
    static Boundary getRightOfBoundary(shared_ptr<const GeometryObject> object, const PathHints& path);

    /**
     * Get boundary which describes all nodes which lies on outer right boundary of a given @p object.
     * @param object object to test
     * @param path (optional) path hints specifying the object
     * @return the boundary
     */
    static Boundary getRightOfBoundary(shared_ptr<const GeometryObject> object, const PathHints* path) {
        return path ? getRightOfBoundary(object, *path) : getRightOfBoundary(object);
    }

    /**
     * Get boundary which describes all nodes which lies on outer left boundary of a given @p object.
     * @param object object to test
     * @param path path hints specifying the object
     * @return the boundary
     */
    static Boundary getLeftOfBoundary(shared_ptr<const GeometryObject> object, const PathHints& path);

    /**
     * Get boundary which describes all nodes which lies on outer left boundary of a given @p object.
     * @param object object to test
     * @param path (optional) path hints specifying the object
     * @return the boundary
     */
    static Boundary getLeftOfBoundary(shared_ptr<const GeometryObject> object, const PathHints* path) {
        return path ? getLeftOfBoundary(object, *path) : getLeftOfBoundary(object);
    }

    /**
     * Get boundary which describes all nodes which lies on outer top boundary of a given @p object.
     * @param object object to test
     * @param path path hints specifying the object
     * @return the boundary
     */
    static Boundary getTopOfBoundary(shared_ptr<const GeometryObject> object, const PathHints& path);

    /**
     * Get boundary which describes all nodes which lies on outer top boundary of a given @p object.
     * @param object object to test
     * @param path (optional) path hints specifying the object
     * @return the boundary
     */
    static Boundary getTopOfBoundary(shared_ptr<const GeometryObject> object, const PathHints* path) {
        return path ? getTopOfBoundary(object, *path) : getTopOfBoundary(object);
    }

    /**
     * Get boundary which describes all nodes which lies on outer bottom boundary of a given @p object.
     * @param object object to test
     * @param path path hints specifying the object
     * @return the boundary
     */
    static Boundary getBottomOfBoundary(shared_ptr<const GeometryObject> object, const PathHints& path);

    /**
     * Get boundary which describes all nodes which lies on outer bottom boundary of a given @p object.
     * @param object object to test
     * @param path (optional) path hints specifying the object
     * @return the boundary
     */
    static Boundary getBottomOfBoundary(shared_ptr<const GeometryObject> object, const PathHints* path) {
        return path ? getBottomOfBoundary(object, *path) : getBottomOfBoundary(object);
    }

    static Boundary getBoundary(const std::string &boundary_desc);

    static Boundary getBoundary(XMLReader &boundary_desc, Manager &manager);

};

template <>
inline TriangularMesh2D::Boundary parseBoundary<TriangularMesh2D::Boundary>(const std::string& boundary_desc, plask::Manager&) { return TriangularMesh2D::getBoundary(boundary_desc); }

template <>
inline TriangularMesh2D::Boundary parseBoundary<TriangularMesh2D::Boundary>(XMLReader& boundary_desc, Manager& env) { return TriangularMesh2D::getBoundary(boundary_desc, env); }

extern template PLASK_API std::set<std::size_t> TriangularMesh2D::boundaryNodes<TriangularMesh2D::BoundaryDir::TOP>(const TriangularMesh2D::SegmentsCounts& segmentsCount) const;
extern template PLASK_API std::set<std::size_t> TriangularMesh2D::boundaryNodes<TriangularMesh2D::BoundaryDir::LEFT>(const TriangularMesh2D::SegmentsCounts& segmentsCount) const;
extern template PLASK_API std::set<std::size_t> TriangularMesh2D::boundaryNodes<TriangularMesh2D::BoundaryDir::RIGHT>(const TriangularMesh2D::SegmentsCounts& segmentsCount) const;
extern template PLASK_API std::set<std::size_t> TriangularMesh2D::boundaryNodes<TriangularMesh2D::BoundaryDir::BOTTOM>(const TriangularMesh2D::SegmentsCounts& segmentsCount) const;


class TriangularMesh2D::ElementMesh: public MeshD<2> {

    /// Original mesh
    const TriangularMesh2D* originalMesh;

  public:
    ElementMesh(const TriangularMesh2D* originalMesh): originalMesh(originalMesh) {}

    LocalCoords at(std::size_t index) const override {
        return originalMesh->element(index).getMidpoint();
    }

    std::size_t size() const override {
        return originalMesh->getElementsCount();
    }

    const TriangularMesh2D& getOriginalMesh() const { return *originalMesh; }

protected:

    bool hasSameNodes(const MeshD<2> &to_compare) const override;
};


// ------------------ Nearest Neighbor interpolation ---------------------

/**
 * Utility struct that allows for using triangular mesh in boost::geometry::index::rtree effectively
 * (without copying coordinates of nodes).
 */
struct TriangularMesh2DGetterForRtree {
    typedef Vec<2, double> result_type;

    const TriangularMesh2D* src_mesh;

    TriangularMesh2DGetterForRtree(const TriangularMesh2D* src_mesh): src_mesh(src_mesh) {}

    result_type operator()(std::size_t index) const {
        return src_mesh->at(index);
    }
};

/// Boost rtree that holds nodes of TriangularMesh2D
typedef boost::geometry::index::rtree<
        std::size_t,
        boost::geometry::index::quadratic<16>, //??
        TriangularMesh2DGetterForRtree
        >
RtreeOfTriangularMesh2DNodes;

template <typename DstT, typename SrcT>
struct PLASK_API NearestNeighborTriangularMesh2DLazyDataImpl: public InterpolatedLazyDataImpl<DstT, TriangularMesh2D, const SrcT>
{
    RtreeOfTriangularMesh2DNodes nodesIndex;

    NearestNeighborTriangularMesh2DLazyDataImpl(
                const shared_ptr<const TriangularMesh2D>& src_mesh,
                const DataVector<const SrcT>& src_vec,
                const shared_ptr<const MeshD<2>>& dst_mesh,
                const InterpolationFlags& flags);

    DstT at(std::size_t index) const override;
};

template <typename SrcT, typename DstT>
struct InterpolationAlgorithm<TriangularMesh2D, SrcT, DstT, INTERPOLATION_NEAREST> {
    static LazyData<DstT> interpolate(const shared_ptr<const TriangularMesh2D>& src_mesh,
                                      const DataVector<const SrcT>& src_vec,
                                      const shared_ptr<const MeshD<2>>& dst_mesh,
                                      const InterpolationFlags& flags)
    {
        if (src_mesh->empty()) throw BadMesh("interpolate", "Source mesh empty");
        return new NearestNeighborTriangularMesh2DLazyDataImpl<typename std::remove_const<DstT>::type,
                                                       typename std::remove_const<SrcT>::type>
            (src_mesh, src_vec, dst_mesh, flags);
    }

};

PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<double, double>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<dcomplex, dcomplex>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<Vec<2,double>, Vec<2,double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<Vec<2,dcomplex>, Vec<2,dcomplex>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<Vec<3,double>, Vec<3,double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<Vec<3,dcomplex>, Vec<3,dcomplex>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<Tensor2<double>, Tensor2<double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<Tensor2<dcomplex>, Tensor2<dcomplex>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<Tensor3<double>, Tensor3<double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborTriangularMesh2DLazyDataImpl<Tensor3<dcomplex>, Tensor3<dcomplex>>);


// ------------------ Barycentric / Linear interpolation ---------------------

template <typename DstT, typename SrcT>
struct PLASK_API BarycentricTriangularMesh2DLazyDataImpl: public InterpolatedLazyDataImpl<DstT, TriangularMesh2D, const SrcT>
{
    TriangularMesh2D::ElementIndex elementIndex;

    BarycentricTriangularMesh2DLazyDataImpl(
                const shared_ptr<const TriangularMesh2D>& src_mesh,
                const DataVector<const SrcT>& src_vec,
                const shared_ptr<const MeshD<2>>& dst_mesh,
                const InterpolationFlags& flags);

    DstT at(std::size_t index) const override;
};

template <typename SrcT, typename DstT>
struct InterpolationAlgorithm<TriangularMesh2D, SrcT, DstT, INTERPOLATION_LINEAR> {
    static LazyData<DstT> interpolate(const shared_ptr<const TriangularMesh2D>& src_mesh,
                                      const DataVector<const SrcT>& src_vec,
                                      const shared_ptr<const MeshD<2>>& dst_mesh,
                                      const InterpolationFlags& flags)
    {
        if (src_mesh->empty()) throw BadMesh("interpolate", "Source mesh empty");
        return new BarycentricTriangularMesh2DLazyDataImpl<typename std::remove_const<DstT>::type,
                                                       typename std::remove_const<SrcT>::type>
            (src_mesh, src_vec, dst_mesh, flags);
    }

};

PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<double, double>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<dcomplex, dcomplex>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<Vec<2,double>, Vec<2,double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<Vec<2,dcomplex>, Vec<2,dcomplex>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<Vec<3,double>, Vec<3,double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<Vec<3,dcomplex>, Vec<3,dcomplex>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<Tensor2<double>, Tensor2<double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<Tensor2<dcomplex>, Tensor2<dcomplex>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<Tensor3<double>, Tensor3<double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(BarycentricTriangularMesh2DLazyDataImpl<Tensor3<dcomplex>, Tensor3<dcomplex>>);


// ------------------ Element mesh Nearest Neighbor interpolation ---------------------

template <typename DstT, typename SrcT>
struct PLASK_API NearestNeighborElementTriangularMesh2DLazyDataImpl: public InterpolatedLazyDataImpl<DstT, TriangularMesh2D::ElementMesh, const SrcT>
{
    TriangularMesh2D::ElementIndex elementIndex;

    NearestNeighborElementTriangularMesh2DLazyDataImpl(
                const shared_ptr<const TriangularMesh2D::ElementMesh>& src_mesh,
                const DataVector<const SrcT>& src_vec,
                const shared_ptr<const MeshD<2>>& dst_mesh,
                const InterpolationFlags& flags);

    DstT at(std::size_t index) const override;
};

template <typename SrcT, typename DstT>
struct InterpolationAlgorithm<TriangularMesh2D::ElementMesh, SrcT, DstT, INTERPOLATION_NEAREST> {
    static LazyData<DstT> interpolate(const shared_ptr<const TriangularMesh2D::ElementMesh>& src_mesh,
                                      const DataVector<const SrcT>& src_vec,
                                      const shared_ptr<const MeshD<2>>& dst_mesh,
                                      const InterpolationFlags& flags)
    {
        if (src_mesh->empty()) throw BadMesh("interpolate", "Source mesh empty");
        return new NearestNeighborElementTriangularMesh2DLazyDataImpl<typename std::remove_const<DstT>::type,
                                                                      typename std::remove_const<SrcT>::type>
            (src_mesh, src_vec, dst_mesh, flags);
    }

};

PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<double, double>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<dcomplex, dcomplex>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<Vec<2,double>, Vec<2,double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<Vec<2,dcomplex>, Vec<2,dcomplex>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<Vec<3,double>, Vec<3,double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<Vec<3,dcomplex>, Vec<3,dcomplex>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<Tensor2<double>, Tensor2<double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<Tensor2<dcomplex>, Tensor2<dcomplex>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<Tensor3<double>, Tensor3<double>>);
PLASK_API_EXTERN_TEMPLATE_STRUCT(NearestNeighborElementTriangularMesh2DLazyDataImpl<Tensor3<dcomplex>, Tensor3<dcomplex>>);

}   // namespace plask

#endif // PLASK__TRIANGULAR2D_H
