#ifndef IHDG_SWE_PROC_INTFACE_HPP
#define IHDG_SWE_PROC_INTFACE_HPP

namespace SWE {
namespace IHDG {
template <typename InterfaceType>
void Problem::prepare_interface_kernel(const RKStepper& stepper, InterfaceType& intface) {
    const uint stage = stepper.GetStage();

    auto& state_in    = intface.data_in.state[stage];
    auto& boundary_in = intface.data_in.boundary[intface.bound_id_in];

    auto& state_ex    = intface.data_ex.state[stage];
    auto& boundary_ex = intface.data_ex.boundary[intface.bound_id_ex];

    intface.ComputeUgpIN(state_in.ze, boundary_in.ze_at_gp);
    intface.ComputeUgpIN(state_in.qx, boundary_in.qx_at_gp);
    intface.ComputeUgpIN(state_in.qy, boundary_in.qy_at_gp);

    intface.ComputeUgpEX(state_ex.ze, boundary_ex.ze_at_gp);
    intface.ComputeUgpEX(state_ex.qx, boundary_ex.qx_at_gp);
    intface.ComputeUgpEX(state_ex.qy, boundary_ex.qy_at_gp);

    uint gp_ex = 0;
    for (uint gp = 0; gp < intface.data_in.get_ngp_boundary(intface.bound_id_in); ++gp) {
        gp_ex = intface.data_in.get_ngp_boundary(intface.bound_id_in) - gp - 1;

        boundary_in.aux_at_gp[gp][SWE::Auxiliaries::h] =
            boundary_in.ze_at_gp[gp] + boundary_in.aux_at_gp[gp][SWE::Auxiliaries::bath];
        boundary_ex.aux_at_gp[gp_ex][SWE::Auxiliaries::h] =
            boundary_ex.ze_at_gp[gp_ex] + boundary_ex.aux_at_gp[gp_ex][SWE::Auxiliaries::bath];
    }

    /* Compute fluxes at boundary states */
    double nx_in, ny_in;
    double u_in, v_in;
    double uuh_in, vvh_in, uvh_in, pe_in;

    double nx_ex, ny_ex;
    double u_ex, v_ex;
    double uuh_ex, vvh_ex, uvh_ex, pe_ex;

    gp_ex = 0;
    for (uint gp = 0; gp < intface.data_in.get_ngp_boundary(intface.bound_id_in); ++gp) {
        gp_ex = intface.data_in.get_ngp_boundary(intface.bound_id_in) - gp - 1;

        /* IN State */

        nx_in = intface.surface_normal_in[gp][GlobalCoord::x];
        ny_in = intface.surface_normal_in[gp][GlobalCoord::y];

        u_in = boundary_in.qx_at_gp[gp] / boundary_in.aux_at_gp[gp][SWE::Auxiliaries::h];
        v_in = boundary_in.qy_at_gp[gp] / boundary_in.aux_at_gp[gp][SWE::Auxiliaries::h];

        uuh_in = u_in * boundary_in.qx_at_gp[gp];
        vvh_in = v_in * boundary_in.qy_at_gp[gp];
        uvh_in = u_in * boundary_in.qy_at_gp[gp];
        pe_in  = Global::g * (0.5 * std::pow(boundary_in.ze_at_gp[gp], 2) +
                             boundary_in.ze_at_gp[gp] * boundary_in.aux_at_gp[gp][SWE::Auxiliaries::bath]);

        boundary_in.ze_flux_dot_n_at_gp[gp] = boundary_in.qx_at_gp[gp] * nx_in + boundary_in.qy_at_gp[gp] * ny_in;
        boundary_in.qx_flux_dot_n_at_gp[gp] = (uuh_in + pe_in) * nx_in + uvh_in * ny_in;
        boundary_in.qy_flux_dot_n_at_gp[gp] = uvh_in * nx_in + (vvh_in + pe_in) * ny_in;

        /* EX State */

        nx_ex = intface.surface_normal_ex[gp_ex][GlobalCoord::x];
        ny_ex = intface.surface_normal_ex[gp_ex][GlobalCoord::y];

        u_ex = boundary_ex.qx_at_gp[gp_ex] / boundary_ex.aux_at_gp[gp_ex][SWE::Auxiliaries::h];
        v_ex = boundary_ex.qy_at_gp[gp_ex] / boundary_ex.aux_at_gp[gp_ex][SWE::Auxiliaries::h];

        uuh_ex = u_ex * boundary_ex.qx_at_gp[gp_ex];
        vvh_ex = v_ex * boundary_ex.qy_at_gp[gp_ex];
        uvh_ex = u_ex * boundary_ex.qy_at_gp[gp_ex];
        pe_ex  = Global::g * (0.5 * std::pow(boundary_ex.ze_at_gp[gp_ex], 2) +
                             boundary_ex.ze_at_gp[gp_ex] * boundary_ex.aux_at_gp[gp_ex][SWE::Auxiliaries::bath]);

        boundary_ex.ze_flux_dot_n_at_gp[gp_ex] =
            boundary_ex.qx_at_gp[gp_ex] * nx_ex + boundary_ex.qy_at_gp[gp_ex] * ny_ex;
        boundary_ex.qx_flux_dot_n_at_gp[gp_ex] = (uuh_ex + pe_ex) * nx_ex + uvh_ex * ny_ex;
        boundary_ex.qy_flux_dot_n_at_gp[gp_ex] = uvh_ex * nx_ex + (vvh_ex + pe_ex) * ny_ex;
    }
}
}
}

#endif
