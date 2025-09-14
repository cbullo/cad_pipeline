#include <vector>

#include "pmp/algorithms/normals.h"
#include "pmp/mat_vec.h"
#include "pmp/surface_mesh.h"

// compute squared area of triangle. used for triangulate().
inline pmp::Scalar Area(const pmp::vec3& p0, const pmp::vec3& p1,
                        const pmp::vec3& p2) {
  return pmp::sqrnorm(pmp::cross(p1 - p0, p2 - p0));
}

struct Triangulation {
  Triangulation(pmp::Scalar a = std::numeric_limits<pmp::Scalar>::max(),
                int s = -1)
      : area(a), split(s) {}
  pmp::Scalar area;
  int split;
};

void Tessellate(const std::vector<pmp::vec3>& points,
                std::vector<pmp::ivec3>& triangles) {
  const int n = points.size();

  triangles.clear();
  triangles.reserve(n - 2);

  // triangle? nothing to do
  if (n == 3) {
    triangles.emplace_back(0, 1, 2);
    return;
  }

  // quad? simply compare to two options
  else if (n == 4) {
    if (Area(points[0], points[1], points[2]) +
            Area(points[0], points[2], points[3]) <
        Area(points[0], points[1], points[3]) +
            Area(points[1], points[2], points[3])) {
      triangles.emplace_back(0, 1, 2);
      triangles.emplace_back(0, 2, 3);
    } else {
      triangles.emplace_back(0, 1, 3);
      triangles.emplace_back(1, 2, 3);
    }
    return;
  }

  std::vector<Triangulation> triangulation_;
  triangulation_.clear();
  triangulation_.resize(n * n);
  auto polygon_valence_ = n;

  // n-gon with n>4? compute triangulation by dynamic programming
  triangulation_.clear();
  triangulation_.resize(n * n);
  polygon_valence_ = n;

  int i, j, m, k, imin;
  pmp::Scalar w, wmin;

  // initialize 2-gons
  for (i = 0; i < n - 1; ++i) {
    Triangulation(i, i + 1) = Triangulation(0.0, -1);
  }

  // n-gons with n>2
  for (j = 2; j < n; ++j) {
    // for all n-gons [i,i+j]
    for (i = 0; i < n - j; ++i) {
      k = i + j;

      wmin = std::numeric_limits<pmp::Scalar>::max();
      imin = -1;

      // find best split i < m < i+j
      for (m = i + 1; m < k; ++m) {
        w = Triangulation(i, m).area + Area(points[i], points[m], points[k]) +
            Triangulation(m, k).area;

        if (w < wmin) {
          wmin = w;
          imin = m;
        }
      }

      triangulation_[polygon_valence_ * i + k] = Triangulation(wmin, imin);
    }
  }

  // build triangles from triangulation table
  std::vector<pmp::ivec2> todo;
  todo.reserve(n);
  todo.emplace_back(0, n - 1);
  while (!todo.empty()) {
    pmp::ivec2 tri = todo.back();
    todo.pop_back();
    const int start = tri[0];
    const int end = tri[1];
    if (end - start < 2) continue;
    const int split = triangulation_[polygon_valence_ * start + end].split;

    triangles.emplace_back(start, split, end);

    todo.emplace_back(start, split);
    todo.emplace_back(split, end);
  }
}

void Convert(const pmp::SurfaceMesh& mesh, std::vector<pmp::ivec3>& triangles,
             std::vector<pmp::vec3>& position_array,
             std::vector<pmp::vec3>& normal_array, float crease_angle) {
  // index array for remapping vertex indices during duplication
  // note: use vertices_size() instead of n_vertices() to also
  // take deleted vertices into account
  std::vector<size_t> vertex_indices(mesh.vertices_size());

  // we have a mesh: fill arrays by looping over faces
  if (mesh.n_faces()) {
    // reserve memory
    position_array.reserve(3 * mesh.n_faces());
    normal_array.reserve(3 * mesh.n_faces());

    // precompute normals for easy cases
    std::vector<pmp::Normal> face_normals;
    std::vector<pmp::Normal> vertex_normals;
    if (crease_angle < 1) {
      // note: use faces_size() instead of n_faces() to
      // take deleted faces into account
      face_normals.resize(mesh.faces_size());
      for (auto f : mesh.faces())
        face_normals[f.idx()] = pmp::face_normal(mesh, f);
    } else if (crease_angle > 170) {
      // note: use vertices_size() instead of n_vertices() to
      // take deleted vertices into account
      vertex_normals.resize(mesh.vertices_size());
      for (auto v : mesh.vertices())
        vertex_normals[v.idx()] = pmp::vertex_normal(mesh, v);
    }

    // data per face (for all corners)
    std::vector<pmp::Halfedge> corner_halfedges;
    std::vector<pmp::Vertex> corner_vertices;
    std::vector<pmp::vec3> corner_positions;
    std::vector<pmp::vec3> corner_normals;

    // convert from degrees to radians
    const pmp::Scalar crease_angle_radians = crease_angle / 180.0 * std::numbers::pi;

    size_t vidx(0);

    auto vpos = mesh.get_vertex_property<pmp::Point>("v:point");

    // loop over all faces
    for (auto f : mesh.faces()) {
      // collect corner positions and normals
      corner_halfedges.clear();
      corner_vertices.clear();
      corner_positions.clear();
      corner_normals.clear();
      pmp::Vertex v;
      pmp::Normal n;

      for (auto h : mesh.halfedges(f)) {
        v = mesh.to_vertex(h);
        corner_halfedges.push_back(h);
        corner_vertices.push_back(v);
        corner_positions.push_back((pmp::vec3)vpos[v]);

        if (crease_angle < 1) {
          n = face_normals[f.idx()];
        } else if (crease_angle > 170) {
          n = vertex_normals[v.idx()];
        } else {
          n = pmp::corner_normal(mesh, h, crease_angle_radians);
        }
        corner_normals.push_back((pmp::vec3)n);
      }
      assert(corner_vertices.size() >= 3);

      // tessellate face into triangles
      Tessellate(corner_positions, triangles);
      for (auto& t : triangles) {
        const int i0 = t[0];
        const int i1 = t[1];
        const int i2 = t[2];

        position_array.push_back(corner_positions[i0]);
        position_array.push_back(corner_positions[i1]);
        position_array.push_back(corner_positions[i2]);

        normal_array.push_back(corner_normals[i0]);
        normal_array.push_back(corner_normals[i1]);
        normal_array.push_back(corner_normals[i2]);

        vertex_indices[corner_vertices[i0].idx()] = vidx++;
        vertex_indices[corner_vertices[i1].idx()] = vidx++;
        vertex_indices[corner_vertices[i2].idx()] = vidx++;
      }
    }
  }

  // we have a point cloud
  else if (mesh.n_vertices()) {
    auto position = mesh.get_vertex_property<pmp::Point>("v:point");
    if (position) {
      position_array.reserve(mesh.n_vertices());
      for (auto v : mesh.vertices())
        position_array.push_back((pmp::vec3)position[v]);
    }

    auto normals = mesh.get_vertex_property<pmp::Point>("v:normal");
    if (normals) {
      normal_array.reserve(mesh.n_vertices());
      for (auto v : mesh.vertices()) normal_array.push_back((pmp::vec3)normals[v]);
    }

  }
}