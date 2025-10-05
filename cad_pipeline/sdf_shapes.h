#include <format>
#include <string>

template <typename Shape>
struct SDF;

template <>
struct SDF<Cube> {
  static constexpr char sdf_function[] = R"(
fn sdBox( p: vec3<f32>, b: vec3<f32> ) -> f32 {
  let q: vec3<f32> = abs(p) - b;
  return length(max(q,vec3<f32>(0.0))) + min(max(q.x,max(q.y,q.z)),0.0);
})";

  static std::string Invocation(const Cube& c) {
    return std::format("v += sdBox(p, vec3<f32>({:.5f}, {:.5f}, {:.5f}));", c.half_extents[0],
                       c.half_extents[1], c.half_extents[2]);
  }
};