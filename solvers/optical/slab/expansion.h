#ifndef PLASK__SOLVER_SLAB_EXPANSION_H
#define PLASK__SOLVER_SLAB_EXPANSION_H

#include <plask/plask.hpp>

#include "matrices.h"

namespace plask { namespace solvers { namespace slab {

struct Expansion {

    struct FieldParams {
        /// Indentification of the field obtained by the getField
        enum Which {
            E,                      ///< Electric field
            H                       ///< Magnetic field
        };
        Which which;                ///< Which field is being computed
        dcomplex k0;                ///< Normalized frequency [1/µm]
        dcomplex klong;             ///< Longitudinal wavevector component [1/µm]
        dcomplex ktran;             ///< Longitudinal wavevector component [1/µm]
        InterpolationMethod method; ///< Interpolation method
    };
    FieldParams field_params;

    /// Solver which performs calculations (and is the interface to the outside world)
    Solver* solver;

    Expansion(Solver* solver): solver(solver) {}

    /**
     * Return number of distinct layers
     * \return number of layers
     */
    virtual size_t lcount() const = 0;

    /**
     * Tell if matrix for i-th layer is diagonal
     * \param l layer number
     * \return \c true if the i-th matrix is diagonal
     */
    virtual bool diagonalQE(size_t l) const { return false; }

    /**
     * Return size of the expansion matrix (equal to the number of expansion coefficients)
     * \return size of the expansion matrix
     */
    virtual size_t matrixSize() const = 0;

    /**
     * Get RE anf RH matrices
     * \param l layer number
     * \param k0 normalized frequency [1/µm]
     * \param klong,ktran horizontal wavevector components [1/µm]
     * \param[out] RE,RH resulting matrix
     */
    virtual void getMatrices(size_t l, dcomplex k0, dcomplex klong, dcomplex ktran, cmatrix& RE, cmatrix& RH) = 0;

    /**
     * Prepare for computatiations of the fields
     * \param field which field is computed
     * \param k0 normalized frequency [1/µm]
     * \param klong,ktran horizontal wavevector components [1/µm]
     * \param method interpolation method
     */
    void initField(FieldParams::Which field, dcomplex k0, dcomplex klong, dcomplex ktran, InterpolationMethod method) {
        field_params.which = field; field_params.k0 = k0; field_params.klong = klong; field_params.ktran = ktran;
        field_params.method = method;
        prepareField();
    };

  protected:
    /**
     * Prepare for computatiations of the fields
     */
    virtual void prepareField() {};

  public:
    /**
     * Cleanup after computatiations of the fields
     */
    virtual void cleanupField() {};

    /**
     * Compute electric og magnetic field on \c dst_mesh at certain level
     * \param l layer number
     * \param dst_mesh destination mesh
     * \param E,H electric and magnetic field coefficientscients
     * \return field distribution at \c dst_mesh
     * \return field distribution at \c dst_mesh
     */
    virtual DataVector<Vec<3,dcomplex>> getField(size_t l, const Mesh& dst_mesh, const cvector& E, const cvector& H) = 0;
};



}}} // namespace plask

#endif // PLASK__SOLVER_SLAB_EXPANSION_H
