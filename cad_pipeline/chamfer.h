#pragma once

#include <cassert>
#include <print>
#include <unordered_map>
#include <vector>

#include "brep.h"
#include "types.h"
#include "visit_helper.h"
  for (auto face : in.faces()) {
    std::vector<pmp::Vertex> face_verts;
    for (auto he : in.halfedges(face)) {
      auto ret0 = edges_to_verts.find(he.idx());
      auto vert = ret0->second[0];
      face_verts.push_back(pmp::Vertex(vert));
      std::println("{} ", vert);
    }

    std::println();
    out.add_face(face_verts);
    //break;
  }
static inline pmp::Point safe_unit(const pmp::Point& v) {
  pmp::Scalar l = norm(v);
  return (l > pmp::Scalar(0)) ? v / l : pmp::Point(0, 0, 0);
}

// Build ONLY per-vertex “floating corner” caps.
// For each original vertex v, create one polygon made of duplicates of (f,v)
// for all faces f incident to v. No connections to other faces are created.
void ChamferAllEdges(const pmp::SurfaceMesh& in, pmp::SurfaceMesh& out,
                     pmp::Scalar L) {
  using namespace pmp;

  auto pin = in.get_vertex_property<Point>("v:point");
  if (!pin) throw std::runtime_error("Input mesh has no v:point property.");

  std::unordered_multimap<pmp::IndexType, pmp::IndexType> faces_to_verts;
  std::unordered_map<pmp::IndexType, std::deque<pmp::IndexType>> edges_to_verts;

  std::vector<std::vector<pmp::Vertex>> corner_faces;

  // For each vertex in the original mesh:
  for (auto v : in.vertices()) {
    // Collect incident faces around v (CCW order).
    std::vector<Face> incf;
    incf.reserve(in.valence(v));
    for (auto f : in.faces(v))
      if (f.is_valid()) incf.push_back(f);

    if (incf.size() < 3) continue;  // need at least a triangle to form a face

    // For each incident face f, find the two boundary neighbors (vp, vn)
    // around v *within that face*, then slide from v along those two edges by
    // L.
    std::vector<Vertex> new_loop;
    new_loop.reserve(incf.size());

    for (auto f : incf) {
      // Find the halfedge on face f whose to-vertex is v
      auto h0 = in.halfedge(f);
      auto h = h0;
      bool found = false;
      do {
        if (in.to_vertex(h) == v) {
          found = true;
          break;
        }
        h = in.next_halfedge(h);
      } while (h != h0);

      if (!found) {
        // Should not happen on a valid manifold mesh
        continue;
      }

      Vertex vp = in.from_vertex(h);  // previous vertex on this face cycle
      Vertex vn =
          in.to_vertex(in.next_halfedge(h));  // next vertex on this face cycle

      const Point pv = pin[v];
      const Point pp = pin[vp];
      const Point pn = pin[vn];

      // Move from v along both incident edges (within this face) by distance L
      // each
      Point dir_prev = safe_unit(pp - pv);  // v -> vp
      Point dir_next = safe_unit(pn - pv);  // v -> vn
      Point q = pv + L * dir_prev + L * dir_next;

      // (Optional) keep q on face plane for numerical cleanliness
      Point n = normalize(cross(pn - pv, pp - pv));
      q = q - dot(q - pv, n) * n;

      // Create a brand new vertex in OUT for this corner
      Vertex vo = out.add_vertex(q);
      faces_to_verts.insert({f.idx(), vo.idx()});

      edges_to_verts[h.idx()].push_front(vo.idx());
      edges_to_verts[in.next_halfedge(h).idx()].push_back(vo.idx());

      new_loop.push_back(vo);
    }

    // Make a single polygon face for this vertex’s corner cap
    if (new_loop.size() >= 3) {
      corner_faces.push_back(new_loop);
    }
    // out.add_face(new_loop);  // edges are unique to this cap; no sharing
  }

  for (auto face : in.faces()) {
    std::vector<pmp::Vertex> face_verts;
    for (auto he : in.halfedges(face)) {
      auto ret0 = edges_to_verts.find(he.idx());
      auto vert = ret0->second[0];
      face_verts.push_back(pmp::Vertex(vert));
      std::println("{} ", vert);
    }

    std::println();
    out.add_face(face_verts);
    // break;
  }

  for (auto edge : in.edges()) {
    std::vector<pmp::Vertex> edge_verts;
    edge_verts.reserve(4);

    auto he0 = in.halfedge(edge, 0);
    auto he1 = in.halfedge(edge, 1);

    auto ret0 = edges_to_verts.find(he0.idx());
    for (auto it : ret0->second) {
      auto vert = it;
      // td::print(" {}", vert);
      edge_verts.push_back(pmp::Vertex(vert));
    }

    auto ret1 = edges_to_verts.find(he1.idx());
    for (auto it : ret1->second) {
      auto vert = it;
      // std::print(" {}", vert);
      edge_verts.push_back(pmp::Vertex(vert));
    }

    // std::println();
    // out.add_face({pmp::Vertex(10), pmp::Vertex(9), pmp::Vertex(6),
    // pmp::Vertex(8)});
    out.add_face(edge_verts);
    // break;
  }

  for (auto cf : corner_faces) {
    out.add_face(cf);
  }

  // for (auto face : in.faces()) {
  //   std::vector<pmp::Vertex> face_verts;
  //   auto ret = faces_to_verts.equal_range(face.idx());
  //   for (auto it = ret.first; it != ret.second; ++it) {
  //     auto vert = it->second;
  //     std::print(" {}", vert);
  //     face_verts.push_back(pmp::Vertex(vert));
  //   }
  //   std::println();
  //   out.add_face(face_verts);
  //   //break;
  // }
}

AnyGeometry Chamfer(const AnyGeometry& geometry, float length) {
  std::print("Chamfer({}, ", length);
  return std::visit(
      overloaded{[length](const std::shared_ptr<BRep>& brep) {
                   std::println("BRep)");

                   auto mesh = brep->GetTopology();
                   pmp::SurfaceMesh out;
                   ChamferAllEdges(mesh, out, length);

                   //  for (auto edge : mesh.edges()) {
                   //    auto v0 = mesh.vertex(edge, 0);
                   //    auto v1 = mesh.vertex(edge, 1);

                   //    auto f0 = mesh.face(mesh.halfedge(edge, 0));
                   //    auto f1 = mesh.face(mesh.halfedge(edge, 1));

                   //    auto n0 = pmp::face_normal(mesh, f0);
                   //    auto n1 = pmp::face_normal(mesh, f1);

                   //    auto n = n0 + n1;
                   //    n = pmp::normalize(n);

                   //    const auto &v_halfedges0 = mesh.halfedges(v0);
                   //    const auto &v_halfedges1 = mesh.halfedges(v1);

                   //    for (auto he : v_halfedges0) {
                   //      if (mesh.to_vertex(he) == v1) {
                   //        continue;
                   //      }
                   //    }

                   //    break;
                   //  }

                   return AnyGeometry(std::make_shared<BRep>(out));
                 },
                 [&geometry](const auto&) {
                   std::println("This geometry is not supported in chamfer!");
                   return geometry;
                 }},
      geometry);
}