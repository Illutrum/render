// FrameGraph: the declarative render graph data structures + immediate
// executor. For the spike there is no topological sort, barrier tracking, or
// transient allocation — passes execute in the order they were recorded.
// Those features arrive in Phase 10.

#ifndef FGFX_BACKEND_FRAME_GRAPH_HPP
#define FGFX_BACKEND_FRAME_GRAPH_HPP

#include "BackendTypes.hpp"
#include "RenderBackend.hpp"

#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <string>

namespace fgfx {

struct ModelViewProjection {
  glm::mat4 multipliedMatrix;
  glm::mat4 projectionMatrix;
  glm::mat4 viewMatrix;
  glm::mat4 modelMatrix;
  glm::vec3 eyePosition;
  void multiply() {
    multipliedMatrix = projectionMatrix * viewMatrix * modelMatrix;
  }
};

// A single draw call. For the spike: one pipeline, one uniform data blob
// (camera matrix at binding 0), one texture (binding 0), up to a few vertex
// buffers keyed by binding index.
struct DrawCall {
  PipelineHandle pipeline = kNullPipeline;
  // Uniform data (copied into the draw call; binding 0 = camera matrix).
  static constexpr int kMaxUniformData = 256;
  int uniformBinding = 0;
  int uniformSize = 0;
  std::array<uint8_t, kMaxUniformData> uniformData{};
  // Texture (binding 0 = sprite atlas).
  int textureBinding = 0;
  TextureHandle texture = kNullTexture;
  // Vertex buffers keyed by binding index.
  std::vector<std::pair<int, BufferHandle>> vertexBuffers;
  int vertexCount = 0;
};

struct RenderPass {
  std::string name;
  TextureHandle colorTarget = kNullTexture;  // 0 = surface
  LoadOp loadOp = LoadOp::Clear;
  ClearValue clear;
  int viewportW = 0;
  int viewportH = 0;
  std::vector<DrawCall> drawCalls;
};

// Context handed to batches during recording.
struct FrameContext {
  ModelViewProjection mvp;
  double time = 0.0;
  float delta = 0.0f;
  RenderBackend* backend = nullptr;
};

// FrameGraph: the recorded passes, executed immediately against a backend.
class FrameGraph {
 public:
  void execute(RenderBackend& backend) {
    backend.beginFrame();
    for (auto& pass : passes_) {
      backend.beginRenderPass(pass.colorTarget, pass.loadOp, pass.clear,
                               pass.viewportW, pass.viewportH);
      for (auto& dc : pass.drawCalls) {
        backend.bindPipeline(dc.pipeline);
        if (dc.uniformSize > 0) {
          backend.bindUniformData(dc.uniformBinding, dc.uniformData.data(),
                                  dc.uniformSize);
        }
        if (dc.texture != kNullTexture) {
          backend.bindTexture(dc.textureBinding, dc.texture);
        }
        for (auto& [binding, buf] : dc.vertexBuffers) {
          backend.bindVertexBuffer(binding, buf);
        }
        backend.draw(dc.vertexCount);
      }
      backend.endRenderPass();
    }
    backend.endFrame();
  }

 private:
  friend class FrameBuilder;
  std::vector<RenderPass> passes_;
};

// FrameBuilder: fluent API used by the scene render function to record passes.
class FrameBuilder {
 public:
  FrameBuilder(RenderBackend& backend, TextureHandle surfaceColor, int vpW,
               int vpH)
      : backend_(&backend),
        surfaceColor_(surfaceColor),
        viewportW_(vpW),
        viewportH_(vpH) {}

  TextureHandle surfaceColor() const { return surfaceColor_; }

  RenderPass& beginPass(const std::string& name, TextureHandle colorTarget,
                        LoadOp loadOp, const ClearValue& clear) {
    RenderPass pass;
    pass.name = name;
    pass.colorTarget = colorTarget;
    pass.loadOp = loadOp;
    pass.clear = clear;
    pass.viewportW = viewportW_;
    pass.viewportH = viewportH_;
    passes_.push_back(std::move(pass));
    return passes_.back();
  }

  void submitPass() {
    // No-op for the spike: passes are collected and executed in order.
  }

  FrameContext& context() { return context_; }

  FrameGraph build() {
    FrameGraph graph;
    graph.passes_ = std::move(passes_);
    return graph;
  }

 private:
  RenderBackend* backend_;
  TextureHandle surfaceColor_;
  int viewportW_;
  int viewportH_;
  FrameContext context_;
  std::vector<RenderPass> passes_;
};

}  // namespace fgfx

#endif  // FGFX_BACKEND_FRAME_GRAPH_HPP
