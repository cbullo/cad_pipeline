#pragma once

#include <cmath>
#include <memory>
#include <print>

#include "Eigen/Core"
#include "brep.h"
#include "cache.h"
#include "pmp/algorithms/normals.h"
#include "pmp/algorithms/shapes.h"
#include "types.h"

inline std::shared_ptr<Cube> MakeCube(ExecutionContext& execution_context,
                                      float half_extents) {
  // std::println("MakeCube({})", half_extents);
  // auto cube = pmp::hexahedron();

  // for (const auto& v : cube.vertices()) {
  //   cube.position(v) *= sqrtf(3.f) * half_extents;
  // }

  // pmp::face_normals(cube);
  // return std::make_shared<BRep>(cube);

  return std::make_shared<Cube>(Cube{.half_extents =
                  Eigen::Vector3f(half_extents, half_extents, half_extents)});
}

template <typename Shape>
struct Transformed {
  Shape shape;
  Eigen::Affine3f transformation;
};
