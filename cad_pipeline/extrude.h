#pragma once

#include "boost/geometry/algorithms/difference.hpp"
#include "earcut.hpp"
#include "polygon.h"
#include "types.h"

namespace mapbox {
namespace util {

template <>
struct nth<0, internal::point_t> {
  inline static auto get(const internal::point_t &t) { return t.x(); };
};
template <>
struct nth<1, internal::point_t> {
  inline static auto get(const internal::point_t &t) { return t.y(); };
};

}  // namespace util
}  // namespace mapbox

AnyGeometry Extrude(const AnyGeometry &geometry, const AnyGeometry &polygon,
                    float face_index, float depth) {
  std::print("Extrude({}, ", face_index);
  return std::visit(
      overloaded{
          [face_index, depth](const std::shared_ptr<BRep> &brep,
                              const std::shared_ptr<Polygon> &polygon) {
            std::println("BRep, Polygon)");

            // Make a copy
            pmp::SurfaceMesh extruded = brep->GetTopology();

            Polygon face_polygon;
            // Convert face into Polygon
            // TODO: Support integers as parameters
            int index = static_cast<int>(std::round(face_index));

            auto face_normal = pmp::face_normal(extruded, pmp::Face(index));
            pmp::Point face_center(0.0, 0.0, 0.0);
            int v_count = 0;
            for (auto v : extruded.vertices(pmp::Face(index))) {
              auto &p = extruded.position(v);
              face_center += p;
              ++v_count;
            }
            face_center /= static_cast<float>(v_count);

            auto look_at = face_center - face_normal;
            auto up = pmp::vec3(0.0, 1.0, 0.0);
            if (std::fabs(face_normal[1] - up[1]) < 1e-5) {
              up = pmp::vec3(1.0, 0.0, 0.0);
            }

            auto transformation = pmp::look_at_matrix(face_center, look_at, up);
            transformation = pmp::translation_matrix(face_center) * transformation;

            for (auto v : extruded.vertices(pmp::Face(0))) {
              auto p = extruded.position(v);
              //p = pmp::affine_transform(inverse_transformation, p);
              boost::geometry::append(face_polygon.outer(),
                                      internal::point_t(p[0], p[1]));

              std::reverse(face_polygon.outer().begin(), face_polygon.outer().end();
            }

            std::deque<Polygon> result;
            boost::geometry::difference(face_polygon, *polygon, result);

            extruded.delete_face(pmp::Face(index));

            // TODO: Optimize, too many memory allocations and copies. Create
            // adapters.

            // TODO: The result is not water-tight! Simplify and improve.

            // PMP doesn't support inner loops, so I need to triangulate here
            // and add faces around hole
            int r = 0;
            for (const auto &p : result) {
              std::vector<internal::point_t> concatenated_points;
              // Add 'base' faces
              std::vector<std::vector<internal::point_t>> earcut_polygon;
              earcut_polygon.push_back(p.outer());
              concatenated_points.insert(concatenated_points.end(),
                                         p.outer().begin(), p.outer().end());
              for (const auto &inner : p.inners()) {
                earcut_polygon.push_back(inner);
                concatenated_points.insert(concatenated_points.end(),
                                           inner.begin(), inner.end());
              }
              const auto &earcut_result = mapbox::earcut(earcut_polygon);

              std::unordered_map<int, pmp::Vertex> earcut_to_pmp;

              for (size_t ti = 0; ti < earcut_result.size(); ti += 3) {
                int i[3];
                i[0] = earcut_result[ti + 0];
                i[1] = earcut_result[ti + 1];
                i[2] = earcut_result[ti + 2];

                pmp::Vertex v[3];

                for (int pi = 0; pi < 3; ++pi) {
                  if (auto it = earcut_to_pmp.find(i[pi]);
                      it != earcut_to_pmp.end()) {
                    v[pi] = it->second;
                  } else {
                    auto pt = concatenated_points[i[pi]];
                    auto pmp_pt = pmp::Point(pt.x(), pt.y(), 0.f);
                    pmp_pt = pmp::affine_transform(transformation, pmp_pt);
                    v[pi] = extruded.add_vertex(pmp_pt);
                  }
                }

                extruded.add_face({v[0], v[1], v[2]});
              }

              // Add side walls now
              if (r > 0) {
                for (size_t pti = 0; pti < p.outer().size(); ++pti) {
                  auto p0 = p.outer()[pti + 0];
                  auto p1 = p.outer()[(pti + 1) % p.outer().size()];

                  auto pmp_p0 = pmp::Point(p0.x(), p0.y(), 0.f);
                  pmp_p0 = pmp::affine_transform(transformation, pmp_p0);
                  auto pmp_p1 = pmp::Point(p1.x(), p1.y(), 0.f);
                  pmp_p1 = pmp::affine_transform(transformation, pmp_p1);
                  auto pmp_p2 = pmp::Point(p1.x(), p1.y(), depth);
                  pmp_p2 = pmp::affine_transform(transformation, pmp_p2);
                  auto pmp_p3 = pmp::Point(p0.x(), p0.y(), depth);
                  pmp_p3 = pmp::affine_transform(transformation, pmp_p3);

                  auto v0 = extruded.add_vertex(pmp_p0);
                  auto v1 = extruded.add_vertex(pmp_p1);
                  auto v2 = extruded.add_vertex(pmp_p2);
                  auto v3 = extruded.add_vertex(pmp_p3);

                  extruded.add_face({v0, v1, v2, v3});
                }
              }

              for (const auto &inner_poly : p.inners()) {
                for (size_t pti = 0; pti < inner_poly.size(); ++pti) {
                  auto p0 = inner_poly[pti + 0];
                  auto p1 = inner_poly[(pti + 1) % inner_poly.size()];

                  auto pmp_p0 = pmp::Point(p0.x(), p0.y(), 0.f);
                  pmp_p0 = pmp::affine_transform(transformation, pmp_p0);
                  auto pmp_p1 = pmp::Point(p1.x(), p1.y(), 0.f);
                  pmp_p1 = pmp::affine_transform(transformation, pmp_p1);
                  auto pmp_p2 = pmp::Point(p1.x(), p1.y(), depth);
                  pmp_p2 = pmp::affine_transform(transformation, pmp_p2);
                  auto pmp_p3 = pmp::Point(p0.x(), p0.y(), depth);
                  pmp_p3 = pmp::affine_transform(transformation, pmp_p3);

                  auto v0 = extruded.add_vertex(pmp_p0);
                  auto v1 = extruded.add_vertex(pmp_p1);
                  auto v2 = extruded.add_vertex(pmp_p2);
                  auto v3 = extruded.add_vertex(pmp_p3);

                  extruded.add_face({v0, v1, v2, v3});
                }
              }

              ++r;
            }

            std::vector<internal::point_t> concatenated_points;
            std::unordered_map<int, pmp::Vertex> earcut_to_pmp;

            // Add 'base' faces
            std::vector<std::vector<internal::point_t>> earcut_polygon;
            earcut_polygon.push_back(polygon->outer());
            concatenated_points.insert(concatenated_points.end(),
                                       polygon->outer().begin(),
                                       polygon->outer().end());
            for (const auto &inner : polygon->inners()) {
              earcut_polygon.push_back(inner);
              concatenated_points.insert(concatenated_points.end(),
                                         inner.begin(), inner.end());
            }
            const auto &earcut_result = mapbox::earcut(earcut_polygon);

            for (size_t ti = 0; ti < earcut_result.size(); ti += 3) {
              int i[3];
              i[0] = earcut_result[ti + 0];
              i[1] = earcut_result[ti + 1];
              i[2] = earcut_result[ti + 2];

              pmp::Vertex v[3];

              for (int pi = 0; pi < 3; ++pi) {
                if (auto it = earcut_to_pmp.find(i[pi]);
                    it != earcut_to_pmp.end()) {
                  v[pi] = it->second;
                } else {
                  auto pt = concatenated_points[i[pi]];
                  auto pmp_pt = pmp::Point(pt.x(), pt.y(), depth);
                  pmp_pt = pmp::affine_transform(transformation, pmp_pt);
                  v[pi] = extruded.add_vertex(pmp_pt);
                }
              }

              extruded.add_face({v[0], v[1], v[2]});
            }

            return AnyGeometry(std::make_shared<BRep>(extruded));
          },
          [&geometry](const auto &, const auto &) {
            std::println("This kind of extrusion is not supported!");
            return geometry;
          },
      },
      geometry, polygon);
}
