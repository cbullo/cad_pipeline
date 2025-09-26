#pragma once

#include <print>
#include <unordered_map>

#include "any_geometry.h"
#include "device_wgpu.h"
#include "types.h"
#include "webgpu/wgpu.h"

using Cache = std::unordered_map<GeomId, AnyGeometry>;

// class WGPUCompiler {
//  public:
//   WGPUShaderModule Compile(WGPUDevice device, const std::string& name,
//                            const std::string& source) {
//     std::println("Shader source: {}", source);
//     auto shader_source = WGPUShaderSourceWGSL{
//         .chain =
//             (const WGPUChainedStruct){
//                 .sType = WGPUSType_ShaderSourceWGSL,
//             },
//         .code = {source.c_str(), source.size()},
//     };
//     auto description = WGPUShaderModuleDescriptor{
//         .nextInChain = (const WGPUChainedStruct*)&shader_source,
//         .label = {name.c_str(), name.size()},
//     };
//     WGPUShaderModule shader =
//         wgpuDeviceCreateShaderModule(device, &description);
//     return shader;
//   }
// };

template <typename Device>
class ShaderCache {
 public:
  ShaderCache(Device device) : _device(device) {};
  typename Device::Shader GetShader(const std::string& source) {
    size_t hash = std::hash<std::string>{}(source);
    auto it = cache_.find(hash);
    if (it == cache_.end()) {
      auto shader = _device.Compile("name", source);
      it = cache_.insert({hash, shader}).first;
    }

    return it->second;
  }

 private:
  Device _device = {};
  std::unordered_map<std::size_t, typename Device::Shader> cache_;
};

template <typename Device>
class BufferPool {
 public:
  BufferPool(Device device) : device_(device) {}
  typename Device::Buffer ReserveBuffer(typename Device::BufferUsage usage,
                                        size_t size) {
    // size--;
    // size |= size >> 1;
    // size |= size >> 2;
    // size |= size >> 4;
    // size |= size >> 8;
    // size |= size >> 16;
    // size++;

    int power_of_2 = std::bit_width(size) - 1;

    auto& usage_pool = pool_[usage];

    if (usage_pool.size() < power_of_2 + 1) {
      usage_pool.resize(power_of_2 + 1);
    }

    if (usage_pool[power_of_2].empty()) {
      auto buffer = device_.CreateBuffer("buffer", usage, size);
      usage_pool[power_of_2].push(buffer);
    }

    auto buffer = usage_pool[power_of_2].top();
    usage_pool[power_of_2].pop();

    return buffer;
  }

 private:
  std::unordered_map<typename Device::BufferUsage,
                     std::vector<std::stack<typename Device::Buffer>>>
      pool_;

  Device device_;
};

struct ExecutionContext {
  Cache cache;
  ShaderCache<DeviceWGPU> shader_cache;
  BufferPool<DeviceWGPU> buffer_pool;
  DeviceWGPU* device;
};
