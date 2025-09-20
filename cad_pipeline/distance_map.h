#pragma once

#include "any_geometry.h"
#include "cache.h"
#include "types.h"

AnyGeometry DistanceMap(ExecutionContext &context, const AnyGeometry &geometry1,
                        const AnyGeometry &geometry2);