#include "device_wgpu.h"

#include <print>

#include "webgpu/wgpu.h"

WGPUBuffer DeviceWGPU::CreateBuffer(const std::string& name,
                                    WGPUBufferUsage usage, size_t size) {
  auto descriptor = WGPUBufferDescriptor{
      .label = {name.c_str(), name.length()},
      .usage = usage,
      .size = size,
      .mappedAtCreation = false,
  };
  WGPUBuffer storage_buffer = wgpuDeviceCreateBuffer(device_, &descriptor);
  return storage_buffer;
}

WGPUShaderModule DeviceWGPU::Compile(const std::string& name,
                                     const std::string& source) {
  //std::println("Shader source: {}", source);
  auto shader_source = WGPUShaderSourceWGSL{
      .chain =
          (const WGPUChainedStruct){
              .next = nullptr,
              .sType = WGPUSType_ShaderSourceWGSL,
          },
      .code = {source.c_str(), source.size()},
  };
  auto description = WGPUShaderModuleDescriptor{
      .nextInChain = (const WGPUChainedStruct*)&shader_source,
      .label = {name.c_str(), name.size()},
  };
  WGPUShaderModule shader = wgpuDeviceCreateShaderModule(device_, &description);
  return shader;
}