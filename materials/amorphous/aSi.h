#ifndef PLASK__aSi_H
#define PLASK__aSi_H

/** @file
This file contains a-Si
*/

#include <plask/material/material.h>

namespace plask { namespace materials {

/**
 * Represent a-Si, its physical properties.
 */
struct aSi: public Dielectric {

    static constexpr const char* NAME = "aSi";

    virtual std::string name() const;
    virtual Tensor2<double> thermk(double T, double h=INFINITY) const;
    virtual double nr(double wl, double T) const;

protected:
    virtual bool isEqual(const Material& other) const;

};


}} // namespace plask::materials

#endif	//PLASK__aSi_H
