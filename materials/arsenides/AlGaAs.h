#ifndef PLASK__AlGaAs_H
#define PLASK__AlGaAs_H

/** @file
This file includes undoped AlGaAs
*/

#include <plask/material/material.h>
#include "GaAs.h"
#include "AlAs.h"

namespace plask {

/**
 * Represent undoped AlGaAs, its physical properties.
 */
struct AlGaAs: public Semiconductor {

    static constexpr const char* NAME = "AlGaAs";

    AlGaAs(const Material::Composition& Comp);
    virtual std::string str() const;
    virtual std::string name() const;
    virtual double lattC(double T, char x) const;
    virtual double Eg(double T, char point) const;
    virtual double Dso(double T) const;
    virtual std::pair<double,double> Me(double T, char point) const;
    virtual std::pair<double,double> Mhh(double T, char point) const;
    virtual std::pair<double,double> Mlh(double T, char point) const;
    virtual double ac(double T) const;
    virtual double av(double T) const;
    virtual double b(double T) const;
    virtual double c11(double T) const;
    virtual double c12(double T) const;
    virtual std::pair<double,double> thermCond(double T, double t) const;
    virtual double nr(double wl, double T) const;
    virtual double absp(double wl, double T) const;

protected:
    double Al,
           Ga;

    GaAs mGaAs;
    AlAs mAlAs;

};

} // namespace plask

#endif	//PLASK__AlGaAs_H
