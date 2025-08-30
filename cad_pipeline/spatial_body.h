#include <variant>
#include <brep.h>
#include <mesh.h>

using SpatialBody = std::variant<BRep, Mesh>;