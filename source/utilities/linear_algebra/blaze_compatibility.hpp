#ifndef BLAZE_COMPATIBILITY_HPP
#define BLAZE_COMPATIBILITY_HPP

#include "general_definitions.hpp"

#include <blaze/Math.h>
#include <blaze/math/Subvector.h>
#include <blaze/math/Submatrix.h>
#include <blaze/math/Column.h>

template <typename T, uint m>
using StatVector = blaze::StaticVector<T, m>;
template <typename T, uint m, uint n>
using StatMatrix = blaze::StaticMatrix<T, m, n>;

template <typename T>
using DynVector = blaze::DynamicVector<T>;
template <typename T>
using DynMatrix = blaze::DynamicMatrix<T>;

template <typename T>
using SparseVector = blaze::CompressedVector<T>;
template <typename T>
using SparseMatrix = blaze::CompressedMatrix<T>;

template <typename T>
using IdentityMatrix = blaze::IdentityMatrix<T>;

/* Vector/Matrix (aka Tensor) Operations */
template <typename TensorType>
double norm(TensorType& tensor) {
    return blaze::norm(tensor);
}

template <typename TensorType>
double sqr_norm(TensorType& tensor) {
    return blaze::sqrNorm(tensor);
}

/* Vector Operations */
template <typename VectorType>
decltype(auto) subvector(VectorType& vector, uint start_row, uint size_row) {
    return blaze::subvector(vector, start_row, size_row);
}

/* Matrix Operations */
template <typename MatrixType>
decltype(auto) submatrix(MatrixType& matrix, uint start_row, uint start_col, uint size_row, uint size_col) {
    return blaze::submatrix(matrix, start_row, start_col, size_row, size_col);
}

template <typename MatrixType, uint col>
decltype(auto) column(MatrixType& matrix) {
    return blaze::column<col>(matrix);
}

template <typename MatrixType>
decltype(auto) inverse(MatrixType& matrix) {
    return blaze::inv(matrix);
}

/* Solving Linear System */
template <typename MatrixType, typename VectorType>
void solve_sle(MatrixType& A, VectorType& b) {
    int ipiv[b.size()];

    blaze::gesv(A, b, ipiv);
}

template <typename VectorType, typename T>
void solve_sle(SparseMatrix<T>& A_sparse, VectorType& b) {
    // Avoid using this function, use a library with sparse solvers, e.g. Eigen
    uint rows = A_sparse.rows();
    uint cols = A_sparse.columns();

    DynMatrix<double> A_dense(rows, cols, 0.0);

    A_dense = A_sparse;

    int ipiv[b.size()];

    blaze::gesv(A_dense, b, ipiv);
}

#endif