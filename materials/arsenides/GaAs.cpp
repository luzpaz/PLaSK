#include "GaAs.h"

#include <cmath>
#include <plask/material/db.h>  //MaterialsDB::Register
#include <plask/material/info.h>    //MaterialInfo::DB::Register

namespace plask {

std::string GaAs::name() const { return NAME; }

MI_PROPERTY(GaAs, cond,
            MISource("-"),
			MIComment("fit to n-GaAs:Si (based on 8 papers 1982 - 2003)"),
			MIComment("no temperature dependence")
            )
std::pair<double,double> GaAs::cond(double T) const {
    return (std::make_pair(940.,940.));
}

MI_PROPERTY(GaAs, condT,
            MISource("S. Adachi et al., Properties of Semiconductor Alloys: Group-IV, III–V and II–VI Semiconductors, Wiley 2009"),
            MIComment("no temperature dependence")
            )
std::pair<double,double> GaAs::condT(double T, double t) const {
    return(std::make_pair(45.,45.));
 }

MI_PROPERTY(GaAs, absp,
            MISource(""),
            MIComment("TODO")
            )
double GaAs::absp(double wl, double T) const {
    return ( 0 );
}

MI_PROPERTY(GaAs, nr,
            MISource(""),
            MIComment("TODO")            )
double GaAs::nr(double wl, double T) const {
    return ( 0 );
}

MI_PROPERTY(GaAs, lattC,
            MISource("S. Adachi et al., Properties of Semiconductor Alloys: Group-IV, III–V and II–VI Semiconductors, Wiley 2009")
            )
double GaAs::lattC(double T, char x) const {
    double tLattC(0.);
    if (x == 'a') tLattC = 5.6533;
    return (tLattC);
}

MI_PROPERTY(GaAs, Eg,
            MISource("S. Adachi et al., Properties of Semiconductor Alloys: Group-IV, III–V and II–VI Semiconductors, Wiley 2009"),
            MIComment("only for Gamma point"),
            MIComment("no temperature dependence")
            )
double GaAs::Eg(double T, char point) const {
    double tEg(0.);
    if (point == 'G') tEg = 1.43;
    return (tEg);
}

MI_PROPERTY(GaAs, Me,
            MISource("S. Adachi et al., Properties of Semiconductor Alloys: Group-IV, III–V and II–VI Semiconductors, Wiley 2009"),
            MIComment("only for Gamma point"),
            MIComment("no temperature dependence")
            )
std::pair<double,double> GaAs::Me(double T, char point) const {
    std::pair<double,double> tMe(0.,0.);
    if (point == 'G') {
        tMe.first = 0.067;
        tMe.second = 0.067;
    }
    return (tMe);
}

MI_PROPERTY(GaAs, Mhh,
            MISource("")
            )
std::pair<double,double> GaAs::Mhh(double T, char point) const {
    return (std::make_pair(0.,0.));
}

MI_PROPERTY(GaAs, Mlh,
            MISource("")
            )
std::pair<double,double> GaAs::Mlh(double T, char point) const {
    return (std::make_pair(0.,0.));
}

static MaterialsDB::Register<GaAs> materialDB_register_GaAs;

}       // namespace plask
