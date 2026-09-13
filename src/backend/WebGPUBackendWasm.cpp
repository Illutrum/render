// WebGPUBackendWasm.cpp — browser init for Emscripten (emdawnwebgpu).
// JS must set Module.preinitializedWebGPUDevice before main() (see index.html).

#include "WebGPUBackend.hpp"
#include "config.hpp"

#include <webgpu/webgpu.hpp>

#include <cstring>

extern "C" WGPUDevice emscripten_webgpu_get_device(void);

namespace fgfx {

void WebGPUBackend::initWasm(const char* canvasSelector, int width, int height) {
  wgpu::Instance instance = wgpu::createInstance();
  if (!instance) {
    fgfx_log("WebGPU: could not create instance\n");
    return;
  }

  WGPUDevice rawDevice = emscripten_webgpu_get_device();
  if (!rawDevice) {
    fgfx_log("WebGPU: emscripten_webgpu_get_device returned null "
             "(Module.preinitializedWebGPUDevice missing?)\n");
    return;
  }
  wgpu::Device device = rawDevice;

  const char* selector =
      (canvasSelector && canvasSelector[0]) ? canvasSelector : "#fgfx_canvas";
  wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasSrc = wgpu::Default;
  canvasSrc.selector = WGPUStringView{selector, std::strlen(selector)};

  wgpu::SurfaceDescriptor surfaceDesc = wgpu::Default;
  surfaceDesc.nextInChain =
      reinterpret_cast<WGPUChainedStruct*>(&canvasSrc);

  wgpu::Surface surface = instance.createSurface(surfaceDesc);
  if (!surface) {
    fgfx_log("WebGPU: could not create canvas surface (%s)\n", selector);
    return;
  }

  if (width <= 0) width = 1280;
  if (height <= 0) height = 1024;
  initWithDevice(instance, device, surface, width, height, {});
  fgfx_log("WebGPU: initialized wasm/browser backend (%dx%d, %s)\n",
           width, height, selector);
}

}  // namespace fgfx
