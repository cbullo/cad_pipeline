#include "sdf_draw.h"

#include <memory>
#include <string>

#include "any_geometry.h"
#include "boost/algorithm/string/replace.hpp"
#include "cache.h"
#include "cube.h"
#include "sdf_shapes.h"
#include "types.h"
#include "visit_helper.h"
#include "webgpu/wgpu.h"

AnyGeometry SDFDraw(ExecutionContext& execution_context,
                    const AnyGeometry& geometry) {
  std::println("SDFDraw()");
  static constexpr char shader_template[] = {
  // clang-format off
#embed <sdf.template.wgsl>
      // clang-format on
      , '\0'};

  constexpr auto fmt = shader_template;
  return std::visit(
      overloaded{
          [&execution_context, &geometry,
           &fmt](const std::shared_ptr<Cube>& cube) {
            std::string shader_src(shader_template);
            boost::algorithm::replace_all(shader_src, "//SDF_FUNCTIONS",
                                          SDF<Cube>::sdf_function);
            boost::algorithm::replace_all(shader_src, "//SDF_INVOCATIONS",
                                          SDF<Cube>::Invocation(*cube));
            //  const std::string& val2 = SDF<Cube>::Invocation(*cube);

            //  std::println("{}", shader_template);
            //  std::string shader_src = std::vformat(
            //      shader_template, std::make_format_args(val1, val2));

            auto shader = execution_context.shader_cache.GetShader(shader_src);

            return geometry;
          },
          [&geometry](const auto& geom) { return geometry; }},
      geometry);
}