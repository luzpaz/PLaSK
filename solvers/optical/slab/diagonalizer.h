/**
 *  \file   diagonalizer.h  Module responsible for calculating and holding diagonalized matrices
 */
#ifndef PLASK__SOLVER_SLAB_DIAGONALIZER_H
#define PLASK__SOLVER_SLAB_DIAGONALIZER_H

#include <utility>

#ifdef OPENMP_FOUND
#   include <omp.h>
#endif

#include <plask/plask.hpp>

#include "matrices.h"
#include "expansion.h"

namespace plask { namespace  solvers { namespace slab {

/**
 * Base for the class determining and holding the necessary matrices
 * This is the abstract base class for all diagonalizers (multi-threaded,
 * disk-storage, MPI-using etc.
 * This class should compute gamma, Te and Th matrices for each layer and
 * store it until the next initDiagonalization(...) is called or the
 * object is destroyed (this is necessary for computation of the fields
 * after the convergence).
 */
class Diagonalizer
{
  protected:
    Expansion* src;                     ///< Information about the matrices to diagonalize
    std::vector<bool> diagonalized;     ///< True if the given layer was diagonalized

  public:
    const int lcount;                   // number of layers

    Diagonalizer(Expansion* src) :
        src(src), diagonalized(src->lcount(), false), lcount(src->lcount())
        {}

    virtual ~Diagonalizer() {}

    /// Return the overall matrix size
    virtual int matrixSize() const = 0;

    /// Return the reference to the source object
    inline const Expansion* source() const { return src; }

    /// Return the reference to the source object
    inline Expansion* source() { return src; }

    /// Initiate the diagonalization
    virtual void initDiagonalization() = 0;

    /// Calculate the diagonalization of given layer
    /// \return \c true if any work has been done and \c false if it was not necessary
    virtual bool diagonalizeLayer(size_t layer) = 0;

    /// Return true is layer is diagonalized
    bool isDiagonalized(size_t layer) {
        return diagonalized[layer];
    }

    /// Return diagnoal matrix of eignevalues
    virtual const cdiagonal& Gamma(size_t layer) const = 0;

    /// Return matrix of eigenvectors of QE
    virtual const cmatrix& TE(size_t layer) const = 0;

    /// Return matrix of eigenvectors of QH
    virtual const cmatrix& TH(size_t layer) const = 0;

    /// Return inverse matrix of eigenvectors of QE
    virtual const cmatrix& invTE(size_t layer) const = 0;

    /// Return inverse matrix of eigenvectors of QH
    virtual const cmatrix& invTH(size_t layer) const = 0;

    // Diagonalization function to compute the smallest eigenvalue of the provided matrix
//     virtual dcomplex smallest_eigevalue(const cmatrix& M) = 0;
};


/**
 * Simple diagonalizer
 * This class is a simple diagonalizer. It calculates all its results
 * immediately and stores them in the memory.
 */
class SimpleDiagonalizer : public Diagonalizer
{
  protected:
    std::vector<cdiagonal> gamma;       ///< Diagonal matrices Gamma
    std::vector<cmatrix> Te, Th;        ///< Matrices TE and TH
    std::vector<cmatrix> Te1, Th1;      ///< Matrices TE^-1 and TH^-1

    cmatrix* tmpmx;                     ///< QE matrices for temporary storage

    #ifdef OPENMP_FOUND
        omp_lock_t* tmplx;                 ///< Locks of allocated temporary matrices
    #endif

  public:
    SimpleDiagonalizer(Expansion* g);
    ~SimpleDiagonalizer();

    virtual int matrixSize() const { return src->matrixSize(); }

    virtual void initDiagonalization();

    virtual bool diagonalizeLayer(size_t layer);

    // Functions returning references to calculated matrices
    virtual const cdiagonal& Gamma(size_t layer) const { return gamma[layer]; }
    virtual const cmatrix& TE(size_t layer) const { return Te[layer]; }
    virtual const cmatrix& TH(size_t layer) const { return Th[layer]; }
    virtual const cmatrix& invTE(size_t layer) const { return Te1[layer]; }
    virtual const cmatrix& invTH(size_t layer) const { return Th1[layer]; }
};

}}} // namespace plask::solvers::slab
#endif // PLASK__SOLVER_SLAB_DIAGONALIZER_H
