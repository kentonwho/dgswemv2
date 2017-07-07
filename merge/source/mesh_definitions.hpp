#ifndef MESH_DEFINITIONS_H
#define MESH_DEFINITIONS_H

#include "general_definitions.hpp"

#include "element.hpp"
#include "master/master_elements_2D.hpp"
#include "shape/shapes_2D.hpp"

#include "basis/bases_2D.hpp"

#include "integration/integrations_1D.hpp"
#include "integration/integrations_2D.hpp"

#include "boundary.hpp"

namespace Geometry {
	template<typename Data>
	using ElementTypeTuple = std::tuple<
		Element<2, Master::Triangle<Basis::Dubiner_2D, Integration::Dunavant_2D>, Shape::StraightTriangle, Data>
	>;

	template<typename Data>
	using InterfaceTypeTuple = std::tuple<
		Interface<1, Integration::GaussLegendre_1D, Data>
	>;

	template<typename Data>
	using BoundaryTypeTuple = std::tuple<
		Boundary<1, Integration::GaussLegendre_1D, Data, SWE::Land>,
		Boundary<1, Integration::GaussLegendre_1D, Data, SWE::Tide>
	>;

	template<typename Data>
	using MeshType = Mesh<ElementTypeTuple<Data>, InterfaceTypeTuple<Data>, BoundaryTypeTuple<Data>>;
};

#endif