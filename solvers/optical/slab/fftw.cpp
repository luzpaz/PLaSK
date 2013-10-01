#include "fft.h"

#ifdef USE_FFTW

#ifdef OPENMP_FOUND
#include <omp.h>
#endif

namespace plask { namespace solvers { namespace slab { namespace FFT {

namespace detail {
    struct FftwInitializer {
        FftwInitializer() {
#if defined(OPENMP_FOUND) && defined(USE_PARALLEL_FFT)
            fftw_init_threads();
            fftw_plan_with_nthreads(omp_get_max_threads());
#endif
        }
        ~FftwInitializer() {
#if defined(OPENMP_FOUND) && defined(USE_PARALLEL_FFT)
            fftw_cleanup_threads();
#else
            fftw_cleanup();
#endif
        }
    };
    FftwInitializer fftwinitializer;
}

Forward1D::Forward1D(): plan(nullptr) {}

Forward1D::Forward1D(Forward1D&& old):
    lot(old.lot), n(old.n), st(old.st),
    symmetry(old.symmetry),
    plan(old.plan) {
    old.plan = nullptr;
}

Forward1D& Forward1D::operator=(Forward1D&& old) {
    lot = old.lot; n = old.n; st = old.st;
    symmetry = old.symmetry;
    plan = old.plan;
    old.plan = nullptr;
    return *this;
}

Forward1D::Forward1D(int lot, int n, Symmetry symmetry, int st):
    lot(lot), n(n), st(st), symmetry(symmetry) {
    if (symmetry == SYMMETRY_NONE) {
        fftw_complex* data = new fftw_complex[n*lot*st];
        plan = fftw_plan_many_dft(1, &this->n, lot,
                                  data, nullptr, lot*st, st,
                                  data, nullptr, lot*st, st,
                                  FFTW_FORWARD, FFTW_ESTIMATE);
        delete[] data;
    } else if (symmetry == SYMMETRY_EVEN) {
        double* data = new double[2*n*lot*st];
        static const fftw_r2r_kind kinds[] = { FFTW_REDFT10 };
        plan = fftw_plan_many_r2r(1, &this->n, 2*lot,
                                  reinterpret_cast<double*>(data), nullptr, 2*lot*st, st,
                                  reinterpret_cast<double*>(data), nullptr, 2*lot*st, st,
                                  kinds, FFTW_ESTIMATE);
        delete[] data;
    } else
        throw NotImplemented("forward FFT for odd symmetry");
}

void Forward1D::execute(dcomplex* data) {
    if (!plan) throw CriticalException("No FFTW plan");
    double factor;
    if (symmetry == SYMMETRY_NONE) {
        fftw_execute_dft(plan, reinterpret_cast<fftw_complex*>(data), reinterpret_cast<fftw_complex*>(data));
        factor = 1.0 / n;
    } else {
        fftw_execute_r2r(plan, reinterpret_cast<double*>(data), reinterpret_cast<double*>(data));
        factor = 0.5 / n;
    }
    for (int N = lot*n, i = 0; i < N; ++i) data[i] *= factor;
}

Forward1D::~Forward1D() {
    if (plan) fftw_destroy_plan(plan);
}




Forward2D::Forward2D(): plan(nullptr) {}

Forward2D::Forward2D(Forward2D&& old):
    lot(old.lot), n1(old.n1), n2(old.n2), st(old.st),
    symmetry1(old.symmetry1), symmetry2(old.symmetry2),
    plan(old.plan) {
    old.plan = nullptr;
}

Forward2D& Forward2D::operator=(Forward2D&& old) {
    lot = old.lot; n1 = old.n1; n2 = old.n2; st = old.st;
    symmetry1 = old.symmetry1; symmetry2 = old.symmetry2;
    plan = old.plan;
    old.plan = nullptr;
    return *this;
}

Forward2D::Forward2D(int lot, int n1, int n2, Symmetry symmetry1, Symmetry symmetry2, int st):
    lot(lot), n1(n1), n2(n2), st(st), symmetry1(symmetry1), symmetry2(symmetry2) {
    if (symmetry1 == SYMMETRY_NONE && symmetry2 == SYMMETRY_NONE) {
        fftw_complex* data = new fftw_complex[n1*n2*lot*st];
        plan = fftw_plan_many_dft(2, &this->n1, lot,
                                  data, nullptr, lot*st, st,
                                  data, nullptr, lot*st, st,
                                  FFTW_FORWARD, FFTW_ESTIMATE);
        delete[] data;
    } else if (symmetry1 == SYMMETRY_EVEN && symmetry2 == SYMMETRY_EVEN) {
        throw NotImplemented("FFTW even,even");//TODO
    } else if (symmetry1 == SYMMETRY_NONE && symmetry2 == SYMMETRY_EVEN) {
        throw NotImplemented("FFTW none,even");//TODO
    } else if (symmetry1 == SYMMETRY_EVEN && symmetry2 == SYMMETRY_NONE) {
        throw NotImplemented("FFTW even,none");//TODO
    } else
        throw NotImplemented("forward FFT for odd symmetry");
}

void Forward2D::execute(dcomplex* data) {
    if (!plan) throw CriticalException("No FFTW plan");
    double factor;
    if (symmetry1 == SYMMETRY_NONE && symmetry2 == SYMMETRY_NONE) {
        fftw_execute_dft(plan, reinterpret_cast<fftw_complex*>(data), reinterpret_cast<fftw_complex*>(data));
        factor = 0.5 / n1 / n2;
    } else
        throw NotImplemented("Forward2D");
    for (int N = lot*n1*n2, i = 0; i < N; ++i) data[i] *= factor;
}

Forward2D::~Forward2D() {
    if (plan) fftw_destroy_plan(plan);
}




Backward1D::Backward1D(): plan(nullptr) {}

Backward1D::Backward1D(Backward1D&& old):
    lot(old.lot), n(old.n), st(old.st),
    symmetry(old.symmetry),
    plan(old.plan) {
    old.plan = nullptr;
}

Backward1D& Backward1D::operator=(Backward1D&& old) {
    lot = old.lot; n = old.n; st = old.st;
    symmetry = old.symmetry;
    plan = old.plan;
    old.plan = nullptr;
    return *this;
}

Backward1D::Backward1D(int lot, int n, Symmetry symmetry, int st):
    lot(lot), n(n), st(st), symmetry(symmetry) {
    if (symmetry == SYMMETRY_NONE) {
        fftw_complex* data = new fftw_complex[n*lot*st];
        plan = fftw_plan_many_dft(1, &this->n, lot,
                                  reinterpret_cast<fftw_complex*>(data), nullptr, lot*st, st,
                                  reinterpret_cast<fftw_complex*>(data), nullptr, lot*st, st,
                                  FFTW_BACKWARD, FFTW_ESTIMATE);
        delete[] data;
    } else if (symmetry == SYMMETRY_EVEN) {
        double* data = new double[2*n*lot*st];
        static const fftw_r2r_kind kinds[] = { FFTW_REDFT01 };
        plan = fftw_plan_many_r2r(1, &this->n, 2*lot,
                                  reinterpret_cast<double*>(data), nullptr, 2*lot*st, st,
                                  reinterpret_cast<double*>(data), nullptr, 2*lot*st, st,
                                  kinds, FFTW_ESTIMATE);
        delete[] data;
    } else {
        double* data = new double[2*n*lot*st];
        static const fftw_r2r_kind kinds[] = { FFTW_RODFT01 };
        plan = fftw_plan_many_r2r(1, &this->n, 2*lot,
                                  reinterpret_cast<double*>(data), nullptr, 2*lot*st, st,
                                  reinterpret_cast<double*>(data), nullptr, 2*lot*st, st,
                                  kinds, FFTW_ESTIMATE);
        delete[] data;
    }
}

void Backward1D::execute(dcomplex* data) {
    if (!plan) throw CriticalException("No FFTW plan");
    if (symmetry == SYMMETRY_NONE)
        fftw_execute_dft(plan, reinterpret_cast<fftw_complex*>(data), reinterpret_cast<fftw_complex*>(data));
    else
        fftw_execute_r2r(plan, reinterpret_cast<double*>(data), reinterpret_cast<double*>(data));
}

Backward1D::~Backward1D() {
    if (plan) fftw_destroy_plan(plan);
}




Backward2D::Backward2D(): plan(nullptr) {}

Backward2D::Backward2D(Backward2D&& old):
    lot(old.lot), n1(old.n1), n2(old.n2), st(old.st),
    symmetry1(old.symmetry1), symmetry2(old.symmetry2),
    plan(old.plan) {
    old.plan = nullptr;
}

Backward2D& Backward2D::operator=(Backward2D&& old) {
    lot = old.lot; n1 = old.n1; n2 = old.n2; st = old.st;
    symmetry1 = old.symmetry1; symmetry2 = old.symmetry2;
    plan = old.plan;
    old.plan = nullptr;
    return *this;
}

Backward2D::Backward2D(int lot, int n1, int n2, Symmetry symmetry1, Symmetry symmetry2, int st):
    lot(lot), n1(n1), n2(n2), st(st), symmetry1(symmetry1), symmetry2(symmetry2) {
    plan = nullptr; //TODO
}

void Backward2D::execute(dcomplex* data) {
    if (!plan) throw CriticalException("No FFTW plan");
//     fftw_execute(plan);
}

Backward2D::~Backward2D() {
    if (plan) fftw_destroy_plan(plan);
}


}}}} // namespace plask::solvers::slab

#endif // USE_FFTW
