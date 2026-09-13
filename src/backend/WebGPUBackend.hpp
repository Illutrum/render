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

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

struct GLFWwindow;

namespace fgfx {

class WebGPUBackend : public RenderBackend {
 public:
  WebGPUBackend();
  ~WebGPUBackend() override;

  BackendType type() const override { return BackendType::WebGPU; }
  bool supports(Feature feature) const override {
    switch (feature) {
      case Feature::Compute:
      case Feature::StorageTexture:
      case Feature::RasterizeToTexture: return true;
    }
    return false;
  }

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
                       int viewportH,
                       TextureHandle depthTarget = kNullTexture) override;
  void clearSurface(const ClearValue& clear) override;
  void bindPipeline(PipelineHandle pipeline) override;
  uint32_t uniformRingCapacity() const override;
  uint32_t uniformRingAlignment() const override;
  void uploadToUniformRing(uint32_t offset, const void* data, int size) override;
  void resetUniformRing() override;
  void bindUniformData(int binding, uint32_t offset, int size) override;
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

  // Resource tables (indexed by handle). Handles are indices into these
  // vectors; destroy* nulls the slot via RAII but indices are never recycled.
  std::vector<PipelineInfo> pipelines_;
  std::vector<BufferInfo> buffers_;
  std::vector<TextureInfo> textures_;

  // Frame state. The swapchain texture must stay alive until present().
  wgpu::raii::CommandEncoder commandEncoder_;
  wgpu::raii::RenderPassEncoder passEncoder_;
  wgpu::raii::Texture currentBackbufferTexture_;
  wgpu::raii::TextureView currentBackbufferView_;
  PipelineInfo* currentPipeline_ = nullptr;

  // Texture / uniform offset pending for the next bind group.
  size_t pendingTexture_ = 0;
  uint32_t pendingUniformOffset_ = 0;

  // Ring uniform buffer (replaces per-draw scratch UBO).
  wgpu::raii::Buffer scratchUniformBuffer_;
  uint32_t uniformRingCapacity_ = 1u << 20;

  struct BindGroupKey {
    WGPUBindGroupLayout layout = nullptr;
    uint32_t uniformOffset = 0;
    TextureHandle texture = kNullTexture;
    bool operator==(const BindGroupKey& o) const {
      return layout == o.layout && uniformOffset == o.uniformOffset &&
             texture == o.texture;
    }
  };
  struct BindGroupKeyHash {
    size_t operator()(const BindGroupKey& k) const {
      size_t h = std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(k.layout));
      h ^= std::hash<uint32_t>{}(k.uniformOffset) + 0x9e3779b9 + (h << 6) +
           (h >> 2);
      h ^= std::hash<uint32_t>{}(k.texture) + 0x9e3779b9 + (h << 6) + (h >> 2);
      return h;
    }
  };
  std::unordered_map<BindGroupKey, wgpu::raii::BindGroup, BindGroupKeyHash>
      bindGroupCache_;
  static constexpr size_t kMaxBindGroupCache = 256;

  // Default sampler (created once, reused for all draw calls).
  wgpu::raii::Sampler defaultSampler_;

  WGPUTextureFormat surfaceFormat_ = WGPUTextureFormat_BGRA8Unorm;

  WGPUVertexFormat vertexFormatOf(PixelFormat f);
  WGPUBufferUsage bufferUsageOf(BufferUsage u);
  WGPUTextureFormat textureFormatOf(PixelFormat f);
};

}  // namespace fgfx

#endif  // FGFX_BACKEND_WEBGPU_BACKEND_HPP
