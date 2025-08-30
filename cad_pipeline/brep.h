#include "pmp/surface_mesh.h"

class BRep {
 public:
  const pmp::SurfaceMesh& GetTopology() const { return _mesh; }


  
 private:
  pmp::SurfaceMesh _mesh;
};