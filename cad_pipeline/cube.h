#pragma once

#include <cmath>
#include <memory>
#include <print>

#include "brep.h"
#include "pmp/algorithms/normals.h"
#include "pmp/algorithms/shapes.h"
#include "types.h"

struct Cube {
  Eigen::Vector3f half_extent;
};

inline std::shared_ptr<BRep> MakeCube(float half_extent) {
  std::println("MakeCube({})", half_extent);
  auto cube = pmp::hexahedron();

  for (const auto& v : cube.vertices()) {
    cube.position(v) *= sqrtf(3.f) * half_extent;
  }

  pmp::face_normals(cube);
  return std::make_shared<BRep>(cube);
}

template <typename Shape>
struct Transformed {
  Shape shape;
  Eigen::Affine3f transformation;
};
