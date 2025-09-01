#pragma once

#include "pmp/surface_mesh.h"

class BRep {
 public:
  BRep() {};
  BRep(const pmp::SurfaceMesh& mesh) : _mesh(mesh) {};

  const pmp::SurfaceMesh& GetTopology() const { return _mesh; }
  pmp::SurfaceMesh GetTopology() { return _mesh; }

 private:
  pmp::SurfaceMesh _mesh;
};