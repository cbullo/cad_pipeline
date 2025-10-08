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

  static constexpr char sdf_normal[] = R"(
fn sdBoxNormal( p: vec3f, b: vec3f ) -> vec4f {
  let slack = b - abs(p);                   // ~0 on the touching face(s)
  // Pick the face with minimal slack (ties -> consistent pick).
  if (slack.x <= slack.y && slack.x <= slack.z) {
      return vec4f(signnz(p.x), 0.0, 0.0, 0.0);
  } else if (slack.y <= slack.z) {
      return vec4f(0.0, signnz(p.y), 0.0, 0.0);
  } else {
      return vec4f(0.0, 0.0, signnz(p.z), 0.0);
  }
})";

  static std::string Invocation(const Cube& c) {
    return std::format("v += sdBox(p, vec3<f32>({:.5f}, {:.5f}, {:.5f}));", c.half_extents[0],
                       c.half_extents[1], c.half_extents[2]);
  }

  static std::string NormalInvocation(const Cube& c) {
    return std::format("n = sdBoxNormal(p, vec3<f32>({:.5f}, {:.5f}, {:.5f}));", c.half_extents[0],
                       c.half_extents[1], c.half_extents[2]);
  }
};