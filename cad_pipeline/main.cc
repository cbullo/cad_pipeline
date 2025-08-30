#include "executor.h"
#include "interpreter.h"
#include "primitives.h"

int main() {
  Executor ex;
  ParseAndExecute("N1N2N3C", ex);

  return 0;
};