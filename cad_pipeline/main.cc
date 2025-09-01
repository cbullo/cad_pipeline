#include "cube.h"
#include "executor.h"
#include "interpreter.h"
#include "primitives.h"
#include "triangulate.h"
#include "write_stl.h"

using CubeOp = Op<'C', true, &MakeCube, float>;
using TextOp = Op<'D', &MakeCharacterPolygon, char>;
// TODO: Template should take 'callable' parameter types, not runtime types.
// Or even better, parameters should be deduced from the function.
using TriangulateOp = Op<'T', true, &Triangulate, GeomId>;
using WriteOp = Op<'W', false, &WriteSTL, GeomId>;

int main() {
  Executor e;
  Cache c;

  e.Register<CubeOp>();
  e.Register<WriteOp>();
  e.Register<TriangulateOp>();

  ParseAndProcess("N5CTW", e, c);

  return 0;
};