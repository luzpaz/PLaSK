#include "InAs_C.h"

#include <cmath>
#include <plask/material/db.h>  //MaterialsDB::Register
#include <plask/material/info.h>    //MaterialInfo::DB::Register

namespace plask {

std::string InAs_C::name() const { return NAME; }

std::string InAs_C::str() const { return StringBuilder("InAs").dopant("C", NA); }

InAs_C::InAs_C(DopingAmountType Type, double Val) {
    Nf_RT = 0.; //TODO
    NA = 0.; //TODO
    mob_RT = 0.; //TODO
}

MI_PROPERTY(InAs_C, mob,
            MIComment("TODO")
            )
std::pair<double,double> InAs_C::mob(double T) const {
    return ( std::make_pair(mob_RT,mob_RT) );
}

MI_PROPERTY(InAs_C, Nf,
            MIComment("TODO")
            )
double InAs_C::Nf(double T) const {
    return ( Nf_RT );
}

double InAs_C::Dop() const {
    return ( NA );
}

MI_PROPERTY(InAs_C, cond, // TODO
            MIComment("no temperature dependence")
            )
std::pair<double,double> InAs_C::cond(double T) const {
    double tCond = phys::qe * Nf_RT*1e6 * mob_RT;
    return (std::make_pair(tCond, tCond));
}

static MaterialsDB::Register<InAs_C> materialDB_register_InAs_C;

} // namespace plask
