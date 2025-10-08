// #include "chamfer.h"
#include "cube.h"
// #include "distance_map.h"
#include "executor.h"
#include "extrude.h"
#include "interpreter.h"
// #include "make_character.h"
#include "sdf_draw.h"
#include "triangulate.h"
// #include "write_ply.h"
// #include "write_stl.h"

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

using CubeOp = Op<'C', true, &MakeCube, float>;
// using TextOp = Op<'L', true, &MakeCharacterPolygon, char>;
//  TODO: Template should take 'callable' parameter types, not runtime types.
//  Or even better, parameters should be deduced from the function.
using TriangulateOp = Op<'T', true, &Triangulate, GeomId>;
// using WriteSTLOp = Op<'W', false, &WriteSTL, GeomId>;
// using WritePLYOp = Op<'P', false, &WritePLY, GeomId>;
using ExtrudeOp = Op<'E', true, &Extrude, GeomId, GeomId, float, float>;
// using ChamferOp = Op<'B', true, &Chamfer, GeomId, float>;
using SDFDrawOp = Op<'F', false, &SDFDraw, GeomId>;
// using DistanceOp = Op<'D', true, &DistanceMap, GeomId, GeomId>;

static void handle_request_adapter(WGPURequestAdapterStatus /*status*/,
                                   WGPUAdapter adapter,
                                   WGPUStringView /*message*/, void *userdata1,
                                   void * /*userdata2*/) {
  *(WGPUAdapter *)userdata1 = adapter;
}
static void handle_request_device(WGPURequestDeviceStatus status,
                                  WGPUDevice device, WGPUStringView message,
                                  void *userdata1, void *userdata2) {
  *(WGPUDevice *)userdata1 = device;
}

WGPUDevice CreateDevice(WGPUInstance instance) {
  WGPURequestAdapterOptions options{
      .featureLevel = WGPUFeatureLevel_Core,
      .powerPreference = WGPUPowerPreference_HighPerformance,
      .forceFallbackAdapter = false,
      .backendType = WGPUBackendType_Vulkan};

  WGPUAdapter adapter = NULL;
  wgpuInstanceRequestAdapter(
      instance, &options,
      (const WGPURequestAdapterCallbackInfo){.callback = handle_request_adapter,
                                             .userdata1 = &adapter});

  assert(adapter);

  WGPUDevice device = NULL;
  wgpuAdapterRequestDevice(
      adapter, NULL,
      (const WGPURequestDeviceCallbackInfo){.callback = handle_request_device,
                                            .userdata1 = &device});
  assert(device);
  return device;
}

GLFWwindow *CreateWindow() {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window = glfwCreateWindow(1024, 1024, "SDF", NULL, NULL);
  assert(window);
  // glfwMakeContextCurrent(window);

  // glfwSetWindowUserPointer(window, (void *)&demo);
  // glfwSetKeyCallback(window, handle_glfw_key);
  // glfwSetFramebufferSizeCallback(window, handle_glfw_framebuffer_size);
  return window;
}

auto CreateSurface(WGPUInstance instance, WGPUDevice device,
                   GLFWwindow *window) {
  if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
    struct wl_display *wayland_display = glfwGetWaylandDisplay();
    struct wl_surface *wayland_surface = glfwGetWaylandWindow(window);

    WGPUSurfaceSourceWaylandSurface wayland_surface_descriptor{
        .chain =
            (const WGPUChainedStruct){
                .sType = WGPUSType_SurfaceSourceWaylandSurface,
            },
        .display = wayland_display,
        .surface = wayland_surface,
    };

    WGPUSurfaceDescriptor surface_descriptor{
        .nextInChain = (WGPUChainedStruct *)&wayland_surface_descriptor};

    auto surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);
    std::println("CREATED");

    auto options = WGPURequestAdapterOptions{
        .compatibleSurface = surface,
    };

    WGPUAdapter adapter = nullptr;
    auto callback = WGPURequestAdapterCallbackInfo{
        .callback = handle_request_adapter, .userdata1 = &adapter};
    wgpuInstanceRequestAdapter(
        instance, &options,
        (const WGPURequestAdapterCallbackInfo){
            .callback = handle_request_adapter, .userdata1 = &adapter});

    WGPUSurfaceCapabilities surface_capabilities = {0};
    wgpuSurfaceGetCapabilities(surface, adapter, &surface_capabilities);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    auto config = WGPUSurfaceConfiguration{
        .device = device,
        .format = surface_capabilities.formats[0],
        .usage = WGPUTextureUsage_RenderAttachment,

        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .alphaMode = surface_capabilities.alphaModes[0],
        .presentMode = WGPUPresentMode_Fifo,

    };

    wgpuSurfaceConfigure(surface, &config);

    return surface;
  }
  return WGPUSurface(nullptr);
}

#include <cstdio>
static void error_callback(int error, const char *description) {
  std::println("{}", description);
}

static void log_callback(WGPULogLevel level, WGPUStringView message,
                         void *userdata) {
  char *level_str;
  switch (level) {
    case WGPULogLevel_Error:
      level_str = "error";
      break;
    case WGPULogLevel_Warn:
      level_str = "warn";
      break;
    case WGPULogLevel_Info:
      level_str = "info";
      break;
    case WGPULogLevel_Debug:
      level_str = "debug";
      break;
    case WGPULogLevel_Trace:
      level_str = "trace";
      break;
    default:
      level_str = "unknown_level";
  }
  fprintf(stderr, "[wgpu] [%s] %.*s\n", level_str, (int)message.length,
          message.data);
}

int main() {
  if (!glfwInit()) {
    return -1;
  }
  glfwSetErrorCallback(error_callback);

  //wgpuSetLogCallback(log_callback, NULL);
  //wgpuSetLogLevel(WGPULogLevel_Trace);
  auto *instance = wgpuCreateInstance(NULL);
  assert(instance);
  GLFWwindow *window = CreateWindow();
  assert(window);

  Executor e;

  auto *wgpu_device = CreateDevice(instance);
  DeviceWGPU device(wgpu_device);
  auto *surface = CreateSurface(instance, wgpu_device, window);

  ExecutionContext c{.shader_cache = ShaderCache(device),
                     .buffer_pool = BufferPool(device),
                     .device = &device,
                     .surface_texture = nullptr};

  e.Register<CubeOp>();
  // e.Register<TextOp>();
  e.Register<TriangulateOp>();
  // e.Register<WriteSTLOp>();
  // e.Register<WritePLYOp>();
  e.Register<ExtrudeOp>();
  // e.Register<ChamferOp>();
  e.Register<SDFDrawOp>();
  // e.Register<DistanceOp>();

  // ParseAndProcess("N2.1N0S3LN2.1N4 SHL N-3.1N5 SHL N2.1N1SDLN-2.1N2S3L
  // N-2.1 N3 SDL N1.2 N5C B E EEEEETW", e, c);

  std::println("Finished");

  bool first = true;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(surface, &surface_texture);
    switch (surface_texture.status) {
      case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
      case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        // All good, could handle suboptimal here
        break;
      case WGPUSurfaceGetCurrentTextureStatus_Timeout:
      case WGPUSurfaceGetCurrentTextureStatus_Outdated:
      case WGPUSurfaceGetCurrentTextureStatus_Lost: {
        // Skip this frame, and re-configure surface.
        // if (surface_texture.texture != NULL) {
        //   wgpuTextureRelease(surface_texture.texture);
        // }
        // int width, height;
        // glfwGetWindowSize(window, &width, &height);
        // if (width != 0 && height != 0) {
        //   demo.config.width = width;
        //   demo.config.height = height;
        //   wgpuSurfaceConfigure(demo.surface, &demo.config);
        // }
        // continue;
        std::println("LOST");
        break;
      }
      case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
      case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
      case WGPUSurfaceGetCurrentTextureStatus_Force32:
        // Fatal error
        printf(" get_current_texture status=%#.8x\n",
               surface_texture.status);
        abort();
    }

    c.surface_texture = surface_texture;

    ParseAndProcess("N0.3CF", e, c);
    first = false;

    wgpuSurfacePresent(surface);

    wgpuTextureRelease(surface_texture.texture);
  }

  return 0;
};
