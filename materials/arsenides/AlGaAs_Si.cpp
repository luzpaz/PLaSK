#include "AlGaAs_Si.h"

#include <cmath>
#include <plask/material/db.h>  //MaterialsDB::Register
#include <plask/material/info.h>    //MaterialInfo::DB::Register

namespace plask {

std::string AlGaAs_Si::name() const { return NAME; }

std::string AlGaAs_Si::str() const { return StringBuilder("Al", Al)("Ga")("As").dopant("Si", ND); }

MI_PARENT(AlGaAs_Si, AlGaAs)

AlGaAs_Si::AlGaAs_Si(const Material::Composition& Comp, DopingAmountType Type, double Val): AlGaAs(Comp), mGaAs_Si(Type,Val), mAlAs_Si(Type,Val)
{
    double fx1A = (1.-7.8*Al*Al); // x < 0.35
    double fx1B = (1.14*Al-0.36); // else
    if (Type == CARRIER_CONCENTRATION) {
        Nf_RT = Val;
        if (Al < 0.35) ND = mGaAs_Si.Dop()*fx1A;
        else ND = mGaAs_Si.Dop()*fx1B;
    }
    else {
        if (Al < 0.35) Nf_RT = mGaAs_Si.Nf(300.)*fx1A;
        else Nf_RT = mGaAs_Si.Nf(300.)*fx1B;
        ND = Val;
    }
    double fx2A = exp(-16.*Al*Al); // x < 0.5
    double fx2B = 0.054*Al-0.009; // else
    if (Al < 0.5) mob_RT = mGaAs_Si.mob(300.).c00 * fx2A;
    else mob_RT = mGaAs_Si.mob(300.).c00 * fx2B;
}

MI_PROPERTY(AlGaAs_Si, mob,
            MISource("based on 3 papers 1982-1990 about Si-doped AlGaAs"),
            MISource("based on Si-doped GaAs")
            )
Tensor2<double> AlGaAs_Si::mob(double T) const {
    return ( Tensor2<double>(mob_RT, mob_RT) );
}

MI_PROPERTY(AlGaAs_Si, Nf,
            MISource("based on 2 papers 1982, 1984 about Si-doped AlGaAs"),
            MIComment("no temperature dependence")
            )
double AlGaAs_Si::Nf(double T) const {
    return ( Nf_RT );
}

double AlGaAs_Si::Dop() const {
    return ( ND );
}

Tensor2<double> AlGaAs_Si::cond(double T) const {
    double tCond = phys::qe * Nf_RT*1e6 * mob_RT;
    return ( Tensor2<double>(tCond, tCond) );
}

static MaterialsDB::Register<AlGaAs_Si> materialDB_register_AlGaAs_Si;

}       // namespace plask
