#pragma once

#include <memory>
#include <variant>

#include "brep.h"
#include "mesh.h"
#include "pmp/types.h"
#include "polygon.h"

using Scalar = float;
using Vector3 = pmp::Vector<Scalar, 3>;

// TODO: Don't use strings as ids, convert to a fixed length hash, e.g. MD5
using GeomId = std::string;

using AnyGeometry = std::variant<std::shared_ptr<BRep>, std::shared_ptr<Mesh>,
                                 std::shared_ptr<Polygon>>;
