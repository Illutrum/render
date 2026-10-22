#include "LineBatch.hpp"
#include "Engine.hpp"
#include "backend/RenderBackend.hpp"
#include "backend/FrameGraph.hpp"

#include <cstring>

namespace render {

namespace {
#ifdef __USE_OPENGL
constexpr const char* kVertexGLSL = R"(#version 330
in vec3 aPosition;
in vec4 aColor;
layout(std140) uniform Camera {
  mat4 matrix;
};
out vec4 vColor;
void main() {
  gl_Position = matrix * vec4(aPosition, 1.0);
  vColor = aColor;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 330
in vec4 vColor;
out vec4 fragColor;
void main() {
  fragColor = vColor;
}
)";
#else
constexpr const char* kVertexGLSL = R"(#version 300 es
in vec3 aPosition;
in vec4 aColor;
layout(std140) uniform Camera {
  mat4 matrix;
};
out vec4 vColor;
void main() {
  gl_Position = matrix * vec4(aPosition, 1.0);
  vColor = aColor;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() {
  fragColor = vColor;
}
)";
#endif

constexpr const char* kVertexWGSL = R"(
struct Camera {
  matrix : mat4x4<f32>,
}
@group(0) @binding(0) var<uniform> camera : Camera;

struct VertexInput {
  @location(0) position : vec3<f32>,
  @location(1) color : vec4<f32>,
}

struct VertexOutput {
  @builtin(position) position : vec4<f32>,
  @location(0) vColor : vec4<f32>,
}

@vertex
fn vs_main(input : VertexInput) -> VertexOutput {
  var output : VertexOutput;
  output.position = camera.matrix * vec4<f32>(input.position, 1.0);
  output.vColor = input.color;
  return output;
}

@fragment
fn fs_main(input : VertexOutput) -> @location(0) vec4<f32> {
  return input.vColor;
}
)";
}  // namespace

LineBatch::LineBatch(Engine* engine) : engine_(engine) {}

LineBatch::~LineBatch() {
  RenderBackend* backend = engine_ ? engine_->getBackend() : nullptr;
  if (pipeline_ != kNullPipeline && backend) {
    backend->destroyPipeline(pipeline_);
  }
  if (gpuBuffersCreated_ && backend) {
    backend->destroyBuffer(positionBuf_);
    backend->destroyBuffer(colorBuf_);
  }
  uploads_.destroyAll(backend);
}

void LineBatch::ensurePipeline() {
  if (pipeline_ != kNullPipeline) return;
  PipelineDesc desc;
  desc.vertexGLSL = kVertexGLSL;
  desc.fragmentGLSL = kFragmentGLSL;
  desc.vertexWGSL = kVertexWGSL;
  desc.fragmentWGSL.clear();
  desc.blend.enabled = true;
  desc.blend.srcColor = BlendFactor::SrcAlpha;
  desc.blend.dstColor = BlendFactor::OneMinusSrcAlpha;
  desc.topology = PrimitiveTopology::LineList;
  desc.uniformSize = 64;
  desc.hasTexture = false;
  VertexBinding posBinding;
  posBinding.binding = 0;
  posBinding.stride = sizeof(glm::vec3);
  posBinding.attributes.push_back({0, 0, PixelFormat::RGB32Float, "aPosition"});
  desc.vertexBindings.push_back(posBinding);
  VertexBinding colorBinding;
  colorBinding.binding = 1;
  colorBinding.stride = sizeof(glm::vec4);
  colorBinding.attributes.push_back({1, 0, PixelFormat::RGBA32Float, "aColor"});
  desc.vertexBindings.push_back(colorBinding);
  pipeline_ = engine_->getBackend()->createPipeline(desc);
}

void LineBatch::bufferPoint(glm::vec3 pos, glm::vec4 color) {
  positions_.push_back(pos);
  colors_.push_back(color);
  pointCount_++;
}

void LineBatch::bufferLine(glm::vec3 p1, glm::vec3 p2, glm::vec4 color) {
  bufferPoint(p1, color);
  bufferPoint(p2, color);
}

void LineBatch::reset() {
  positions_.clear();
  colors_.clear();
  pointCount_ = 0;
}

void LineBatch::record(RenderPass& pass, FrameContext& ctx) {
  if (pointCount_ == 0) return;
  ensurePipeline();
  RenderBackend* backend = engine_->getBackend();
  uploads_.gc(engine_ ? engine_->frameIndex : 0, backend);

  if (gpuBuffersCreated_) {
    uploads_.retire(positionBuf_, colorBuf_);
    positionBuf_ = kNullBuffer;
    colorBuf_ = kNullBuffer;
    positionBufSize_ = 0;
    colorBufSize_ = 0;
    gpuBuffersCreated_ = false;
  }

  int posSize = static_cast<int>(pointCount_ * sizeof(glm::vec3));
  int colSize = static_cast<int>(pointCount_ * sizeof(glm::vec4));

  positionBuf_ = backend->createBuffer({posSize, BufferUsage::Vertex});
  positionBufSize_ = posSize;
  colorBuf_ = backend->createBuffer({colSize, BufferUsage::Vertex});
  colorBufSize_ = colSize;
  gpuBuffersCreated_ = true;

  backend->updateBuffer(positionBuf_, positions_.data(), posSize);
  backend->updateBuffer(colorBuf_, colors_.data(), colSize);

  glm::mat4 mvp = ctx.mvp.multipliedMatrix;
  DrawCall dc;
  dc.pipeline = pipeline_;
  dc.uniformBinding = 0;
  dc.uniformOffset = ctx.uniforms->alloc(&mvp, static_cast<int>(sizeof(mvp)));
  dc.uniformSize = static_cast<uint16_t>(sizeof(glm::mat4));
  dc.texture = kNullTexture;
  dc.vertexBuffers.push_back({0, positionBuf_});
  dc.vertexBuffers.push_back({1, colorBuf_});
  dc.vertexCount = pointCount_;
  pass.drawCalls.push_back(std::move(dc));
}

}  // namespace render
