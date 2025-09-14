#include "executor.h"

#include <cassert>

#include "webgpu/webgpu.h"

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
static void handle_buffer_map(WGPUMapAsyncStatus status, WGPUStringView message,
                              void *userdata1, void *userdata2) {}

Executor::Executor() {
  // WGPUInstance instance = wgpuCreateInstance(NULL);
  // assert(instance);

  // WGPUAdapter adapter = NULL;
  // wgpuInstanceRequestAdapter(
  //     instance, NULL,
  //     (const WGPURequestAdapterCallbackInfo){.callback = handle_request_adapter,
  //                                            .userdata1 = &adapter});

  // assert(adapter);

  // WGPUDevice device = NULL;
  // wgpuAdapterRequestDevice(
  //     adapter, NULL,
  //     (const WGPURequestDeviceCallbackInfo){.callback = handle_request_device,
  //                                           .userdata1 = &device});
  // assert(device);

  // WGPUQueue queue = wgpuDeviceGetQueue(device);
  // assert(queue);

  // WGPUBuffer index_buffer = frmwrk_device_create_buffer_init(
  //     device, &(const WGPUBufferDescriptor){
  //                      .label = {"index_buffer", WGPU_STRLEN},
  //                      .content = (void *)indices,
  //                      .content_size = indices_size,
  //                      .usage = WGPUBufferUsage_Index,
  //                  });
  // assert(index_buffer);

  // WGPUBuffer staging_buffer = wgpuDeviceCreateBuffer(
  //     device, &(const WGPUBufferDescriptor){
  //                 .label = {"staging_buffer", WGPU_STRLEN},
  //                 .usage = WGPUBufferUsage_Vertex,
  //                 .size = positions_size,
  //                 .mappedAtCreation = false,
  //             });
  // assert(staging_buffer);
}