#pragma once

#include <memory>
#include <variant>

#include "Eigen/Core"
#include "pmp/types.h"

using Scalar = float;
using Vector3 = pmp::Vector<Scalar, 3>;

// TODO: Don't use strings as ids, convert to a fixed length hash, e.g. MD5
using GeomId = std::string;

struct Cube {
  Eigen::Vector3f half_extents;
};
