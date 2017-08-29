#ifndef MESH_HPP
#define MESH_HPP

#include "../utilities/heterogeneous_containers.hpp"

#include "mesh_utilities.hpp"

namespace Geometry {
// Since elements types already come in a tuple. We can use specialization
// to get easy access to the parameter packs for the element and edge types.
template <typename ElementTypeTuple,
          typename InterfaceTypeTuple,
          typename BoundaryTypeTuple,
          typename DistributedBoundaryTypeTuple>
class Mesh;

template <typename... Elements, typename... Interfaces, typename... Boundaries, typename... DistributedBoundaries>
class Mesh<std::tuple<Elements...>,
           std::tuple<Interfaces...>,
           std::tuple<Boundaries...>,
           std::tuple<DistributedBoundaries...>> {
  private:
    std::string mesh_name;

    using MasterElementTypes = typename make_master_type<std::tuple<Elements...>>::type;
    using ElementContainer = Utilities::HeterogeneousMap<Elements...>;
    using InterfaceContainer = Utilities::HeterogeneousVector<Interfaces...>;
    using BoundaryContainer = Utilities::HeterogeneousVector<Boundaries...>;
    using DistributedBoundariesContainer = Utilities::HeterogeneousVector<DistributedBoundaries...>;

    MasterElementTypes masters;
    ElementContainer elements;
    InterfaceContainer interfaces;
    BoundaryContainer boundaries;
    DistributedBoundariesContainer distributed_boundaries;

  public:
    Mesh(uint p) : masters(master_maker<MasterElementTypes>::construct_masters(p)) {}

    void SetMeshName(const std::string& mesh_name) { this->mesh_name = mesh_name; }
    std::string GetMeshName() { return this->mesh_name; }

    uint GetNumberElements() { return this->elements.size(); }
    uint GetNumberInterfaces() { return this->interfaces.size(); }
    uint GetNumberBoundaries() { return this->boundaries.size(); }
    uint GetNumberDistributedBoundaries() { return this->distributed_boundaries.size(); }

    template <typename Element, typename... Args>
    void CreateElement(uint n, Args&&... args) {
        using MasterType = typename Element::ElementMasterType;

        MasterType& master_elt = std::get<Utilities::index<MasterType, MasterElementTypes>::value>(masters);

        this->elements.template emplace<Element>(n, Element(n, master_elt, std::forward<Args>(args)...));
    }

    template <typename Interface, typename... Args>
    void CreateInterface(Args&&... args) {
        this->interfaces.template emplace_back<Interface>(std::forward<Args>(args)...);
    }

    template <typename Boundary, typename... Args>
    void CreateBoundary(Args&&... args) {
        this->boundaries.template emplace_back<Boundary>(std::forward<Args>(args)...);
    }

    template <typename DistributedBoundary, typename... Args>
    void CreateDistributedBoundary(Args&&... args) {
        this->distributed_boundaries.template emplace_back<DistributedBoundary>(std::forward<Args>(args)...);
    }

    template <typename F>
    void CallForEachElement(const F& f) {
        Utilities::for_each_in_tuple(elements.data, [&f](auto& element_map) {
            std::for_each(element_map.begin(), element_map.end(), [&f](auto& pair) { f(pair.second); });
        });
    }

    template <typename F>
    void CallForEachInterface(const F& f) {
        Utilities::for_each_in_tuple(interfaces.data, [&f](auto& interface_vector) {
            std::for_each(interface_vector.begin(), interface_vector.end(), f);
        });
    }

    template <typename F>
    void CallForEachBoundary(const F& f) {
        Utilities::for_each_in_tuple(boundaries.data, [&f](auto& boundary_vector) {
            std::for_each(boundary_vector.begin(), boundary_vector.end(), f);
        });
    }

    template <typename F>
    void CallForEachDistributedBoundary(const F& f) {
        Utilities::for_each_in_tuple(distributed_boundaries.data, [&f](auto& distributed_boundaries_vector) {
            std::for_each(distributed_boundaries_vector.begin(), distributed_boundaries_vector.end(), f);
        });
    }
};
}

#endif