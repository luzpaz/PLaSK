#ifndef PLASK__SOLVER_SLAB_REFLECTION_CYL_H
#define PLASK__SOLVER_SLAB_REFLECTION_CYL_H

#include <plask/plask.hpp>

#include "reflection_base.h"

namespace plask { namespace solvers { namespace slab {

/**
 * Reflection transformation solver in Cartesian 2D geometry.
 */
struct FourierReflectionCyl: public ReflectionSolver<Geometry2DCylindrical> {

    std::string getClassName() const { return "slab.FourierReflectionCyl"; }

  protected:

    /// Maximum order of orthogonal base
    size_t size;

    /// Mesh multiplier for finer computation of the refractive indices
    size_t refine;

    void onInitialize();

  public:

    FourierReflectionCyl(const std::string& name="");

    void loadConfiguration(XMLReader& reader, Manager& manager);

    /**
     * Find the mode around the specified effective index.
     * This method remembers the determined mode, for retrieval of the field profiles.
     * \param neff initial effective index to search the mode around
     * \return determined effective index
     */
    size_t findMode(dcomplex neff);

    /// Get order of the orthogonal base
    size_t getSize() const { return size; }
    /// Set order of the orthogonal base
    void setSize(size_t n) {
        size = n;
        invalidate();
    }

  protected:

    /**
     * Compute normalized electric field intensity 1/2 E conj(E) / P
     */
    const DataVector<const double> getIntensity(size_t num, const MeshD<2>& dst_mesh, InterpolationMethod method);

};


}}} // namespace

#endif

