#include "GaInAs_Zn.h"

#include <cmath>
#include <plask/material/db.h>  //MaterialsDB::Register
#include <plask/material/info.h>    //MaterialInfo::DB::Register

namespace plask {

std::string GaInAs_Zn::name() const { return NAME; }

std::string GaInAs_Zn::str() const { return StringBuilder("Ga")("In", In)("As").dopant("Zn", NA); }

MI_PARENT(GaInAs_Zn, GaInAs)

GaInAs_Zn::GaInAs_Zn(const Material::Composition& Comp, DopingAmountType Type, double Val): GaInAs(Comp)/*, mGaAs_Zn(Type,Val), mInAs_Zn(Type,Val)*/
{
    if (Type == CARRIER_CONCENTRATION) {
        Nf_RT = Val;
        if (In == 0.53) NA = Val/0.90;
        else NA = Val;
    }
    else {
        if (In == 0.53) Nf_RT = 0.90*Val;
        else Nf_RT = Val;
        NA = Val;
    }
    if (In == 0.53)
        mob_RT = 250e-4/(1+pow((Nf_RT/6e17),0.34)); // 1e-4: cm^2/(V*s) -> m^2/(V*s)
    else
        mob_RT = 0.; // TODO
}

MI_PROPERTY(GaInAs_Zn, mob,
            MISource("TODO"),
            MISource("based on Zn-doped GaInAs")
            )
std::pair<double,double> GaInAs_Zn::mob(double T) const {
    return ( std::make_pair(mob_RT, mob_RT) );
}

MI_PROPERTY(GaInAs_Zn, Nf,
            MISource("TODO"),
            MIComment("no temperature dependence")
            )
double GaInAs_Zn::Nf(double T) const {
    return ( Nf_RT );
}

double GaInAs_Zn::Dop() const {
    return ( NA );
}

MI_PROPERTY(GaInAs_Zn, cond,
            MIComment("no temperature dependence")
            )
std::pair<double,double> GaInAs_Zn::cond(double T) const {
    double tCond = phys::qe * Nf_RT*1e6 * mob_RT;
    return ( std::make_pair(tCond, tCond) );
}

MI_PROPERTY(GaInAs_Zn, absp,
            MISource("fit to ..."), // TODO
            MIComment("no temperature dependence")
            )
double GaInAs_Zn::absp(double wl, double T) const {
    double tAbsp(0.);
    if ((wl > 1200.) && (wl < 1400.)) // only for 1300 nm TODO
        tAbsp = 60500. * pow(Nf_RT/1e18+23.3, -0.54);
    else if ((wl > 1450.) && (wl < 1650.)) // only for 1550 nm TODO
        tAbsp = 24000. * pow(Nf_RT/1e18+9.7, -0.61);
    else if ((wl > 2230.) && (wl < 2430.)) // only for 2330 nm TODO
        tAbsp = 63. * pow(Nf_RT/1e18, -0.7);
    else if ((wl > 8900.) && (wl < 9100.)) // only for 9000 nm TODO
        tAbsp = 250. * pow(Nf_RT/1e18, -0.7);
    return ( tAbsp );
}

static MaterialsDB::Register<GaInAs_Zn> materialDB_register_GaInAs_Zn;

} // namespace plask
