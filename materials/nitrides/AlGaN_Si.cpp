#include "AlGaN_Si.h"

#include <cmath>
#include <plask/material/db.h>  //MaterialsDB::Register
#include <plask/material/info.h>    //MaterialInfo::DB::Register

namespace plask {

std::string AlGaN_Si::name() const { return NAME; }

std::string AlGaN_Si::str() const { return StringBuilder("Al", Al)("Ga")("N").dopant("Si", ND); }

MI_PARENT(AlGaN_Si, AlGaN)

AlGaN_Si::AlGaN_Si(const Material::Composition& Comp, DopingAmountType Type, double Val): AlGaN(Comp), mGaN_Si(Type,Val), mAlN_Si(Type,Val)
{
    if (Type == CARRIER_CONCENTRATION)
        ND = mAlN_Si.Dop()*Al + mGaN_Si.Dop()*Ga;
    else
        ND = Val;
}

MI_PROPERTY(AlGaN_Si, mob,
            MISource("based on 11 papers 1997-2008 about Si-doped AlGaN"),
            MISource("based on Si-doped GaN and AlN")
            )
Tensor2<double> AlGaN_Si::mob(double T) const {
    double lMob = Al*mAlN_Si.mob(T).c00 + pow(Ga,1.415+19.63*exp(-5.456*Al))*mGaN_Si.mob(T).c00,
           vMob = Al*mAlN_Si.mob(T).c11 + pow(Ga,1.415+19.63*exp(-5.456*Al))*mGaN_Si.mob(T).c11;
    return (Tensor2<double>(lMob, vMob));
}

MI_PROPERTY(AlGaN_Si, Nf,
            MISource("linear interpolation: Si-doped GaN, AlN")
            )
double AlGaN_Si::Nf(double T) const {
    return ( mAlN_Si.Nf(T)*Al + mGaN_Si.Nf(T)*Ga );
}

double AlGaN_Si::Dop() const {
    return ND;
}

Tensor2<double> AlGaN_Si::cond(double T) const {
    return (Tensor2<double>(1.602E-17*Nf(T)*mob(T).c00, 1.602E-17*Nf(T)*mob(T).c11));
}

MI_PROPERTY(AlGaN_Si, thermk,
            MISeeClass<AlGaN>(MaterialInfo::thermk),
            MIComment("Si doping dependence for GaN")
            )
Tensor2<double> AlGaN_Si::thermk(double T, double t) const {
    double lCondT = 1/(Al/mAlN_Si.thermk(T,t).c00 + Ga/mGaN_Si.thermk(T,t).c00 + Al*Ga*0.4),
           vCondT = 1/(Al/mAlN_Si.thermk(T,t).c11 + Ga/mGaN_Si.thermk(T,t).c11 + Al*Ga*0.4);
    return(Tensor2<double>(lCondT, vCondT));
 }

MI_PROPERTY(AlGaN_Si, absp,
            MISeeClass<AlGaN>(MaterialInfo::absp)
            )
double AlGaN_Si::absp(double wl, double T) const {
    double Eg = 6.28*Al + 3.42*(1-Al) - 0.7*Al*(1-Al);
    double a = 1239.84190820754/wl - Eg,
           b = ND/1e18;
    return ( (19000+4000*b)*exp(a/(0.019+0.001*b)) + (330+200*b)*exp(a/(0.07+0.016*b)) );
}

static MaterialsDB::Register<AlGaN_Si> materialDB_register_AlGaN_Si;

}       // namespace plask
