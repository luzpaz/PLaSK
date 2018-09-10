#ifndef PLASK__TRIANGULAR2D_H
#define PLASK__TRIANGULAR2D_H

#include "mesh.h"
#include <array>

namespace plask {

struct TriangularMesh2D: public MeshD<2> {

    using MeshD<2>::LocalCoords;
    typedef std::vector<LocalCoords> LocalCoordsVec;
    typedef LocalCoordsVec::iterator iterator;
    typedef LocalCoordsVec::const_iterator const_iterator;

    LocalCoordsVec nodes;

    typedef std::array<std::size_t, 3> TriangleNodeIndexes;

    std::vector< TriangleNodeIndexes > elementsNodes;

    struct Element {
        TriangleNodeIndexes triangleNodes;
        TriangularMesh2D& mesh;   // for getting access to the nodes

        Element(TriangularMesh2D& mesh, TriangleNodeIndexes triangleNodes)
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
         * Get coordinate of the triangle vertex.
         * @param index index of vertex in the triangle corresponded to this element; equals to 0, 1 or 2
         * @return coordinate of the triangle vertex
         */
        LocalCoords getNode(std::size_t index) const noexcept {
            return mesh.nodes[getNodeIndex(index)];
        }

        /**
         * Get centroid of the triangle corresponded to this element.
         * @return centroid of the triangle corresponded to this element
         */
        LocalCoords getMidpoint() const {
            return (getNode(0)+getNode(1)+getNode(2)) / 3.0;
        }

        double getArea() const noexcept {
            // formula comes from http://www.mathguru.com/level2/application-of-coordinate-geometry-2007101600011139.aspx
            const LocalCoords A = getNode(0);
            const LocalCoords B = getNode(1);
            const LocalCoords C = getNode(2);
            return abs( (A.c0 - C.c0) * (B.c1 - A.c1)
                      - (A.c0 - B.c0) * (C.c1 - A.c1) ) / 2.0;
        }
    };

    struct Elements {
        TriangularMesh2D& mesh;
    };

    /**
     * Instance of this class allows for adding triangles to the mesh effectively.
     */
    struct Builder {
        std::map<LocalCoords, std::size_t> indexOfNode; ///< map nodes to their indexes in mesh.nodes vector
        TriangularMesh2D& mesh;

        /**
         * Construct builder which will add triangles to the given @p mesh.
         * @param mesh triangles destination
         */
        explicit Builder(TriangularMesh2D& mesh);

        /**
         * Add a triangle to the mesh.
         * @param p1, p2, p3 coordinates of vertices of the triangle to add
         * @return <code>*this</code>
         */
        Builder& add(LocalCoords p1, LocalCoords p2, LocalCoords p3);

    private:
        std::size_t addNode(LocalCoords node);
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
    iterator begin() { return nodes.begin(); }
    iterator end() { return nodes.end(); }
    const_iterator begin() const { return nodes.begin(); }
    const_iterator end() const { return nodes.end(); }
};

}   // namespace plask

#endif // PLASK__TRIANGULAR2D_H
