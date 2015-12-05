#ifndef PLASK__SOLVER_SLAB_ADMITTANCE_H
#define PLASK__SOLVER_SLAB_ADMITTANCE_H

#include "matrices.h"
#include "transfer.h"
#include "solver.h"


namespace plask { namespace solvers { namespace slab {

/**
 * Base class for all solvers using reflection matrix method.
 */
struct PLASK_SOLVER_API AdmittanceTransfer: public Transfer {

    /// The structure holding the set of diagonalized fields at the layer boundaries
    struct FieldsDiagonalized {
        cvector E0, H0, Ed, Hd;
    };

  protected:

    cmatrix Y;                                  ///< Admittance matrix

    bool needAllY;                              ///< Do we need to keep all Y matrices?

    std::vector<FieldsDiagonalized> fields;     ///< Vector of fields computed for each layer

  private:

    std::vector<cmatrix> memY;                  ///< admittance matrices for each layer

  public:

    cvector getReflectionVector(const cvector& incident, IncidentDirection direction) override {
        throw NotImplemented("Reflection calculations are not supported by the admittance transfer method");
    }

    cvector getTransmissionVector(const cvector& incident, IncidentDirection side) override {
        throw NotImplemented("Reflection calculations are not supported by the admittance transfer method");
    }

    AdmittanceTransfer(SlabBase* solver, Expansion& expansion);

  protected:

    void getFinalMatrix() override;

    void determineFields() override;

    void determineReflectedFields(const cvector& incident, IncidentDirection side) override {
        throw NotImplemented("Reflection calculations are not supported by the admittance transfer method");
    }

    cvector getFieldVectorE(double z, int n) override;

    cvector getFieldVectorH(double z, int n) override;


    /**
     * Find admittance matrix for the part of the structure
     * \param start starting layer
     * \param end last layer (reflection matrix is computed for this layer)
     */
    void findAdmittance(int start, int end);

    /**
     * Store the Y matrix for the layer prepared before
     * \param n layer number
     */
    void storeY(size_t n);

    /**
     * Get the Y matrix for n-th layer
     * \param n layer number
     */
    const cmatrix& getY(int n) {
        if (memY.size() == solver->stack.size() && needAllY)
            return memY[n];
        else
            throw CriticalException("{0}: Y matrices are not stored", solver->getId());
    }

    /// Determine the y1 efficiently
    inline void get_y1(const cdiagonal& gamma, double d, cdiagonal& y1) const {
        int N = gamma.size();
        assert(y1.size() == N);
        for (int i = 0; i < N; i++) {
            dcomplex t = tanh(I*gamma[i]*d);
            if (isinf(real(t)) || isinf(imag(t))) y1[i] = 0.;
            else if (t == 0.) throw ComputationError(solver->getId(), "y1 has some infinite value");
            else y1[i] = 1. / t;
        }
    }

    /// Determine the y2 efficiently
    inline void get_y2(const cdiagonal& gamma, double d, cdiagonal& y2) const {
        int N = gamma.size();
        assert(y2.size() == N);
        for (int i = 0; i < N; i++) {
            dcomplex s = sinh(I*gamma[i]*d);
            if (isinf(real(s)) || isinf(imag(s))) y2[i] = 0.;
            else if (s == 0.) throw ComputationError(solver->getId(), "y2 has some infinite value");
            else y2[i] = - 1. / s;
        }
    }

};


}}} // namespace plask::solvers::slab

#endif // PLASK__SOLVER_SLAB_ADMITTANCE_H
