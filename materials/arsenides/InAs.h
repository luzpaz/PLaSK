#ifndef PLASK__InAs_H
#define PLASK__InAs_H

/** @file
This file contains undoped InAs
*/

#include <plask/material/material.h>

namespace plask { namespace materials {

/**
 * Represent undoped InAs, its physical properties.
 */
struct InAs: public Semiconductor {

    static constexpr const char* NAME = "InAs";

    virtual std::string name() const;
    virtual double lattC(double T, char x) const;
    virtual double Eg(double T, double e, char point) const;
    virtual double Dso(double T, double e) const;
    virtual Tensor2<double> Me(double T, double e, char point) const;
    virtual Tensor2<double> Mhh(double T, double e) const;
    virtual Tensor2<double> Mlh(double T, double e) const;
    virtual double ac(double T) const;
    virtual double av(double T) const;
    virtual double b(double T) const;
    virtual double c11(double T) const;
    virtual double c12(double T) const;
    virtual Tensor2<double> thermk(double T, double t) const;

protected:
    virtual bool isEqual(const Material& other) const;

};


}} // namespace plask::materials

#endif	//PLASK__InAs_H
