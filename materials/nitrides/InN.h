#ifndef PLASK__InN_H
#define PLASK__InN_H

/** @file
This file contains undoped InN
*/

#include <plask/material/material.h>


namespace plask { namespace materials {

/**
 * Represent undoped InN, its physical properties.
 */
struct InN: public Semiconductor {

    static constexpr const char* NAME = "InN";

    virtual std::string name() const;
    virtual Tensor2<double> thermk(double T, double h=INFINITY) const;
    virtual double lattC(double T, char x) const;
    virtual double Eg(double T, double e, char point) const;
    virtual Tensor2<double> Me(double T, double e, char point) const;
    virtual Tensor2<double> Mhh(double T, double e) const;
    virtual Tensor2<double> Mlh(double T, double e) const;

protected:
    virtual bool isEqual(const Material& other) const;

};


}} // namespace plask::materials

#endif	//PLASK__InN_H
