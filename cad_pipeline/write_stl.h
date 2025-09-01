#include <iostream>
#include <print>

#include "pmp/io/write_stl.h"
#include "types.h"
#include "visit_helper.h"

AnyGeometry WriteSTL(const AnyGeometry &geometry) {
  std::print("MakeCube(");
  std::visit(overloaded{
                 [](const std::shared_ptr<BRep> &brep) {
                   std::print("BRep");
                   std::flush(std::cout);
                   const auto &surf_mesh = brep->GetTopology();
                   pmp::write_stl(surf_mesh, "out.stl", pmp::IOFlags{});
                 },
                 [](const std::shared_ptr<Mesh> &mesh) {
                   std::print("Mesh");
                   std::flush(std::cout);
                   const auto &surf_mesh = *mesh;
                   pmp::write_stl(surf_mesh, "out.stl",
                                  pmp::IOFlags{use_face_normals : false});
                 },
                 [](const std::shared_ptr<Polygon> &brep) {

                 },
             },
             geometry);
  std::println();

  return geometry;
}