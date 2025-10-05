#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "webgpu/wgpu.h"

// -------- utilities ----------
inline void hash_combine(std::size_t& h, std::size_t v) {
  h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
}
inline std::size_t hash_string_view(WGPUStringView sv) {
  // FNV-1a 64
  std::size_t h = 1469598103934665603ULL;
  const unsigned char* p = reinterpret_cast<const unsigned char*>(sv.data);
  for (size_t i = 0; i < sv.length; ++i) {
    h ^= p[i];
    h *= 1099511628211ULL;
  }
  hash_combine(h, sv.length);
  return h;
}
inline std::size_t hash_float_bits(float f) {
  std::uint32_t u;
  std::memcpy(&u, &f, sizeof u);
  return static_cast<std::size_t>(u);
}
inline void hash_u64(std::size_t& h, std::uint64_t v) {
  hash_combine(h, static_cast<std::size_t>(v));
#if SIZE_MAX < UINT64_MAX
  hash_combine(h, static_cast<std::size_t>(v >> 32));
#endif
}

// --- Iterator form (runtime length) ---
template <class It>
std::size_t hash_pointers(It first, It last) {
  std::size_t h = 0xcbf29ce484222325ULL;
  for (; first != last; ++first) {
    hash_combine(h, reinterpret_cast<std::size_t>(*first));
  }
  return h;
}

// --- Convenience for vector ---
template <typename T>
inline std::size_t hash_pointers(const std::vector<T*>& ptrs) {
  return hash_pointers(ptrs.begin(), ptrs.end());
}

// inline std::size_t hash_string_view(WGPUStringView sv) {
//   // FNV-1a 64 over bytes
//   std::size_t h = 1469598103934665603ULL;
//   const unsigned char* p = reinterpret_cast<const unsigned char*>(sv.data);
//   for (size_t i = 0; i < sv.length; ++i) {
//     h ^= p[i];
//     h *= 1099511628211ULL;
//   }
//   hash_combine(h, sv.length);
//   return h;
// }

// -------- vertex state (ignore constants) ----------
inline std::size_t hash_vertex_attribute(const WGPUVertexAttribute& a) {
  std::size_t h = 0;
  hash_combine(h, static_cast<std::size_t>(a.format));
  hash_u64(h, a.offset);
  hash_combine(h, static_cast<std::size_t>(a.shaderLocation));
  return h;
}

inline std::size_t hash_vertex_buffer_layout(const WGPUVertexBufferLayout& b) {
  std::size_t h = 0;
  hash_u64(h, b.arrayStride);
  hash_combine(h, static_cast<std::size_t>(b.stepMode));

  // Canonicalize attributes by shaderLocation so attribute order doesn't matter
  std::vector<WGPUVertexAttribute> attrs;
  attrs.reserve(b.attributeCount);
  for (uint32_t i = 0; i < b.attributeCount; ++i)
    attrs.push_back(b.attributes[i]);
  std::sort(attrs.begin(), attrs.end(), [](auto const& x, auto const& y) {
    return x.shaderLocation < y.shaderLocation;
  });
  for (auto const& a : attrs) hash_combine(h, hash_vertex_attribute(a));
  hash_combine(h, static_cast<std::size_t>(b.attributeCount));
  return h;
}

inline std::size_t hash_vertex_state_wo_consts(const WGPUVertexState& v) {
  std::size_t h = 0xcbf29ce484222325ULL;
  hash_combine(h, reinterpret_cast<std::size_t>(v.module));
  hash_combine(h, hash_string_view(v.entryPoint));
  // buffers are ordered by slot index
  hash_combine(h, static_cast<std::size_t>(v.bufferCount));
  for (uint32_t i = 0; i < v.bufferCount; ++i) {
    hash_combine(h, i);
    hash_combine(h, hash_vertex_buffer_layout(v.buffers[i]));
  }
  return h;
}

// -------- primitive ----------
inline std::size_t hash_primitive_state(const WGPUPrimitiveState& p) {
  std::size_t h = 0;
  hash_combine(h, static_cast<std::size_t>(p.topology));
  hash_combine(h, static_cast<std::size_t>(p.stripIndexFormat));
  hash_combine(h, static_cast<std::size_t>(p.frontFace));
  hash_combine(h, static_cast<std::size_t>(p.cullMode));
#if defined(__cpp_consteval) || 1
  // Many headers include 'unclippedDepth'; include if present in your SDK
  // If your header doesn't have it, just remove the next line.
  hash_combine(h, static_cast<std::size_t>(p.unclippedDepth));
#endif
  return h;
}

// -------- depth/stencil ----------
inline std::size_t hash_stencil_face(const WGPUStencilFaceState& s) {
  std::size_t h = 0;
  hash_combine(h, static_cast<std::size_t>(s.compare));
  hash_combine(h, static_cast<std::size_t>(s.failOp));
  hash_combine(h, static_cast<std::size_t>(s.depthFailOp));
  hash_combine(h, static_cast<std::size_t>(s.passOp));
  return h;
}
inline std::size_t hash_depth_stencil_state(const WGPUDepthStencilState& d) {
  std::size_t h = 0;
  hash_combine(h, static_cast<std::size_t>(d.format));
  hash_combine(h, static_cast<std::size_t>(d.depthWriteEnabled));
  hash_combine(h, static_cast<std::size_t>(d.depthCompare));
  hash_combine(h, hash_stencil_face(d.stencilFront));
  hash_combine(h, hash_stencil_face(d.stencilBack));
  hash_combine(h, static_cast<std::size_t>(d.stencilReadMask));
  hash_combine(h, static_cast<std::size_t>(d.stencilWriteMask));
  hash_combine(h, static_cast<std::size_t>(d.depthBias));
  hash_combine(h, hash_float_bits(d.depthBiasSlopeScale));
  hash_combine(h, hash_float_bits(d.depthBiasClamp));
  return h;
}

// -------- multisample ----------
inline std::size_t hash_multisample_state(const WGPUMultisampleState& m) {
  std::size_t h = 0;
  hash_combine(h, static_cast<std::size_t>(m.count));
  hash_combine(h, static_cast<std::size_t>(m.mask));
  hash_combine(h, static_cast<std::size_t>(m.alphaToCoverageEnabled));
  return h;
}

// -------- fragment (ignore constants) ----------
inline std::size_t hash_blend_component(const WGPUBlendComponent& c) {
  std::size_t h = 0;
  hash_combine(h, static_cast<std::size_t>(c.operation));
  hash_combine(h, static_cast<std::size_t>(c.srcFactor));
  hash_combine(h, static_cast<std::size_t>(c.dstFactor));
  return h;
}
inline std::size_t hash_blend_state(const WGPUBlendState& b) {
  std::size_t h = 0;
  hash_combine(h, hash_blend_component(b.color));
  hash_combine(h, hash_blend_component(b.alpha));
  return h;
}
inline std::size_t hash_color_target(const WGPUColorTargetState& t) {
  std::size_t h = 0;
  hash_combine(h, static_cast<std::size_t>(t.format));
  if (t.blend) {
    hash_combine(h, 1);
    hash_combine(h, hash_blend_state(*t.blend));
  } else {
    hash_combine(h, 0);
  }
  hash_combine(h, static_cast<std::size_t>(t.writeMask));
  return h;
}
inline std::size_t hash_fragment_state_wo_consts(const WGPUFragmentState& f) {
  std::size_t h = 0xcbf29ce484222325ULL;
  hash_combine(h, reinterpret_cast<std::size_t>(f.module));
  hash_combine(h, hash_string_view(f.entryPoint));
  // targets are ordered by color attachment index
  hash_combine(h, static_cast<std::size_t>(f.targetCount));
  for (uint32_t i = 0; i < f.targetCount; ++i) {
    hash_combine(h, i);
    hash_combine(h, hash_color_target(f.targets[i]));
  }
  return h;
}

// -------- final: render pipeline descriptor (ignore constants) ----------
inline std::size_t hash_render_pipeline_desc_wo_consts(
    const WGPURenderPipelineDescriptor& d) {
  std::size_t h = 0xcbf29ce484222325ULL;

  // layout (nullable)
  hash_combine(h, reinterpret_cast<std::size_t>(d.layout));

  // vertex stage (module + entry + buffers/attrs)
  hash_combine(h, hash_vertex_state_wo_consts(d.vertex));

  // primitive
  hash_combine(h, hash_primitive_state(d.primitive));

  // depth/stencil (nullable)
  if (d.depthStencil) {
    hash_combine(h, 1);
    hash_combine(h, hash_depth_stencil_state(*d.depthStencil));
  } else {
    hash_combine(h, 0);
  }

  // multisample
  hash_combine(h, hash_multisample_state(d.multisample));

  // fragment (nullable; ignore constants)
  if (d.fragment) {
    hash_combine(h, 1);
    hash_combine(h, hash_fragment_state_wo_consts(*d.fragment));
  } else {
    hash_combine(h, 0);
  }

  return h;
}

// --- hash the stage WITHOUT constants ---
inline std::size_t hash_stage_wo_consts(
    const WGPUProgrammableStageDescriptor& s) {
  std::size_t h = 0xcbf29ce484222325ULL;
  hash_combine(
      h, reinterpret_cast<std::size_t>(s.module));  // shader module handle
  hash_combine(h, hash_string_view(s.entryPoint));  // entry point name
  return h;
}

// --- final: compute-pipeline descriptor WITHOUT constants ---
inline std::size_t hash_compute_pipeline_desc_wo_consts(
    const WGPUComputePipelineDescriptor& d) {
  std::size_t h = 0xcbf29ce484222325ULL;
  hash_combine(h,
               reinterpret_cast<std::size_t>(d.layout));  // nullable allowed
  hash_combine(h, hash_stage_wo_consts(d.compute));
  return h;
}

inline std::size_t hash_bind_group_entry(const WGPUBindGroupEntry& e) {
  std::size_t h = 0;
  // Binding index always matters
  hash_combine(h, static_cast<std::size_t>(e.binding));

  // Exactly one of these should be set; fold a tag + handle
  if (e.buffer) {
    hash_combine(h, 0xb00f);  // tag: buffer
    hash_combine(h, reinterpret_cast<std::size_t>(e.buffer));
    hash_combine(h, static_cast<std::size_t>(e.offset));
    hash_combine(h, static_cast<std::size_t>(e.size));
  } else if (e.textureView) {
    hash_combine(h, 0x7ef7);  // tag: texture view
    hash_combine(h, reinterpret_cast<std::size_t>(e.textureView));
  } else if (e.sampler) {
    hash_combine(h, 0x5a9d);  // tag: sampler
    hash_combine(h, reinterpret_cast<std::size_t>(e.sampler));
  } else {
    // If you ever use chain extensions, fold them here as well
    hash_combine(h, 0xdead);  // empty/unexpected
  }
  return h;
}

inline std::size_t hash_bind_group_desc(const WGPUBindGroupDescriptor& d) {
  std::size_t h = 0xcbf29ce484222325ULL;  // a seed
  // Layout handle

  hash_combine(h, reinterpret_cast<std::size_t>(d.layout));
  // Entry count
  hash_combine(h, static_cast<std::size_t>(d.entryCount));

  // If your entries might arrive out-of-order, either sort a copy by
  // `binding` first, or also fold the binding (we already do) so order
  // changes won’t collide.
  for (uint32_t i = 0; i < d.entryCount; ++i)
    hash_combine(h, hash_bind_group_entry(d.entries[i]));

  return h;
}
