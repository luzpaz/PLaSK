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
#ifndef PLASK__VECTOR2D_H
#define PLASK__VECTOR2D_H

/** @file
This file contains implementation of vector in 2D space.
*/

#include <iostream>

#include "../math.hpp"
#include "plask/exceptions.hpp"

#include "common.hpp"
#include <cassert>

#include "../utils/metaprog.hpp"   // for is_callable
#include "../utils/warnings.hpp"

namespace plask {

/**
 * Vector in 2D space.
 */
template <typename T>
struct Vec<2,T> {

    static const int DIMS = 2;

    T c0, c1;

    T& tran() { return c0; }
    constexpr const T& tran() const { return c0; }

    T& vert() { return c1; }
    constexpr const T& vert() const { return c1; }

    // radial coordinates
    T& rad_r() { return c0; }
    constexpr const T& rad_r() const { return c0; }
    T& rad_z() { return c1; }
    constexpr const T& rad_z() const { return c1; }

    // for surface-emitting lasers (z-axis up)
    T& se_y() { return c0; }
    constexpr const T& se_y() const { return c0; }
    T& se_z() { return c1; }
    constexpr const T& se_z() const { return c1; }

    // for surface-emitting lasers (z-axis up)
    T& zup_y() { return c0; }
    constexpr const T& z_up_y() const { return c0; }
    T& zup_z() { return c1; }
    constexpr const T& z_up_z() const { return c1; }

    // for edge emitting lasers (y-axis up), we keep the coordinates right-handed
    T& ee_x() { return c0; }
    constexpr const T& ee_x() const { return c0; }
    T& ee_y() { return c1; }
    constexpr const T& ee_y() const { return c1; }

    // for edge emitting lasers (y-axis up), we keep the coordinates right-handed
    T& yup_x() { return c0; }
    constexpr const T& y_up_x() const { return c0; }
    T& yup_y() { return c1; }
    constexpr const T& y_up_y() const { return c1; }


    /**
     * Type of iterator over components.
     */
    typedef T* iterator;

    /**
     * Type of const iterator over components.
     */
    typedef const T* const_iterator;

    /// Construct uninitialized vector.
    Vec() {}

    /**
     * Copy constructor from all other 2D vectors.
     * @param p vector to copy from
     */
    template <typename OtherT>
    constexpr Vec(const Vec<2,OtherT>& p): c0(p.c0), c1(p.c1) {}

    /**
     * Construct vector with given components.
     * @param c0__tran, c1__up components
     */
    constexpr Vec(const T& c0__tran, const T& c1__up): c0(c0__tran), c1(c1__up) {}

    /**
     * Construct vector components given in std::pair.
     * @param comp components
     */
    template <typename T0, typename T1>
    constexpr Vec(const std::pair<T0,T1>& comp): c0(comp.first), c1(comp.second) {}

    /**
     * Construct vector with components read from input iterator (including C array).
     * @param inputIt input iterator with minimum 3 objects available
     * @tparam InputIteratorType input iterator type, must allow for postincrementation and derefrence operation
     */
    template <typename InputIteratorType>
    static inline Vec<2,T> fromIterator(InputIteratorType inputIt) {
        Vec<2,T> result;
        result.c0 = T(*inputIt);
        result.c1 = T(*++inputIt);
        return result;
    }

    /**
     * Get begin iterator over components.
     * @return begin iterator over components
     */
    iterator begin() { return &c0; }

    /**
     * Get begin const iterator over components.
     * @return begin const iterator over components
     */
    const_iterator begin() const { return &c0; }

    /**
     * Get end iterator over components.
     * @return end iterator over components
     */
    iterator end() { return &c0 + 2; }

    /**
     * Get end const iterator over components.
     * @return end const iterator over components
     */
    const_iterator end() const { return &c0 + 2; }

    /**
     * Compare two vectors, this and @p p.
     * @param p vector to compare
     * @return true only if this vector and @p p have equals coordinates
     */
    template <typename OtherT>
    constexpr bool operator==(const Vec<2,OtherT>& p) const { return p.c0 == c0 && p.c1 == c1; }

    /**
     * Check if two vectors, this and @p p are almost equal.
     * @param p vector to compare
     * @param abs_supremum maximal allowed difference for one coordinate
     * @return @c true only if this vector and @p p have almost equals coordinates
     */
    template <typename OtherT, typename SuprType>
    constexpr bool equals(const Vec<2, OtherT>& p, const SuprType& abs_supremum) const {
        return is_zero(p.c0 - c0, abs_supremum) && is_zero(p.c1 - c1, abs_supremum); }

    /**
     * Check if two vectors, this and @p p are almost equal.
     * @param p vector to compare
     * @return @c true only if this vector and @p p have almost equals coordinates
     */
    template <typename OtherT>
    constexpr bool equals(const Vec<2, OtherT>& p) const {
        return is_zero(p.c0 - c0) && is_zero(p.c1 - c1); }

    /**
     * Compare two vectors, this and @p p.
     * @param p vector to compare
     * @return true only if this vector and @p p don't have equals coordinates
     */
    template <typename OtherT>
    constexpr bool operator!=(const Vec<2,OtherT>& p) const { return p.c0 != c0 || p.c1 != c1; }

    /**
     * Get i-th component
     * WARNING This function does not check if it is valid (for efficiency reasons)
     * @param i number of coordinate
     * @return i-th component
     */
    inline T& operator[](size_t i) {
        assert(i < 2);
        return *(&c0 + i);
    }

    /**
     * Get i-th component
     * WARNING This function does not check if it is valid (for efficiency reasons)
     * @param i number of coordinate
     * @return i-th component
     */
    inline const T& operator[](size_t i) const {
        assert(i < 2);
        return *(&c0 + i);
    }

    /**
     * Calculate sum of two vectors, @c this and @p other.
     * @param other vector to add, can have different data type (than result type will be found using C++ types promotions rules)
     * @return vectors sum
     */
    template <typename OtherT>
    constexpr auto operator+(const Vec<2,OtherT>& other) const -> Vec<2,decltype(c0 + other.c0)> {
        return Vec<2,decltype(this->c0 + other.c0)>(c0 + other.c0, c1 + other.c1);
    }

    /**
     * Increase coordinates of this vector by coordinates of other vector @p other.
     * @param other vector to add
     * @return *this (after increase)
     */
    Vec<2,T>& operator+=(const Vec<2,T>& other) {
        c0 += other.c0;
        c1 += other.c1;
        return *this;
    }

    /**
     * Calculate difference of two vectors, @c this and @p other.
     * @param other vector to subtract from this, can have different data type (than result type will be found using C++ types promotions rules)
     * @return vectors difference
     */
    template <typename OtherT>
    constexpr auto operator-(const Vec<2,OtherT>& other) const -> Vec<2,decltype(c0 - other.c0)> {
        return Vec<2,decltype(this->c0 - other.c0)>(c0 - other.c0, c1 - other.c1);
    }

    /**
     * Decrease coordinates of this vector by coordinates of other vector @p other.
     * @param other vector to subtract
     * @return *this (after decrease)
     */
    Vec<2,T>& operator-=(const Vec<2,T>& other) {
        c0 -= other.c0;
        c1 -= other.c1;
        return *this;
    }

    /**
     * Calculate this vector multiplied by scalar @p scale.
     * @param scale scalar
     * @return this vector multiplied by scalar
     */
    template <typename OtherT>
    constexpr auto operator*(const OtherT scale) const -> Vec<2,decltype(c0*scale)> {
PLASK_NO_CONVERSION_WARNING_BEGIN
        return Vec<2,decltype(c0*scale)>(c0 * scale, c1 * scale);
PLASK_NO_WARNING_END
    }

    /**
     * Multiple coordinates of this vector by @p scalar.
     * @param scalar scalar
     * @return *this (after scale)
     */
    Vec<2,T>& operator*=(const T scalar) {
        c0 *= scalar;
        c1 *= scalar;
        return *this;
    }

    /**
     * Calculate this vector divided by scalar @p scale.
     * @param scale scalar
     * @return this vector divided by scalar
     */
    constexpr Vec<2,T> operator/(const T scale) const { return Vec<2,T>(c0 / scale, c1 / scale); }

    /**
     * Divide coordinates of this vector by @p scalar.
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
    constexpr Vec<2,T> operator-() const {
        return Vec<2,T>(-c0, -c1);
    }

    /**
     * Square each component of tensor
     * \return squared tensor
     */
    Vec<2,T> sqr() const {
        return Vec<2,T>(c0*c0, c1*c1);
    }

    /**
     * Square each component of tensor in place
     * \return *this (squared)
     */
    Vec<2,T>& sqr_inplace() {
        c0 *= c0; c1 *= c1;
        return *this;
    }

    /**
     * Square root of each component of tensor
     * \return squared tensor
     */
    Vec<2,T> sqrt() const {
        return Vec<2,T>(std::sqrt(c0), std::sqrt(c1));
    }

    /**
     * Square root of each component of tensor in place
     * \return *this (squared)
     */
    Vec<2,T>& sqrt_inplace() {
        c0 = std::sqrt(c0); c1 = std::sqrt(c1);
        return *this;
    }

    /**
     * Power of each component of tensor
     * \return squared tensor
     */
    template <typename OtherT>
    Vec<2,T> pow(OtherT a) const {
        return Vec<2,T>(std::pow(c0, a), std::pow(c1, a));
    }

    /**
     * Change i-th coordinate to oposite.
     * WARNING This function does not check if it is valid (for efficiency reasons)
     * @param i number of coordinate
     */
    inline void flip(size_t i) {
        assert(i < 2);
        operator[](i) = -operator[](i);
    }

    /**
     * Get vector similar to this but with changed i-th component to oposite.
     * WARNING This function does not check if it is valid (for efficiency reasons)
     * @param i number of coordinate
     * @return vector similar to this but with changed i-th component to oposite
     */
    inline Vec<2,T> flipped(size_t i) {
        Vec<2,T> res = *this;
        res.flip(i);
        return res;
    }

    /**
     * Print vector to stream using format (where c0 and c1 are vector components): [c0, c1]
     * @param out print destination, output stream
     * @param to_print vector to print
     * @return out stream
     */
    friend inline std::ostream& operator<<(std::ostream& out, const Vec<2,T>& to_print) {
        return out << '[' << to_print.c0 << ", " << to_print.c1 << ']';
    }

    /**
     * A lexical comparison of two vectors, allow to use vector in std::set and std::map as key type.
     *
     * It supports NaN-s (which, due to this method, is greater than all other numbers).
     * @param v vectors to compare
     * @return @c true only if @c this is smaller than the @p v
     */
    template<class OT> inline
    bool operator< (Vec<2, OT> const& v) const {
        if (dbl_compare_lt(this->c0, v.c0)) return true;
        if (dbl_compare_gt(this->c0, v.c0)) return false;
        return dbl_compare_lt(this->c1, v.c1);
    }
};

/**
 * Calculate vector conjugate.
 * @param v a vector
 * @return conjugate vector
 */
template <typename T>
inline constexpr Vec<2,T> conj(const Vec<2,T>& v) { return Vec<2,T>(conj(v.c0), conj(v.c1)); }

/**
 * Compute dot product of two vectors @p v1 and @p v2.
 * @param v1 first vector
 * @param v2 second vector
 * @return dot product v1·v2
 */
template <typename T1, typename T2>
inline auto dot(const Vec<2,T1>& v1, const Vec<2,T2>& v2) -> decltype(v1.c0*v2.c0) {
    return ::plask::fma(v1.c0, v2.c0, v1.c1 * v2.c1);	//MSVC needs ::plask::
}

/**
 * Compute (analog of 3d) cross product of two vectors @p v1 and @p v2.
 * @param v1, v2 vectors
 * @return (analog of 3d) cross product of v1 and v2
 */
template <typename T1, typename T2>
inline auto cross(const Vec<2,T1>& v1, const Vec<2,T2>& v2) -> decltype(v1.c0*v2.c1) {
    return ::plask::fma(v1.c0, v2.c1, - v1.c1 * v2.c0);	//MSVC needs ::plask::
}

/**
 * Compute dot product of two vectors @p v1 and @p v2.
 * @param v1 first vector
 * @param v2 second vector
 * @return dot product v1·v2
 */
//template <>   //MSVC2015 doesn't support this specialization, and using overloding shouldn't have negative consequences
inline auto dot(const Vec<2,dcomplex>& v1, const Vec<2,double>& v2) -> decltype(v1.c0*v2.c0) {
    return ::plask::fma(conj(v1.c0), v2.c0, conj(v1.c1) * v2.c1);	//MSVC needs ::plask::
}

/**
 * Compute dot product of two vectors @p v1 and @p v2.
 * @param v1 first vector
 * @param v2 second vector
 * @return dot product v1·v2
 */
//template <>   //MSVC2015 doesn't support this specialization, and using overloding shouldn't have negative consequences
inline auto dot(const Vec<2,dcomplex>& v1, const Vec<2,dcomplex>& v2) -> decltype(v1.c0*v2.c0) {
    return ::plask::fma(conj(v1.c0), v2.c0, conj(v1.c1) * v2.c1);	//MSVC needs ::plask::
}

/**
 * Helper to create 2D vector.
 * @param c0__tran, c1__up vector coordinates
 * @return constructed vector
 */
template <typename T>
inline constexpr Vec<2,T> vec(const T c0__tran, const T c1__up) {
    return Vec<2,T>(c0__tran, c1__up);
}

/// Specialization of NaNImpl which add support for 2D vectors.
template <typename T>
struct NaNImpl<Vec<2,T>> {
    static constexpr Vec<2,T> get() { return Vec<2,T>(NaN<T>(), NaN<T>()); }
};

/// Specialization of ZeroImpl which add support for 2D vectors.
template <typename T>
struct ZeroImpl<Vec<2,T>> {
    static constexpr Vec<2,T> get() { return Vec<2,T>(0., 0.); }
};

/// Check if the vector is almost zero
/// \param v vector to verify
template <typename T>
inline bool is_zero(const Vec<2,T>& v) {
    return is_zero(v.c0) && is_zero(v.c1);
}


PLASK_API_EXTERN_TEMPLATE_SPECIALIZATION_STRUCT(Vec<2, double>)
PLASK_API_EXTERN_TEMPLATE_SPECIALIZATION_STRUCT(Vec<2, std::complex<double> >)
PLASK_API_EXTERN_TEMPLATE_SPECIALIZATION_STRUCT(Vec<2, int>)

} //namespace plask

namespace std {
    template <typename T>
    plask::Vec<2,T> sqrt(plask::Vec<2,T> vec) {
        return vec.sqrt();
    }

    template <typename T, typename OtherT>
    plask::Vec<2,T> pow(plask::Vec<2,T> vec, OtherT a) {
        return vec.pow(a);
    }
}

#endif // PLASK__VECTOR2D_H
