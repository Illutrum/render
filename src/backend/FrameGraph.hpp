// FrameGraph: the declarative render graph data structures + immediate
// executor. Passes execute in the order they were recorded. Topological sort,
// barrier tracking, and transient allocation are later work.

#ifndef FGFX_BACKEND_FRAME_GRAPH_HPP
#define FGFX_BACKEND_FRAME_GRAPH_HPP

#include "BackendTypes.hpp"
#include "RenderBackend.hpp"
#include "UniformAllocator.hpp"
#include "../config.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <functional>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace render {

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

// A single draw call. Uniforms live in the backend ring buffer; DrawCall
// stores the offset. Texture at binding 0 is the sprite atlas.
struct DrawCall {
  PipelineHandle pipeline = kNullPipeline;
  int uniformBinding = 0;
  uint32_t uniformOffset = 0;
  uint16_t uniformSize = 0;
  int textureBinding = 0;
  TextureHandle texture = kNullTexture;
  std::vector<std::pair<int, BufferHandle>> vertexBuffers;
  int vertexCount = 0;
};

enum class PassKind { Render, External };

struct RenderPass {
  std::string name;
  TextureHandle colorTarget = kNullTexture;  // 0 = surface
  TextureHandle depthTarget = kNullTexture;
  LoadOp loadOp = LoadOp::Clear;
  ClearValue clear;
  int viewportW = 0;
  int viewportH = 0;
  std::vector<DrawCall> drawCalls;
  PassKind kind = PassKind::Render;
  std::function<void(RenderBackend&)> externalCallback;
  FeatureSet requiredFeatures = 0;
  bool opaque = false;
  // Named `require` because `requires` is a C++20 keyword.
  RenderPass& require(Feature f) {
    requiredFeatures |= featureBit(f);
    return *this;
  }
};

// Context handed to batches during recording.
struct FrameContext {
  ModelViewProjection mvp;
  double time = 0.0;
  float delta = 0.0f;
  RenderBackend* backend = nullptr;
  UniformAllocator* uniforms = nullptr;
};

inline std::string missingFeatureNames(const RenderBackend& backend,
                                       FeatureSet set) {
  std::string out;
  auto append = [&](Feature f) {
    if (hasFeature(set, f) && !backend.supports(f)) {
      if (!out.empty()) out += ", ";
      out += featureName(f);
    }
  };
  append(Feature::Compute);
  append(Feature::StorageTexture);
  append(Feature::RasterizeToTexture);
  return out;
}

inline void logUnsupportedPass(const std::string& name, FeatureSet set,
                               const RenderBackend& backend) {
  static std::unordered_set<std::string> logged;
  if (!logged.insert(name).second) {
    return;
  }
  std::ostringstream msg;
  msg << "FrameGraph: skip pass \"" << name << "\" (missing "
      << missingFeatureNames(backend, set) << ")";
  render_log("%s", msg.str().c_str());
}

// FrameGraph: the recorded passes, executed immediately against a backend.
class FrameGraph {
 public:
  void execute(RenderBackend& backend) {
    backend.beginFrame();
    bool wroteSurface = false;
    for (auto& pass : passes_) {
      if (!backend.supportsAll(pass.requiredFeatures)) {
        logUnsupportedPass(pass.name, pass.requiredFeatures, backend);
        continue;
      }
      if (pass.kind == PassKind::External) {
        if (pass.externalCallback) {
          pass.externalCallback(backend);
        }
        continue;
      }
      if (pass.colorTarget == kNullTexture) {
        wroteSurface = true;
      }
      backend.beginRenderPass(pass.colorTarget, pass.loadOp, pass.clear,
                               pass.viewportW, pass.viewportH,
                               pass.depthTarget);
      if (pass.opaque) {
        std::stable_sort(
            pass.drawCalls.begin(), pass.drawCalls.end(),
            [](const DrawCall& a, const DrawCall& b) {
              return std::tie(a.pipeline, a.texture) <
                     std::tie(b.pipeline, b.texture);
            });
      }
      for (auto& dc : pass.drawCalls) {
        if (dc.pipeline == kNullPipeline) {
          continue;
        }
        backend.bindPipeline(dc.pipeline);
        if (dc.uniformSize > 0) {
          backend.bindUniformData(dc.uniformBinding, dc.uniformOffset,
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
    if (!wroteSurface) {
      backend.clearSurface(surfaceClear_);
    }
    backend.endFrame();
  }

 private:
  friend class FrameBuilder;
  std::vector<RenderPass> passes_;
  ClearValue surfaceClear_{0.31f, 0.52f, 0.16f, 1.0f};
};

// FrameBuilder: fluent API used by the scene render function to record passes.
class FrameBuilder {
 public:
  FrameBuilder(RenderBackend& backend, UniformAllocator& uniforms,
               TextureHandle surfaceColor, int vpW, int vpH)
      : backend_(&backend),
        surfaceColor_(surfaceColor),
        viewportW_(vpW),
        viewportH_(vpH) {
    context_.backend = &backend;
    context_.uniforms = &uniforms;
  }

  TextureHandle surfaceColor() const { return surfaceColor_; }
  RenderBackend& backend() { return *backend_; }

  FrameBuilder& setSurfaceClear(const ClearValue& clear) {
    surfaceClear_ = clear;
    return *this;
  }
  const ClearValue& surfaceClear() const { return surfaceClear_; }

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
    graph.surfaceClear_ = surfaceClear_;
    return graph;
  }

 private:
  RenderBackend* backend_;
  TextureHandle surfaceColor_;
  int viewportW_;
  int viewportH_;
  FrameContext context_;
  std::vector<RenderPass> passes_;
  ClearValue surfaceClear_{0.31f, 0.52f, 0.16f, 1.0f};
};

}  // namespace render

#endif  // FGFX_BACKEND_FRAME_GRAPH_HPP
