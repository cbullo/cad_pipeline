#include "chamfer.h"
#include "cube.h"
#include "executor.h"
#include "extrude.h"
#include "interpreter.h"
#include "make_character.h"
#include "primitives.h"
#include "triangulate.h"
#include "write_stl.h"

using CubeOp = Op<'C', true, &MakeCube, float>;
using TextOp = Op<'L', true, &MakeCharacterPolygon, char>;
// TODO: Template should take 'callable' parameter types, not runtime types.
// Or even better, parameters should be deduced from the function.
using TriangulateOp = Op<'T', true, &Triangulate, GeomId>;
using WriteOp = Op<'W', false, &WriteSTL, GeomId>;
using ExtrudeOp = Op<'E', true, &Extrude, GeomId, GeomId, float, float>;
using ChamferOp = Op<'B', true, &Chamfer, GeomId, float>;

int main() {
  Executor e;
  Cache c;

  e.Register<CubeOp>();
  e.Register<TextOp>();
  e.Register<TriangulateOp>();
  e.Register<WriteOp>();
  e.Register<ExtrudeOp>();
  e.Register<ChamferOp>();

  ParseAndProcess("N2.1N0SDLN1.2N5CBETW", e, c);
  //ParseAndProcess("N1.0N5.0CBTW", e, c);

  std::println("Finished");
  return 0;
};