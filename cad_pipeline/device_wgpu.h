#pragma once

#include <print>
#include <string>
#include <unordered_map>
#include <vector>

#include "webgpu/wgpu.h"

// Standard hash_combine (from Boost)
inline void hash_combine(std::size_t& seed, std::size_t v) {
  seed ^= v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

// // --- Variadic form (compile-time length) ---
// template <class... Ptrs>
// std::size_t hash_pointers(Ptrs... ptrs) {
//   std::size_t h = 0xcbf29ce484222325ULL;  // some seed
//   (hash_combine(h, reinterpret_cast<std::size_t>(ptrs)), ...);
//   return h;
// }

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

class DeviceWGPU {
 public:
  using Device = WGPUDevice;
  using Shader = WGPUShaderModule;
  using Buffer = WGPUBuffer;
  using BufferUsage = WGPUBufferUsage;
  using PipelineLayout = WGPUPipelineLayout;
  // using LayoutBuilder = LayoutBuilderWGPU<DeviceWGPU>;

  DeviceWGPU(WGPUDevice dev) { device_ = dev; }
  WGPUShaderModule Compile(const std::string& name, const std::string& source);

  WGPUBuffer CreateBuffer(const std::string& name, WGPUBufferUsage usage,
                          size_t size);

  template <typename... GroupTuple>
  void BindGroups(DeviceWGPU::Shader shader,
                  const GroupTuple&... bind_group_tuple);

  void BeginFrame() {
    auto command_encoder_descriptor = WGPUCommandEncoderDescriptor{
        .label = {"command_encoder", WGPU_STRLEN},
    };

    current_encoder =
        wgpuDeviceCreateCommandEncoder(device_, &command_encoder_descriptor);
    // assert(current_encoder);
  }

  void BeginPass() {
    WGPUComputePassDescriptor desc{
        .label = {"compute_pass", WGPU_STRLEN},
    };
    current_pass_encoder =
        wgpuCommandEncoderBeginComputePass(current_encoder, &desc);
  }
  void EndPass() {
    wgpuComputePassEncoderDispatchWorkgroups(current_pass_encoder, 1024, 1024, 1);

    wgpuComputePassEncoderEnd(current_pass_encoder);
    wgpuComputePassEncoderRelease(current_pass_encoder);
    current_pass_encoder = nullptr;
  }

  void EndFrame() {
    WGPUCommandBufferDescriptor desc{
      .label = {"command_buffer", WGPU_STRLEN},
    };
    auto command_buffer = wgpuCommandEncoderFinish(current_encoder, &desc);
    //wgpuCommandEncoderRelease(current_encoder);
    //current_encoder = nullptr;

    auto queue = wgpuDeviceGetQueue(device_);
    wgpuQueueSubmit(queue, 1, &command_buffer);

    std::println("HERE1");
    wgpuDevicePoll(device_, true, nullptr);
    std::println("HERE2");
    //wgpuCommandBufferRelease(command_buffer);
  }

 private:
  Device device_;
  WGPUCommandEncoder current_encoder = nullptr;
  WGPUComputePassEncoder current_pass_encoder = nullptr;
  std::unordered_map<size_t, WGPUComputePipeline> pipelines_cache;
};

template <typename Device>
class LayoutBuilderWGPU {
 public:
  template <typename Tuple>
  LayoutBuilderWGPU& BindGroup(typename Device::Device device) {
    return BindGroupImpl<Tuple>(
        device, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
  }

  typename Device::PipelineLayout Finalize(typename Device::Device device) {
    auto layouts_hash = hash_pointers(layouts_);
    WGPUPipelineLayout pipeline_layout = nullptr;

    auto it = pipeline_layouts.find(layouts_hash);
    if (it == pipeline_layouts.end()) {
      WGPUPipelineLayoutDescriptor layout_descriptor{
          .bindGroupLayoutCount = layouts_.size(),
          .bindGroupLayouts = &layouts_[0],
      };
      pipeline_layout =
          wgpuDeviceCreatePipelineLayout(device, &layout_descriptor);

    } else {
      pipeline_layout = it->second;
    }

    return pipeline_layout;
  }

  const std::vector<WGPUBindGroupLayout>& GetBindGroupLayouts() const {
    return layouts_;
  };

 private:
  template <typename Tuple, std::size_t... I>
  LayoutBuilderWGPU& BindGroupImpl(typename Device::Device device,
                                   std::index_sequence<I...>);
  static std::unordered_map<size_t, WGPUPipelineLayout> pipeline_layouts;
  std::vector<WGPUBindGroupLayout> layouts_;
};

template <typename Device>
class GroupsBinderWGPU {
 public:
  template <typename Tuple>
  GroupsBinderWGPU& BindGroup(typename Device::Device device,
                              WGPUBindGroupLayout bind_group_layout,
                              WGPUComputePassEncoder encoder,
                              const Tuple& group, int group_index) {
    return BindGroupImpl<Tuple>(
        device, bind_group_layout, group, group_index, encoder,
        std::make_index_sequence<std::tuple_size_v<Tuple>>{});
  }

 private:
  template <typename Tuple, std::size_t... I>
  GroupsBinderWGPU& BindGroupImpl(typename Device::Device device,
                                  WGPUBindGroupLayout bind_group_layout,
                                  const Tuple& group, int group_index,
                                  WGPUComputePassEncoder encoder,
                                  std::index_sequence<I...>);
  static std::unordered_map<size_t, WGPUPipelineLayout> pipeline_layouts;
  std::vector<WGPUBindGroupLayout> layouts_;
};

template <typename Device>
std::unordered_map<size_t, WGPUPipelineLayout>
    LayoutBuilderWGPU<Device>::pipeline_layouts;

inline std::size_t hash_string_view(WGPUStringView sv) {
  // FNV-1a 64 over bytes
  std::size_t h = 1469598103934665603ULL;
  const unsigned char* p = reinterpret_cast<const unsigned char*>(sv.data);
  for (size_t i = 0; i < sv.length; ++i) {
    h ^= p[i];
    h *= 1099511628211ULL;
  }
  hash_combine(h, sv.length);
  return h;
}

// --- hash the stage WITHOUT constants ---
inline std::size_t hash_stage_wo_consts(
    const WGPUProgrammableStageDescriptor& s) {
  std::size_t h = 0xcbf29ce484222325ULL;
  hash_combine(
      h, reinterpret_cast<std::size_t>(s.module));  // shader module handle
  // hash_combine(h, hash_string_view(s.entryPoint));  // entry point name
  return h;
}

// --- final: compute-pipeline descriptor WITHOUT constants ---
inline std::size_t hash_compute_pipeline_desc_wo_consts(
    const WGPUComputePipelineDescriptor& d) {
  std::size_t h = 0xcbf29ce484222325ULL;
  hash_combine(h, reinterpret_cast<std::size_t>(d.layout));  // nullable allowed
  hash_combine(h, hash_stage_wo_consts(d.compute));
  return h;
}

template <typename... GroupTuple>
void DeviceWGPU::BindGroups(DeviceWGPU::Shader shader,
                            const GroupTuple&... bind_group_tuple) {
  LayoutBuilderWGPU<DeviceWGPU> layout_builder;

  (layout_builder.BindGroup<GroupTuple>(device_), ...);
  auto pipeline_layout = layout_builder.Finalize(device_);

  WGPUProgrammableStageDescriptor desc{.module = shader,
                                       .entryPoint = {"main", WGPU_STRLEN},
                                       .constantCount = 0};

  auto pipeline_descriptor = WGPUComputePipelineDescriptor{
      .layout = pipeline_layout,
      .compute = desc,
  };

  auto pipeline_hash =
      hash_compute_pipeline_desc_wo_consts(pipeline_descriptor);

  std::println("HERE");
  WGPUComputePipeline pipeline = nullptr;
  auto it = this->pipelines_cache.find(pipeline_hash);
  if (it == pipelines_cache.end()) {
    pipeline = wgpuDeviceCreateComputePipeline(device_, &pipeline_descriptor);
    pipelines_cache[pipeline_hash] = pipeline;
  } else {
    pipeline = it->second;
  }

  wgpuComputePassEncoderSetPipeline(current_pass_encoder, pipeline);

  GroupsBinderWGPU<DeviceWGPU> binder;
  (binder.BindGroup(device_, layout_builder.GetBindGroupLayouts()[0],
                    current_pass_encoder, bind_group_tuple, 0),
   ...);
}

template <typename Device, typename TypeToMap>
struct BindingType {};

template <>
struct BindingType<DeviceWGPU, typename DeviceWGPU::Buffer> {
  static const WGPUBufferBindingType binding_type =
      WGPUBufferBindingType_Storage;
};

template <typename Device>
template <typename Tuple, std::size_t... I>
LayoutBuilderWGPU<Device>& LayoutBuilderWGPU<Device>::BindGroupImpl(
    typename Device::Device device, std::index_sequence<I...>) {
  static std::unordered_map<typename Device::Device, WGPUBindGroupLayout>
      bind_group_layouts;

  auto it = bind_group_layouts.find(device);
  if (it != bind_group_layouts.end()) {
    layouts_.push_back(it->second);
    return *this;
  }

  // auto bindings_array[] = {Bindings...};
  WGPUBindGroupLayoutEntry entries[] = {WGPUBindGroupLayoutEntry{
      .binding = I,
      .visibility = WGPUShaderStage_Compute,
      .buffer =
          {
              .type = BindingType<Device, typename std::tuple_element<
                                              I, Tuple>::type>::binding_type,
          },
  }...};

  WGPUBindGroupLayoutDescriptor bind_group_descriptor = {
      .label = {"bind_group_layout", WGPU_STRLEN},
      .nextInChain = NULL,
      .entryCount = sizeof(entries) / sizeof(entries[0]),
      .entries = entries,
  };

  WGPUBindGroupLayout bind_group_layout =
      wgpuDeviceCreateBindGroupLayout(device, &bind_group_descriptor);

  bind_group_layouts[device] = bind_group_layout;
  layouts_.push_back(bind_group_layout);

  return *this;
}

// inline void hash_combine(std::size_t& h, std::size_t v) {
//   h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
// }

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

template <typename Device>
template <typename Tuple, std::size_t... I>
GroupsBinderWGPU<Device>& GroupsBinderWGPU<Device>::BindGroupImpl(
    typename Device::Device device, WGPUBindGroupLayout bind_group_layout,
    const Tuple& group, int group_index, WGPUComputePassEncoder encoder,
    std::index_sequence<I...>) {
  WGPUBindGroupEntry entries[] = {
      WGPUBindGroupEntry{.binding = I,
                         .buffer = std::get<I>(group),
                         .offset = 0,
                         .size = wgpuBufferGetSize(std::get<I>(group))}...};

  auto bind_group_description = WGPUBindGroupDescriptor{
      .label = {"bind_group", WGPU_STRLEN},
      .layout = bind_group_layout,
      .entryCount = sizeof(entries) / sizeof(entries[0]),
      .entries = entries};

  auto hash = hash_bind_group_desc(bind_group_description);
  static std::unordered_map<size_t, WGPUBindGroup> bind_groups;
  WGPUBindGroup bind_group = nullptr;
  auto it = bind_groups.find(hash);

  if (it == bind_groups.end()) {
    bind_group = wgpuDeviceCreateBindGroup(device, &bind_group_description);
    bind_groups[hash] = bind_group;
  } else {
    bind_group = it->second;
  }

  wgpuComputePassEncoderSetBindGroup(encoder, group_index, bind_group, 0,
                                     nullptr);

  return *this;
}