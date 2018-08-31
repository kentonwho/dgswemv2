#ifndef IHDG_GN_PROC_HPX_STAGE_HPP
#define IHDG_GN_PROC_HPX_STAGE_HPP

#include "general_definitions.hpp"

#include "ihdg_gn_kernels_processor.hpp"

namespace GN {
namespace IHDG {
template <typename HPXSimUnitType>
decltype(auto) Problem::hpx_stage_kernel(HPXSimUnitType* sim_unit) {
    if (sim_unit->writer.WritingVerboseLog()) {
        sim_unit->writer.GetLogFile() << "Current (time, stage): (" << sim_unit->stepper.GetTimeAtCurrentStage() << ','
                                      << sim_unit->stepper.GetStage() << ')' << std::endl;

        sim_unit->writer.GetLogFile() << "Exchanging data" << std::endl;
    }

    hpx::future<void> receive_future = sim_unit->communicator.ReceiveAll(sim_unit->stepper.GetTimestamp());

    sim_unit->discretization.mesh.CallForEachDistributedBoundary(
        [sim_unit](auto& dbound) { Problem::global_swe_distributed_boundary_kernel(sim_unit->stepper, dbound); });

    sim_unit->communicator.SendAll(sim_unit->stepper.GetTimestamp());

    if (sim_unit->writer.WritingVerboseLog()) {
        sim_unit->writer.GetLogFile() << "Starting work before receive" << std::endl;
    }

    if (sim_unit->parser.ParsingInput()) {
        if (sim_unit->writer.WritingVerboseLog()) {
            sim_unit->writer.GetLogFile() << "Parsing input" << std::endl;
        }

        sim_unit->parser.ParseInput(sim_unit->stepper, sim_unit->discretization.mesh);
    }

    /* Global Pre Receive Step */
    sim_unit->discretization.mesh.CallForEachInterface(
        [sim_unit](auto& intface) { Problem::global_swe_interface_kernel(sim_unit->stepper, intface); });

    sim_unit->discretization.mesh.CallForEachBoundary(
        [sim_unit](auto& bound) { Problem::global_swe_boundary_kernel(sim_unit->stepper, bound); });

    sim_unit->discretization.mesh_skeleton.CallForEachEdgeInterface(
        [sim_unit](auto& edge_int) { Problem::global_swe_edge_interface_kernel(sim_unit->stepper, edge_int); });

    sim_unit->discretization.mesh_skeleton.CallForEachEdgeBoundary(
        [sim_unit](auto& edge_bound) { Problem::global_swe_edge_boundary_kernel(sim_unit->stepper, edge_bound); });
    /* Global Pre Receive Step */

    /* Local Pre Receive Step */
    sim_unit->discretization.mesh.CallForEachElement(
        [sim_unit](auto& elt) { Problem::local_swe_volume_kernel(sim_unit->stepper, elt); });

    sim_unit->discretization.mesh.CallForEachElement(
        [sim_unit](auto& elt) { Problem::local_swe_source_kernel(sim_unit->stepper, elt); });

    sim_unit->discretization.mesh.CallForEachInterface(
        [sim_unit](auto& intface) { Problem::local_swe_interface_kernel(sim_unit->stepper, intface); });

    sim_unit->discretization.mesh.CallForEachBoundary(
        [sim_unit](auto& bound) { Problem::local_swe_boundary_kernel(sim_unit->stepper, bound); });
    /* Local Pre Receive Step */

    if (sim_unit->writer.WritingVerboseLog()) {
        sim_unit->writer.GetLogFile() << "Finished work before receive" << std::endl
                                      << "Starting to wait on receive with timestamp: "
                                      << sim_unit->stepper.GetTimestamp() << std::endl;
    }

    return receive_future.then([sim_unit](auto&&) {
        if (sim_unit->writer.WritingVerboseLog()) {
            sim_unit->writer.GetLogFile() << "Starting work after receive" << std::endl;
        }

        /* Global Post Receive Step */
        sim_unit->discretization.mesh_skeleton.CallForEachEdgeDistributed([sim_unit](auto& edge_dbound) {
            Problem::global_swe_edge_distributed_kernel(sim_unit->stepper, edge_dbound);
        });
        /* Global Post Receive Step */

        /* Local Post Receive Step */
        sim_unit->discretization.mesh.CallForEachDistributedBoundary(
            [sim_unit](auto& dbound) { Problem::local_swe_distributed_boundary_kernel(sim_unit->stepper, dbound); });

        sim_unit->discretization.mesh.CallForEachElement(
            [sim_unit](auto& elt) { Problem::update_kernel(sim_unit->stepper, elt); });

        sim_unit->discretization.mesh.CallForEachElement([sim_unit](auto& elt) {
            bool nan_found = Problem::scrutinize_solution_kernel(sim_unit->stepper, elt);

            if (nan_found)
                hpx::terminate();
        });
        /* Local Post Receive Step */

        ++(sim_unit->stepper);

        if (sim_unit->writer.WritingVerboseLog()) {
            sim_unit->writer.GetLogFile() << "Finished work after receive" << std::endl << std::endl;
        }
    });
}
}
}

#endif