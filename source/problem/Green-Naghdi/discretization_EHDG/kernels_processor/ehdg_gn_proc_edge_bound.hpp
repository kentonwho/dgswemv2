#ifndef EHDG_GN_PROC_EDGE_BOUND_HPP
#define EHDG_GN_PROC_EDGE_BOUND_HPP

namespace GN {
namespace EHDG {
template <typename EdgeBoundaryType>
void Problem::global_swe_edge_boundary_kernel(const RKStepper& stepper, EdgeBoundaryType& edge_bound) {
    auto& edge_state    = edge_bound.edge_data.edge_state;
    auto& edge_internal = edge_bound.edge_data.edge_internal;

    auto& boundary = edge_bound.boundary.data.boundary[edge_bound.boundary.bound_id];

    /* Take in state as initial edge state */
    edge_internal.q_init_at_gp = boundary.q_at_gp;

    edge_state.q_hat = edge_bound.L2Projection(edge_internal.q_init_at_gp);

    /* Newton-Raphson iterator */

    uint iter = 0;
    while (true) {
        iter++;

        Problem::global_swe_edge_boundary_iteration(stepper, edge_bound);

        if (iter == 100) {
            break;
        }

        double delta_hat_norm = norm(edge_internal.rhs_global) / edge_internal.rhs_global.size();

        if (delta_hat_norm < 1.0e-8) {
            break;
        }
    }

    /* Compute Numerical Flux */

    edge_internal.q_hat_at_gp = edge_bound.ComputeUgp(edge_state.q_hat);

    row(edge_internal.aux_hat_at_gp, GN::Auxiliaries::h) =
        row(edge_internal.q_hat_at_gp, GN::Variables::ze) + row(boundary.aux_at_gp, GN::Auxiliaries::bath);

    edge_bound.boundary.boundary_condition.ComputeNumericalFluxSWE(edge_bound);
}

template <typename EdgeBoundaryType>
void Problem::global_swe_edge_boundary_iteration(const RKStepper& stepper, EdgeBoundaryType& edge_bound) {
    auto& edge_state    = edge_bound.edge_data.edge_state;
    auto& edge_internal = edge_bound.edge_data.edge_internal;

    auto& boundary = edge_bound.boundary.data.boundary[edge_bound.boundary.bound_id];

    edge_internal.q_hat_at_gp = edge_bound.ComputeUgp(edge_state.q_hat);

    row(edge_internal.aux_hat_at_gp, GN::Auxiliaries::h) =
        row(edge_internal.q_hat_at_gp, GN::Variables::ze) + row(boundary.aux_at_gp, GN::Auxiliaries::bath);

    /* Assemble global kernels */

    edge_bound.boundary.boundary_condition.ComputeGlobalKernelsSWE(stepper, edge_bound);

    /* Assemble global system */

    for (uint dof_i = 0; dof_i < edge_bound.edge_data.get_ndof(); ++dof_i) {
        for (uint dof_j = 0; dof_j < edge_bound.edge_data.get_ndof(); ++dof_j) {
            submatrix(edge_internal.delta_hat_global,
                      GN::n_variables * dof_i,
                      GN::n_variables * dof_j,
                      GN::n_variables,
                      GN::n_variables) =
                reshape<double, GN::n_variables>(
                    edge_bound.IntegrationLambdaLambda(dof_i, dof_j, edge_internal.delta_hat_global_kernel_at_gp));
        }

        subvector(edge_internal.rhs_global, GN::n_variables * dof_i, GN::n_variables) =
            -edge_bound.IntegrationLambda(dof_i, edge_internal.rhs_global_kernel_at_gp);
    }

    /* Solve global system for delta_q */

    solve_sle(edge_internal.delta_hat_global, edge_internal.rhs_global);

    /* Increment q */

    for (uint dof = 0; dof < edge_bound.edge_data.get_ndof(); ++dof) {
        edge_state.q_hat(GN::Variables::ze, dof) += edge_internal.rhs_global[3 * dof];
        edge_state.q_hat(GN::Variables::qx, dof) += edge_internal.rhs_global[3 * dof + 1];
        edge_state.q_hat(GN::Variables::qy, dof) += edge_internal.rhs_global[3 * dof + 2];
    }
}

template <typename EdgeBoundaryType>
void Problem::local_dc_edge_boundary_kernel(const RKStepper& stepper, EdgeBoundaryType& edge_bound) {
    auto& edge_internal = edge_bound.edge_data.edge_internal;

    auto& internal = edge_bound.boundary.data.internal;
    auto& boundary = edge_bound.boundary.data.boundary[edge_bound.boundary.bound_id];

    // at this point h_hat_at_gp
    // has been computed in swe edge_boundary kernel

    double tau = -2000.0;  // hardcode the tau value here

    // set kernels up
    for (uint gp = 0; gp < edge_bound.edge_data.get_ngp(); ++gp) {
        column(boundary.w1_w1_kernel_at_gp, gp) =
            -NDParameters::alpha / 3.0 * tau * IdentityVector<double>(GN::n_dimensions);

        column(boundary.w1_w1_hat_kernel_at_gp, gp) =
            NDParameters::alpha / 3.0 * tau * IdentityVector<double>(GN::n_dimensions);
    }

    row(boundary.w2_w1_hat_kernel_at_gp, GlobalCoord::x) = -cwise_division(
        row(edge_bound.boundary.surface_normal, GlobalCoord::x), row(edge_internal.aux_hat_at_gp, GN::Auxiliaries::h));
    row(boundary.w2_w1_hat_kernel_at_gp, GlobalCoord::y) = -cwise_division(
        row(edge_bound.boundary.surface_normal, GlobalCoord::y), row(edge_internal.aux_hat_at_gp, GN::Auxiliaries::h));

    for (uint dof_i = 0; dof_i < edge_bound.boundary.data.get_ndof(); ++dof_i) {
        for (uint dof_j = 0; dof_j < edge_bound.boundary.data.get_ndof(); ++dof_j) {
            submatrix(internal.w1_w1,
                      GN::n_dimensions * dof_i,
                      GN::n_dimensions * dof_j,
                      GN::n_dimensions,
                      GN::n_dimensions) +=
                reshape<double, GN::n_dimensions>(
                    edge_bound.boundary.IntegrationPhiPhi(dof_i, dof_j, boundary.w1_w1_kernel_at_gp));
        }
    }

    for (uint dof_i = 0; dof_i < edge_bound.boundary.data.get_ndof(); ++dof_i) {
        for (uint dof_j = 0; dof_j < edge_bound.edge_data.get_ndof(); ++dof_j) {
            submatrix(boundary.w1_w1_hat,
                      GN::n_dimensions * dof_i,
                      GN::n_dimensions * dof_j,
                      GN::n_dimensions,
                      GN::n_dimensions) =
                reshape<double, GN::n_dimensions>(
                    edge_bound.IntegrationPhiLambda(dof_i, dof_j, boundary.w1_w1_hat_kernel_at_gp));

            boundary.w2_w1_hat(dof_i, GN::n_dimensions * dof_j + GlobalCoord::x) =
                edge_bound.IntegrationPhiLambda(dof_i, dof_j, row(boundary.w2_w1_hat_kernel_at_gp, GlobalCoord::x));

            boundary.w2_w1_hat(dof_i, GN::n_dimensions * dof_j + GlobalCoord::y) =
                edge_bound.IntegrationPhiLambda(dof_i, dof_j, row(boundary.w2_w1_hat_kernel_at_gp, GlobalCoord::y));
        }
    }
}

template <typename EdgeBoundaryType>
void Problem::global_dc_edge_boundary_kernel(const RKStepper& stepper, EdgeBoundaryType& edge_bound) {
    auto& edge_internal = edge_bound.edge_data.edge_internal;

    auto& boundary = edge_bound.boundary.data.boundary[edge_bound.boundary.bound_id];

    edge_bound.boundary.boundary_condition.ComputeGlobalKernelsDC(stepper, edge_bound);

    for (uint dof_i = 0; dof_i < edge_bound.edge_data.get_ndof(); ++dof_i) {
        for (uint dof_j = 0; dof_j < edge_bound.edge_data.get_ndof(); ++dof_j) {
            submatrix(edge_internal.w1_hat_w1_hat,
                      GN::n_dimensions * dof_i,
                      GN::n_dimensions * dof_j,
                      GN::n_dimensions,
                      GN::n_dimensions) =
                reshape<double, GN::n_dimensions>(
                    edge_bound.IntegrationLambdaLambda(dof_i, dof_j, edge_internal.w1_hat_w1_hat_kernel_at_gp));
        }
    }

    for (uint dof_i = 0; dof_i < edge_bound.edge_data.get_ndof(); ++dof_i) {
        for (uint dof_j = 0; dof_j < edge_bound.boundary.data.get_ndof(); ++dof_j) {
            submatrix(boundary.w1_hat_w1,
                      GN::n_dimensions * dof_i,
                      GN::n_dimensions * dof_j,
                      GN::n_dimensions,
                      GN::n_dimensions) =
                reshape<double, GN::n_dimensions>(
                    edge_bound.IntegrationPhiLambda(dof_j, dof_i, boundary.w1_hat_w1_kernel_at_gp));
        }
    }
}
}
}

#endif