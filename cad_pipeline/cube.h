#pragma once

#include <memory>
#include <print>

#include "brep.h"
#include "pmp/algorithms/normals.h"
#include "pmp/algorithms/shapes.h"
#include "types.h"

AnyGeometry MakeCube(float half_extent) {
  std::println("MakeCube({})", half_extent);
  auto cube = pmp::hexahedron();
  pmp::face_normals(cube);
  auto& positions = cube.positions();
  for (auto& p : positions) {
    p *= half_extent;
  }
  return std::make_shared<BRep>(cube);
}