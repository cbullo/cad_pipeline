//#include "distance_map.h"

#include <format>
#include <optional>
#include <print>
#include <stack>
#include <unordered_set>
#include <vector>

#include "pmp/algorithms/barycentric_coordinates.h"
#include "pmp/algorithms/distance_point_triangle.h"
#include "pmp/algorithms/normals.h"
#include "pmp/surface_mesh.h"
#include "visit_helper.h"

struct Group {
  std::vector<pmp::Face> faces;
  std::vector<pmp::Face> seed_faces;
};

void GetTriangle(const pmp::SurfaceMesh &mesh, pmp::Face f, pmp::Vertex &v0,
                 pmp::Vertex &v1, pmp::Vertex &v2) {
  pmp::Vertex vert[3];
  auto i = 0;
  for (auto v : mesh.vertices(f)) {
    vert[i] = v;
    ++i;
  }
  v0 = vert[0];
  v1 = vert[1];
  v2 = vert[2];
}

std::vector<Group> FindGroups(const pmp::SurfaceMesh &mesh) {
  std::vector<Group> result;

  std::vector<bool> visited(mesh.faces_size(), false);
  std::stack<pmp::Face> next_group;

  next_group.push(*mesh.faces_begin());

  while (!next_group.empty()) {
    auto next_group_index = next_group.top();
    next_group.pop();

    if (visited[next_group_index.idx()]) {
      continue;
    }

    result.resize(result.size() + 1);

    std::stack<pmp::Face> group_visit;
    group_visit.push(next_group_index);

    std::optional<bool> patch_sign;

    while (!group_visit.empty()) {
      auto next_face = group_visit.top();
      group_visit.pop();
      if (visited[next_face.idx()]) {
        continue;
      }

      visited[next_face.idx()] = true;
      result.back().faces.push_back(next_face);

      auto this_normal = pmp::face_normal(mesh, next_face);
      for (auto he : mesh.halfedges(next_face)) {
        auto opposite_face = mesh.face(mesh.opposite_halfedge(he));
        auto other_normal = pmp::face_normal(mesh, opposite_face);

        auto edge_dir = mesh.position(mesh.to_vertex(he)) -
                        mesh.position(mesh.from_vertex(he));

        auto sign = pmp::dot(pmp::cross(this_normal, other_normal), edge_dir);
        auto new_sign = sign >= 0.f ? true : false;
        if (patch_sign.has_value() && patch_sign != new_sign) {
          next_group.push(opposite_face);
          continue;
        }

        patch_sign = new_sign;
        group_visit.push(opposite_face);
      }
    }
  }

  std::vector<float> distances(mesh.faces_size(), 0.f);
  std::fill(visited.begin(), visited.end(), false);
  for (auto &group : result) {
    pmp::Point centroid(0.f, 0.f, 0.f);
    std::unordered_set<pmp::IndexType> group_set;

    for (const auto &face : group.faces) {
      pmp::Vertex v[3];
      pmp::Point p[3];

      group_set.insert(face.idx());

      GetTriangle(mesh, face, v[0], v[1], v[2]);
      p[0] = mesh.position(v[0]);
      p[1] = mesh.position(v[1]);
      p[2] = mesh.position(v[2]);

      centroid +=
          (p[0] + p[1] + p[2]) / static_cast<float>(3 * group.faces.size());
    }

    for (const auto &face : group.faces) {
      pmp::Vertex v[3];
      pmp::Point p[3];

      GetTriangle(mesh, face, v[0], v[1], v[2]);
      p[0] = mesh.position(v[0]);
      p[1] = mesh.position(v[1]);
      p[2] = mesh.position(v[2]);

      pmp::Point nearest;
      distances[face.idx()] =
          pmp::dist_point_triangle(centroid, p[0], p[1], p[2], nearest);
    }

    for (const auto &face : group.faces) {
      auto face_d = distances[face.idx()];
      bool all_closer = true;
      bool all_farther = true;
      for (const auto &he : mesh.halfedges(face)) {
        auto other_face = mesh.face(mesh.opposite_halfedge(he));
        if (!group_set.contains(other_face.idx())) {
          continue;
        }
        if (face_d < distances[other_face.idx()]) {
          all_closer = false;
        } else if (face_d > distances[other_face.idx()]) {
          all_farther = false;
        }
      }

      if (all_closer || all_farther) {
        group.seed_faces.push_back(face);
      }
    }
  }

  return result;
}

struct VertexResult {
  pmp::Vertex vertex;
  pmp::Point point;
  float distance;
};

struct FaceResult {
  pmp::Face face;
  pmp::Point point_on_face;
  float distance;
};

struct EdgeResult {
  pmp::Halfedge halfedge;
  pmp::Point point_on_edge;
  float distance;
};

using ResultType = std::variant<FaceResult, EdgeResult, VertexResult>;

ResultType FindClosestPointFromGroup(const pmp::Point &src,
                                     const pmp::SurfaceMesh &mesh,
                                     const Group &group) {
  float minimum_distance = std::numeric_limits<float>::max();
  ResultType return_value;

  std::vector<bool> visited(mesh.faces_size(), false);
  std::stack<pmp::Face> visit_stack;

  // Add at least 2 points in case we're unlucky finding an antipodal point.
  // visit_stack.push(group.front());
  // visit_stack.push(*(group.begin() + 1));

  for (auto f : group.seed_faces) {
    visit_stack.push(f);
  }

  std::unordered_set<pmp::IndexType> group_set;
  for (auto f : group.faces) {
    group_set.insert(f.idx());
    visited[f.idx()] = false;
  }

  while (!visit_stack.empty()) {
    auto current_face = visit_stack.top();
    visit_stack.pop();

    // std::println("CURRENT FACE: {}", current_face.idx());

    if (visited[current_face.idx()]) {
      // std::println("VISITED");
      continue;
    }

    visited[current_face.idx()] = true;
    pmp::Vertex vert[3];
    pmp::Point pos[3];

    GetTriangle(mesh, current_face, vert[0], vert[1], vert[2]);
    pos[0] = mesh.position(vert[0]);
    pos[1] = mesh.position(vert[1]);
    pos[2] = mesh.position(vert[2]);

    pmp::Point triangle_point;
    auto distance =
        pmp::dist_point_triangle(src, pos[0], pos[1], pos[2], triangle_point);

    auto dv = pmp::dot(pmp::face_normal(mesh, current_face), src - pos[0]);
    distance = distance * (-fabsf(dv) / dv);

    auto uvw =
        pmp::barycentric_coordinates(triangle_point, pos[0], pos[1], pos[2]);

    // std::println("uvw: {} {} {}, distance: {}", uvw[0], uvw[1], uvw[2],
    //              distance);

    bool vertex_result = false;
    for (int i = 0; i < 3; ++i) {
      if (1.f - uvw[i] < 1e-6f) {
        for (auto next_face : mesh.faces(vert[i])) {
          if (next_face == current_face) {
            continue;
          }

          if (group_set.contains(next_face.idx())) {
            visit_stack.push(next_face);
          } else {
            // std::println("DOES NOT CONTAIN");
          }
        }

        // std::println("VERTEX RESULT");
        vertex_result = true;
        if (fabsf(distance) < minimum_distance) {
          minimum_distance = fabsf(distance);
          return_value = VertexResult{vert[i], triangle_point, -distance};
        }
        break;
      }
    }
    if (vertex_result) {
      continue;
    }

    bool edge_result = false;
    for (int i = 0; i < 3; ++i) {
      if (uvw[i] < 1e-6f) {
        auto ev1 = vert[(i + 1) % 3];
        auto ev2 = vert[(i + 2) % 3];

        pmp::Halfedge he;
        for (auto fhe : mesh.halfedges(current_face)) {
          if (mesh.from_vertex(fhe) == ev1 && mesh.to_vertex(fhe) == ev2) {
            he = fhe;
            break;
          }
        }

        assert(he.is_valid());

        auto opposite_face = mesh.face(mesh.opposite_halfedge(he));
        if (group_set.contains(opposite_face.idx())) {
          visit_stack.push(opposite_face);
          // std::println("current: {}, next: {}", current_face.idx(),
          //              opposite_face.idx());
        } else {
          // std::println("DOES NOT CONTAIN");
        }

        edge_result = true;
        if (fabsf(distance) < minimum_distance) {
          minimum_distance = distance;
          return_value = EdgeResult{he, triangle_point, -distance};
        }
        break;
      }
    }

    if (edge_result) {
      continue;
    }

    // std::println("uvw: {} {} {}", uvw[0], uvw[1], uvw[2]);
    // std::println("d {} md {}", distance, minimum_distance);
    if (fabsf(distance) <= minimum_distance) {
      // std::println("minimum_distance {}", minimum_distance);
      minimum_distance = fabsf(distance);
      return_value = FaceResult{current_face, triangle_point, -distance};
    }
  }

  // std::println("minimum_distance {}", minimum_distance);
  return return_value;
}

AnyGeometry DistanceMap(ExecutionContext &execution_context,
                        const AnyGeometry &geometry1,
                        const AnyGeometry &geometry2) {
  std::println("DistanceMap()");

  auto mesh1 = geometry2;
  auto mesh2 = geometry1;

  auto groups1 = FindGroups(mesh1);
  auto groups2 = FindGroups(mesh2);

  std::println("g1 {} seeds: {}", groups1[0].faces.size(),
               groups1[0].seed_faces.size());
  std::println("g2 {} seeds: {}", groups2[0].faces.size(),
               groups2[0].seed_faces.size());

  auto r = FindClosestPointFromGroup(pmp::Point(-20.f, 20.f, 0.f), mesh2,
                                     groups2[0]);

  {
    pmp::VertexProperty<float> distance_prop2 =
        mesh2.vertex_property<float>("v:distance");
    for (auto v : mesh2.vertices()) {
      auto p = mesh2.position(v);
      std::println("POSITION: {} {} {}", p[0], p[1], p[2]);
      auto r = FindClosestPointFromGroup(p, mesh1, groups1[0]);

      std::visit(
          overloaded{
              [&distance_prop2, v](const VertexResult &pt) {
                std::println("Vertex: {} {}, position: {} {} {}",
                             pt.vertex.idx(), pt.distance, pt.point[0],
                             pt.point[1], pt.point[2]);
                distance_prop2[v] = pt.distance;
              },
              [&distance_prop2, v](const EdgeResult &e) {
                std::println("Edge: {} {}, position: {} {} {}",
                             e.halfedge.idx(), e.distance, e.point_on_edge[0],
                             e.point_on_edge[1], e.point_on_edge[2]);
                distance_prop2[v] = e.distance;
              },
              [&distance_prop2, v](const FaceResult &f) {
                std::println("Face: {} {}, position: {} {} {}", f.face.idx(),
                             f.distance, f.point_on_face[0], f.point_on_face[1],
                             f.point_on_face[2]);
                distance_prop2[v] = f.distance;
              },
          },
          r);
    }

    pmp::VertexProperty<float> distance_prop1 =
        mesh1.vertex_property<float>("v:distance");
    for (auto v : mesh1.vertices()) {
      auto p = mesh1.position(v);
      std::println("POSITION: {} {} {}", p[0], p[1], p[2]);
      auto r = FindClosestPointFromGroup(p, mesh2, groups2[0]);

      bool split = false;
      std::visit(
          overloaded{
              [&distance_prop1, v](const VertexResult &pt) {
                std::println("Vertex: {} {}, position: {} {} {}",
                             pt.vertex.idx(), pt.distance, pt.point[0],
                             pt.point[1], pt.point[2]);
                distance_prop1[v] = pt.distance;
              },
              [&distance_prop1, &distance_prop2, &mesh2, &groups2, &split,
               &mesh1, &groups1, v](const EdgeResult &e) {
                std::println("Edge: {} {}, position: {} {} {}",
                             e.halfedge.idx(), e.distance, e.point_on_edge[0],
                             e.point_on_edge[1], e.point_on_edge[2]);
                distance_prop1[v] = e.distance;
                auto edge = mesh2.edge(e.halfedge);
                auto f0 = mesh2.face(edge, 0);
                auto f1 = mesh2.face(edge, 1);

                auto new_he = mesh2.split(edge, e.point_on_edge);
                auto new_v = mesh2.to_vertex(new_he);

                for (auto new_f : mesh2.faces(new_v)) {
                  if (new_f != f0 && new_f != f1) {
                    groups2[0].faces.push_back(new_f);
                  }
                }

                auto res = FindClosestPointFromGroup(e.point_on_edge, mesh1,
                                                     groups1[0]);
                float &dist = distance_prop2[new_v];
                std::visit(
                    overloaded{
                        [&dist](const VertexResult &pt) { dist = pt.distance; },
                        [&dist](const EdgeResult &e) { dist = e.distance; },
                        [&dist](const FaceResult &f) { dist = f.distance; }},
                    res);
              },
              [&distance_prop1, &distance_prop2, &mesh2, &groups2, &split,
               &mesh1, &groups1, v](const FaceResult &f) {
                std::println("Face: {} {}, position: {} {} {}", f.face.idx(),
                             f.distance, f.point_on_face[0], f.point_on_face[1],
                             f.point_on_face[2]);
                distance_prop1[v] = f.distance;
                auto new_v = mesh2.split(f.face, f.point_on_face);
                for (auto new_f : mesh2.faces(new_v)) {
                  if (new_f != f.face) {
                    groups2[0].faces.push_back(new_f);
                  }
                }

                auto res = FindClosestPointFromGroup(f.point_on_face, mesh1,
                                                     groups1[0]);
                float &dist = distance_prop2[new_v];
                std::visit(
                    overloaded{
                        [&dist](const VertexResult &pt) { dist = pt.distance; },
                        [&dist](const EdgeResult &e) { dist = e.distance; },
                        [&dist](const FaceResult &f) { dist = f.distance; }},
                    res);
              },
          },
          r);
    }
  }

  // auto r = FindClosestPointFromGroup(pmp::Point(-20.f, 20.f, 0.f),
  //                                    mesh1, groups1[0]);

  return AnyGeometry(std::make_shared<Mesh>(mesh2));
}
, [&geometry1](const auto &, const auto &) {
  std::println("This geometry is not supported in distance!");
  return geometry1;
}
},
      geometry1, geometry2);
}