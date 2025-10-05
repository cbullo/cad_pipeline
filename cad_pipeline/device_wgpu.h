#pragma once

#include <initializer_list>
#include <optional>
#include <print>
#include <string>
#include <unordered_map>
#include <vector>

#include "hash_webgpu.h"
#include "webgpu/wgpu.h"

// // --- Variadic form (compile-time length) ---
// template <class... Ptrs>
// std::size_t hash_pointers(Ptrs... ptrs) {
//   std::size_t h = 0xcbf29ce484222325ULL;  // some seed
//   (hash_combine(h, reinterpret_cast<std::size_t>(ptrs)), ...);
//   return h;
// }

// void frmwrk_setup_logging(WGPULogLevel level) {

//}

enum class GPUPassType { Compute, Render };

template <class... Ts>
struct Groups {
  //  std::tuple<Ts...> t;
};
// template <class... Ts>
// Groups(Ts...) -> Groups<Ts...>;

template <class... Ts>
struct Attrs {
  //  std::tuple<Ts...> t;
};
// template <class... Ts>
// Attrs(Ts...) -> Attrs<Ts...>;

class DeviceWGPU {
 public:
  using Device = WGPUDevice;
  using Shader = WGPUShaderModule;
  using Buffer = WGPUBuffer;
  using BufferUsage = WGPUBufferUsage;
  using PipelineLayout = WGPUPipelineLayout;
  // using LayoutBuilder = LayoutBuilderWGPU<DeviceWGPU>;

  DeviceWGPU(WGPUDevice dev) { device_ = dev; }

  static inline int GetVertexFormatSize(WGPUVertexFormat format) {
    switch (format) {
      case WGPUVertexFormat_Uint8:
      case WGPUVertexFormat_Sint8:
      case WGPUVertexFormat_Unorm8:
      case WGPUVertexFormat_Snorm8:
        return 1;

      case WGPUVertexFormat_Uint8x2:
      case WGPUVertexFormat_Sint8x2:
      case WGPUVertexFormat_Unorm8x2:
      case WGPUVertexFormat_Snorm8x2:
      case WGPUVertexFormat_Sint16:
      case WGPUVertexFormat_Uint16:
      case WGPUVertexFormat_Unorm16:
      case WGPUVertexFormat_Snorm16:
      case WGPUVertexFormat_Float16:
        return 2;

      case WGPUVertexFormat_Uint8x4:
      case WGPUVertexFormat_Sint8x4:
      case WGPUVertexFormat_Unorm8x4:
      case WGPUVertexFormat_Snorm8x4:
      case WGPUVertexFormat_Uint16x2:
      case WGPUVertexFormat_Sint16x2:
      case WGPUVertexFormat_Unorm16x2:
      case WGPUVertexFormat_Snorm16x2:
      case WGPUVertexFormat_Float16x2:
      case WGPUVertexFormat_Float32:
      case WGPUVertexFormat_Uint32:
      case WGPUVertexFormat_Sint32:
      case WGPUVertexFormat_Unorm10_10_10_2:
      case WGPUVertexFormat_Unorm8x4BGRA:
        return 4;

      case WGPUVertexFormat_Uint16x4:
      case WGPUVertexFormat_Sint16x4:
      case WGPUVertexFormat_Unorm16x4:
      case WGPUVertexFormat_Snorm16x4:
      case WGPUVertexFormat_Float16x4:
      case WGPUVertexFormat_Float32x2:
      case WGPUVertexFormat_Uint32x2:
      case WGPUVertexFormat_Sint32x2:
        return 8;

      case WGPUVertexFormat_Float32x3:
      case WGPUVertexFormat_Uint32x3:
      case WGPUVertexFormat_Sint32x3:
        return 12;

      case WGPUVertexFormat_Float32x4:
      case WGPUVertexFormat_Uint32x4:
      case WGPUVertexFormat_Sint32x4:
        return 16;
    }
  }

  WGPUShaderModule Compile(const std::string& name, const std::string& source);

  WGPUBuffer CreateBuffer(const std::string& name, WGPUBufferUsage usage,
                          size_t size);

  template <typename... GroupTuple>
  void BindGroups(DeviceWGPU::Shader shader, const std::string& entry_point,
                  const GroupTuple&... bind_group_tuple);

  template <typename... GroupTuple, typename... AttributeTuple>
  void BindGroups(DeviceWGPU::Shader vertex_shader,
                  DeviceWGPU::Shader fragment_shader,
                  Groups<const GroupTuple&...> bind_group_tuple,
                  Attrs<const AttributeTuple&...> attribute_group_tuple);
  void BeginFrame() {
    auto command_encoder_descriptor = WGPUCommandEncoderDescriptor{
        .label = {"command_encoder", WGPU_STRLEN},
    };

    current_encoder =
        wgpuDeviceCreateCommandEncoder(device_, &command_encoder_descriptor);
  }

  void DrawIndirect(Buffer buffer, int offset = 0) {
    wgpuRenderPassEncoderDrawIndirect(current_render_pass_encoder, buffer,
                                      offset);
  }

  void DrawIndirectIndexed(Buffer buffer, int offset = 0) {
    wgpuRenderPassEncoderDrawIndexedIndirect(current_render_pass_encoder,
                                             buffer, offset);
  }

  void BeginComputePass() {
    WGPUComputePassDescriptor desc{
        .label = {"compute_pass", WGPU_STRLEN},
    };
    current_pass_encoder =
        wgpuCommandEncoderBeginComputePass(current_encoder, &desc);
  }

  void BeginRenderPass(WGPUTexture target) {
    WGPUTextureView frame = wgpuTextureCreateView(target, NULL);

    WGPURenderPassColorAttachment color_attachment{
        .view = frame,
        .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue =
            (const WGPUColor){
                .r = 1.0,
                .g = 1.0,
                .b = 0.0,
                .a = 1.0,
            },
    };

    WGPURenderPassDescriptor render_pass_desc{
        .label = {"render_pass_encoder", WGPU_STRLEN},
        .colorAttachmentCount = 1,
        .colorAttachments = &color_attachment

    };

    current_render_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(current_encoder, &render_pass_desc);

    wgpuTextureViewRelease(frame);
  }

  void DispatchWorgroups(int x, int y, int z) {
    wgpuComputePassEncoderDispatchWorkgroups(current_pass_encoder, x, y, z);
  }

  template <GPUPassType type>
  void EndPass();

  template <>
  void EndPass<GPUPassType::Compute>() {
    wgpuComputePassEncoderEnd(current_pass_encoder);
    wgpuComputePassEncoderRelease(current_pass_encoder);
    current_pass_encoder = nullptr;
  }

  template <>
  void EndPass<GPUPassType::Render>() {
    wgpuRenderPassEncoderEnd(current_render_pass_encoder);
    wgpuRenderPassEncoderRelease(current_render_pass_encoder);
    current_render_pass_encoder = nullptr;
  }

  void EndFrame() {
    WGPUCommandBufferDescriptor desc{
        .label = {"command_buffer", WGPU_STRLEN},
    };
    auto command_buffer = wgpuCommandEncoderFinish(current_encoder, &desc);
    wgpuCommandEncoderRelease(current_encoder);
    current_encoder = nullptr;

    auto queue = wgpuDeviceGetQueue(device_);
    wgpuQueueSubmit(queue, 1, &command_buffer);

    // wgpuDevicePoll(device_, true, nullptr);
    wgpuCommandBufferRelease(command_buffer);
  }

  Device GetDevice() { return device_; };
  WGPURenderPassEncoder GetRenderEncoder() {
    return current_render_pass_encoder;
  }

 private:
  Device device_;
  WGPUCommandEncoder current_encoder = nullptr;
  WGPUComputePassEncoder current_pass_encoder = nullptr;
  WGPURenderPassEncoder current_render_pass_encoder = nullptr;
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
          .nextInChain = nullptr,
          .bindGroupLayoutCount = layouts_.size(),
          .bindGroupLayouts = &layouts_[0],
      };

      pipeline_layout =
          wgpuDeviceCreatePipelineLayout(device, &layout_descriptor);
      pipeline_layouts[layouts_hash] = pipeline_layout;
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

template <typename... GroupTuple>
void DeviceWGPU::BindGroups(DeviceWGPU::Shader shader,
                            const std::string& entry_point,
                            const GroupTuple&... bind_group_tuple) {
  LayoutBuilderWGPU<DeviceWGPU> layout_builder;

  (layout_builder.BindGroup<GroupTuple>(device_), ...);
  auto pipeline_layout = layout_builder.Finalize(device_);

  WGPUProgrammableStageDescriptor desc{
      .nextInChain = nullptr,
      .module = shader,
      .entryPoint = {entry_point.data(), entry_point.size()},
  };

  std::println("{}", (uint64_t)pipeline_layout);
  auto pipeline_descriptor = WGPUComputePipelineDescriptor{
      .nextInChain = nullptr,
      .label = {"compute_pipeline", WGPU_STRLEN},
      .layout = pipeline_layout,
      .compute = desc,
  };

  auto pipeline_hash =
      hash_compute_pipeline_desc_wo_consts(pipeline_descriptor);
  std::println("pipeline_hash {}", pipeline_hash);

  WGPUComputePipeline pipeline = nullptr;
  auto it = pipelines_cache.find(pipeline_hash);
  if (it == pipelines_cache.end()) {
    pipeline = wgpuDeviceCreateComputePipeline(device_, &pipeline_descriptor);
    pipelines_cache[pipeline_hash] = pipeline;
  } else {
    pipeline = it->second;
  }

  wgpuComputePassEncoderSetPipeline(current_pass_encoder, pipeline);

  int i = 0;
  GroupsBinderWGPU<DeviceWGPU> binder;
  (
      [&bind_group_tuple, &layout_builder, &binder, this](int i) {
        binder.BindGroup(device_, layout_builder.GetBindGroupLayouts()[i],
                         current_pass_encoder, bind_group_tuple, i);
      }(i++),
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
      .nextInChain = nullptr,
      .binding = I,
      .visibility = WGPUShaderStage_Compute,
      .buffer =
          WGPUBufferBindingLayout{
              .nextInChain = nullptr,
              .type = BindingType<Device, typename std::tuple_element<
                                              I, Tuple>::type>::binding_type,
              .hasDynamicOffset = false,

          },
  }...};

  std::println("{}", sizeof(entries) / sizeof(entries[0]));

  WGPUBindGroupLayoutDescriptor bind_group_descriptor = {
      .nextInChain = NULL,
      .label = {"bind_group_layout", WGPU_STRLEN},
      .entryCount = sizeof(entries) / sizeof(entries[0]),
      .entries = entries,
  };

  WGPUBindGroupLayout bind_group_layout =
      wgpuDeviceCreateBindGroupLayout(device, &bind_group_descriptor);

  bind_group_layouts[device] = bind_group_layout;
  layouts_.push_back(bind_group_layout);

  return *this;
}

template <typename Device>
template <typename Tuple, std::size_t... I>
GroupsBinderWGPU<Device>& GroupsBinderWGPU<Device>::BindGroupImpl(
    typename Device::Device device, WGPUBindGroupLayout bind_group_layout,
    const Tuple& group, int group_index, WGPUComputePassEncoder encoder,
    std::index_sequence<I...>) {
  WGPUBindGroupEntry entries[] = {
      WGPUBindGroupEntry{.nextInChain = nullptr,
                         .binding = I,
                         .buffer = std::get<I>(group),
                         .offset = 0,
                         .size = wgpuBufferGetSize(std::get<I>(group)),
                         .sampler = nullptr,
                         .textureView = nullptr}...};

  auto bind_group_description = WGPUBindGroupDescriptor{
      .nextInChain = nullptr,
      .label = {"bind_group", WGPU_STRLEN},
      .layout = bind_group_layout,
      .entryCount = sizeof(entries) / sizeof(entries[0]),
      .entries = entries};

  auto hash = hash_bind_group_desc(bind_group_description);
  std::println("Bind groups count: {}", bind_group_description.entryCount);
  std::println("Bind group hash: {}", hash);
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

template <class Device>
class RenderPipelineBuilder {
 public:
  RenderPipelineBuilder()
      : descriptor_{}, depth_stencil_{}, fragment_state_{} {}

  RenderPipelineBuilder& AttachAttributeBuffer(
      DeviceWGPU::Buffer buffer,
      std::initializer_list<WGPUVertexAttribute> attributes, int offset = 0,
      int stride = -1) {
    attributes_per_buffer_.push_back(attributes);
    buffer_layouts_.push_back({.stepMode = WGPUVertexStepMode_Vertex,
                               .arrayStride = static_cast<uint64_t>(stride)});

    buffers_.push_back({buffer, offset});
    return *this;
  }
  RenderPipelineBuilder& SetVertexShader(WGPUShaderModule shader,
                                         const std::string& entry_point) {
    descriptor_.vertex.module = shader;
    descriptor_.vertex.entryPoint = {entry_point.data(), entry_point.size()};
    return *this;
  }
  RenderPipelineBuilder& SetFragmentShader(WGPUShaderModule shader,
                                           const std::string& entry_point) {
    fragment_state_.module = shader;
    fragment_state_.entryPoint = {entry_point.data(), entry_point.size()};
    return *this;
  }
  RenderPipelineBuilder& SetPrimitiveState(
      const WGPUPrimitiveState& primitive_state) {
    descriptor_.primitive = primitive_state;
    return *this;
  }
  RenderPipelineBuilder& SetIndexBuffer(DeviceWGPU::Buffer buffer,
                                        int offset = 0) {
    index_buffer_ = buffer;
    index_buffer_offset_ = offset;
    return *this;
  }

  RenderPipelineBuilder& SetDepthStencilState(
      const WGPUDepthStencilState& depth_stencil_state) {
    depth_stencil_ = depth_stencil_state;
    descriptor_.depthStencil = &depth_stencil_;
    return *this;
  }
  RenderPipelineBuilder& SetMultisampleState(
      const WGPUMultisampleState& multisample_state) {
    descriptor_.multisample = multisample_state;
    return *this;
  }
  RenderPipelineBuilder& AttachColorTarget(
      const WGPUColorTargetState& color_target_state) {
    color_targets_.push_back(color_target_state);
    return *this;
  }

  WGPURenderPipeline Finalize(Device& device) {
    UpdateDescriptorPointers();

    auto hash = hash_render_pipeline_desc_wo_consts(descriptor_);
    auto it = this->render_pipeline_cache_.find(hash);
    if (it == this->render_pipeline_cache_.end()) {
      auto pipeline =
          wgpuDeviceCreateRenderPipeline(device.GetDevice(), &descriptor_);
      it = this->render_pipeline_cache_.insert({hash, pipeline}).first;
    }

    wgpuRenderPassEncoderSetPipeline(device.GetRenderEncoder(), it->second);

    for (int i = 0; i < this->buffers_.size(); ++i) {
      wgpuRenderPassEncoderSetVertexBuffer(
          device.GetRenderEncoder(), i, buffers_[i].buffer, buffers_[i].offset,
          wgpuBufferGetSize(buffers_[i].buffer));
    }

    if (index_buffer_.has_value()) {
      wgpuRenderPassEncoderSetIndexBuffer(
          device.GetRenderEncoder(), *index_buffer_, WGPUIndexFormat_Uint32,
          index_buffer_offset_, wgpuBufferGetSize(*index_buffer_));
    }

    return it->second;
  }

 private:
  void UpdateDescriptorPointers() {
    descriptor_.fragment = &fragment_state_;
    if (!color_targets_.empty()) {
      fragment_state_.targets = color_targets_.data();
      fragment_state_.targetCount = color_targets_.size();
    }

    if (!buffer_layouts_.empty()) {
      int i = 0;
      int vertex_stride = 0;
      for (auto& bl : buffer_layouts_) {
        bl.attributes = attributes_per_buffer_[i].data();
        bl.attributeCount = attributes_per_buffer_[i].size();
        for (const auto& a : attributes_per_buffer_[i]) {
          vertex_stride += DeviceWGPU::GetVertexFormatSize(a.format);
        }
        if (bl.arrayStride == static_cast<uint64_t>(-1)) {
          bl.arrayStride = vertex_stride;
        }
        i++;
      }
      descriptor_.vertex.buffers = buffer_layouts_.data();
      descriptor_.vertex.bufferCount = buffer_layouts_.size();
    }
  }

  WGPURenderPipelineDescriptor descriptor_;

  WGPUDepthStencilState depth_stencil_;
  WGPUFragmentState fragment_state_;
  std::vector<WGPUColorTargetState> color_targets_;
  std::vector<WGPUVertexBufferLayout> buffer_layouts_;
  std::vector<std::vector<WGPUVertexAttribute>> attributes_per_buffer_;
  struct BufferInfo {
    WGPUBuffer buffer;
    int offset;
  };
  std::vector<BufferInfo> buffers_;
  std::optional<WGPUBuffer> index_buffer_;
  int index_buffer_offset_ = 0;

  static std::unordered_map<size_t, WGPURenderPipeline> render_pipeline_cache_;
};

template <class Device>
std::unordered_map<size_t, WGPURenderPipeline>
    RenderPipelineBuilder<Device>::render_pipeline_cache_;
