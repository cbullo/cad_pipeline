#include <memory>

#include "brep.h"
#include "pmp/algorithms/shapes.h"

std::shared_ptr<BRep> MakeCube(float half_extent) {
  auto cube = pmp::hexahedron();
  auto& positions = cube.positions();
  for (auto& p: positions) {
    p *= half_extent;
  }
  return std::make_shared<BRep>(cube);
}