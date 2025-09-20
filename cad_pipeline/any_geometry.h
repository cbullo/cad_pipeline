#pragma once

#include "brep.h"
#include "mesh.h"
#include "polygon.h"
#include "types.h"

using AnyGeometry =
    std::variant<std::shared_ptr<BRep>, std::shared_ptr<Mesh>,
                 std::shared_ptr<Polygon>, std::shared_ptr<Cube>>;
