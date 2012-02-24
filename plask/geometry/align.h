#ifndef PLASK__GEOMETRY_ALIGN_H
#define PLASK__GEOMETRY_ALIGN_H

#include "transform.h"

namespace plask {

namespace align {

enum DIRECTION_2D {
    DIR2D_TRAN, 
    DIR2D_UP
};

enum DIRECTION_3D {
    DIR3D_LON, 
    DIR3D_TRAN, 
    DIR3D_UP
};

/**
 * Base class for one direction aligner in 2d space.
 */
template <DIRECTION_2D _direction>
struct Aligner2d {
    
    static const DIRECTION_2D direction = _direction;
    
    virtual double getAlign(double low, double hi) const = 0;
    
    //This version is called if caller knows bounding box.
    inline double align(const Translation<2>& toAlign, const Box2d& childBoundingBox) const {
        toAlign.translation.components[direction] = getAlign(childBoundingBox.lower.components[direction], childBoundingBox.upper.components[direction]);
    }
    
    virtual double align(const Translation<2>& toAlign) const {
        align(toAlign, toAlign.getChild()->getBoundingBox());
    }
    
    //@return @c true only if bounding box is needed to calculate position
    //virtual bool useBoundingBox() const { return true; }
    
    virtual Aligner2d<direction>* clone() const = 0;
    
    virtual ~Aligner2d() {}
    
};

template <DIRECTION_2D direction>
struct TranslationAligner2d: public Aligner2d<direction> {
    
    ///Translation in aligner direction
    double translation;
    
    TranslationAligner2d(double translation): translation(translation) {}
    
    virtual double getAlign(double low, double hi) const {
        return translation;
    }
    
    virtual double align(const Translation<2>& toAlign) const {
        toAlign.translation.components[direction] = translation;
    }
    
    virtual TranslationAligner2d<direction>* clone() const {
        return new TranslationAligner2d<direction>(translation);
    }
};

/**
 * Base class for two directions aligner in 3d space.
 */
template <DIRECTION_3D _direction1, DIRECTION_3D _direction2>
struct Aligner3d {
    
    static_assert(_direction1 == _direction2, "Wrong Aligner3d template parameters, two different directions are required.");
    
    virtual ~Aligner3d() {}
    
    static const DIRECTION_3D direction1 = _direction1, direction2 = _direction2;
    
    //This version is called if caller knows bounding box.
    virtual double align(const Translation<3>& toAlign, const Box3d& childBoundingBox) const = 0;
    
    virtual double align(const Translation<3>& toAlign) const {
        align(toAlign, toAlign.getChild()->getBoundingBox());
    }
    
    virtual Aligner3d<direction1, direction2>* clone() const = 0;

};

template <DIRECTION_3D direction1, DIRECTION_3D direction2>
struct TranslationAligner3d: public Aligner3d<direction1, direction2> {
    
    ///Translations in aligner directions.
    double dir1translation, dir2translation;
    
    TranslationAligner3d(double dir1translation, double dir2translation): dir1translation(dir1translation), dir2translation(dir2translation) {}
    
    virtual double align(const Translation<2>& toAlign, const Box2d&) const {
        align(toAlign);
    }
    
    virtual double align(const Translation<2>& toAlign) const {
        toAlign.translation.components[direction1] = dir1translation;
        toAlign.translation.components[direction2] = dir2translation;
    }
    
    virtual TranslationAligner3d<direction1, direction2>* clone() const {
        return new TranslationAligner3d<direction1, direction2>(dir1translation, dir2translation);
    }
};

/**
 * Aligner 3d which compose and use two 2d aligners. 
 */
template <DIRECTION_3D direction1, DIRECTION_3D direction2>
class ComposeAligner3d: public Aligner3d<direction1, direction2> {
    
    Aligner2d<direction1>* dir1aligner;
    Aligner2d<direction2>* dir2aligner;
    
public:
    
    ComposeAligner3d(const Aligner2d<direction1>& dir1aligner, const Aligner2d<direction2>& dir2aligner)
        : dir1aligner(dir1aligner.clone()), dir2aligner(dir2aligner.clone()) {}
    
    ComposeAligner3d(const ComposeAligner3d<direction1, direction2>& toCopy)
        : dir1aligner(toCopy.dir1aligner->clone()), dir2aligner(toCopy.dir2aligner->clone()) {}
    
    ComposeAligner3d(const ComposeAligner3d<direction2, direction1>& toCopy)
        : dir1aligner(toCopy.dir2aligner->clone()), dir2aligner(toCopy.dir1aligner->clone()) {}
    
    ComposeAligner3d(ComposeAligner3d<direction1, direction2>&& toMove)
        : dir1aligner(toMove.dir1aligner), dir2aligner(toMove.dir2aligner) {
        toMove.dir1aligner = 0; toMove.dir2aligner = 0;
    }
    
    ComposeAligner3d(ComposeAligner3d<direction2, direction1>&& toMove)
        : dir1aligner(toMove.dir2aligner), dir2aligner(toMove.dir1aligner) {
        toMove.dir1aligner = 0; toMove.dir2aligner = 0;
    }
    
    ~ComposeAligner3d() { delete dir1aligner; delete dir2aligner; }
    
    virtual double align(const Translation<3>& toAlign, const Box3d& childBoundingBox) const {
         toAlign.translation.components[direction1] =
                 dir1aligner->getAlign(childBoundingBox.lower.components[direction1], childBoundingBox.upper.components[direction1]);
         toAlign.translation.components[direction2] =
                 dir2aligner->getAlign(childBoundingBox.lower.components[direction2], childBoundingBox.upper.components[direction2]);
    }
    
    virtual ComposeAligner3d<direction1, direction2>* clone() const {
        return new ComposeAligner3d<direction1, direction2>(*this);
    }
    
};

template <DIRECTION_3D direction1, DIRECTION_3D direction2>
inline ComposeAligner3d<direction1, direction2> operator&(const Aligner2d<direction1>& dir1aligner, const Aligner2d<direction2>& dir2aligner) {
    return ComposeAligner3d<direction1, direction2>(dir1aligner, dir2aligner);
}

namespace details {

typedef double alignStrategy(double lo, double hi);
inline double lowToZero(double lo, double hi) { return -lo; }
inline double hiToZero(double lo, double hi) { return -hi; }
inline double centerToZero(double lo, double hi) { return -(lo+hi)/2.0; }

template <DIRECTION_2D direction, alignStrategy strategy>
struct Aligner2dImpl: public Aligner2d<direction> {
    
    virtual double getAlign(double low, double hi) const {
        return strategy(low, hi);
    }
    
    virtual Aligner2dImpl<direction, strategy>* clone() const {
        return new Aligner2dImpl<direction, strategy>();
    }
};

template <DIRECTION_3D direction1, alignStrategy strategy1, DIRECTION_3D direction2, alignStrategy strategy2>
struct Aligner3dImpl: public Aligner3d<direction1, direction2> {
    
    virtual double align(const Translation<3>& toAlign, const Box3d& childBoundingBox) const {
        toAlign.translation.components[direction1] = strategy1(childBoundingBox.lower.components[direction1], childBoundingBox.upper.components[direction1]);
        toAlign.translation.components[direction2] = strategy2(childBoundingBox.lower.components[direction2], childBoundingBox.upper.components[direction2]);
    }
    
    virtual Aligner3dImpl<direction1, strategy1, direction2, strategy2>* clone() const {
        return new Aligner3dImpl<direction1, strategy1, direction2, strategy2>();
    }
};

}   // namespace details

//2d trasnlation aligners:
typedef details::Aligner2dImpl<DIR2D_TRAN, details::lowToZero> Left;
typedef details::Aligner2dImpl<DIR2D_TRAN, details::hiToZero> Right;
typedef details::Aligner2dImpl<DIR2D_TRAN, details::centerToZero> LRCenter;
typedef TranslationAligner2d<DIR2D_TRAN> Tran;

//3d lon/tran aligners:
typedef details::Aligner3dImpl<DIR3D_LON, details::lowToZero, DIR3D_TRAN, details::lowToZero> NearLeft;
typedef details::Aligner3dImpl<DIR3D_LON, details::lowToZero, DIR3D_TRAN, details::hiToZero> NearRight;
typedef details::Aligner3dImpl<DIR3D_LON, details::lowToZero, DIR3D_TRAN, details::centerToZero> NearLRCenter;
typedef details::Aligner3dImpl<DIR3D_LON, details::hiToZero, DIR3D_TRAN, details::lowToZero> FarLeft;
typedef details::Aligner3dImpl<DIR3D_LON, details::hiToZero, DIR3D_TRAN, details::hiToZero> FarRight;
typedef details::Aligner3dImpl<DIR3D_LON, details::hiToZero, DIR3D_TRAN, details::centerToZero> FarLRCenter;
typedef details::Aligner3dImpl<DIR3D_LON, details::centerToZero, DIR3D_TRAN, details::lowToZero> NFCenerLeft;
typedef details::Aligner3dImpl<DIR3D_LON, details::centerToZero, DIR3D_TRAN, details::hiToZero> NFCenerRight;
typedef details::Aligner3dImpl<DIR3D_LON, details::centerToZero, DIR3D_TRAN, details::centerToZero> NFCenerLRCenter;
typedef TranslationAligner3d<DIR3D_LON, DIR3D_TRAN> LonTran;
typedef ComposeAligner3d<DIR3D_LON, DIR3D_TRAN> NFLR;
//TODO mixed variants

}   // namespace align
}   // namespace plask

#endif // PLASK__GEOMETRY_ALIGN_H
