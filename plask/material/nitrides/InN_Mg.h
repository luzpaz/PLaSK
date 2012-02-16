#ifndef PLASK__InN_Mg_H
#define PLASK__InN_Mg_H

/** @file
This file includes Mg-doped InN
*/

#include "../material.h"
#include "InN.h"

namespace plask {

/**
 * Represent Mg-doped InN, its physical properties.
 */
struct InN_Mg: public InN {

	InN_Mg(DOPING_AMOUNT_TYPE Type, double Mg);
	virtual std::string name() const;
    virtual double mob(double T) const;
	virtual double Nf(double T) const;
    virtual double cond(double T) const;
    //virtual double nr(double wl, double T) const;
    //virtual double absp(double wl, double T) const;
private:
	double Nf_RT,
		   mob_RT,
		   cond_RT,
		   condTmax_RT;
	
};


} // namespace plask

#endif	//PLASK__InN_Mg_H
