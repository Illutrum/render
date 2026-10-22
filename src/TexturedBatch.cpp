#include "TexturedBatch.hpp"
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
in vec2 aCoord;
layout(std140) uniform Camera {
  mat4 matrix;
};
out vec4 vColor;
out vec2 vCoord;
void main() {
  gl_Position = matrix * vec4(aPosition, 1.0);
  vColor = aColor;
  vCoord = aCoord;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 330
in vec4 vColor;
in vec2 vCoord;
uniform sampler2D uSampler;
out vec4 fragColor;
void main() {
  vec4 texColor = texture(uSampler, vCoord);
  fragColor = vColor * texColor;
}
)";
#else
constexpr const char* kVertexGLSL = R"(#version 300 es
in vec3 aPosition;
in vec4 aColor;
in vec2 aCoord;
layout(std140) uniform Camera {
  mat4 matrix;
};
out vec4 vColor;
out vec2 vCoord;
void main() {
  gl_Position = matrix * vec4(aPosition, 1.0);
  vColor = aColor;
  vCoord = aCoord;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 300 es
precision mediump float;
in vec4 vColor;
in vec2 vCoord;
uniform sampler2D uSampler;
out vec4 fragColor;
void main() {
  fragColor = vColor * texture(uSampler, vCoord);
}
)";
#endif

constexpr const char* kVertexWGSL = R"(
struct Camera {
  matrix : mat4x4<f32>,
}
@group(0) @binding(0) var<uniform> camera : Camera;
@group(0) @binding(1) var uTexture : texture_2d<f32>;
@group(0) @binding(2) var uSampler : sampler;

struct VertexInput {
  @location(0) position : vec3<f32>,
  @location(1) color : vec4<f32>,
  @location(2) coord : vec2<f32>,
}

struct VertexOutput {
  @builtin(position) position : vec4<f32>,
  @location(0) vColor : vec4<f32>,
  @location(1) vCoord : vec2<f32>,
}

@vertex
fn vs_main(input : VertexInput) -> VertexOutput {
  var output : VertexOutput;
  output.position = camera.matrix * vec4<f32>(input.position, 1.0);
  output.vColor = input.color;
  output.vCoord = input.coord;
  return output;
}

@fragment
fn fs_main(input : VertexOutput) -> @location(0) vec4<f32> {
  let texColor = textureSample(uTexture, uSampler, input.vCoord);
  return input.vColor * texColor;
}
)";
}  // namespace

TexturedBatch::TexturedBatch(Engine* engine) : engine_(engine) {}

TexturedBatch::~TexturedBatch() {
  RenderBackend* backend = engine_ ? engine_->getBackend() : nullptr;
  if (pipeline_ != kNullPipeline && backend) {
    backend->destroyPipeline(pipeline_);
  }
  for (auto& buf : buffers_) {
    if (buf.gpuBuffersCreated && backend) {
      backend->destroyBuffer(buf.positionBuf);
      backend->destroyBuffer(buf.colorBuf);
      backend->destroyBuffer(buf.coordBuf);
    }
  }
}

TexturedBatch::Buffer& TexturedBatch::getBuffer(int textureId) {
  for (auto& buf : buffers_) {
    if (buf.textureId == textureId) return buf;
  }
  buffers_.emplace_back();
  buffers_.back().textureId = textureId;
  return buffers_.back();
}

void TexturedBatch::ensurePipeline() {
  if (pipeline_ != kNullPipeline) return;
  PipelineDesc desc;
  desc.vertexGLSL = kVertexGLSL;
  desc.fragmentGLSL = kFragmentGLSL;
  desc.vertexWGSL = kVertexWGSL;
  desc.fragmentWGSL.clear();
  desc.blend.enabled = true;
  desc.blend.srcColor = BlendFactor::SrcAlpha;
  desc.blend.dstColor = BlendFactor::OneMinusSrcAlpha;
  desc.topology = PrimitiveTopology::TriangleList;
  desc.uniformSize = 64;
  desc.hasTexture = true;
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
  VertexBinding coordBinding;
  coordBinding.binding = 2;
  coordBinding.stride = sizeof(glm::vec2);
  coordBinding.attributes.push_back({2, 0, PixelFormat::RG32Float, "aCoord"});
  desc.vertexBindings.push_back(coordBinding);
  pipeline_ = engine_->getBackend()->createPipeline(desc);
}

void TexturedBatch::bufferTexturedPoint(const std::shared_ptr<Texture>& texture,
                                        glm::vec2 point, glm::vec2 coord,
                                        glm::vec4 color) {
  bufferTexturedPoint(texture, glm::vec3(point.x, point.y, 0), coord, color);
}

void TexturedBatch::bufferTexturedPoint(const std::shared_ptr<Texture>& texture,
                                        glm::vec3 point, glm::vec2 coord,
                                        glm::vec4 color) {
  if (texture->unloaded) engine_->reloadTexture(texture);
  if (texture->texture == -1) return;
  Buffer& buf = getBuffer(texture->texture);
  buf.positions.push_back(point);
  buf.colors.push_back(color);
  buf.coords.push_back(coord);
  buf.pointCount++;
}

void TexturedBatch::reset() {
  for (auto& buf : buffers_) {
    buf.positions.clear();
    buf.colors.clear();
    buf.coords.clear();
    buf.pointCount = 0;
  }
}

void TexturedBatch::record(RenderPass& pass, FrameContext& ctx) {
  ensurePipeline();
  RenderBackend* backend = engine_->getBackend();
  glm::mat4 mvp = ctx.mvp.multipliedMatrix;
  uint32_t uniformOffset =
      ctx.uniforms->alloc(&mvp, static_cast<int>(sizeof(mvp)));
  for (auto& buf : buffers_) {
    if (buf.pointCount == 0) continue;

    int posSize = static_cast<int>(buf.pointCount * sizeof(glm::vec3));
    int colSize = static_cast<int>(buf.pointCount * sizeof(glm::vec4));
    int crdSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));

    if (!buf.gpuBuffersCreated || posSize > buf.positionBufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.positionBuf);
      buf.positionBuf = backend->createBuffer({posSize, BufferUsage::Vertex});
      buf.positionBufSize = posSize;
    }
    if (!buf.gpuBuffersCreated || colSize > buf.colorBufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.colorBuf);
      buf.colorBuf = backend->createBuffer({colSize, BufferUsage::Vertex});
      buf.colorBufSize = colSize;
    }
    if (!buf.gpuBuffersCreated || crdSize > buf.coordBufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.coordBuf);
      buf.coordBuf = backend->createBuffer({crdSize, BufferUsage::Vertex});
      buf.coordBufSize = crdSize;
    }
    buf.gpuBuffersCreated = true;

    backend->updateBuffer(buf.positionBuf, buf.positions.data(), posSize);
    backend->updateBuffer(buf.colorBuf, buf.colors.data(), colSize);
    backend->updateBuffer(buf.coordBuf, buf.coords.data(), crdSize);

    DrawCall dc;
    dc.pipeline = pipeline_;
    dc.uniformBinding = 0;
    dc.uniformOffset = uniformOffset;
    dc.uniformSize = static_cast<uint16_t>(sizeof(glm::mat4));
    dc.textureBinding = 0;
    dc.texture = static_cast<TextureHandle>(buf.textureId);
    dc.vertexBuffers.push_back({0, buf.positionBuf});
    dc.vertexBuffers.push_back({1, buf.colorBuf});
    dc.vertexBuffers.push_back({2, buf.coordBuf});
    dc.vertexCount = buf.pointCount;
    pass.drawCalls.push_back(std::move(dc));
  }
}

}  // namespace render
