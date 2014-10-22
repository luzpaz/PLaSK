#ifndef PLASK__SOLVER__OPTICAL__SLAB_FOURIER_REFLECTION_3D_H
#define PLASK__SOLVER__OPTICAL__SLAB_FOURIER_REFLECTION_3D_H

#include <plask/plask.hpp>

#include "../solver.h"
#include "../reflection.h"
#include "expansion3d.h"

namespace plask { namespace solvers { namespace slab {

/**
 * Reflection transformation solver in Cartesian 3D geometry.
 */
struct PLASK_SOLVER_API FourierSolver3D: public SlabSolver<Geometry3D> {

    std::string getClassName() const { return "optical.Fourier3D"; }

    /// Indication of parameter to search
    enum What {
        WHAT_WAVELENGTH,    ///< Search for wavelength
        WHAT_K0,            ///< Search for normalized frequency
        WHAT_KLONG,         ///< Search for longitudinal wavevector
        WHAT_KTRAN          ///< Search for transverse wavevector
    };

    struct Mode {
        FourierSolver3D* solver;                            ///< Solver this mode belongs to
        Expansion::Component symmetry_long;                 ///< Mode symmetry in long direction
        Expansion::Component symmetry_tran;                 ///< Mode symmetry in tran direction
        dcomplex k0;                                            ///< Stored mode frequency
        dcomplex klong;                                         ///< Stored mode effective index
        dcomplex ktran;                                         ///< Stored mode transverse wavevector
        double power;                                           ///< Mode power [mW]

        Mode(FourierSolver3D* solver): solver(solver), power(1e-9) {}

        bool operator==(const Mode& other) const {
            return is_zero(k0 - other.k0) && is_zero(klong - other.klong) && is_zero(ktran - other.ktran)
                && (!solver->expansion.symmetric_long() || symmetry_long == other.symmetry_long)
                && (!solver->expansion.symmetric_tran() || symmetry_tran == other.symmetry_tran)
            ;
        }
    };

    /// Maximum order of the orthogonal base in longitudinal direction
    size_t size_long;
    /// Maximum order of the orthogonal base in transverse direction
    size_t size_tran;

  protected:

    /// Class responsoble for computing expansion coefficients
    ExpansionPW3D expansion;

    void onInitialize();

    void onInvalidate();

    void computeCoefficients() override {
        expansion.computeMaterialCoefficients();
    }

  public:

    /// Computed modes
    std::vector<Mode> modes;

    /// Mesh multiplier for finer computation of the refractive indices in the longitudinal direction
    size_t refine_long;
    /// Mesh multiplier for finer computation of the refractive indices in the transverse direction
    size_t refine_tran;

    /// Longitudinal PMLs
    PML pml_long;
    /// Transverse PMLs
    PML pml_tran;

    FourierSolver3D(const std::string& name="");

    void loadConfiguration(XMLReader& reader, Manager& manager);

    /**
     * Find the mode around the specified effective index.
     * This method remembers the determined mode, for retrieval of the field profiles.
     * \param what what to search for
     * \param start initial value of \a what to search the mode around
     * \return determined effective index
     */
    size_t findMode(What what, dcomplex start);

    /// Get order of the orthogonal base in the longitudinal direction
    size_t getLongSize() const { return size_long; }

    /// Get order of the orthogonal base in the transverse direction
    size_t getTranSize() const { return size_tran; }

    /// Set order of the orthogonal base in the longitudinal direction
    void setLongSize(size_t n) {
        size_long = n;
        invalidate();
    }
    /// Set order of the orthogonal base in the transverse direction
    void setTranSize(size_t n) {
        size_tran = n;
        invalidate();
    }

    /// Set order of the orthogonal base
    void setSizes(size_t nl, size_t nt) {
        size_long = nl;
        size_tran = nt;
        invalidate();
    }

    /// Return current mode symmetry
    Expansion::Component getSymmetryLong() const { return expansion.symmetry_long; }

    /// Set new mode symmetry
    void setSymmetryLong(Expansion::Component symmetry) {
        if (symmetry != Expansion::E_UNSPECIFIED && geometry && !geometry->isSymmetric(Geometry3D::DIRECTION_LONG))
            throw BadInput(getId(), "Longitudinal symmetry not allowed for asymmetric structure");
        if ((expansion.symmetric_long() && symmetry == Expansion::E_UNSPECIFIED) ||
            (!expansion.symmetric_long() && symmetry != Expansion::E_UNSPECIFIED))
            invalidate();
        if (klong != 0. && symmetry != Expansion::E_UNSPECIFIED) {
            Solver::writelog(LOG_WARNING, "Resetting klong to 0.");
            klong = 0.;
        }
        if (transfer) transfer->fields_determined = Transfer::DETERMINED_NOTHING;
        expansion.symmetry_long = symmetry;
    }

    /// Return current mode symmetry
    Expansion::Component getSymmetryTran() const { return expansion.symmetry_tran; }

    /// Set new mode symmetry
    void setSymmetryTran(Expansion::Component symmetry) {
        if (symmetry != Expansion::E_UNSPECIFIED && geometry && !geometry->isSymmetric(Geometry3D::DIRECTION_TRAN))
            throw BadInput(getId(), "Transverse symmetry not allowed for asymmetric structure");
        if ((expansion.symmetric_tran() && symmetry == Expansion::E_UNSPECIFIED) ||
            (!expansion.symmetric_tran() && symmetry != Expansion::E_UNSPECIFIED))
            invalidate();
        if (ktran != 0. && symmetry != Expansion::E_UNSPECIFIED) {
            Solver::writelog(LOG_WARNING, "Resetting ktran to 0.");
            ktran = 0.;
        }
        if (transfer) transfer->fields_determined = Transfer::DETERMINED_NOTHING;
        expansion.symmetry_tran = symmetry;
    }

    /// Set longitudinal wavevector
    void setKlong(dcomplex k)  {
        if (k != 0. && expansion.symmetric_long()) {
            Solver::writelog(LOG_WARNING, "Resetting longitudinal mode symmetry");
            expansion.symmetry_long = Expansion::E_UNSPECIFIED;
            invalidate();
        }
        if (k != klong && transfer) transfer->fields_determined = Transfer::DETERMINED_NOTHING;
        klong = k;
    }

    /// Set transverse wavevector
    void setKtran(dcomplex k)  {
        if (k != 0. && expansion.symmetric_tran()) {
            Solver::writelog(LOG_WARNING, "Resetting transverse mode symmetry");
            expansion.symmetry_tran = Expansion::E_UNSPECIFIED;
            invalidate();
        }
        if (k != ktran && transfer) transfer->fields_determined = Transfer::DETERMINED_NOTHING;
        ktran = k;
    }

    /// Get mesh at which material parameters are sampled along longitudinal axis
    RegularAxis getLongMesh() const { return expansion.long_mesh; }

    /// Get mesh at which material parameters are sampled along transverse axis
    RegularAxis getTranMesh() const { return expansion.tran_mesh; }

  private:

    /**
     * Get incident field vector for given polarization.
     * \param polarization polarization of the perpendicularly incident light
     * \param savidx pointer to which optionally save nonzero incident index
     * \return incident field vector
     */
    cvector incidentVector(Expansion::Component polarization, size_t* savidx=nullptr) {
        if (polarization == ExpansionPW3D::E_UNSPECIFIED)
            throw BadInput(getId(), "Wrong incident polarization specified for the reflectivity computation");
        if (expansion.symmetry_long == Expansion::Component(2-polarization))
            throw BadInput(getId(), "Current longitudinal symmetry is inconsistent with the specified incident polarization");
        if (expansion.symmetry_tran == Expansion::Component(2-polarization))
            throw BadInput(getId(), "Current transverse symmetry is inconsistent with the specified incident polarization");
        size_t idx = (polarization == ExpansionPW3D::E_LONG)? expansion.iEx(0,0) : expansion.iEy(0,0);
        if (savidx) *savidx = idx;
        cvector incident(expansion.matrixSize(), 0.);
        incident[idx] = 1.;
        return incident;
    }

  public:

    /**
     * Get amplitudes of reflected diffraction orders
     * \param polarization polarization of the perpendicularly incident light
     * \param incidence incidence side
     * \param savidx pointer to which optionally save nonzero incident index
     */
    cvector getReflectedAmplitudes(Expansion::Component polarization,
                                   Transfer::IncidentDirection incidence,
                                   size_t* savidx=nullptr);

    /**
     * Get amplitudes of transmitted diffraction orders
     * \param polarization polarization of the perpendicularly incident light
     * \param incidence incidence side
     * \param savidx pointer to which optionally save nonzero incident index
     */
    cvector getTransmittedAmplitudes(Expansion::Component polarization,
                                     Transfer::IncidentDirection incidence,
                                     size_t* savidx=nullptr);

    /**
     * Get reflection coefficient
     * \param polarization polarization of the perpendicularly incident light
     * \param incidence incidence side
     */
    double getReflection(Expansion::Component polarization, Transfer::IncidentDirection incidence);

    /**
     * Get reflection coefficient
     * \param polarization polarization of the perpendicularly incident light
     * \param incidence incidence side
     */
    double getTransmission(Expansion::Component polarization, Transfer::IncidentDirection incidence);

    /**
     * Get electric field at the given mesh for reflected light.
     * \param Ei incident field vector
     * \param incident incidence direction
     * \param dst_mesh target mesh
     * \param method interpolation method
     */
    DataVector<Vec<3,dcomplex>> getReflectedFieldE(Expansion::Component polarization,
                                                   Transfer::IncidentDirection incident,
                                                   const shared_ptr<const MeshD<3>>& dst_mesh,
                                                   InterpolationMethod method) {
        initCalculation();
        return transfer->getReflectedFieldE(incidentVector(polarization), incident, dst_mesh, method);
    }

    /**
     * Get magnetic field at the given mesh for reflected light.
     * \param Ei incident field vector
     * \param incident incidence direction
     * \param dst_mesh target mesh
     * \param method interpolation method
     */
    DataVector<Vec<3,dcomplex>> getReflectedFieldH(Expansion::Component polarization,
                                                   Transfer::IncidentDirection incident,
                                                   const shared_ptr<const MeshD<3>>& dst_mesh,
                                                   InterpolationMethod method) {
        initCalculation();
        return transfer->getReflectedFieldH(incidentVector(polarization), incident, dst_mesh, method);
    }

    /**
     * Get light intensity for reflected light.
     * \param Ei incident field vector
     * \param incident incidence direction
     * \param dst_mesh destination mesh
     * \param method interpolation method
     */
    DataVector<double> getReflectedFieldMagnitude(Expansion::Component polarization,
                                                  Transfer::IncidentDirection incident,
                                                  const shared_ptr<const MeshD<3>>& dst_mesh,
                                                  InterpolationMethod method) {
        initCalculation();
        return transfer->getReflectedFieldMagnitude(incidentVector(polarization), incident, dst_mesh, method);
    }


  protected:

    /// Insert mode to the list or return the index of the exiting one
    size_t insertMode() {
        Mode mode(this);
        mode.k0 = k0; mode.klong = klong; mode.ktran = ktran;
        mode.symmetry_long = expansion.symmetry_long; mode.symmetry_tran = expansion.symmetry_tran;
        for (size_t i = 0; i != modes.size(); ++i)
            if (modes[i] == mode) return i;
        modes.push_back(mode);
        outLightMagnitude.fireChanged();
        outElectricField.fireChanged();
        outMagneticField.fireChanged();
        return modes.size()-1;
    }

    size_t nummodes() const { return 1; }

     /**
      * Return mode effective index
      * \param n mode number
      */
     dcomplex getEffectiveIndex(size_t n) {
         if (n >= modes.size()) throw NoValue(EffectiveIndex::NAME);
         return modes[n].klong / modes[n].k0;
     }

    const DataVector<const Vec<3,dcomplex>> getE(size_t num, shared_ptr<const MeshD<3>> dst_mesh, InterpolationMethod method) override;

    const DataVector<const Vec<3,dcomplex>> getH(size_t num, shared_ptr<const MeshD<3>> dst_mesh, InterpolationMethod method) override;

    const DataVector<const double> getIntensity(size_t num, shared_ptr<const MeshD<3>> dst_mesh, InterpolationMethod method) override;

   public:

     /**
      * Proxy class for accessing reflected fields
      */
     struct Reflected {

         /// Provider of the optical electric field
         typename ProviderFor<LightE,Geometry3D>::Delegate outElectricField;

         /// Provider of the optical magnetic field
         typename ProviderFor<LightH,Geometry3D>::Delegate outMagneticField;

         /// Provider of the optical field intensity
         typename ProviderFor<LightMagnitude,Geometry3D>::Delegate outLightMagnitude;

         /// Return one as the number of the modes
         static size_t size() { return 1; }

         /**
          * Construct proxy.
          * \param wavelength incident light wavelength
          * \param polarization polarization of the perpendicularly incident light
          * \param side incidence side
          */
         Reflected(FourierSolver3D* parent, double wavelength, Expansion::Component polarization, Transfer::IncidentDirection side):
             outElectricField([=](size_t, const shared_ptr<const MeshD<3>>& dst_mesh, InterpolationMethod method) -> DataVector<const Vec<3,dcomplex>> {
                 parent->setWavelength(wavelength);
                 return parent->getReflectedFieldE(polarization, side, dst_mesh, method); }, size),
             outMagneticField([=](size_t, const shared_ptr<const MeshD<3>>& dst_mesh, InterpolationMethod method) -> DataVector<const Vec<3,dcomplex>> {
                 parent->setWavelength(wavelength);
                 return parent->getReflectedFieldH(polarization, side, dst_mesh, method); }, size),
             outLightMagnitude([=](size_t, const shared_ptr<const MeshD<3>>& dst_mesh, InterpolationMethod method) -> DataVector<const double> {
                 parent->setWavelength(wavelength);
                 return parent->getReflectedFieldMagnitude(polarization, side, dst_mesh, method); }, size)
         {}
     };
};


}}} // namespace

#endif

