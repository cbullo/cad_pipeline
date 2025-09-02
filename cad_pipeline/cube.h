#pragma once

#include <memory>
#include <print>
#include <cmath>

#include "brep.h"
#include "pmp/algorithms/normals.h"
#include "pmp/algorithms/shapes.h"
#include "types.h"

AnyGeometry MakeCube(float half_extent) {
  std::println("MakeCube({})", half_extent);
  auto cube = pmp::hexahedron();
  
  for (const auto& v : cube.vertices()) {
    cube.position(v) *= sqrtf(3.f) * half_extent;
  }

  pmp::face_normals(cube);

  return std::make_shared<BRep>(cube);
}