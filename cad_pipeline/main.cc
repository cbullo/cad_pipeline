#include "executor.h"
#include "interpreter.h"
#include "cube.h"
#include "primitives.h"

using CubeOp = Op<'C', MakeCube, float>;

int main() {
  Planner p;
  Scheduler s;
  Executor e;
  e.Register<CubeOp>();

  ParseAndProcess("N1CN2C", e);
  PlanAndSchedule(e.GetRequests(), p, s);

  return 0;
};