#ifndef PLASK__SOLVER_SLAB_EXPANSION_PW2D_H
#define PLASK__SOLVER_SLAB_EXPANSION_PW2D_H

#include <plask/plask.hpp>

#include "expansion.h"
#include "fft.h"

namespace plask { namespace solvers { namespace slab {

struct FourierReflection2D;

struct ExpansionPW2D: public Expansion {

    /// Type of the mode symmetry
    enum Symmetry {
        SYMMETRIC_E_TRAN,               ///< E_tran and H_long are symmetric and E_long and H_tran anti-symmetric
        SYMMETRIC_E_LONG                ///< E_long and H_tran are symmetric and E_tran and H_long anti-symmetric
    };
    
    /// Polarization of separated modes
    enum Polarization {
        TE,                             ///< E_z and H_x exist
        TM                              ///< H_z and E_x exist
    };
    
    FourierReflection2D* solver;        ///< Solver which performs calculations (and is the interface to the outside world)

    RegularAxis xmesh;                  ///< Horizontal axis for structure sampling
    RegularAxis xpoints;                ///< Horizontal points in which fields will be computed by the inverse FFT

    size_t N;                           ///< Number of expansion coefficients
    size_t nN;                          ///< Number of of required coefficients for material parameters
    double left;                        ///< Left side of the sampled area
    double right;                       ///< Right side of the sampled area
    bool symmetric;                     ///< Indicates if the expansion is a symmetric one
    bool periodic;                      ///< Indicates if the geometry is periodic (otherwise use PMLs)
    bool separated;                     ///< Indicates whether TE and TM modes can be separated

    Symmetry symmetry;                  ///< Indicates symmetry if `symmetric`
    
    
    
    size_t pil,                         ///< Index of the beginning of the left PML
           pir;                         ///< Index of the beginning of the right PML

    /**
     * Create new expansion
     * \param solver solver which performs calculations
     * \param long_zero \c true if \f$ k_z = 0 \f$)
     * \param tran_zero \c true if \f$ k_x = 0 \f$)
     */
    ExpansionPW2D(FourierReflection2D* solver, bool long_zero, bool tran_zero);

    virtual size_t lcount() const;

    virtual bool diagonalQE(size_t l) const;

    virtual size_t matrixSize() const { return separated? N : 2*N; }

    virtual void getMatrices(size_t l, dcomplex k0, dcomplex beta, dcomplex kx, cmatrix& RE, cmatrix& RH);

    /**
     * Get refractive index back from expansion
     * \param l layer number
     * \param mesh mesh to get parameters to
     * \param interp interpolation method
     * \return computed refractive indices
     */
    DataVector<const Tensor3<dcomplex>> getMaterialNR(size_t l, const RectilinearAxis mesh,
                                                      InterpolationMethod interp=INTERPOLATION_DEFAULT);

  protected:

    DataVector<Tensor2<dcomplex>> mag;      ///< Magnetic permeability coefficients (used with for PMLs)
    DataVector<Tensor3<dcomplex>> coeffs;   ///< Material coefficients

    FFT::Forward1D matFFT;                  ///< FFT object for material coeffictiens

    /**
     * Compute expansion coefficients for material parameters
     * \param l layer number
     */
    void getMaterialCoefficients(size_t l);
    
    /// Get \f$ \varepsilon_{zz} \f$
    dcomplex epszz(int i) { return coeffs[(i>=0)?i:i+nN].c00; }
    
    /// Get \f$ \varepsilon_{xx} \f$
    dcomplex epsxx(int i) { return coeffs[(i>=0)?i:i+nN].c11; }
    
    /// Get \f$ \varepsilon_{yy}^{-1} \f$
    dcomplex iepsyy(int i) { return coeffs[(i>=0)?i:i+nN].c22; }
    
    /// Get \f$ \varepsilon_{zx} \f$
    dcomplex epszx(int i) { return coeffs[(i>=0)?i:i+nN].c01; }
    
    /// Get \f$ \varepsilon_{xz} \f$
    dcomplex epsxz(int i) { return coeffs[(i>=0)?i:i+nN].c10; }
    
    /// Get \f$ \mu_{xx} \f$
    dcomplex muzz(int i) { return mag[(i>=0)?i:i+nN].c00; }
    
    /// Get \f$ \mu_{xx} \f$
    dcomplex muxx(int i) { return mag[(i>=0)?i:i+nN].c00; }
    
    /// Get \f$ \mu_{xx} \f$
    dcomplex imuyy(int i) { return mag[(i>=0)?i:i+nN].c11; }
    
    /// Get \f$ E_x \f$ index
    size_t iEx(int i) { return 2 * ((i>=0)?i:i+nN); }
    
    /// Get \f$ E_x \f$ index
    size_t iEz(int i) { return 2 * ((i>=0)?i:i+nN) + 1; }
    
    /// Get \f$ E_x \f$ index
    size_t iHx(int i) { return 2 * ((i>=0)?i:i+nN); }
    
    /// Get \f$ E_x \f$ index
    size_t iHz(int i) { return 2 * ((i>=0)?i:i+nN) + 1; }
    
    /// Get \f$ E_x \f$ index
    size_t iE(int i) { return (i>=0)?i:i+nN; }
    
    /// Get \f$ E_x \f$ index
    size_t iH(int i) { return (i>=0)?i:i+nN; }
};

}}} // namespace plask

#endif // PLASK__SOLVER_SLAB_EXPANSION_PW2D_H
