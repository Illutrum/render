// RenderBackend: the abstraction layer over GL (WebGLBackend) and WebGPU
// (WebGPUBackend). For the spike only the sprite-relevant surface is defined;
// the rest of the methods are added as more batches migrate.

#ifndef FGFX_BACKEND_RENDER_BACKEND_HPP
#define FGFX_BACKEND_RENDER_BACKEND_HPP

#include "BackendTypes.hpp"

namespace fgfx {

class RenderBackend {
 public:
  virtual ~RenderBackend() = default;

  virtual BackendType type() const = 0;
  virtual bool supports(Feature feature) const = 0;
  bool supportsAll(FeatureSet set) const {
    if (hasFeature(set, Feature::Compute) && !supports(Feature::Compute)) {
      return false;
    }
    if (hasFeature(set, Feature::StorageTexture) &&
        !supports(Feature::StorageTexture)) {
      return false;
    }
    if (hasFeature(set, Feature::RasterizeToTexture) &&
        !supports(Feature::RasterizeToTexture)) {
      return false;
    }
    return true;
  }

  // --- Resource creation -------------------------------------------------
  virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;
  virtual void destroyPipeline(PipelineHandle pipeline) = 0;

  virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
  virtual void updateBuffer(BufferHandle buffer, const void* data, int size) = 0;
  virtual void destroyBuffer(BufferHandle buffer) = 0;

  virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
  virtual void updateTexture(TextureHandle texture, int width, int height,
                             const void* data) = 0;
  virtual void destroyTexture(TextureHandle texture) = 0;

  // --- Frame / pass -------------------------------------------------------
  // colorTarget == kNullTexture means the surface (default framebuffer /
  // swapchain backbuffer).
  virtual uint32_t uniformRingCapacity() const = 0;
  virtual uint32_t uniformRingAlignment() const = 0;
  virtual void uploadToUniformRing(uint32_t offset, const void* data,
                                   int size) = 0;
  virtual void resetUniformRing() = 0;

  virtual void beginFrame() = 0;
  virtual void beginRenderPass(TextureHandle colorTarget, LoadOp loadOp,
                               const ClearValue& clear, int viewportW,
                               int viewportH,
                               TextureHandle depthTarget = kNullTexture) = 0;
  virtual void clearSurface(const ClearValue& clear) = 0;
  virtual void bindPipeline(PipelineHandle pipeline) = 0;
  virtual void bindUniformData(int binding, uint32_t offset, int size) = 0;
  virtual void bindTexture(int binding, TextureHandle texture) = 0;
  virtual void bindVertexBuffer(int binding, BufferHandle buffer) = 0;
  virtual void draw(int vertexCount) = 0;
  virtual void endRenderPass() = 0;
  virtual void endFrame() = 0;  // present
};

}  // namespace fgfx

#endif  // FGFX_BACKEND_RENDER_BACKEND_HPP
