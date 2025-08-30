#include <variant>
#include <memory>

#include "pmp/types.h"


using Scalar = double;
using Vector3 = pmp::Vector<Scalar, 3>;

using Resource = std::variant<std::shared_ptr<BRep>, std::shared_ptr<Mesh>, std::shared_ptr<Polygon>>>;