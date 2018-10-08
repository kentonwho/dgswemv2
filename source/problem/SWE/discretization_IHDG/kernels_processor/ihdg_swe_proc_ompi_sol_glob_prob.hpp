#ifndef IHDG_SWE_PROC_OMPI_SOL_GLOB_PROB_HPP
#define IHDG_SWE_PROC_OMPI_SOL_GLOB_PROB_HPP

namespace SWE {
namespace IHDG {
template <typename OMPISimUnitType>
bool Problem::ompi_solve_global_problem(std::vector<std::unique_ptr<OMPISimUnitType>>& sim_units) {
    auto& global_data = sim_units[0]->discretization.global_data;

    Mat& delta_hat_global = global_data.delta_hat_global;
    Vec& rhs_global       = global_data.rhs_global;

    for (uint su_id = 0; su_id < sim_units.size(); ++su_id) {
        sim_units[su_id]->discretization.mesh.CallForEachElement([](auto& elt) {
            auto& internal = elt.data.internal;

            internal.delta_local_inv = inverse(internal.delta_local);
        });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeInterface([&rhs_global,
                                                                                 &delta_hat_global](auto& edge_int) {
            auto& edge_internal = edge_int.edge_data.edge_internal;

            auto& internal_in = edge_int.interface.data_in.internal;
            auto& internal_ex = edge_int.interface.data_ex.internal;

            auto& boundary_in = edge_int.interface.data_in.boundary[edge_int.interface.bound_id_in];
            auto& boundary_ex = edge_int.interface.data_ex.boundary[edge_int.interface.bound_id_ex];

            std::vector<uint>& global_dof_indx = edge_internal.global_dof_indx;

            edge_internal.delta_hat_global -=
                boundary_in.delta_global * internal_in.delta_local_inv * boundary_in.delta_hat_local +
                boundary_ex.delta_global * internal_ex.delta_local_inv * boundary_ex.delta_hat_local;

            edge_internal.rhs_global -= boundary_in.delta_global * internal_in.delta_local_inv * internal_in.rhs_local +
                                        boundary_ex.delta_global * internal_ex.delta_local_inv * internal_ex.rhs_local;

            std::vector<double> rhs_global_container(global_dof_indx.size());
            std::vector<double> delta_hat_global_container(global_dof_indx.size() * global_dof_indx.size());

            for (uint i = 0; i < global_dof_indx.size(); ++i) {
                rhs_global_container[i] = edge_internal.rhs_global[i];

                for (uint j = 0; j < global_dof_indx.size(); ++j) {
                    delta_hat_global_container[i * global_dof_indx.size() + j] = edge_internal.delta_hat_global(i, j);
                }
            }

            VecSetValues(rhs_global,
                         global_dof_indx.size(),
                         (int*)&global_dof_indx.front(),
                         &rhs_global_container.front(),
                         ADD_VALUES);

            MatSetValues(delta_hat_global,
                         global_dof_indx.size(),
                         (int*)&global_dof_indx.front(),
                         global_dof_indx.size(),
                         (int*)&global_dof_indx.front(),
                         &delta_hat_global_container.front(),
                         ADD_VALUES);

            for (uint bound_id = 0; bound_id < edge_int.interface.data_in.get_nbound(); ++bound_id) {
                if (bound_id == edge_int.interface.bound_id_in)
                    continue;

                auto& boundary_con = edge_int.interface.data_in.boundary[bound_id];

                edge_internal.delta_hat_global =
                    -boundary_in.delta_global * internal_in.delta_local_inv * boundary_con.delta_hat_local;

                std::vector<uint>& global_dof_con_indx = boundary_con.global_dof_indx;

                for (uint i = 0; i < global_dof_indx.size(); ++i) {
                    for (uint j = 0; j < global_dof_con_indx.size(); ++j) {
                        delta_hat_global_container[i * global_dof_indx.size() + j] =
                            edge_internal.delta_hat_global(i, j);
                    }
                }

                MatSetValues(delta_hat_global,
                             global_dof_indx.size(),
                             (int*)&global_dof_indx.front(),
                             global_dof_con_indx.size(),
                             (int*)&global_dof_con_indx.front(),
                             &delta_hat_global_container.front(),
                             ADD_VALUES);
            }

            for (uint bound_id = 0; bound_id < edge_int.interface.data_ex.get_nbound(); ++bound_id) {
                if (bound_id == edge_int.interface.bound_id_ex)
                    continue;

                auto& boundary_con = edge_int.interface.data_ex.boundary[bound_id];

                edge_internal.delta_hat_global =
                    -boundary_ex.delta_global * internal_ex.delta_local_inv * boundary_con.delta_hat_local;

                std::vector<uint>& global_dof_con_indx = boundary_con.global_dof_indx;

                for (uint i = 0; i < global_dof_indx.size(); ++i) {
                    for (uint j = 0; j < global_dof_con_indx.size(); ++j) {
                        delta_hat_global_container[i * global_dof_indx.size() + j] =
                            edge_internal.delta_hat_global(i, j);
                    }
                }

                MatSetValues(delta_hat_global,
                             global_dof_indx.size(),
                             (int*)&global_dof_indx.front(),
                             global_dof_con_indx.size(),
                             (int*)&global_dof_con_indx.front(),
                             &delta_hat_global_container.front(),
                             ADD_VALUES);
            }
        });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeBoundary([&rhs_global,
                                                                                &delta_hat_global](auto& edge_bound) {
            auto& edge_internal = edge_bound.edge_data.edge_internal;

            auto& internal = edge_bound.boundary.data.internal;
            auto& boundary = edge_bound.boundary.data.boundary[edge_bound.boundary.bound_id];

            std::vector<uint>& global_dof_indx = edge_internal.global_dof_indx;

            edge_internal.delta_hat_global -=
                boundary.delta_global * internal.delta_local_inv * boundary.delta_hat_local;

            edge_internal.rhs_global -= boundary.delta_global * internal.delta_local_inv * internal.rhs_local;

            std::vector<double> rhs_global_container(global_dof_indx.size());
            std::vector<double> delta_hat_global_container(global_dof_indx.size() * global_dof_indx.size());

            for (uint i = 0; i < global_dof_indx.size(); ++i) {
                rhs_global_container[i] = edge_internal.rhs_global[i];

                for (uint j = 0; j < global_dof_indx.size(); ++j) {
                    delta_hat_global_container[i * global_dof_indx.size() + j] = edge_internal.delta_hat_global(i, j);
                }
            }

            VecSetValues(rhs_global,
                         global_dof_indx.size(),
                         (int*)&global_dof_indx.front(),
                         &rhs_global_container.front(),
                         ADD_VALUES);

            MatSetValues(delta_hat_global,
                         global_dof_indx.size(),
                         (int*)&global_dof_indx.front(),
                         global_dof_indx.size(),
                         (int*)&global_dof_indx.front(),
                         &delta_hat_global_container.front(),
                         ADD_VALUES);

            for (uint bound_id = 0; bound_id < edge_bound.boundary.data.get_nbound(); ++bound_id) {
                if (bound_id == edge_bound.boundary.bound_id)
                    continue;

                auto& boundary_con = edge_bound.boundary.data.boundary[bound_id];

                edge_internal.delta_hat_global =
                    -boundary.delta_global * internal.delta_local_inv * boundary_con.delta_hat_local;

                std::vector<uint>& global_dof_con_indx = boundary_con.global_dof_indx;

                for (uint i = 0; i < global_dof_indx.size(); ++i) {
                    for (uint j = 0; j < global_dof_con_indx.size(); ++j) {
                        delta_hat_global_container[i * global_dof_indx.size() + j] =
                            edge_internal.delta_hat_global(i, j);
                    }
                }

                MatSetValues(delta_hat_global,
                             global_dof_indx.size(),
                             (int*)&global_dof_indx.front(),
                             global_dof_con_indx.size(),
                             (int*)&global_dof_con_indx.front(),
                             &delta_hat_global_container.front(),
                             ADD_VALUES);
            }
        });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeDistributed([&rhs_global, &delta_hat_global](
                                                                                      auto& edge_dbound) {
            auto& edge_internal = edge_dbound.edge_data.edge_internal;

            auto& internal = edge_dbound.boundary.data.internal;
            auto& boundary = edge_dbound.boundary.data.boundary[edge_dbound.boundary.bound_id];

            std::vector<uint>& global_dof_indx = edge_internal.global_dof_indx;

            edge_internal.delta_hat_global -=
                boundary.delta_global * internal.delta_local_inv * boundary.delta_hat_local;

            edge_internal.rhs_global -= boundary.delta_global * internal.delta_local_inv * internal.rhs_local;

            std::vector<double> rhs_global_container(global_dof_indx.size());
            std::vector<double> delta_hat_global_container(global_dof_indx.size() * global_dof_indx.size());

            for (uint i = 0; i < global_dof_indx.size(); ++i) {
                rhs_global_container[i] = edge_internal.rhs_global[i];

                for (uint j = 0; j < global_dof_indx.size(); ++j) {
                    delta_hat_global_container[i * global_dof_indx.size() + j] = edge_internal.delta_hat_global(i, j);
                }
            }

            VecSetValues(rhs_global,
                         global_dof_indx.size(),
                         (int*)&global_dof_indx.front(),
                         &rhs_global_container.front(),
                         ADD_VALUES);

            MatSetValues(delta_hat_global,
                         global_dof_indx.size(),
                         (int*)&global_dof_indx.front(),
                         global_dof_indx.size(),
                         (int*)&global_dof_indx.front(),
                         &delta_hat_global_container.front(),
                         ADD_VALUES);

            for (uint bound_id = 0; bound_id < edge_dbound.boundary.data.get_nbound(); ++bound_id) {
                if (bound_id == edge_dbound.boundary.bound_id)
                    continue;

                auto& boundary_con = edge_dbound.boundary.data.boundary[bound_id];

                edge_internal.delta_hat_global =
                    -boundary.delta_global * internal.delta_local_inv * boundary_con.delta_hat_local;

                std::vector<uint>& global_dof_con_indx = boundary_con.global_dof_indx;

                for (uint i = 0; i < global_dof_indx.size(); ++i) {
                    for (uint j = 0; j < global_dof_con_indx.size(); ++j) {
                        delta_hat_global_container[i * global_dof_indx.size() + j] =
                            edge_internal.delta_hat_global(i, j);
                    }
                }

                MatSetValues(delta_hat_global,
                             global_dof_indx.size(),
                             (int*)&global_dof_indx.front(),
                             global_dof_con_indx.size(),
                             (int*)&global_dof_con_indx.front(),
                             &delta_hat_global_container.front(),
                             ADD_VALUES);
            }
        });
    }

    MatAssemblyBegin(delta_hat_global, MAT_FINAL_ASSEMBLY);
    MatAssemblyEnd(delta_hat_global, MAT_FINAL_ASSEMBLY);

    VecAssemblyBegin(rhs_global);
    VecAssemblyEnd(rhs_global);

    KSP& ksp            = global_data.ksp;
    Vec& sol            = global_data.sol;
    VecScatter& scatter = global_data.scatter;

    KSPSolve(ksp, rhs_global, rhs_global);

    VecScatterBegin(scatter, rhs_global, sol, INSERT_VALUES, SCATTER_FORWARD);
    VecScatterEnd(scatter, rhs_global, sol, INSERT_VALUES, SCATTER_FORWARD);

    double* sol_ptr;
    VecGetArray(sol, &sol_ptr);

    int sol_size;
    VecGetLocalSize(sol, &sol_size);

    auto solution = vector_from_array(sol_ptr, sol_size);

    uint sol_offset = 0;
    for (uint su_id = 0; su_id < sim_units.size(); ++su_id) {
        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeInterface(
            [&sol_offset, &solution](auto& edge_int) {
                auto& edge_state = edge_int.edge_data.edge_state;

                auto& internal_in = edge_int.interface.data_in.internal;
                auto& internal_ex = edge_int.interface.data_ex.internal;

                auto& boundary_in = edge_int.interface.data_in.boundary[edge_int.interface.bound_id_in];
                auto& boundary_ex = edge_int.interface.data_ex.boundary[edge_int.interface.bound_id_ex];

                uint n_global_dofs = edge_int.edge_data.edge_internal.global_dof_indx.size();

                auto del_q_hat = subvector(solution, sol_offset, n_global_dofs);

                internal_in.rhs_local -= boundary_in.delta_hat_local * del_q_hat;
                internal_ex.rhs_local -= boundary_ex.delta_hat_local * del_q_hat;

                edge_state.q_hat +=
                    reshape<double, SWE::n_variables, SO::ColumnMajor>(del_q_hat, edge_int.edge_data.get_ndof());

                sol_offset += n_global_dofs;
            });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeBoundary(
            [&sol_offset, &solution](auto& edge_bound) {
                auto& edge_state = edge_bound.edge_data.edge_state;

                auto& internal = edge_bound.boundary.data.internal;
                auto& boundary = edge_bound.boundary.data.boundary[edge_bound.boundary.bound_id];

                uint n_global_dofs = edge_bound.edge_data.edge_internal.global_dof_indx.size();

                auto del_q_hat = subvector(solution, sol_offset, n_global_dofs);

                internal.rhs_local -= boundary.delta_hat_local * del_q_hat;

                edge_state.q_hat +=
                    reshape<double, SWE::n_variables, SO::ColumnMajor>(del_q_hat, edge_bound.edge_data.get_ndof());

                sol_offset += n_global_dofs;
            });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeDistributed(
            [&sol_offset, &solution](auto& edge_dbound) {
                auto& edge_state = edge_dbound.edge_data.edge_state;

                auto& internal = edge_dbound.boundary.data.internal;
                auto& boundary = edge_dbound.boundary.data.boundary[edge_dbound.boundary.bound_id];

                uint n_global_dofs = edge_dbound.edge_data.edge_internal.global_dof_indx.size();

                auto del_q_hat = subvector(solution, sol_offset, n_global_dofs);

                internal.rhs_local -= boundary.delta_hat_local * del_q_hat;

                edge_state.q_hat +=
                    reshape<double, SWE::n_variables, SO::ColumnMajor>(del_q_hat, edge_dbound.edge_data.get_ndof());

                sol_offset += n_global_dofs;
            });

        sim_units[su_id]->discretization.mesh.CallForEachElement([&sim_units, su_id](auto& elt) {
            const uint stage = sim_units[su_id]->stepper.GetStage();

            auto& state = elt.data.state[stage + 1];

            auto& internal = elt.data.internal;

            internal.rhs_local = internal.delta_local_inv * internal.rhs_local;

            state.q += reshape<double, SWE::n_variables, SO::ColumnMajor>(internal.rhs_local, elt.data.get_ndof());
        });
    }

    VecRestoreArray(sol, &sol_ptr);

    int size;
    double delta_norm;

    VecNorm(rhs_global, NORM_2, &delta_norm);
    VecGetSize(rhs_global, &size);

    delta_norm /= size;

    MatZeroEntries(delta_hat_global);
    VecZeroEntries(rhs_global);

    if (delta_norm < 1e-8) {
        return true;
    }

    return false;
}
}
}

#endif