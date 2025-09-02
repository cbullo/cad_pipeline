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
            std::flush(std::cout);

            const auto &surf_mesh = brep->GetTopology();
            pmp::SurfaceMesh triangulated = surf_mesh;
            pmp::triangulate(triangulated);
            pmp::face_normals(triangulated);

            return AnyGeometry(std::make_shared<Mesh>(triangulated));
          },
          [&geometry](const std::shared_ptr<Mesh> &mesh) {
            std::println("Mesh)");
            std::flush(std::cout);
            return geometry;
          },
          [&geometry](const std::shared_ptr<Polygon> &polygon) {
            std::println("Polygon)");
            std::flush(std::cout);

            pmp::SurfaceMesh mesh;

            std::vector<pmp::Vertex> vertices;
            vertices.reserve(polygon->outer().size());
            for (const auto &pt : polygon->outer()) {
              vertices.push_back(
                  mesh.add_vertex(pmp::Point(pt.x(), pt.y(), 0.f)));
            }
            mesh.add_face(vertices);
            pmp::triangulate(mesh);
            pmp::face_normals(mesh);

            return AnyGeometry(std::make_shared<Mesh>(mesh));
          },
      },
      geometry);
}