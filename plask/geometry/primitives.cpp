#include "primitives.h"

#include <algorithm>

namespace plask {

inline void ensureLo(double& to_be_lo, double how_lo) {
    if (how_lo < to_be_lo) to_be_lo = how_lo;
}

inline void ensureHi(double& to_be_hi, double how_hi) {
    if (how_hi > to_be_hi) to_be_hi = how_hi;
}

//------------- Rect2d ---------------------

bool Rect2d::operator ==(const Rect2d &r) const {
    return lower == r.lower && upper == r.upper;
}

bool Rect2d::operator !=(const Rect2d &r) const {
    return lower != r.lower || upper != r.upper;
}

void Rect2d::fix() {
    if (lower.c0 > upper.c0) std::swap(lower.c0, upper.c0);
    if (lower.c1 > upper.c1) std::swap(lower.c1, upper.c1);
}


bool Rect2d::inside(const Vec<2, double >& p) const {
    return lower.c0/**/ <= p.c0 && p.c0 <= upper.c0 &&
           lower.c1 <= p.c1 && p.c1 <= upper.c1;
}

bool Rect2d::intersect(const plask::Rect2d& other) const {
    return !(
        lower.c0 > other.upper.c0 ||
        lower.c1 > other.upper.c1 ||
        upper.c0 < other.lower.c0 ||
        upper.c1 < other.lower.c1
    );
}

void Rect2d::include(const Vec<2, double >& p) {
    if (p.c0 < lower.c0) lower.c0 = p.c0; else ensureHi(upper.c0, p.c0);
    if (p.c1 < lower.c1) lower.c1 = p.c1; else ensureHi(upper.c1, p.c1);
}

void Rect2d::include(const plask::Rect2d& other) {
    ensureLo(lower.c0, other.lower.c0);
    ensureLo(lower.c1, other.lower.c1);
    ensureHi(upper.c0, other.upper.c0);
    ensureHi(upper.c1, other.upper.c1);
}

//------------- Rect3d ---------------------

bool Rect3d::operator ==(const Rect3d &r) const {
    return lower == r.lower && upper == r.upper;
}

bool Rect3d::operator !=(const Rect3d &r) const {
    return lower != r.lower || upper != r.upper;
}

void Rect3d::fix() {
    if (lower.c0 > upper.c0) std::swap(lower.c0, upper.c0);
    if (lower.c1 > upper.c1) std::swap(lower.c1, upper.c1);
    if (lower.c2 > upper.c2) std::swap(lower.c2, upper.c2);
}


bool Rect3d::inside(const Vec<3, double >& p) const {
    return lower.c0 <= p.c0 && p.c0 <= upper.c0 &&
           lower.c1 <= p.c1 && p.c1 <= upper.c1 &&
           lower.c2 <= p.c2 && p.c2 <= upper.c2;
}

bool Rect3d::intersect(const plask::Rect3d& other) const {
    return !(
        lower.c0 > other.upper.c0 ||
        lower.c1 > other.upper.c1 ||
        lower.c2 > other.upper.c2 ||
        upper.c0 < other.lower.c0 ||
        upper.c1 < other.lower.c1 ||
        upper.c2 < other.lower.c2
    );
}

void Rect3d::include(const Vec<3, double >& p) {
    if (p.c0 < lower.c0) lower.c0 = p.c0; else ensureHi(upper.c0, p.c0);
    if (p.c1 < lower.c1) lower.c1 = p.c1; else ensureHi(upper.c1, p.c1);
    if (p.c2 < lower.c2) lower.c2 = p.c2; else ensureHi(upper.c2, p.c2);
}

void Rect3d::include(const plask::Rect3d& other) {
    ensureLo(lower.c0, other.lower.c0);
    ensureLo(lower.c1, other.lower.c1);
    ensureLo(lower.c2, other.lower.c2);
    ensureHi(upper.c0, other.upper.c0);
    ensureHi(upper.c1, other.upper.c1);
    ensureHi(upper.c2, other.upper.c2);
}

const Primitive<2>::DVec Primitive<2>::ZERO_VEC = Vec<2>(0.0, 0.0);
const Primitive<3>::DVec Primitive<3>::ZERO_VEC = Vec<3>(0.0, 0.0, 0.0);

}
