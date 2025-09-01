#pragma once

#include <memory>
#include <print>

#include "pmp/algorithms/triangulation.h"
#include "types.h"
#include "visit_helper.h"

AnyGeometry Triangulate(const AnyGeometry &geometry) {
  std::print("Triangulate(");
  return std::visit(
      overloaded{
          [](const std::shared_ptr<BRep> &brep) {
            std::println("BRep)");
            const auto &surf_mesh = brep->GetTopology();
            pmp::SurfaceMesh triangulated = surf_mesh;
            pmp::triangulate(triangulated);

            return AnyGeometry(std::make_shared<Mesh>(triangulated));
          },
          [&geometry](const std::shared_ptr<Mesh> &mesh) {
            std::println("Mesh)");
            return geometry;
          },
          [&geometry](const std::shared_ptr<Polygon> &brep) {
            std::println("Polygon)");
            // TODO
            return geometry;
          },
      },
      geometry);
}