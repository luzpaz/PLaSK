#ifndef PLASK__AuZn_H
#define PLASK__AuZn_H

/** @file
This file contains AuZn
*/

#include <plask/material/material.h>

namespace plask { namespace materials {

/**
 * Represent AuZn, its physical properties.
 */
struct AuZn: public Metal {

    static constexpr const char* NAME = "AuZn";

    virtual std::string name() const;
    virtual Tensor2<double> cond(double T) const;
    virtual Tensor2<double> thermk(double T, double t) const;
    virtual double nr(double wl, double T) const;
    virtual double absp(double wl, double T) const;

protected:
    virtual bool isEqual(const Material& other) const;
};


}} // namespace plask::materials

#endif	//PLASK__AuZn_H
