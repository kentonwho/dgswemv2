#ifndef EHDG_GN_PROC_OMPI_STEP_HPP
#define EHDG_GN_PROC_OMPI_STEP_HPP

#include "ehdg_gn_kernels_processor.hpp"
#include "ehdg_gn_proc_ompi_sol_glob_prob.hpp"
#include "ehdg_gn_proc_derivatives_ompi.hpp"

namespace GN {
namespace EHDG {
template <typename OMPISimType>
void Problem::step_ompi(OMPISimType* sim, uint begin_sim_id, uint end_sim_id) {
    auto& sim_units = sim->sim_units;

    // Here one assumes that there is at lease one sim unit present
    // This is of course not always true
    for (uint stage = 0; stage < sim_units[0]->stepper.GetNumStages(); ++stage) {
        for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
            if (sim_units[su_id]->parser.ParsingInput()) {
                sim_units[su_id]->parser.ParseInput(sim_units[su_id]->stepper, sim_units[su_id]->discretization.mesh);
            }
        }

        SWE_SIM::Problem::stage_ompi(sim, begin_sim_id, end_sim_id);
    }

    // Here one assumes that there is at lease one sim unit present
    // This is of course not always true
    for (uint stage = 0; stage < sim_units[0]->stepper.GetNumStages(); ++stage) {
        for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
            if (sim_units[su_id]->parser.ParsingInput()) {
                sim_units[su_id]->parser.ParseInput(sim_units[su_id]->stepper, sim_units[su_id]->discretization.mesh);
            }
        }

        Problem::dispersive_correction_ompi(sim, begin_sim_id, end_sim_id);
    }

    // Here one assumes that there is at lease one sim unit present
    // This is of course not always true
    for (uint stage = 0; stage < sim_units[0]->stepper.GetNumStages(); ++stage) {
        for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
            if (sim_units[su_id]->parser.ParsingInput()) {
                sim_units[su_id]->parser.ParseInput(sim_units[su_id]->stepper, sim_units[su_id]->discretization.mesh);
            }
        }

        SWE_SIM::Problem::stage_ompi(sim, begin_sim_id, end_sim_id);
    }

    for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
        if (sim_units[su_id]->writer.WritingOutput()) {
            sim_units[su_id]->writer.WriteOutput(sim_units[su_id]->stepper, sim_units[su_id]->discretization.mesh);
        }
    }
}

template <typename OMPISimType>
void Problem::dispersive_correction_ompi(OMPISimType* sim, uint begin_sim_id, uint end_sim_id) {
    auto& sim_units = sim->sim_units;

    Problem::compute_derivatives_ompi(sim, begin_sim_id, end_sim_id);

    for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
        sim_units[su_id]->discretization.mesh.CallForEachElement(
            [&sim_units, su_id](auto& elt) { Problem::local_dc_volume_kernel(sim_units[su_id]->stepper, elt); });

        sim_units[su_id]->discretization.mesh.CallForEachElement(
            [&sim_units, su_id](auto& elt) { Problem::local_dc_source_kernel(sim_units[su_id]->stepper, elt); });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeInterface([&sim_units, su_id](auto& edge_int) {
            Problem::local_dc_edge_interface_kernel(sim_units[su_id]->stepper, edge_int);
        });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeBoundary([&sim_units, su_id](auto& edge_bound) {
            Problem::local_dc_edge_boundary_kernel(sim_units[su_id]->stepper, edge_bound);
        });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeDistributed(
            [&sim_units, su_id](auto& edge_dbound) {
                Problem::local_dc_edge_distributed_kernel(sim_units[su_id]->stepper, edge_dbound);
            });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeInterface([&sim_units, su_id](auto& edge_int) {
            Problem::global_dc_edge_interface_kernel(sim_units[su_id]->stepper, edge_int);
        });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeBoundary([&sim_units, su_id](auto& edge_bound) {
            Problem::global_dc_edge_boundary_kernel(sim_units[su_id]->stepper, edge_bound);
        });

        sim_units[su_id]->discretization.mesh_skeleton.CallForEachEdgeDistributed(
            [&sim_units, su_id](auto& edge_dbound) {
                Problem::global_dc_edge_distributed_kernel(sim_units[su_id]->stepper, edge_dbound);
            });
    }

    Problem::ompi_solve_global_dc_problem(sim, begin_sim_id, end_sim_id);

    for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
        sim_units[su_id]->discretization.mesh.CallForEachElement([&sim_units, su_id](auto& elt) {
            Problem::dispersive_correction_kernel(sim_units[su_id]->stepper, elt);

            auto& state = elt.data.state[sim_units[su_id]->stepper.GetStage()];

            state.solution = elt.ApplyMinv(state.rhs);

            sim_units[su_id]->stepper.UpdateState(elt);
        });

        ++(sim_units[su_id]->stepper);
    }
}
}
}

#endif