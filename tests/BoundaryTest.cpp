
#define CATCH_CONFIG_MAIN

#include <catch.hpp>

#include "mesh/DofAllocator.hpp"

#include "interpolations/Triangle3.hpp"
#include "quadrature/TriangleQuadrature.hpp"

#include "mesh/SubMesh.hpp"

#include "mesh/solid/boundary/Boundary.hpp"
#include "mesh/solid/boundary/NonFollowerLoad.hpp"

#include <json/json.h>
#include <range/v3/view.hpp>

#include <memory>

#include "CubeJson.hpp"

using namespace neon;
using namespace ranges;

TEST_CASE("Dof List Allocation", "[DofAllocator]")
{
    SECTION("One element")
    {
        std::vector<List> const nodal_connectivity = {{0, 1, 2, 3}};

        std::vector<List> const known_dof_list{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};

        std::vector<List> const computed_list = allocate_dof_list(3, nodal_connectivity);

        REQUIRE(view::set_difference(computed_list.at(0), known_dof_list.at(0)).empty());
    }
    SECTION("Two elements")
    {
        std::vector<List> const nodal_connectivity = {{0, 1, 2, 3}, {4, 2, 1, 5}};

        std::vector<List> const known_dof_list{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
                                               {12, 13, 14, 6, 7, 8, 3, 4, 5, 15, 16, 17}};

        std::vector<List> const computed_list = allocate_dof_list(3, nodal_connectivity);

        REQUIRE(view::set_difference(computed_list.at(0), known_dof_list.at(0)).empty());
        REQUIRE(view::set_difference(computed_list.at(1), known_dof_list.at(1)).empty());
    }
}
TEST_CASE("Dof List Filter", "[DofAllocator]")
{
    SECTION("One element 0 offset")
    {
        std::vector<List> const nodal_connectivity = {{0, 1, 2, 3}};

        std::vector<List> const known_dof_list{{0, 3, 6, 9}};

        std::vector<List> const computed_list = filter_dof_list(3, 0, nodal_connectivity);

        REQUIRE(view::set_difference(computed_list.at(0), known_dof_list.at(0)).empty());
    }
    SECTION("One element 1 offset")
    {
        std::vector<List> const nodal_connectivity = {{0, 1, 2, 3}};

        std::vector<List> const known_dof_list{{1, 4, 7, 10}};

        std::vector<List> const computed_list = filter_dof_list(3, 1, nodal_connectivity);

        REQUIRE(view::set_difference(computed_list.at(0), known_dof_list.at(0)).empty());
    }
}
TEST_CASE("Boundary unit test", "[Boundary]")
{
    SECTION("Ramped load")
    {
        Boundary boundary(2.0, true);
        REQUIRE(boundary.interpolate_prescribed_value(0.5) == Approx(1.0));
        REQUIRE(boundary.interpolate_prescribed_value(1.0) == Approx(2.0));
    }
    SECTION("Instantaneous load")
    {
        Boundary boundary(2.0, false);
        REQUIRE(boundary.interpolate_prescribed_value(0.5) == Approx(2.0));
        REQUIRE(boundary.interpolate_prescribed_value(1.0) == Approx(2.0));
    }
}
TEST_CASE("Traction test for triangle", "[Traction]")
{
    using namespace neon::solid;

    // Build a right angled triangle
    Vector coordinates(9);
    coordinates << 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0;

    auto material_coordinates = std::make_shared<MaterialCoordinates>(coordinates);

    std::vector<List> nodal_connectivity = {{0, 1, 2}};
    std::vector<List> dof_list = {{0, 3, 6}};

    SECTION("Unit load")
    {
        Traction traction(std::make_unique<Triangle3>(TriangleQuadrature::Rule::OnePoint),
                          nodal_connectivity,
                          material_coordinates,
                          1.0,  // Prescribed load
                          true, // Is load ramped
                          0     // Dof offset
        );

        REQUIRE(traction.elements() == 1);

        auto const & [ dofs, t ] = traction.external_force(0, 1.0);

        REQUIRE((t - 1.0 / 6.0 * Vector3::Ones()).norm() == Approx(0.0));
        REQUIRE(view::set_difference(dof_list.at(0), dofs).empty());
    }
    SECTION("Twice unit load")
    {
        Traction traction(std::make_unique<Triangle3>(TriangleQuadrature::Rule::OnePoint),
                          nodal_connectivity,
                          material_coordinates,
                          2.0,  // Prescribed load
                          true, // Is load ramped
                          0     // Dof offset
        );

        REQUIRE(traction.elements() == 1);

        auto const & [ dofs, t ] = traction.external_force(0, 1.0);

        REQUIRE((t - 2.0 / 6.0 * Vector3::Ones()).norm() == Approx(0.0));
        REQUIRE(view::set_difference(dof_list.at(0), dofs).empty());
    }
}
TEST_CASE("Traction test for mixed mesh", "[NonFollowerLoadBoundary]")
{
    // Test the construction and population of a mixed quadrilateral and
    // triangle mesh
    using namespace neon::solid;

    Vector coordinates(3 * 5);
    coordinates << 0.0, 0.0, 0.0, //
        1.0, 0.0, 0.0,            //
        1.0, 1.0, 0.0,            //
        0.0, 1.0, 0.0,            //
        2.0, 1.0, 0.0;

    auto material_coordinates = std::make_shared<MaterialCoordinates>(coordinates);

    std::string trimesh = "{\"Name\" : \"Ysym\", \"NodalConnectivity\" : [ [ 1, 4, 2 "
                          "] ], \"Type\" : 2 }";
    std::string quadmesh = "{\"Name\" : \"Ysym\", \"NodalConnectivity\" : [ [ 0, 1, 2, 3 "
                           "] ], \"Type\" : 3 }";

    std::array<int, 3> const known_dofs_tri{{4, 13, 7}};
    std::array<int, 4> const known_dofs_quad{{1, 4, 7, 10}};

    Json::Value tri_mesh_data, quad_mesh_data, simulation_data;
    Json::Reader tri_mesh_file, quad_mesh_file, simulation_file;

    REQUIRE(tri_mesh_file.parse(trimesh.c_str(), tri_mesh_data));
    REQUIRE(quad_mesh_file.parse(quadmesh.c_str(), quad_mesh_data));
    REQUIRE(simulation_file.parse(simulation_data_traction_json().c_str(), simulation_data));

    std::vector<SubMesh> submeshes = {tri_mesh_data, quad_mesh_data};

    REQUIRE(submeshes.at(0).elements() == 1);
    REQUIRE(submeshes.at(1).elements() == 1);

    REQUIRE(submeshes.at(0).topology() == ElementTopology::Triangle3);
    REQUIRE(submeshes.at(1).topology() == ElementTopology::Quadrilateral4);

    REQUIRE(submeshes.at(0).nodes_per_element() == 3);
    REQUIRE(submeshes.at(1).nodes_per_element() == 4);

    // Insert this information into the nonfollower load boundary class
    // using the simulation data for the cube
    NonFollowerLoadBoundary nf_loads(material_coordinates,
                                     submeshes,
                                     1,
                                     1.0e-3,
                                     simulation_data);

    auto const & [ dofs_tri, f_tri ] = nf_loads.boundaries().at(0).external_force(0, 1.0);
    auto const & [ dofs_quad, f_quad ] = nf_loads.boundaries().at(1).external_force(0, 1.0);

    // Check sizes
    REQUIRE(dofs_tri.size() == 3);
    REQUIRE(f_tri.rows() == 3);

    REQUIRE(dofs_quad.size() == 4);
    REQUIRE(f_quad.rows() == 4);

    // Check dofs are correctly written
    REQUIRE(view::set_difference(dofs_tri, known_dofs_tri).empty());
    REQUIRE(view::set_difference(dofs_quad, known_dofs_quad).empty());
}