#pragma once

#include <memory>
#include <variant>

#include "brep.h"
#include "mesh.h"
#include "pmp/types.h"
#include "polygon.h"

using Scalar = float;
using Vector3 = pmp::Vector<Scalar, 3>;

// TODO: Not ideal
using GeomId = std::string;

using Resource = std::variant<std::shared_ptr<BRep>, std::shared_ptr<Mesh>,
                              std::shared_ptr<Polygon>>;