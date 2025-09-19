#include "sdf_draw.h"

#include "cube.h"
#include "types.h"
#include "visit_helper.h"

const char shaderTemplate[] = {
#embed <sdf.template.wgsl>
};

AnyGeometry SDFDraw(const AnyGeometry &geometry) {
  return std::visit(
      overloaded{
          [&geometry](const std::shared_ptr<Cube> &mesh) { return geometry; },
          [&geometry](const auto &) { return geometry; }},

      geometry);
}