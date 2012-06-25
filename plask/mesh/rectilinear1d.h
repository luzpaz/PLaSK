#ifndef PLASK__RECTILINEAR1D_H
#define PLASK__RECTILINEAR1D_H

/** @file
This file includes rectilinear mesh for 1d space.
*/

#include <vector>
#include <algorithm>
#include <initializer_list>

#include "../vec.h"
#include "../utils/iterators.h"
#include "../utils/interpolation.h"

namespace plask {

/**
 * Rectilinear mesh in 1d space.
 */
class RectilinearMesh1d {

    /// Points coordinates in ascending order.
    std::vector<double> points;

public:

    /// Type of points in this mesh.
    typedef double PointType;

    /// Random access iterator type which allow iterate over all points in this mesh, in ascending order.
    typedef std::vector<double>::const_iterator const_iterator;

    /// @return iterator referring to the first point in this mesh
    const_iterator begin() const { return points.begin(); }

    /// @return iterator referring to the past-the-end point in this mesh
    const_iterator end() const { return points.end(); }

    /// @return vector of points (reference to internal vector)
    const std::vector<double>& getPointsVector() const { return points; }

    /**
     * Find position where @p to_find point could be inserted.
     * @param to_find point to find
     * @return First position where to_find could be insert.
     *         Refer to value equal to @p to_find only if @p to_find is already in mesh.
     *         Can be equal to end() if to_find is higher than all points in mesh
     *         (in such case returned iterator can't be dereferenced).
     */
    const_iterator find(double to_find) const;

    /**
     * Find index where @p to_find point could be inserted.
     * @param to_find point to find
     * @return First index where to_find could be inserted.
     *         Refer to value equal to @p to_find only if @p to_find is already in mesh.
     *         Can be equal to size() if to_find is higher than all points in mesh.
     */
    std::size_t findIndex(double to_find) const { return find(to_find) - begin(); }

    //should we allow for non-const iterators?
    /*typedef std::vector<double>::iterator iterator;
    iterator begin() { return points.begin(); }
    iterator end() { return points.end(); }*/

    /// Construct an empty mesh.
    RectilinearMesh1d() {}

    /**
     * Construct mesh with given points.
     * It use algorithm which has logarithmic time complexity.
     * @param points points, in any order
     */
    RectilinearMesh1d(std::initializer_list<PointType> points);

    /**
     * Construct mesh with points given in a vector.
     * It use algorithm which has logarithmic time complexity pew point in @p points.
     * @param points points, in any order
     */
    RectilinearMesh1d(std::vector<PointType> points);

    /**
     * Compares meshes
     * It use algorithm which has linear time complexity.
     * @param to_compare mesh to compare
     * @return @c true only if this mesh and @p to_compare represents the same set of points
     */
    bool operator==(const RectilinearMesh1d& to_compare) const;

    /**
     * Print mesh to stream
     * @param out stream to print
     * @return out
     */
    friend inline std::ostream& operator<<(std::ostream& out, const RectilinearMesh1d& self) {
        out << "[";
        for (auto p: self.points) {
            out << p << ((p != self.points.back())? ", " : "");
        }
        out << "]";
        return out;
    }

    /// @return number of points in mesh
    std::size_t size() const { return points.size(); }

    /// @return true only if there are no points in mesh
    bool empty() const { return points.empty(); }

    /**
     * Add (1d) point to this mesh.
     * Point is added to mesh only if it is not already included in it.
     * It use algorithm which has O(size()) time complexity.
     * @param new_node_cord coordinate of point to add
     */
    void addPoint(double new_node_cord);

    /**
     * Add points from ordered range.
     * It uses algorithm which has linear time complexity.
     * @param begin, end ordered range of points in ascending order
     * @param points_count_hint number of points in range (can be approximate, or 0)
     * @tparam IteratorT input iterator
     */
    template <typename IteratorT>
    void addOrderedPoints(IteratorT begin, IteratorT end, std::size_t points_count_hint);

    /**
     * Add points from ordered range.
     * It uses algorithm which has linear time complexity.
     * @param begin, end ordered range of points in ascending order
     * @tparam RandomAccessIteratorT random access iterator
     */
    //TODO use iterator traits and write version for input iterator
    template <typename RandomAccessIteratorT>
    void addOrderedPoints(RandomAccessIteratorT begin, RandomAccessIteratorT end) { addOrderedPoints(begin, end, end - begin); }

    /**
     * Add to mesh points: first + i * (last-first) / points_count, where i is in range [0, points_count].
     * It uses algorithm which has linear time complexity.
     * @param first coordinate of the first point
     * @param last coordinate of the last point
     * @param points_count number of points to add
     */
    void addPointsLinear(double first, double last, std::size_t points_count);

    /**
     * Get point by index.
     * @param index index of point, from 0 to size()-1
     * @return point with given @p index
     */
    const double& operator[](std::size_t index) const { return points[index]; }

    /**
     * Remove all points from mesh.
     */
    void clear();

    /**
     * Calculate (using linear interpolation) value of data in point using data in points describe by this mesh.
     * @param data values of data in points describe by this mesh
     * @param point point in which value should be calculate
     * @return interpolated value in point @p point
     */
    template <typename RandomAccessContainer>
    auto interpolateLinear(const RandomAccessContainer& data, double point) -> typename std::remove_reference<decltype(data[0])>::type;

};

// RectilinearMesh1d method templates implementation
template <typename RandomAccessContainer>
auto RectilinearMesh1d::interpolateLinear(const RandomAccessContainer& data, double point) -> typename std::remove_reference<decltype(data[0])>::type {
    std::size_t index = findIndex(point);
    if (index == size()) return data[index - 1];     //TODO what should do if mesh is empty?
    if (index == 0 || points[index] == point) return data[index]; //hit exactly
    // here: points[index-1] < point < points[index]
    return interpolation::linear(points[index-1], data[index-1], points[index], data[index], point);
}

template <typename IteratorT>
inline void RectilinearMesh1d::addOrderedPoints(IteratorT begin, IteratorT end, std::size_t points_count_hint) {
    std::vector<double> result;
    result.reserve(this->size() + points_count_hint);
    std::set_union(this->points.begin(), this->points.end(), begin, end, std::back_inserter(result));
    this->points = std::move(result);
};

}   // namespace plask

#endif // PLASK__RECTILINEAR1D_H
