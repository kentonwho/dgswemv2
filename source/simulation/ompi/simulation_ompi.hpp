#ifndef SIMULATION_OMPI_HPP
#define SIMULATION_OMPI_HPP

#include <omp.h>

#include "general_definitions.hpp"
#include "preprocessor/input_parameters.hpp"
#include "utilities/file_exists.hpp"
#include "sim_unit_ompi.hpp"

template <typename ProblemType>
class OMPISimulation : public OMPISimulationBase {
    /* Need to find a way to make this private */
    /* Cannot make it private for GC problem */
  public:
    MPI_Comm global_comm;

    std::vector<std::unique_ptr<OMPISimulationUnit<ProblemType>>> sim_units;

  private:
    uint n_steps;

  public:
    OMPISimulation() = default;
    OMPISimulation(const std::string& input_string);

    void Run();
    void ComputeL2Residual();
    void Finalize();

  private:
    friend ProblemType;
};

template <typename ProblemType>
OMPISimulation<ProblemType>::OMPISimulation(const std::string& input_string) {
    int locality_id;
    MPI_Comm_rank(MPI_COMM_WORLD, &locality_id);

    InputParameters<typename ProblemType::ProblemInputType> input(input_string);

    this->n_steps = (uint)std::ceil(input.stepper_input.run_time / input.stepper_input.dt);

    std::string submesh_file_prefix =
        input.mesh_input.mesh_file_name.substr(0, input.mesh_input.mesh_file_name.find_last_of('.')) + "_" +
        std::to_string(locality_id) + '_';
    std::string submesh_file_postfix = input.mesh_input.mesh_file_name.substr(
        input.mesh_input.mesh_file_name.find_last_of('.'), input.mesh_input.mesh_file_name.size());

    uint submesh_id = 0;

    while (Utilities::file_exists(submesh_file_prefix + std::to_string(submesh_id) + submesh_file_postfix)) {
        this->sim_units.emplace_back(new OMPISimulationUnit<ProblemType>(input_string, locality_id, submesh_id));

        ++submesh_id;
    }

    if (this->sim_units.empty()) {
        std::cerr << "Warning: MPI Rank " << locality_id << " has not been assigned any work. This may inidicate\n"
                  << "         poor partitioning and imply degraded performance." << std::endl;

        // Split into comm world that has partitions to work on
        MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, locality_id, &(this->global_comm));
    } else {
        // Split into comm world that has partitions to work on
        MPI_Comm_split(MPI_COMM_WORLD, 0, locality_id, &(this->global_comm));
    }
}

template <typename ProblemType>
void OMPISimulation<ProblemType>::Run() {
    if (this->sim_units.empty()) {
        return;
    }

#pragma omp parallel
    {
        uint n_threads, thread_id, sim_per_thread, begin_sim_id, end_sim_id;

        n_threads = (uint)omp_get_num_threads();
        thread_id = (uint)omp_get_thread_num();

        sim_per_thread = (this->sim_units.size() + n_threads - 1) / n_threads;

        begin_sim_id = sim_per_thread * thread_id;
        end_sim_id   = std::min(sim_per_thread * (thread_id + 1), (uint)this->sim_units.size());

        ProblemType::preprocessor_ompi(this, begin_sim_id, end_sim_id);

        for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
            if (this->sim_units[su_id]->writer.WritingLog()) {
                this->sim_units[su_id]->writer.GetLogFile() << std::endl
                                                            << "Launching Simulation!" << std::endl
                                                            << std::endl;
            }

            if (this->sim_units[su_id]->writer.WritingOutput()) {
                this->sim_units[su_id]->writer.WriteFirstStep(this->sim_units[su_id]->stepper,
                                                              this->sim_units[su_id]->discretization.mesh);
            }

            uint n_stages = this->sim_units[su_id]->stepper.GetNumStages();

            this->sim_units[su_id]->discretization.mesh.CallForEachElement(
                [n_stages](auto& elt) { elt.data.resize(n_stages + 1); });
        }

        for (uint step = 1; step <= this->n_steps; ++step) {
            ProblemType::step_ompi(this, begin_sim_id, end_sim_id);
        }
    }  // close omp parallel region
}

template <typename ProblemType>
void OMPISimulation<ProblemType>::ComputeL2Residual() {
    if (this->sim_units.empty()) {
        return;
    }

    int locality_id;
    MPI_Comm_rank(this->global_comm, &locality_id);

    double global_l2{0};
    double residual_l2{0};

    for (auto& sim_unit : this->sim_units) {
        sim_unit->discretization.mesh.CallForEachElement([&sim_unit, &residual_l2](auto& elt) {
            residual_l2 += ProblemType::compute_residual_L2(sim_unit->stepper, elt);
        });
    }

    MPI_Reduce(&residual_l2, &global_l2, 1, MPI_DOUBLE, MPI_SUM, 0, this->global_comm);

    if (locality_id == 0) {
        std::cout << "L2 error: " << std::setprecision(15) << std::sqrt(global_l2) << std::endl;
    }
}

template <typename ProblemType>
void OMPISimulation<ProblemType>::Finalize() {
    if (this->sim_units.empty()) {
        return;
    }

    ProblemType::finalize_simulation(this);
}

#endif