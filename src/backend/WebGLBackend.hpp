// WebGLBackend: RenderBackend implemented over the existing OpenGL ES 3.0 /
// WebGL2 calls. For the spike only the sprite path is implemented.
//
// Handle convention: TextureHandle / BufferHandle alias the GL object name
// (GLuint). PipelineHandle aliases the GL program name; per-pipeline metadata
// (vertex bindings, sampler location) is kept in an internal map.

#ifndef FGFX_BACKEND_WEBGL_BACKEND_HPP
#define FGFX_BACKEND_WEBGL_BACKEND_HPP

#include "RenderBackend.hpp"
#include "../config.hpp"

#include <array>
#include <unordered_map>
#include <vector>

namespace fgfx {

class WebGLBackend : public RenderBackend {
 public:
  WebGLBackend() = default;
  ~WebGLBackend() override;

  BackendType type() const override { return BackendType::GL; }
  bool supports(Feature feature) const override {
    switch (feature) {
      case Feature::RasterizeToTexture: return true;
      case Feature::Compute:
      case Feature::StorageTexture: return false;
    }
    return false;
  }

  PipelineHandle createPipeline(const PipelineDesc& desc) override;
  void destroyPipeline(PipelineHandle pipeline) override;

  BufferHandle createBuffer(const BufferDesc& desc) override;
  void updateBuffer(BufferHandle buffer, const void* data, int size) override;
  void destroyBuffer(BufferHandle buffer) override;

  TextureHandle createTexture(const TextureDesc& desc) override;
  void updateTexture(TextureHandle texture, int width, int height,
                     const void* data) override;
  void destroyTexture(TextureHandle texture) override;
  void generateMipmaps(TextureHandle texture) override;
  void setColorTargetSampling(TextureHandle texture,
                              float maxAnisotropy) override;

  GLuint ensureFramebuffer(TextureHandle texture);
  void bindColorTarget(TextureHandle texture, int viewportW, int viewportH);
  void attachDepthStencil(int width, int height);
  void unbindColorTarget();

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
    GLuint program = 0;
    // Per binding: the attributes that live in that binding's vertex buffer.
    struct AttrInfo {
      GLint location;
      GLint size;  // component count (1..4)
      int offset;
    };
    std::unordered_map<int, std::vector<AttrInfo>> bindingAttrs;
    GLint samplerLocation = -1;
    GLenum topology = GL_TRIANGLES;
    bool blendEnabled = true;
    GLenum blendSrc = GL_SRC_ALPHA;
    GLenum blendDst = GL_ONE_MINUS_SRC_ALPHA;
  };

  std::unordered_map<GLuint, PipelineInfo> pipelines_;
  // Track the GL target for each buffer so updateBuffer binds correctly.
  std::unordered_map<GLuint, GLenum> bufferTargets_;
  std::unordered_map<GLuint, GLuint> fboCache_;
  GLuint stencilRbo_ = 0;
  int stencilW_ = 0;
  int stencilH_ = 0;
  GLuint currentProgram_ = 0;
  GLuint scratchUbo_ = 0;
  mutable uint32_t uboAlignment_ = 256;
  mutable bool uboAlignQueried_ = false;
  std::array<TextureHandle, 16> lastTexture_{};

  void ensureUniformRing();
};

}  // namespace fgfx

#endif  // FGFX_BACKEND_WEBGL_BACKEND_HPP
