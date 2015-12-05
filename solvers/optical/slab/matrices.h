/**
 *  \file   matrix.h Definitions of matrices and matrix operations
 */
#ifndef PLASK__SOLVER_VSLAB_MATRIX_H
#define PLASK__SOLVER_VSLAB_MATRIX_H

#include <cstring>
#include <cmath>

#include <plask/plask.hpp>
#include "fortran.h"

namespace plask { namespace  solvers { namespace slab {

template <typename T> class MatrixDiagonal;

/// General simple matrix template
template <typename T>
class Matrix {
  protected:
    int r, c;

    T* data_;               ///< The data of the matrix
    int* gc;                ///< the reference count for the garbage collector

  public:

    Matrix() : gc(nullptr) {}

    Matrix(int m, int n) : r(m), c(n) {
        data_ = aligned_new_array<T>(m*n); gc = new int;
        write_debug("allocating matrix {:d}x{:d} ({:.3f} MB) at {:p}", r, c, r*c*sizeof(T)/1048576., (void*)data_);
        *gc = 1;
    }

    Matrix(int m, int n, T val) : r(m), c(n) {
        data_ = aligned_new_array<T>(m*n); gc = new int;
        write_debug("allocating matrix {:d}x{:d} ({:.3f} MB) at {:p}", r, c, r*c*sizeof(T)/1048576., (void*)data_);
        std::fill_n(data_, m*n, val);
        *gc = 1;
    }

    Matrix(const Matrix<T>& M) : r(M.r), c(M.c), data_(M.data_), gc(M.gc) {
        if (gc) (*gc)++;
    }

    Matrix<T>& operator=(const Matrix<T>& M) {
        if (gc) {
            (*gc)--;
            if (*gc == 0) {
                delete gc; aligned_delete_array(r*c, data_);
                write_debug("freeing matrix {:d}x{:d} ({:.3f} MB) at {:p}", r, c, r*c*sizeof(T)/1048576., (void*)data_);
            }
        }
        r = M.r; c = M.c; data_ = M.data_; gc = M.gc; if (gc) (*gc)++;
        return *this;
    }

    Matrix(const MatrixDiagonal<T>& M) {
        r = c = M.size();
        data_ = aligned_new_array<T>(r*c); gc = new int;
        write_debug("allocating matrix {:d}x{:d} ({:.3f} MB) at {:p} (from diagonal)", r, c, r*c*sizeof(T)/1048576., (void*)data_);
        int size = r*c;
        std::fill_n(data_, size, 0);
        for (int j = 0, n = 0; j < r; j++, n += c+1) data_[n] = M[j];
        *gc = 1;
    }

    Matrix(int m, int n, T* existing_data) : r(m), c(n), gc(nullptr) {
        // Create matrix using exiting data. This data is never garbage-collected
        data_ = existing_data;
    }

    ~Matrix() {
        if (gc) {
            (*gc)--;
            if (*gc == 0) {
                delete gc; aligned_delete_array(r*c, data_);
                write_debug("freeing matrix {:d}x{:d} ({:.3f} MB) at {:p}", r, c, r*c*sizeof(T)/1048576., (void*)data_);
            }
        }
    }

    inline const T* data() const { return data_; }
    inline T* data() { return data_; }

    inline const T& operator[](int i) const {
        assert(i < r*c);
        return data_[i];
    }
    inline T& operator[](int i) {
        assert(i < r*c);
        return data_[i];
    }

    inline const T& operator()(int m, int n) const {
        assert(m < r);
        assert(n < c);
        return data_[n*r + m];
    }
    inline T& operator()(int m, int n) {
        assert(m < r);
        assert(n < c);
        return data_[n*r + m];
    }

    inline int rows() const { return r; }
    inline int cols() const { return c; }

    Matrix<T> copy() const {
        Matrix<T> copy_(r, c);
        memcpy(copy_.data_, data_, r*c*sizeof(T));
        return copy_;
    }

    Matrix<T>& operator*=(T a) {
        int size = r*c; for (int i = 0; i < size; i++) data_[i] *= a;
        return *this;
    }
    Matrix<T>& operator/=(T a) {
        int size = r*c; for (int i = 0; i < size; i++) data_[i] /= a;
        return *this;
    }

    /// Check if the matrix contains any NaN
    inline bool isnan() const {
        int n = r * c;
        for (int i = 0; i < n; ++i)
            if (std::isnan(real(data_[i])) || std::isnan(imag(data_[i]))) return true;
        return false;
    }

};


/// General simple diagonal matrix template
template <typename T>
class MatrixDiagonal {
  protected:
    int siz;

    int* gc;                //< the reference count for the garbage collector
    T* data_;                //< The data of the matrix

  public:

    MatrixDiagonal() : gc(nullptr) {}

    MatrixDiagonal(int n) : siz(n) {
        data_ = aligned_new_array<T>(n); gc = new int;
        write_debug("allocating diagonal matrix {0}x{0} (%2$.3f MB) at {2}", siz, siz*sizeof(T)/1048576., (void*)data_);
        *gc = 1;
    }

    MatrixDiagonal(int n, T val) : siz(n) {
        data_ = aligned_new_array<T>(n); gc = new int;
        write_debug("allocating and filling diagonal matrix {0}x{0} (%2$.3f MB) at {2}", siz, siz*sizeof(T)/1048576., (void*)data_);
        std::fill_n(data_, n, val);
        *gc = 1;
    }

    MatrixDiagonal(const MatrixDiagonal<T>& M) : siz(M.siz), gc(M.gc), data_(M.data_) {
        if (gc) (*gc)++;
    }

    MatrixDiagonal<T>& operator=(const MatrixDiagonal<T>& M) {
        if (gc) {
            (*gc)--;
            if (*gc == 0) {
                delete gc; aligned_delete_array(siz, data_);
                write_debug("freeing diagonal matrix {0}x{0} (%2$.3f MB) at {2}", siz, siz*sizeof(T)/1048576., (void*)data_);
            }
        }
        siz = M.siz; data_ = M.data_; gc = M.gc; if (gc) (*gc)++;
        return *this;
    }

    ~MatrixDiagonal() {
        if (gc) {
            (*gc)--;
            if (*gc == 0) {
                delete gc; aligned_delete_array(siz, data_);
                write_debug("freeing diagonal matrix {0}x{0} (%2$.3f MB) at {2}", siz, siz*sizeof(T)/1048576., (void*)data_);
            }
        }
    }

    inline const T* data() const { return data_; }
    inline T* data() { return data_; }

    inline const T& operator[](int n) const {
        assert(n < siz);
        return data_[n];
    }
    inline T& operator[](int n) {
        assert(n < siz);
        return data_[n];
    }

    inline const T& operator()(int m, int n) const {
        assert(m < siz);
        assert(n < siz);
        return (m == n)? data_[n] : 0;
    }
    inline T& operator()(int m, int n) {
        assert(m < siz);
        assert(n < siz);
        if (m !=n) throw ComputationError("MatrixDiagonal::operator()", "wrong index");
        else return data_[n];
    }

    inline int size() const { return siz; }

    MatrixDiagonal<T> copy() const {
        MatrixDiagonal<T> C(siz);
        memcpy(C.data, data_, siz*sizeof(T));
        return C;
    }

    MatrixDiagonal<T>& operator*=(T a) { for (int i = 0; i < siz; i++) data_[i] *= a; return *this; }
    MatrixDiagonal<T>& operator/=(T a) { for (int i = 0; i < siz; i++) data_[i] /= a; return *this; }

    /// Check if the matrix contains any NaN
    inline bool isnan() const {
        for (int i = 0; i != siz; ++i)
            if (std::isnan(real(data_[i])) || std::isnan(imag(data_[i]))) return true;
        return false;
    }

};

//**************************************************************************
// Rectangular matrix of real and complex numbers
typedef Matrix<double> dmatrix;
typedef Matrix<dcomplex> cmatrix;

// Column vector and diagonal matrix
typedef DataVector<dcomplex> cvector;
typedef DataVector<const dcomplex> const_cvector;
typedef MatrixDiagonal<dcomplex> cdiagonal;

//**************************************************************************
/// Multiplication operator of the matrices (using BLAS level3)
inline cmatrix operator*(const cmatrix& A, const cmatrix& B) {
    if (A.cols() != B.rows()) throw ComputationError("operator*<cmatrix,cmatrix>", "Cannot multiply: A.cols != B.rows");
    cmatrix C(A.rows(), B.cols());
    zgemm('n', 'n', A.rows(), B.cols(), A.cols(), 1., A.data(), A.rows(), B.data(), B.rows(), 0., C.data(), C.rows());
    return C;
}

/// Multiplication operator of the matrix-vector product (using BLAS level3)
inline cvector operator*(const cmatrix& A, const cvector& v) {
    int n = A.cols(), m = A.rows();
    if (n != v.size()) throw ComputationError("mult_matrix_by_vector", "A.cols != v.size");
    cvector dst(m);
    zgemv('n', m, n, 1., A.data(), m, v.data(), 1, 0., dst.data(), 1);
    return dst;
}

/// Multiplication by the diagonal matrix (right)
template <typename T>
inline cmatrix operator*(const Matrix<T>& A, const MatrixDiagonal<T>& B) {
    if (A.cols() != B.size()) throw ComputationError("operator*<cmatrix,cdiagonal>", "Cannot multiply: A.cols != B.size");
    cmatrix C(A.rows(), B.size());
    int n = 0;
    int r = A.rows(), c = A.cols();
    for (int j = 0; j < c; j++)
    {
        T b = B[j];
        for (int i = 0; i < r; i++, n++)
            C[n] = A[n] * b;
    }
    return C;
}

/// Multiplication by the diagonal matrix (left)
template <typename T>
inline cmatrix operator*(const MatrixDiagonal<T>& A, const Matrix<T>& B) {
    if (A.size() != B.rows()) throw ComputationError("operator*<cdiagonal,cmatrix>", "Cannot multiply: A.size != B.rows");
    cmatrix C(A.size(), B.cols());
    int n = 0;
    int r = B.rows(), c = B.cols();
    for (int j = 0; j < c; j++)
        for (int i = 0; i < r; i++, n++)
            C[n] = A[i] * B[n];
    return C;
}

/// Multiplication of matrix by diagonal in-place (replacing A)
template <typename T>
inline void mult_matrix_by_diagonal(Matrix<T>& A, const MatrixDiagonal<T>& B) {
    if (A.cols() != B.size()) throw ComputationError("mult_matrix_by_diagonal", "Cannot multiply: A.cols != B.size");
    int n = 0;
    int r = A.rows(), c = A.cols();
    for (int j = 0; j < c; j++) {
        T b = B[j];
        for (int i = 0; i < r; i++, n++)
            A[n] *= b;
    }
}

/// Multiplication of diagonal by matrix in-place (replacing B)
template <typename T>
inline void mult_diagonal_by_matrix(const MatrixDiagonal<T>& A, Matrix<T>& B) {
    if (A.size() != B.rows()) throw ComputationError("mult_diagonal_by_matrix", "Cannot multiply: A.size != B.rows");
    int n = 0;
    int r = B.rows(), c = B.cols();
    for (int j = 0; j < c; j++)
        for (int i = 0; i < r; i++, n++)
            B[n] *= A[i];
}


// BLAS wrappers for multiplications without allocating additional storage
inline void mult_matrix_by_vector(const cmatrix& A, const const_cvector& v, cvector& dst) {
    int m, n;
    if ((n = A.cols()) != v.size()) throw ComputationError("mult_matrix_by_vector", "A.cols != v.size");
    if ((m = A.rows()) != dst.size()) throw ComputationError("mult_matrix_by_vector", "A.rows != dst.size");
    zgemv('n', m, n, 1., A.data(), m, v.data(), 1, 0., dst.data(), 1);
}

inline void mult_matrix_by_matrix(const cmatrix& A, const cmatrix& B, cmatrix& dst) {
    int m, n, k;
    if ((k = A.cols()) != B.rows()) throw ComputationError("mult_matrix_by_matrix", "cannot multiply: A.cols != B.rows");
    if ((m = A.rows()) != dst.rows()) throw ComputationError("mult_matrix_by_matrix", "A.rows != dst.rows");
    if ((n = B.cols()) != dst.cols()) throw ComputationError("mult_matrix_by_matrix", "B.cols != dst.cols");
    zgemm('n', 'n', m, n, k, 1., A.data(), m, B.data(), k, 0., dst.data(), m);
}

inline void add_mult_matrix_by_vector(const cmatrix& A, const cvector& v, cvector& dst) {
    int m, n;
    if ((n = A.cols()) != v.size()) throw ComputationError("add_mult_matrix_by_vector", "A.cols != v.size");
    if ((m = A.rows()) != dst.size()) throw ComputationError("add_mult_matrix_by_vector", "A.rows != dst.size");
    zgemv('n', m, n, 1., A.data(), m, v.data(), 1, 1., dst.data(), 1);
}

inline void add_mult_matrix_by_matrix(const cmatrix& A, const cmatrix& B, cmatrix& dst) {
    int m, n, k;
    if ((k = A.cols()) != B.rows()) throw ComputationError("add_mult_matrix_by_matrix", "A.cols != B.rows");
    if ((m = A.rows()) != dst.rows()) throw ComputationError("add_mult_matrix_by_matrix", "A.rows != dst.rows");
    if ((n = B.cols()) != dst.cols()) throw ComputationError("add_mult_matrix_by_matrix", "B.cols != dst.cols");
    zgemm('n', 'n', m, n, k, 1., A.data(), m, B.data(), k, 1., dst.data(), m);
}


// Some LAPACK wrappers
cmatrix invmult(cmatrix& A, cmatrix& B);
cvector invmult(cmatrix& A, cvector& B);
cmatrix inv(cmatrix& A);
dcomplex det(cmatrix& A);
int eigenv(cmatrix& A, cdiagonal& vals, cmatrix* rightv=NULL, cmatrix* leftv=NULL);

}}}
#endif // PLASK__SOLVER_VSLAB_MATRIX_H
