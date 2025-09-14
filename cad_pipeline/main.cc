#include "chamfer.h"
#include "cube.h"
#include "distance_map.h"
#include "executor.h"
#include "extrude.h"
#include "interpreter.h"
#include "make_character.h"
#include "triangulate.h"
#include "write_stl.h"
#include "write_ply.h"

using CubeOp = Op<'C', true, &MakeCube, float>;
using TextOp = Op<'L', true, &MakeCharacterPolygon, char>;
// TODO: Template should take 'callable' parameter types, not runtime types.
// Or even better, parameters should be deduced from the function.
using TriangulateOp = Op<'T', true, &Triangulate, GeomId>;
using WriteSTLOp = Op<'W', false, &WriteSTL, GeomId>;
using WritePLYOp = Op<'P', false, &WritePLY, GeomId>;
using ExtrudeOp = Op<'E', true, &Extrude, GeomId, GeomId, float, float>;
using ChamferOp = Op<'B', true, &Chamfer, GeomId, float>;
using DistanceOp = Op<'D', true, &DistanceMap, GeomId, GeomId>;

int main() {
  Executor e;
  Cache c;

  e.Register<CubeOp>();
  e.Register<TextOp>();
  e.Register<TriangulateOp>();
  e.Register<WriteSTLOp>();
  e.Register<WritePLYOp>();
  e.Register<ExtrudeOp>();
  e.Register<ChamferOp>();
  e.Register<DistanceOp>();

  // ParseAndProcess("N2.1N0S3LN2.1N4 SHL N-3.1N5 SHL N2.1N1SDLN-2.1N2S3L N-2.1
  // N3 SDL N1.2 N5C B E EEEEETW", e, c);
  ParseAndProcess("N5CTN1N6CBTDP", e, c);

  std::println("Finished");
  return 0;
};
