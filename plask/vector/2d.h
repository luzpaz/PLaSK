#ifndef PLASK__VECTOR2D_H
#define PLASK__VECTOR2D_H

#include <iostream>
#include <config.h>

namespace plask {

#ifndef VEC_TEMPLATE_DEFINED
#define VEC_TEMPLATE_DEFINED
    /// Generic template for 2D and 3D vectors
    template <int dim, typename T=double>
    struct Vec {};
#endif // VEC_TEMPLATE_DEFINED

/**
 * Vector in 2d space.
 */
template <typename T>
struct Vec<2, T> {

    union {
        /// Allow to access to vector coordinates by index.
        T components[2];
        /// Allow to access to vector coordinates by names.
        struct { T c0, c1; };
        struct { T tran, up; };
        struct { T r, z; } rad;     // radial coordinates
        struct { T y, z; } se;      // for surface-emitting lasers (z-axis up)
        struct { T y, z; } z_up;    // for surface-emitting lasers (z-axis up)
        struct { T x, y; } ee;      // for edge emitting lasers (y-axis up), we keep the coordinates right-handed
        struct { T x, y; } y_up;    // for edge emitting lasers (y-axis up), we keep the coordinates right-handed
    };

    /// Construct uninitialized vector.
    Vec() {}

    /**
     * Copy constructor from all other 2d vectors.
     * @param p vector to copy from
     */
    template <typename OtherT>
    Vec(const Vec<2,OtherT>& p): c0(p.c0), c1(p.c1) {}

    /**
     * Construct vector with given coordinates.
     * @param c0, c1 coordinates
     */
    Vec(const T c0, const T c1): c0(c0), c1(c1) {}

    /**
     * Compare two vectors, this and @a p.
     * @param p vector to compare
     * @return true only if this vector and @a p have equals coordinates
     */
    template <typename OtherT>
    bool operator==(const Vec<2,OtherT>& p) const { return p.c0 == c0 && p.c1 == c1; }

    /**
     * Compare two vectors, this and @a p.
     * @param p vector to compare
     * @return true only if this vector and @a p don't have equals coordinates
     */
    template <typename OtherT>
    bool operator!=(const Vec<2,OtherT>& p) const { return p.c0 != c0 || p.c1 != c1; }

    /**
     * Get i-th component
     * WARNING This function does not check if i is valid (for efficiency reasons)
     * @param i number of coordinate
     * @return i-th component
     */
    inline T& operator[](size_t i) {
        return components[i];
    }

    /**
     * Get i-th component
     * WARNING This function does not check if i is valid (for efficiency reasons)
     * @param i number of coordinate
     * @return i-th component
     */
    inline const T& operator[](size_t i) const {
        return components[i];
    }

    /**
     * Calculate sum of two vectors, @a this and @a to_add.
     * @param to_add vector to add, can have different data type (than result type will be found using C++ types promotions rules)
     * @return vectors sum
     */
    template <typename OtherT>
    auto operator+(const Vec<2,OtherT>& to_add) const -> Vec<2,decltype(c0 + to_add.c0)> {
        return Vec<2,decltype(this->c0 + to_add.c0)>(c0 + to_add.c0, c1 + to_add.c1);
    }

    /**
     * Increase coordinates of this vector by coordinates of other vector @a to_add.
     * @param to_add vector to add
     * @return *this (after increase)
     */
    Vec<2,T>& operator+=(const Vec<2,T>& to_add) {
        c0 += to_add.c0;
        c1 += to_add.c1;
        return *this;
    }

    /**
     * Calculate difference of two vectors, @a this and @a to_sub.
     * @param to_sub vector to subtract from this, can have different data type (than result type will be found using C++ types promotions rules)
     * @return vectors difference
     */
    template <typename OtherT>
    auto operator-(const Vec<2,OtherT>& to_sub) const -> Vec<2,decltype(c0 - to_sub.c0)> {
        return Vec<2,decltype(this->c0 - to_sub.c0)>(c0 - to_sub.c0, c1 - to_sub.c1);
    }

    /**
     * Decrease coordinates of this vector by coordinates of other vector @a to_sub.
     * @param to_sub vector to subtract
     * @return *this (after decrease)
     */
    Vec<2,T>& operator-=(const Vec<2,T>& to_sub) {
        c0 -= to_sub.c0;
        c1 -= to_sub.c1;
        return *this;
    }

    /**
     * Calculate this vector multiplied by scalar @a scale.
     * @param scale scalar
     * @return this vector multiplied by scalar
     */
    Vec<2,T> operator*(const T scale) const { return Vec<2,T>(c0 * scale, c1 * scale); }

    /**
     * Multiple coordinates of this vector by @a scalar.
     * @param scalar scalar
     * @return *this (after scale)
     */
    Vec<2,T>& operator*=(const T scalar) {
        c0 *= scalar;
        c1 *= scalar;
        return *this;
    }

    /**
     * Calculate this vector divided by scalar @a scale.
     * @param scale scalar
     * @return this vector divided by scalar
     */
    Vec<2,T> operator/(const T scale) const { return Vec<2,T>(c0 / scale, c1 / scale); }

    /**
     * Divide coordinates of this vector by @a scalar.
     * @param scalar scalar
     * @return *this (after divide)
     */
    Vec<2,T>& operator/=(const T scalar) {
        c0 /= scalar;
        c1 /= scalar;
        return *this;
    }

    /**
     * Calculate vector opposite to this.
     * @return Vec<2,T>(-c0, -c1)
     */
    Vec<2,T> operator-() const {
        return Vec<2,T>(-c0, -c1);
    }

    /**
     * Print vector to stream using format (where c0 and c1 are vector coordinates): [c0, c1]
     * @param out print destination, output stream
     * @param to_print vector to print
     * @return out stream
     */
    friend inline std::ostream& operator<<(std::ostream& out, const Vec<2,T>& to_print) {
        return out << '[' << to_print.c0 << ", " << to_print.c1 << ']';
    }

};


/**
 * Multiple vector @a v by scalar @a scale.
 * @param scale scalar
 * @param v vector
 * @return vector multiplied by scalar
 */
template <typename T>
inline Vec<2,T> operator*(const T scale, const Vec<2,T>& v) { return v*scale; }

/**
 * Calculate vector conjugate.
 * @param v a vector
 * @return conjugate vector
 */
template <typename T>
inline Vec<2,T> conj(const Vec<2,T>& v) { return Vec<2,T> {conj(v.c0), conj(v.c1)}; }

/**
 * Compute dot product of two vectors @a v1 and @a v2
 * @param v1 first vector
 * @param v2 second vector
 * @return dot product v1·v2
 */
template <typename T1, typename T2>
inline auto dot(const Vec<2,T1>& v1, const Vec<2,T2>& v2) -> decltype(v1.c0*v2.c0) {
    return v1.c0 * v2.c0 + v1.c1 * v2.c1;
}

/**
 * Compute dot product of two vectors @a v1 and @a v2
 * @param v1 first vector
 * @param v2 second vector
 * @return dot product v1·v2
 */
template <>
inline auto dot(const Vec<2,double>& v1, const Vec<2,complex<double>>& v2) -> decltype(v1.c0*v2.c0) {
    return v1.c0 * conj(v2.c0) + v1.c1 * conj(v2.c1);
}

/**
 * Compute dot product of two vectors @a v1 and @a v2
 * @param v1 first vector
 * @param v2 second vector
 * @return dot product v1·v2
 */
template <>
inline auto dot(const Vec<2,complex<double>>& v1, const Vec<2,complex<double>>& v2) -> decltype(v1.c0*v2.c0) {
    return v1.c0 * conj(v2.c0) + v1.c1 * conj(v2.c1);
}

/**
 * Helper to create 2d vector.
 * @param c0, c1 vector coordinates.
 */
template <typename T>
inline Vec<2,T> vec(const T c0, const T c1) {
    return Vec<2,T>(c0, c1);
}

} //namespace plask

#endif // PLASK__VECTOR2D_H
