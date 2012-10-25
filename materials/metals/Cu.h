#ifndef PLASK__Cu_H
#define PLASK__Cu_H

/** @file
This file includes Cu
*/

#include <plask/material/material.h>

namespace plask {

/**
 * Represent Cu, its physical properties.
 */
struct Cu: public Metal {

    static constexpr const char* NAME = "Cu";

    virtual std::string name() const;
    virtual std::pair<double,double> cond(double T) const;
    virtual std::pair<double,double> thermk(double T, double t) const;
    virtual double nr(double wl, double T) const;
    virtual double absp(double wl, double T) const;

};


} // namespace plask

#endif	//PLASK__Cu_H
