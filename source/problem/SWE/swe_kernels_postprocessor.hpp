#ifndef SWE_KERNELS_POSTPROCESSOR_HPP
#define SWE_KERNELS_POSTPROCESSOR_HPP

namespace SWE {
template <typename ElementType>
void Problem::extract_VTK_data_kernel(ElementType& elt, Array2D<double>& cell_data, Array2D<double>& point_data) {
    elt.WriteCellDataVTK(elt.data.state[0].ze, cell_data[0]);
    elt.WriteCellDataVTK(elt.data.state[0].qx, cell_data[1]);
    elt.WriteCellDataVTK(elt.data.state[0].qy, cell_data[2]);
    elt.WriteCellDataVTK(elt.data.state[0].bath, cell_data[3]);

    elt.WritePointDataVTK(elt.data.state[0].ze, point_data[0]);
    elt.WritePointDataVTK(elt.data.state[0].qx, point_data[1]);
    elt.WritePointDataVTK(elt.data.state[0].qy, point_data[2]);
    elt.WritePointDataVTK(elt.data.state[0].bath, point_data[3]);
}

template <typename MeshType>
void Problem::write_VTK_data_kernel(MeshType& mesh, std::ofstream& raw_data_file) {
    Array2D<double> cell_data;
    Array2D<double> point_data;

    cell_data.resize(4);
    point_data.resize(4);

    auto extract_VTK_data_kernel = [&cell_data, &point_data](auto& elt) {
        Problem::extract_VTK_data_kernel(elt, cell_data, point_data);
    };

    mesh.CallForEachElement(extract_VTK_data_kernel);

    raw_data_file << "CELL_DATA " << (*cell_data.begin()).size() << '\n';
    raw_data_file << "SCALARS ze_cell float 1\n";
    raw_data_file << "LOOKUP_TABLE default\n";
    for (auto it = cell_data[0].begin(); it != cell_data[0].end(); it++)
        raw_data_file << *it << '\n';

    raw_data_file << "SCALARS qx_cell float 1\n";
    raw_data_file << "LOOKUP_TABLE default\n";
    for (auto it = cell_data[1].begin(); it != cell_data[1].end(); it++)
        raw_data_file << *it << '\n';

    raw_data_file << "SCALARS qy_cell float 1\n";
    raw_data_file << "LOOKUP_TABLE default\n";
    for (auto it = cell_data[2].begin(); it != cell_data[2].end(); it++)
        raw_data_file << *it << '\n';

    raw_data_file << "SCALARS bath_cell float 1\n";
    raw_data_file << "LOOKUP_TABLE default\n";
    for (auto it = cell_data[3].begin(); it != cell_data[3].end(); it++)
        raw_data_file << *it << '\n';

    raw_data_file << "POINT_DATA " << (*point_data.begin()).size() << '\n';
    raw_data_file << "SCALARS ze_point float 1\n";
    raw_data_file << "LOOKUP_TABLE default\n";
    for (auto it = point_data[0].begin(); it != point_data[0].end(); it++)
        raw_data_file << *it << '\n';

    raw_data_file << "SCALARS qx_point float 1\n";
    raw_data_file << "LOOKUP_TABLE default\n";
    for (auto it = point_data[1].begin(); it != point_data[1].end(); it++)
        raw_data_file << *it << '\n';

    raw_data_file << "SCALARS qy_point float 1\n";
    raw_data_file << "LOOKUP_TABLE default\n";
    for (auto it = point_data[2].begin(); it != point_data[2].end(); it++)
        raw_data_file << *it << '\n';

    raw_data_file << "SCALARS bath_point float 1\n";
    raw_data_file << "LOOKUP_TABLE default\n";
    for (auto it = point_data[3].begin(); it != point_data[3].end(); it++)
        raw_data_file << *it << '\n';
}

template <typename ElementType>
void Problem::extract_modal_data_kernel(ElementType& elt, std::vector<std::pair<uint, Array2D<double>>>& modal_data) {
    modal_data.push_back(std::make_pair(elt.GetID(), Array2D<double>{elt.data.state[0].ze, elt.data.state[0].qx,
                                                                     elt.data.state[0].qy, elt.data.state[0].bath}));
}

template <typename MeshType>
void Problem::write_modal_data_kernel(const Stepper& stepper, MeshType& mesh, const std::string& output_path) {
    std::vector<std::pair<uint, Array2D<double>>> modal_data;

    auto extract_modal_data_kernel = [&modal_data](auto& elt) { Problem::extract_modal_data_kernel(elt, modal_data); };

    mesh.CallForEachElement(extract_modal_data_kernel);

    std::ofstream file;

    std::string file_name = output_path + mesh.GetMeshName() + "_modal_ze.txt";
    if (stepper.get_step() == 0) {
        file = std::ofstream(file_name);
    } else {
        file = std::ofstream(file_name, std::ios::app);
    }

    file << std::to_string(stepper.get_t_at_curr_stage()) << '\n';
    for (auto it = modal_data.begin(); it != modal_data.end(); it++) {
        for (auto itt = (*it).second[0].begin(); itt != (*it).second[0].end(); itt++) {
            file << (*it).first << ' ' << std::scientific << (*itt) << '\n';
        }
    }

    file.close();

    file_name = output_path + mesh.GetMeshName() + "_modal_qx.txt";
    if (stepper.get_step() == 0) {
        file = std::ofstream(file_name);
    } else {
        file = std::ofstream(file_name, std::ios::app);
    }

    file << std::to_string(stepper.get_t_at_curr_stage()) << '\n';
    for (auto it = modal_data.begin(); it != modal_data.end(); it++) {
        for (auto itt = (*it).second[1].begin(); itt != (*it).second[1].end(); itt++) {
            file << (*it).first << ' ' << std::scientific << (*itt) << '\n';
        }
    }

    file.close();

    file_name = output_path + mesh.GetMeshName() + "_modal_qy.txt";
    if (stepper.get_step() == 0) {
        file = std::ofstream(file_name);
    } else {
        file = std::ofstream(file_name, std::ios::app);
    }

    file << std::to_string(stepper.get_t_at_curr_stage()) << '\n';
    for (auto it = modal_data.begin(); it != modal_data.end(); it++) {
        for (auto itt = (*it).second[2].begin(); itt != (*it).second[2].end(); itt++) {
            file << (*it).first << ' ' << std::scientific << (*itt) << '\n';
        }
    }

    file.close();

    file_name = output_path + mesh.GetMeshName() + "_modal_bath.txt";
    if (stepper.get_step() == 0) {
        file = std::ofstream(file_name);
    } else {
        file = std::ofstream(file_name, std::ios::app);
    }

    file << std::to_string(stepper.get_t_at_curr_stage()) << '\n';
    for (auto it = modal_data.begin(); it != modal_data.end(); it++) {
        for (auto itt = (*it).second[3].begin(); itt != (*it).second[3].end(); itt++) {
            file << (*it).first << ' ' << std::scientific << (*itt) << '\n';
        }
    }

    file.close();
}

template <typename ElementType>
double Problem::compute_residual_L2_kernel(const Stepper& stepper, ElementType& elt) {
    double t = stepper.get_t_at_curr_stage();

    auto true_ze = [t](Point<2>& pt) { return SWE::true_ze(t, pt); };

    return elt.ComputeResidualL2(true_ze, elt.data.state[0].ze);
}
}

#endif
