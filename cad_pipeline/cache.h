#pragma once

#include <print>
#include <unordered_map>

#include "any_geometry.h"
#include "types.h"
#include "webgpu/wgpu.h"

using Cache = std::unordered_map<GeomId, AnyGeometry>;

class WGPUCompiler {
 public:
  WGPUShaderModule Compile(WGPUDevice device, const std::string& name,
                           const std::string& source) {
    std::println("Shader source: {}", source);
    auto shader_source = WGPUShaderSourceWGSL{
        .chain =
            (const WGPUChainedStruct){
                .sType = WGPUSType_ShaderSourceWGSL,
            },
        .code = {source.c_str(), source.size()},
    };
    auto description = WGPUShaderModuleDescriptor{
        .nextInChain = (const WGPUChainedStruct*)&shader_source,
        .label = {name.c_str(), name.size()},
    };
    WGPUShaderModule shader =
        wgpuDeviceCreateShaderModule(device, &description);
    return shader;
  }
};

template <typename Compiler, typename Shader>
class ShaderCache {
 public:
  ShaderCache(WGPUDevice device) : _device(device) {};
  Shader GetShader(const std::string& source) {
    size_t hash = std::hash<std::string>{}(source);
    Compiler c;
    auto it = cache_.find(hash);
    if (it == cache_.end()) {
      auto shader = c.Compile(_device, "name", source);
      it = cache_.insert({hash, shader}).first;
    }

    return it->second;
  }

 private:
  WGPUDevice _device = nullptr;
  std::unordered_map<std::size_t, WGPUShaderModule> cache_;
};

struct ExecutionContext {
  Cache cache;
  ShaderCache<WGPUCompiler, WGPUShaderModule> shader_cache;
};
