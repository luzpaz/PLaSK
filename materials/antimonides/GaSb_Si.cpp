#include "GaSb_Si.h"

#include <cmath>
#include <plask/material/db.h>      // MaterialsDB::Register
#include <plask/material/info.h>    // MaterialInfo::DB::Register

namespace plask { namespace materials {

std::string GaSb_Si::name() const { return NAME; }

std::string GaSb_Si::str() const { return StringBuilder("GaSb").dopant("Si", NA); }

GaSb_Si::GaSb_Si(DopingAmountType Type, double Val) {
    Nf_RT = Val;
    NA = Val;
    mob_RT = 190e-4 + 685e-4 / (1.+pow(NA/9e17,0.65)); // 1e-4: cm^2/(V*s) -> m^2/(V*s)
}

MI_PROPERTY(GaSb_Si, mob,
            MISource("D. Martin et al., Semiconductors Science and Technology 19 (2004) 1040-1052"),
            MIComment("for all dopants") // TODO
            )
Tensor2<double> GaSb_Si::mob(double T) const {
    double tmob = 190. + (875.*pow(300./T,1.7)-190.) / (1.+pow(NA/(9e17*pow(T/300.,2.7)),0.65));
    return ( Tensor2<double>(tmob*1e-4,tmob*1e-4) ); // 1e-4: cm^2/(V*s) -> m^2/(V*s)
}

MI_PROPERTY(GaSb_Si, Nf,
            MISource("assumed"), // TODO
            MIComment("no temperature dependence")
            )
double GaSb_Si::Nf(double T) const {
    return ( Nf_RT );
}

double GaSb_Si::Dop() const {
    return ( NA );
}

MI_PROPERTY(GaSb_Si, cond,
            MIComment("100% donor activation assumed") // TODO
            )
Tensor2<double> GaSb_Si::cond(double T) const {
    double tmob = 190. + (875.*pow(300./T,1.7)-190.) / (1.+pow(NA/(9e17*pow(T/300.,2.7)),0.65));
    double tCond = phys::qe * Nf_RT*1e6 * tmob*1e-4;
    return ( Tensor2<double>(tCond, tCond) );
}

bool GaSb_Si::isEqual(const Material &other) const {
    const GaSb_Si& o = static_cast<const GaSb_Si&>(other);
    return o.NA == this->NA && o.Nf_RT == this->Nf_RT && o.mob_RT == this->mob_RT && GaSb::isEqual(other);
}

static MaterialsDB::Register<GaSb_Si> materialDB_register_GaSb_Si;

}} // namespace plask::materials
