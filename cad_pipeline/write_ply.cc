#include "write_ply.h"

#include <iostream>
#include <print>

#include "igl/remesh_along_isoline.h"
#include "igl/writePLY.h"
#include "pmp/algorithms/normals.h"
#include "pmp/algorithms/numerics.h"
#include "types.h"
#include "visit_helper.h"

AnyGeometry WritePLY(const AnyGeometry &geometry) {
  std::print("WritePLY(");
  std::visit(overloaded{
                 [](const std::shared_ptr<BRep> &brep) { std::print("BRep"); },
                 [](const std::shared_ptr<Mesh> &mesh) {
                   std::print("Mesh");
                   pmp::SurfaceMesh local_mesh = *mesh;

                   pmp::vertex_normals(local_mesh);
                   Eigen::MatrixXd V;
                   Eigen::MatrixXi F;
                   Eigen::MatrixXd N;
                   Eigen::MatrixXd VD;
                   Eigen::MatrixXd UV;
                   std::vector<std::string> VDheader = {"quality"};

                   pmp::mesh_to_matrices(local_mesh, V, F);
                   N.resize(V.rows(), V.cols());
                   VD.resize(V.rows(), 1);
                   UV.resize(V.rows(), 2);

                   auto N_prop =
                       local_mesh.get_vertex_property<pmp::Normal>("v:normal");
                   for (auto v : local_mesh.vertices()) {
                     N.row(v.idx()) = static_cast<Eigen::Vector3d>(N_prop[v]);
                   }

                   auto VD_prop =
                       local_mesh.get_vertex_property<float>("v:distance");
                   for (auto v : local_mesh.vertices()) {
                     VD(v.idx(), 0) = static_cast<float>(VD_prop[v]);
                   }

                   Eigen::MatrixXd U;
                   Eigen::MatrixXi G;
                   Eigen::MatrixXd SU;
                   Eigen::MatrixXi J;
                   Eigen::SparseMatrix<double> BC;
                   Eigen::MatrixXi L;

                   //igl::remesh_along_isoline(V, F, VD, 0.0, U, G, SU, J, BC, L);

                   //N.resize(U.rows(), U.cols());
                   //UV.resize(U.rows(), 2);
                   //igl::writePLY("out.ply", U, G, N, UV, SU, VDheader);

                   igl::writePLY("out.ply", V, F, N, UV, VD, VDheader);
                 },
                 [](const auto &brep) {

                 },
             },
             geometry);
  std::println();

  return geometry;
}