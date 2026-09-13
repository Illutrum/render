// WebGPUBackendNative.cpp — Dawn native initialization for desktop.
// Creates a Dawn instance, Vulkan adapter, device, and surface from GLFW.
// Only compiled when FGFX_BACKEND_WEBGPU is defined.

#include "WebGPUBackend.hpp"
#include "config.hpp"

#include <webgpu/webgpu.hpp>
#include <webgpu/webgpu-raii.hpp>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <X11/Xlib.h>

#include <string>

namespace fgfx {
namespace {

std::string wgpuString(WGPUStringView sv) {
  if (!sv.data) return {};
  if (sv.length == WGPU_STRLEN) return std::string(sv.data);
  return std::string(sv.data, sv.length);
}

void onUncapturedError(const WGPUDevice*, WGPUErrorType type,
                       WGPUStringView message, void*, void*) {
  fgfx_log("WebGPU uncaptured error (%d): %s\n",
           static_cast<int>(type), wgpuString(message).c_str());
}

void onDeviceLost(const WGPUDevice*, WGPUDeviceLostReason reason,
                  WGPUStringView message, void*, void*) {
  fgfx_log("WebGPU device lost (%d): %s\n",
           static_cast<int>(reason), wgpuString(message).c_str());
}

}  // namespace

void WebGPUBackend::initNative(GLFWwindow* window) {
  wgpu::Instance instance = wgpu::createInstance();
  if (!instance) {
    fgfx_log("WebGPU: could not create instance\n");
    return;
  }

  // Surface first, so the adapter can be requested as compatible with it.
  Display* xdisplay = glfwGetX11Display();
  Window xwindow = glfwGetX11Window(window);

  wgpu::SurfaceSourceXlibWindow xlibSurface = wgpu::Default;
  xlibSurface.display = xdisplay;
  xlibSurface.window = static_cast<uint64_t>(xwindow);

  wgpu::SurfaceDescriptor surfaceDesc = wgpu::Default;
  surfaceDesc.nextInChain =
      reinterpret_cast<wgpu::ChainedStruct*>(&xlibSurface);

  wgpu::Surface surface = instance.createSurface(surfaceDesc);
  if (!surface) {
    fgfx_log("WebGPU: could not create surface\n");
    return;
  }

  wgpu::RequestAdapterOptions adapterOpts = wgpu::Default;
  adapterOpts.powerPreference = wgpu::PowerPreference::HighPerformance;
  adapterOpts.forceFallbackAdapter = false;
  adapterOpts.backendType = wgpu::BackendType::Vulkan;
  adapterOpts.compatibleSurface = surface;

  wgpu::Adapter adapter = instance.requestAdapter(adapterOpts);
  if (!adapter) {
    fgfx_log("WebGPU: could not get Vulkan adapter\n");
    return;
  }

  wgpu::DeviceDescriptor deviceDesc = wgpu::Default;
  deviceDesc.uncapturedErrorCallbackInfo.callback = onUncapturedError;
  deviceDesc.deviceLostCallbackInfo.callback = onDeviceLost;
  deviceDesc.deviceLostCallbackInfo.mode = wgpu::CallbackMode::AllowSpontaneous;

  wgpu::Device device = adapter.requestDevice(deviceDesc);
  if (!device) {
    fgfx_log("WebGPU: could not get device\n");
    return;
  }

  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  if (width <= 0 || height <= 0) {
    glfwGetWindowSize(window, &width, &height);
  }
  if (width <= 0 || height <= 0) {
    width = 1280;
    height = 1024;
  }

  initWithDevice(instance, device, surface, width, height, adapter);
  fgfx_log("WebGPU: initialized Dawn/Vulkan backend (%dx%d)\n",
           width, height);
}

}  // namespace fgfx
