#ifndef RECTANGULAR_FILTERED_H
#define RECTANGULAR_FILTERED_H

#include <functional>

#include "rectangular.h"
#include "../utils/numbers_set.h"


namespace plask {

template <int DIM>  // TODO this code is mostly for 2D only
class RectangularFilteredMesh: public MeshD<DIM> {

    const RectangularMesh<DIM>* rectangularMesh;    // TODO jaki wskaźnik? może kopia?

    CompressedSetOfNumbers<std::uint32_t> nodes;

    CompressedSetOfNumbers<std::uint32_t> elements;

public:

    using typename MeshD<DIM>::LocalCoords;

    typedef std::function<bool(const typename RectangularMesh<DIM>::Element&)> Predicate;

    struct Element {

    };

    RectangularFilteredMesh(const RectangularMesh<DIM>* rectangularMesh, const Predicate& predicate)
        : rectangularMesh(rectangularMesh)
    {
        for (auto el_it = rectangularMesh->elements.begin(); el_it != rectangularMesh->elements.end(); ++el_it)
            if (predicate(*el_it)) {
                // TODO wersja 3D
                elements.push_back(el_it.index);
                nodes.insert(el_it->getLoLoIndex());
                nodes.insert(el_it->getLoUpIndex());
                nodes.insert(el_it->getUpLoIndex());
                nodes.push_back(el_it->getUpUpIndex());
            }
        nodes.shrink_to_fit();
        elements.shrink_to_fit();
    }

    LocalCoords at(std::size_t index) const override {
        return rectangularMesh->at(nodes.at(index));
    }

    std::size_t size() const override { return nodes.size(); }

    bool empty() const override { return nodes.empty(); }

    enum:std::size_t { NOT_INCLUDED = RectangularMesh<DIM>::NOT_INCLUDED };

    /**
     * Calculate this mesh index using indexes of axis0 and axis1->
     * @param axis0_index index of axis0, from 0 to axis0->size()-1
     * @param axis1_index index of axis1, from 0 to axis1->size()-1
     * @return this mesh index, from 0 to size()-1, or NOT_INCLUDED
     */
    inline std::size_t index(std::size_t axis0_index, std::size_t axis1_index) const {
        return nodes.indexOf(rectangularMesh->index(axis0_index, axis1_index));
    }

    /**
     * Calculate index of axis0 using this mesh index.
     * @param mesh_index this mesh index, from 0 to size()-1
     * @return index of axis0, from 0 to axis0->size()-1
     */
    inline std::size_t index0(std::size_t mesh_index) const {
        return rectangularMesh->index0(nodes.at(mesh_index));
    }

    /**
     * Calculate index of axis0 using this mesh index.
     * @param mesh_index this mesh index, from 0 to size()-1
     * @return index of axis0, from 0 to axis0->size()-1
     */
    inline std::size_t index1(std::size_t mesh_index) const {
        return rectangularMesh->index1(nodes.at(mesh_index));
    }

    /**
     * Calculate index of major axis using given mesh index.
     * @param mesh_index this mesh index, from 0 to size()-1
     * @return index of major axis, from 0 to majorAxis.size()-1
     */
    inline std::size_t majorIndex(std::size_t mesh_index) const {
        return rectangularMesh->majorIndex(nodes.at(mesh_index));
    }

    /**
     * Calculate index of major axis using given mesh index.
     * @param mesh_index this mesh index, from 0 to size()-1
     * @return index of major axis, from 0 to majorAxis.size()-1
     */
    inline std::size_t minorIndex(std::size_t mesh_index) const {
        return rectangularMesh->minorIndex(nodes.at(mesh_index));
    }

    /**
     * Get point with given mesh indices.
     * @param index0 index of point in axis0
     * @param index1 index of point in axis1
     * @return point with given @p index
     */
    inline Vec<2, double> at(std::size_t index0, std::size_t index1) const {
        return rectangularMesh->at(index0, index1);
    }

    /**
     * Get point with given x and y indexes.
     * @param axis0_index index of axis0, from 0 to axis0->size()-1
     * @param axis1_index index of axis1, from 0 to axis1->size()-1
     * @return point with given axis0 and axis1 indexes
     */
    inline Vec<2,double> operator()(std::size_t axis0_index, std::size_t axis1_index) const {
        return rectangularMesh->operator()(axis0_index, axis1_index);
    }


};

}   // namespace plask

#endif // RECTANGULAR_FILTERED_H
