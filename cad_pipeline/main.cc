#include "cube.h"
#include "executor.h"
#include "interpreter.h"
#include "primitives.h"
#include "triangulate.h"
#include "write_stl.h"

using CubeOp = Op<'C', true, &MakeCube, float>;
// using TextOp = Op<'T', &MakeCharacterPolygon, char>;
// TODO: Template should take 'callable' parameter types, not runtime types.
// Or even better, parameters should be deduced from the function.
using WriteOp = Op<'W', false, &WriteSTL, GeomId>;
using TriangulateOp = Op<'T', true, &Triangulate, GeomId>;

int main() {
  Executor e;
  Cache c;

  e.Register<CubeOp>();
  e.Register<WriteOp>();
  e.Register<TriangulateOp>();

  ParseAndProcess("N2CTW", e, c);

  return 0;
};