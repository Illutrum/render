#include "SpriteBatch.hpp"
#include "Engine.hpp"
#include "backend/RenderBackend.hpp"
#include "backend/FrameGraph.hpp"

#include <cmath>
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

SpriteBatch::SpriteBatch(Engine* engine) : engine_(engine) {}

SpriteBatch::~SpriteBatch() {
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
  uploads_.destroyAll(backend);
}

SpriteBatch::Buffer& SpriteBatch::getBuffer(int textureId) {
  for (auto& buf : buffers_) {
    if (buf.textureId == textureId) return buf;
  }
  buffers_.emplace_back();
  buffers_.back().textureId = textureId;
  return buffers_.back();
}

void SpriteBatch::ensurePipeline() {
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

void SpriteBatch::bufferPoint(glm::vec3 pos, glm::vec4 color, glm::vec2 coord,
                              Buffer& buf) {
  buf.positions.push_back(pos);
  buf.colors.push_back(color);
  buf.coords.push_back(coord);
  buf.pointCount++;
}

void SpriteBatch::bufferSprite(const std::shared_ptr<Sprite>& sprite,
                                glm::vec2 pos, glm::vec4 color, float size,
                                float rotation) {
  if (sprite->unloaded) engine_->reloadSprite(sprite);
  if (sprite->textureId == -1) return;
  Buffer& buf = getBuffer(sprite->textureId);

  double sv = sin(rotation);
  double cv = cos(rotation);
  glm::vec2 up(sv * size, -cv * size);
  glm::vec2 right(cv * size * sprite->ratio, sv * size * sprite->ratio);
  glm::vec2 m1 = up - right;
  glm::vec2 m2 = up + right;

  bufferPoint(glm::vec3(pos + m1, 0), color, sprite->coords[0], buf);
  bufferPoint(glm::vec3(pos + m2, 0), color, sprite->coords[1], buf);
  bufferPoint(glm::vec3(pos - m1, 0), color, sprite->coords[2], buf);
  bufferPoint(glm::vec3(pos - m2, 0), color, sprite->coords[3], buf);
  bufferPoint(glm::vec3(pos + m1, 0), color, sprite->coords[0], buf);
  bufferPoint(glm::vec3(pos - m1, 0), color, sprite->coords[2], buf);
}

void SpriteBatch::bufferSprite(const std::shared_ptr<Sprite>& sprite,
                                const glm::mat4& mat, glm::vec4 color) {
  if (sprite->unloaded) engine_->reloadSprite(sprite);
  if (sprite->textureId == -1) return;
  Buffer& buf = getBuffer(sprite->textureId);

  auto r = sprite->ratio;
  glm::vec3 p1 = glm::vec3(mat * glm::vec4(-1 * r, -1, 0, 1));
  glm::vec3 p2 = glm::vec3(mat * glm::vec4(1 * r, -1, 0, 1));
  glm::vec3 p3 = glm::vec3(mat * glm::vec4(1 * r, 1, 0, 1));
  glm::vec3 p4 = glm::vec3(mat * glm::vec4(-1 * r, 1, 0, 1));

  bufferPoint(p1, color, sprite->coords[0], buf);
  bufferPoint(p2, color, sprite->coords[1], buf);
  bufferPoint(p3, color, sprite->coords[2], buf);
  bufferPoint(p4, color, sprite->coords[3], buf);
  bufferPoint(p1, color, sprite->coords[0], buf);
  bufferPoint(p3, color, sprite->coords[2], buf);
}

void SpriteBatch::rawBufferSprite(const std::shared_ptr<Sprite>& sprite,
                                   const glm::mat4& mat, glm::vec4 color) {
  if (sprite->unloaded) engine_->reloadSprite(sprite);
  if (sprite->textureId == -1) return;
  Buffer& buf = getBuffer(sprite->textureId);

  glm::vec3 p1 = glm::vec3(mat * glm::vec4(-1, -1, 0, 1));
  glm::vec3 p2 = glm::vec3(mat * glm::vec4(1, -1, 0, 1));
  glm::vec3 p3 = glm::vec3(mat * glm::vec4(1, 1, 0, 1));
  glm::vec3 p4 = glm::vec3(mat * glm::vec4(-1, 1, 0, 1));

  bufferPoint(p1, color, sprite->coords[0], buf);
  bufferPoint(p2, color, sprite->coords[1], buf);
  bufferPoint(p3, color, sprite->coords[2], buf);
  bufferPoint(p4, color, sprite->coords[3], buf);
  bufferPoint(p1, color, sprite->coords[0], buf);
  bufferPoint(p3, color, sprite->coords[2], buf);
}

void SpriteBatch::reset() {
  for (auto& buf : buffers_) {
    buf.positions.clear();
    buf.colors.clear();
    buf.coords.clear();
    buf.pointCount = 0;
  }
}

void SpriteBatch::record(RenderPass& pass, FrameContext& ctx) {
  ensurePipeline();
  RenderBackend* backend = engine_->getBackend();
  uploads_.gc(engine_ ? engine_->frameIndex : 0, backend);
  glm::mat4 mvp = ctx.mvp.multipliedMatrix;
  uint32_t uniformOffset =
      ctx.uniforms->alloc(&mvp, static_cast<int>(sizeof(mvp)));
  for (auto& buf : buffers_) {
    if (buf.pointCount == 0) continue;

    if (buf.gpuBuffersCreated) {
      uploads_.retire(buf.positionBuf, buf.colorBuf, buf.coordBuf);
      buf.positionBuf = kNullBuffer;
      buf.colorBuf = kNullBuffer;
      buf.coordBuf = kNullBuffer;
      buf.positionBufSize = 0;
      buf.colorBufSize = 0;
      buf.coordBufSize = 0;
      buf.gpuBuffersCreated = false;
    }

    int posSize = static_cast<int>(buf.pointCount * sizeof(glm::vec3));
    int colSize = static_cast<int>(buf.pointCount * sizeof(glm::vec4));
    int crdSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));

    buf.positionBuf = backend->createBuffer({posSize, BufferUsage::Vertex});
    buf.positionBufSize = posSize;
    buf.colorBuf = backend->createBuffer({colSize, BufferUsage::Vertex});
    buf.colorBufSize = colSize;
    buf.coordBuf = backend->createBuffer({crdSize, BufferUsage::Vertex});
    buf.coordBufSize = crdSize;
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
