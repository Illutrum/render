// WebGPUBackend: RenderBackend implemented through the webgpu.hpp C++ API.
// One file for both native (Dawn) and wasm (Emscripten) — only the init path
// differs (see WebGPUBackendNative.cpp / the wasm JS glue).
//
// Handle convention: TextureHandle / BufferHandle / PipelineHandle are
// indices into internal vectors (unlike WebGLBackend where they alias GL names).

#ifndef FGFX_BACKEND_WEBGPU_BACKEND_HPP
#define FGFX_BACKEND_WEBGPU_BACKEND_HPP

#include "RenderBackend.hpp"

#include <webgpu/webgpu.hpp>
#if defined(__EMSCRIPTEN__)
// emdawnwebgpu uses addRef(); webgpu-raii.hpp remaps addRef→reference() for
// the legacy Emscripten WebGPU API. Temporarily hide __EMSCRIPTEN__ so RAII
// calls the Dawn-style addRef().
#  pragma push_macro("__EMSCRIPTEN__")
#  undef __EMSCRIPTEN__
#  include <webgpu/webgpu-raii.hpp>
#  pragma pop_macro("__EMSCRIPTEN__")
#else
#  include <webgpu/webgpu-raii.hpp>
#endif

#include <vector>

struct GLFWwindow;

namespace fgfx {

class WebGPUBackend : public RenderBackend {
 public:
  WebGPUBackend();
  ~WebGPUBackend() override;

  BackendType type() const override { return BackendType::WebGPU; }

#ifdef EMSCRIPTEN
  void initWasm(const char* canvasSelector, int width, int height);
#else
  // Native init (Dawn + GLFW). Called by fastgfx.cpp when FGFX_BACKEND_WEBGPU.
  void initNative(GLFWwindow* window);
#endif

  // Called by native/wasm init. Takes ownership of instance/device/surface.
  void initWithDevice(wgpu::Instance instance, wgpu::Device device,
                      wgpu::Surface surface, int width, int height,
                      wgpu::Adapter adapter = {});
  void reconfigureSurface(int width, int height);

  WGPUDevice wgpuDevice() const { return static_cast<WGPUDevice>(device_); }
  WGPUInstance wgpuInstance() const { return static_cast<WGPUInstance>(instance_); }
  WGPUTexture wgpuTexture(TextureHandle h) const {
    auto idx = static_cast<size_t>(h);
    if (idx == 0 || idx >= textures_.size()) return nullptr;
    if (!textures_[idx].texture) return nullptr;
    return static_cast<WGPUTexture>(*textures_[idx].texture);
  }

  // --- RenderBackend implementation --------------------------------------
  PipelineHandle createPipeline(const PipelineDesc& desc) override;
  void destroyPipeline(PipelineHandle pipeline) override;

  BufferHandle createBuffer(const BufferDesc& desc) override;
  void updateBuffer(BufferHandle buffer, const void* data, int size) override;
  void destroyBuffer(BufferHandle buffer) override;

  TextureHandle createTexture(const TextureDesc& desc) override;
  void updateTexture(TextureHandle texture, int width, int height,
                     const void* data) override;
  void destroyTexture(TextureHandle texture) override;

  void beginFrame() override;
  void beginRenderPass(TextureHandle colorTarget, LoadOp loadOp,
                       const ClearValue& clear, int viewportW,
                       int viewportH) override;
  void bindPipeline(PipelineHandle pipeline) override;
  void bindUniformData(int binding, const void* data, int size) override;
  void bindTexture(int binding, TextureHandle texture) override;
  void bindVertexBuffer(int binding, BufferHandle buffer) override;
  void draw(int vertexCount) override;
  void endRenderPass() override;
  void endFrame() override;

 private:
  struct PipelineInfo {
    wgpu::raii::RenderPipeline pipeline;
    wgpu::raii::BindGroupLayout bindGroupLayout;
    wgpu::raii::PipelineLayout pipelineLayout;
    wgpu::raii::ShaderModule shaderModule;
    int uniformSize = 64;
    bool hasTexture = true;
  };

  struct BufferInfo {
    wgpu::raii::Buffer buffer;
    int size = 0;
    BufferUsage usage = BufferUsage::Vertex;
  };

  struct TextureInfo {
    wgpu::raii::Texture texture;
    wgpu::raii::TextureView view;
    int width = 0;
    int height = 0;
  };

  // WebGPU device/queue (borrowed from native init, not owned).
  wgpu::Instance instance_;
  wgpu::Device device_;
  wgpu::Queue queue_;
  wgpu::Adapter adapter_;
  wgpu::raii::Surface surface_;
  int surfaceWidth_ = 0;
  int surfaceHeight_ = 0;
  bool surfaceConfigured_ = false;

  // Resource tables (indexed by handle).
  std::vector<PipelineInfo> pipelines_;
  std::vector<BufferInfo> buffers_;
  std::vector<TextureInfo> textures_;

  // Frame state. The swapchain texture must stay alive until present().
  wgpu::raii::CommandEncoder commandEncoder_;
  wgpu::raii::RenderPassEncoder passEncoder_;
  wgpu::raii::Texture currentBackbufferTexture_;
  wgpu::raii::TextureView currentBackbufferView_;
  PipelineInfo* currentPipeline_ = nullptr;

  // Texture pending for the next bind group (set by bindTexture, used by draw).
  size_t pendingTexture_ = 0;

  // Scratch uniform buffer for bindUniformData.
  wgpu::raii::Buffer scratchUniformBuffer_;
  int scratchUniformSize_ = 0;

  // Default sampler (created once, reused for all draw calls).
  wgpu::raii::Sampler defaultSampler_;

  WGPUTextureFormat surfaceFormat_ = WGPUTextureFormat_BGRA8Unorm;

  WGPUVertexFormat vertexFormatOf(PixelFormat f);
  WGPUBufferUsage bufferUsageOf(BufferUsage u);
  WGPUTextureFormat textureFormatOf(PixelFormat f);
};

}  // namespace fgfx

#endif  // FGFX_BACKEND_WEBGPU_BACKEND_HPP
