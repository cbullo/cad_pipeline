#include "sdf_draw.h"

#include <memory>
#include <string>

#include "any_geometry.h"
#include "boost/algorithm/string/replace.hpp"
#include "cache.h"
#include "cube.h"
#include "device_wgpu.h"
#include "sdf_shapes.h"
#include "types.h"
#include "visit_helper.h"
#include "webgpu/wgpu.h"

using SDFBindGroup = std::tuple<DeviceWGPU::Buffer, DeviceWGPU::Buffer>;

AnyGeometry SDFDraw(ExecutionContext& execution_context,
                    const AnyGeometry& geometry) {
  // std::println("SDFDraw()");
  static constexpr char compute_shader_template[] = {
  // clang-format off
#embed <sdf.compute.template.wgsl>
      // clang-format on
      , '\0'};

  static constexpr char render_shader_src_raw[] = {
  // clang-format off
#embed <sdf.render.wgsl>
      // clang-format on
      , '\0'};

  return std::visit(
      overloaded{
          [&execution_context, &geometry](const std::shared_ptr<Cube>& cube) {
            std::string compute_shader_src(compute_shader_template);
            boost::algorithm::replace_all(compute_shader_src, "//SDF_FUNCTIONS",
                                          SDF<Cube>::sdf_function);
            boost::algorithm::replace_all(compute_shader_src,
                                          "//SDF_INVOCATIONS",
                                          SDF<Cube>::Invocation(*cube));

            boost::algorithm::replace_all(compute_shader_src,
                                          "//SDF_NORMALS_FUNCTIONS",
                                          SDF<Cube>::sdf_normal);
            boost::algorithm::replace_all(compute_shader_src, "//SDF_NORMALS",
                                          SDF<Cube>::NormalInvocation(*cube));

            auto compute_shader =
                execution_context.shader_cache.GetShader(compute_shader_src);

            std::string render_shader_src(render_shader_src_raw);
            auto render_shader =
                execution_context.shader_cache.GetShader(render_shader_src);

            // std::println("s{}", (uint64_t)shader);
            struct {
              int x = 1024;
              int y = 1024;
            } resolution;

            auto internal_triangles =
                execution_context.buffer_pool.ReserveBuffer(
                    WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect |
                        WGPUBufferUsage_Index,
                    20 + 2 * 3 * 4 * (resolution.x / 16) * (resolution.y / 16));

            auto boundary_triangles =
                execution_context.buffer_pool.ReserveBuffer(
                    WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect |
                        WGPUBufferUsage_Index,
                    20 + 2 * 3 * 4 * (resolution.x / 16) * (resolution.y / 16));

            auto vertices = execution_context.buffer_pool.ReserveBuffer(
                WGPUBufferUsage_Storage | WGPUBufferUsage_Vertex,
                2 * 4 * 4 * (resolution.x / 16 + 1) * (resolution.y / 16 + 1));

            auto time_buf = execution_context.buffer_pool.ReserveBuffer(
                WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, 4);

            execution_context.device->BeginFrame();

            float current_time =
                std::chrono::duration_cast<std::chrono::duration<float>>(
                    std::chrono::high_resolution_clock::now() -
                    execution_context.start_time)
                    .count();
            std::println("time: {}", current_time);
            execution_context.device->WriteBuffer(time_buf, 0, &current_time,
                                                  sizeof(float));

            execution_context.device->BeginComputePass();
            execution_context.device->BindGroups(
                compute_shader, "reset_counter",
                std::tuple(
                    BufferWithBinding<WGPUBufferBindingType_Storage,
                                      WGPUShaderStage_Compute, 0>{
                        internal_triangles},
                    BufferWithBinding<WGPUBufferBindingType_Storage,
                                      WGPUShaderStage_Compute, 1>{
                        boundary_triangles},
                    BufferWithBinding<WGPUBufferBindingType_Storage,
                                      WGPUShaderStage_Compute, 2>{vertices},
                    BufferWithBinding<WGPUBufferBindingType_Uniform,
                                      WGPUShaderStage_Compute, 3>{time_buf}));
            execution_context.device->DispatchWorgroups(1, 1, 1);

            execution_context.device->BindGroups(
                compute_shader, "output_vertices",
                std::tuple(
                    BufferWithBinding<WGPUBufferBindingType_Storage,
                                      WGPUShaderStage_Compute, 0>{
                        internal_triangles},
                    BufferWithBinding<WGPUBufferBindingType_Storage,
                                      WGPUShaderStage_Compute, 1>{
                        boundary_triangles},
                    BufferWithBinding<WGPUBufferBindingType_Storage,
                                      WGPUShaderStage_Compute, 2>{vertices},
                    BufferWithBinding<WGPUBufferBindingType_Uniform,
                                      WGPUShaderStage_Compute, 3>{time_buf}));
            execution_context.device->DispatchWorgroups(
                resolution.x / 16 + 1, resolution.y / 16 + 1, 1);

            // execution_context.device->BeginComputePass();
            execution_context.device->BindGroups(
                compute_shader, "output_indices",
                std::tuple(
                    BufferWithBinding<WGPUBufferBindingType_Storage,
                                      WGPUShaderStage_Compute, 0>{
                        internal_triangles},
                    BufferWithBinding<WGPUBufferBindingType_Storage,
                                      WGPUShaderStage_Compute, 1>{
                        boundary_triangles},
                    BufferWithBinding<WGPUBufferBindingType_Storage,
                                      WGPUShaderStage_Compute, 2>{vertices}));
            execution_context.device->DispatchWorgroups(
                (resolution.x / 16) * (resolution.y / 16), 1, 1);

            execution_context.device->EndPass<GPUPassType::Compute>();

            execution_context.device->BeginRenderPass(
                execution_context.surface_texture.texture);

            RenderPipelineBuilder<DeviceWGPU> render_pipeline_builder;
            auto pipeline =
                render_pipeline_builder
                    .SetVertexShader(render_shader, "vs_main")
                    .SetFragmentShader(render_shader, "fs_main")
                    .AttachAttributeBuffer(
                        vertices,
                        {{WGPUVertexFormat_Float32x4, 0, 0},
                         {WGPUVertexFormat_Float32x4, 16, 1}},
                        0)
                    .SetIndexBuffer(internal_triangles, 20)
                    .AttachColorTarget(
                        {.format = WGPUTextureFormat_BGRA8UnormSrgb,
                         .writeMask = WGPUColorWriteMask_All})
                    .SetMultisampleState(
                        {.count = 1, .mask = WGPUColorWriteMask_All})
                    .SetPrimitiveState(
                        {.topology = WGPUPrimitiveTopology_TriangleList})
                    .Finalize(*execution_context.device);

            execution_context.device->DrawIndirectIndexed(internal_triangles);

            RenderPipelineBuilder<DeviceWGPU> boundary_render_pipeline_builder;
            auto boundary_pipeline =
                boundary_render_pipeline_builder
                    .SetVertexShader(compute_shader, "vs_boundary_main")
                    .SetFragmentShader(compute_shader, "fs_boundary_main")
                    .AttachAttributeBuffer(
                        vertices,
                        {{WGPUVertexFormat_Float32x4, 0, 0},
                         {WGPUVertexFormat_Float32x4, 16, 1}},
                        0)
                    .SetIndexBuffer(boundary_triangles, 20)
                    .AttachColorTarget(
                        {.format = WGPUTextureFormat_BGRA8UnormSrgb,
                         .writeMask = WGPUColorWriteMask_All})
                    .SetMultisampleState(
                        {.count = 1, .mask = WGPUColorWriteMask_All})
                    .SetPrimitiveState(
                        {.topology = WGPUPrimitiveTopology_TriangleList})
                    .BindGroups(
                        *execution_context.device,
                        std::tuple(
                            BufferWithBinding<WGPUBufferBindingType_Uniform,
                                              WGPUShaderStage_Fragment, 3>{
                                time_buf}))
                    .Finalize(*execution_context.device);

            execution_context.device->DrawIndirectIndexed(boundary_triangles);

            execution_context.device->EndPass<GPUPassType::Render>();
            execution_context.device->EndFrame();

            execution_context.buffer_pool.ReleaseBuffer(internal_triangles);
            execution_context.buffer_pool.ReleaseBuffer(boundary_triangles);
            execution_context.buffer_pool.ReleaseBuffer(vertices);
            execution_context.buffer_pool.ReleaseBuffer(time_buf);

            return geometry;
          },
          [&geometry](const auto& geom) { return geometry; }},
      geometry);
}