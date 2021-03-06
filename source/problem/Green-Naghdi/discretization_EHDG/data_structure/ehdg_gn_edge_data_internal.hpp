#ifndef EHDG_GN_EDGE_DATA_INTERNAL_HPP
#define EHDG_GN_EDGE_DATA_INTERNAL_HPP

namespace GN {
namespace EHDG {
struct EdgeInternal : SWE_SIM::EdgeInternal {
    EdgeInternal() = default;
    EdgeInternal(const uint ngp)
        : SWE_SIM::EdgeInternal(ngp), w1_hat_w1_hat_kernel_at_gp(GN::n_dimensions * GN::n_dimensions, ngp) {}

    HybMatrix<double, GN::n_dimensions * GN::n_dimensions> w1_hat_w1_hat_kernel_at_gp;

    DynMatrix<double> w1_hat_w1_hat;
    DynVector<double> w1_hat_rhs;

    DynVector<double> w1_hat_w1_hat_flat;
    std::vector<DynVector<double>> w1_hat_w1_hat_con_flat;

    std::vector<uint> dc_global_dof_indx;
    uint dc_sol_offset;
};
}
}

#endif
