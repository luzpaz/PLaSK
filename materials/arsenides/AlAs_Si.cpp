#include "AlAs_Si.h"

#include <cmath>
#include <plask/material/db.h>  //MaterialsDB::Register
#include <plask/material/info.h>    //MaterialInfo::DB::Register

namespace plask { namespace materials {

std::string AlAs_Si::name() const { return NAME; }

std::string AlAs_Si::str() const { return StringBuilder("AlAs").dopant("Si", ND); }

AlAs_Si::AlAs_Si(DopingAmountType Type, double Val) {
    //double act_GaAs = 1.;
    //double Al = 1.; // AlAs (not AlGaAs)
    //double fx1 = 1.14*Al-0.36;
    if (Type == CARRIER_CONCENTRATION) {
        Nf_RT = Val;
        ND = Val/0.78; // Val/(act_GaAs*fx1);
    }
    else {
        Nf_RT = 0.78*Val; // (act_GaAs*fx1)*Val;
        ND = Val;
    }
    double mob_RT_GaAs = 6600e-4/(1+pow((Nf_RT/5e17),0.53)); // 1e-4: cm^2/(V*s) -> m^2/(V*s)
    double fx2 = 0.045; // 0.054*Al-0.009;
    mob_RT = mob_RT_GaAs * fx2;
}

MI_PROPERTY(AlAs_Si, mob,
            MIComment("TODO")
            )
Tensor2<double> AlAs_Si::mob(double T) const {
    return ( Tensor2<double>(mob_RT,mob_RT) );
}

MI_PROPERTY(AlAs_Si, Nf,
            MIComment("TODO")
            )
double AlAs_Si::Nf(double T) const {
    return ( Nf_RT );
}

double AlAs_Si::Dop() const {
    return ( ND );
}

MI_PROPERTY(AlAs_Si, cond,
            MIComment("no temperature dependence")
            )
Tensor2<double> AlAs_Si::cond(double T) const {
    double tCond = phys::qe * Nf_RT*1e6 * mob_RT;
    return ( Tensor2<double>(tCond, tCond) );
}

bool AlAs_Si::isEqual(const Material &other) const {
    const AlAs_Si& o = static_cast<const AlAs_Si&>(other);
    return o.ND == this->ND && o.Nf_RT == this->Nf_RT && o.mob_RT == this->mob_RT && AlAs::isEqual(other);
}

static MaterialsDB::Register<AlAs_Si> materialDB_register_AlAs_Si;

}} // namespace plask::materials
