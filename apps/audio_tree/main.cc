#include "chamfer.h"
#include "cube.h"
#include "executor.h"
#include "extrude.h"
#include "interpreter.h"
#include "make_character.h"
#include "triangulate.h"
#include "write_stl.h"

using MakeTreeOp = Op<'M', false, &MakeTree, GeomId, float, float, float, float>;
// TODO: Template should take 'callable' parameter types, not runtime types.
// Or even better, parameters should be deduced from the function.
using TriangulateOp = Op<'T', true, &Triangulate, GeomId>;
using WriteOp = Op<'W', false, &WriteSTL, GeomId>;

int main() {
  Executor e;
  Cache c;

  e.Register<CubeOp>();
  e.Register<TextOp>();
  e.Register<TriangulateOp>();
  e.Register<WriteOp>();
  e.Register<ExtrudeOp>();
  e.Register<ChamferOp>();

  //ParseAndProcess("N2.1N0S3LN2.1N4 SHL N-3.1N5 SHL N2.1N1SDLN-2.1N2S3L N-2.1 N3 SDL N1.2 N5C B E EEEEETW", e, c);
  ParseAndProcess("N2N5CBTW", e, c);

  std::println("Finished");
  return 0;
};
