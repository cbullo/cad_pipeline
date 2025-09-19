#include <format>
#include <string>

template <typename Shape>
struct SDF;

template <>
struct SDF<Cube> {
  static sdf_function = R"(
float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
})";

  std::string Invocation(const Cube& c) {
    std::format("sdBox(p, {})", c.half_extents);
  }
};