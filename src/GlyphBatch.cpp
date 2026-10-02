#include "GlyphBatch.hpp"
#include "Engine.hpp"
#include "backend/RenderBackend.hpp"
#include "backend/FrameGraph.hpp"

#include <cmath>
#include <cstring>

namespace fgfx {

namespace {
#ifdef __USE_OPENGL
constexpr const char* kVertexGLSL = R"(#version 330
in vec3 aPosition;
in vec4 aColor;
in vec2 aCoord;
layout(std140) uniform MsdfParams {
  mat4 matrix;
  float pxRange;
  vec2 atlasSize;
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
layout(std140) uniform MsdfParams {
  mat4 matrix;
  float pxRange;
  vec2 atlasSize;
};
out vec4 fragColor;
float median(float r, float g, float b) {
  return max(min(r, g), min(max(r, g), b));
}
float screenPxRange() {
  vec2 unitRange = vec2(pxRange) / max(atlasSize, vec2(1.0));
  vec2 screenTexSize = vec2(1.0) / fwidth(vCoord);
  return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}
void main() {
  vec3 msd = texture(uSampler, vCoord).rgb;
  float sd = median(msd.r, msd.g, msd.b);
  float screenPxDistance = screenPxRange() * (sd - 0.5);
  float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
  fragColor = vec4(vColor.rgb, vColor.a * opacity);
}
)";
#else
constexpr const char* kVertexGLSL = R"(#version 300 es
in vec3 aPosition;
in vec4 aColor;
in vec2 aCoord;
layout(std140) uniform MsdfParams {
  mat4 matrix;
  float pxRange;
  vec2 atlasSize;
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
precision highp float;
in vec4 vColor;
in vec2 vCoord;
uniform sampler2D uSampler;
layout(std140) uniform MsdfParams {
  mat4 matrix;
  float pxRange;
  vec2 atlasSize;
};
out vec4 fragColor;
float median(float r, float g, float b) {
  return max(min(r, g), min(max(r, g), b));
}
float screenPxRange() {
  vec2 unitRange = vec2(pxRange) / max(atlasSize, vec2(1.0));
  vec2 screenTexSize = vec2(1.0) / fwidth(vCoord);
  return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}
void main() {
  vec3 msd = texture(uSampler, vCoord).rgb;
  float sd = median(msd.r, msd.g, msd.b);
  float screenPxDistance = screenPxRange() * (sd - 0.5);
  float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
  fragColor = vec4(vColor.rgb, vColor.a * opacity);
}
)";
#endif

constexpr const char* kVertexWGSL = R"(
struct MsdfParams {
  matrix : mat4x4<f32>,
  pxRange : f32,
  atlasSize : vec2<f32>,
}
@group(0) @binding(0) var<uniform> params : MsdfParams;
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
  output.position = params.matrix * vec4<f32>(input.position, 1.0);
  output.vColor = input.color;
  output.vCoord = input.coord;
  return output;
}

fn median3(r : f32, g : f32, b : f32) -> f32 {
  return max(min(r, g), min(max(r, g), b));
}

fn screenPxRange(vCoord : vec2<f32>) -> f32 {
  let unitRange = vec2<f32>(params.pxRange) / max(params.atlasSize, vec2<f32>(1.0));
  let fw = abs(dpdx(vCoord)) + abs(dpdy(vCoord));
  let screenTexSize = vec2<f32>(1.0) / max(fw, vec2<f32>(1.0e-5));
  return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

@fragment
fn fs_main(input : VertexOutput) -> @location(0) vec4<f32> {
  let msd = textureSample(uTexture, uSampler, input.vCoord).rgb;
  let sd = median3(msd.r, msd.g, msd.b);
  let screenPxDistance = screenPxRange(input.vCoord) * (sd - 0.5);
  let opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
  return vec4<f32>(input.vColor.rgb, input.vColor.a * opacity);
}
)";

struct MsdfUbo {
  glm::mat4 matrix;
  float pxRange;
  float pad;
  glm::vec2 atlasSize;
};
static_assert(sizeof(MsdfUbo) == 80, "MSDF UBO must be 80 bytes (std140)");
}  // namespace

GlyphBatch::GlyphBatch(Engine* engine) : engine_(engine) {}

GlyphBatch::~GlyphBatch() {
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

void GlyphBatch::setPxRange(float range) { pxRange_ = range; }
void GlyphBatch::setAtlasSize(glm::vec2 size) { atlasSize_ = size; }

GlyphBatch::Buffer& GlyphBatch::getBuffer(int textureId) {
  for (auto& buf : buffers_) {
    if (buf.textureId == textureId) return buf;
  }
  buffers_.emplace_back();
  buffers_.back().textureId = textureId;
  return buffers_.back();
}

void GlyphBatch::ensurePipeline() {
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
  desc.uniformSize = 80;
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

void GlyphBatch::bufferPoint(glm::vec3 pos, glm::vec4 color, glm::vec2 coord,
                             Buffer& buf) {
  buf.positions.push_back(pos);
  buf.colors.push_back(color);
  buf.coords.push_back(coord);
  buf.pointCount++;
}

void GlyphBatch::bufferSprite(const std::shared_ptr<Sprite>& sprite,
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

void GlyphBatch::bufferSprite(const std::shared_ptr<Sprite>& sprite,
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

void GlyphBatch::rawBufferSprite(const std::shared_ptr<Sprite>& sprite,
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

void GlyphBatch::reset() {
  for (auto& buf : buffers_) {
    buf.positions.clear();
    buf.colors.clear();
    buf.coords.clear();
    buf.pointCount = 0;
  }
}

void GlyphBatch::record(RenderPass& pass, FrameContext& ctx) {
  ensurePipeline();
  RenderBackend* backend = engine_->getBackend();
  uploads_.gc(engine_ ? engine_->frameIndex : 0, backend);
  MsdfUbo ubo;
  ubo.matrix = ctx.mvp.multipliedMatrix;
  ubo.pxRange = pxRange_;
  ubo.pad = 0.0f;
  ubo.atlasSize = atlasSize_;
  uint32_t uniformOffset =
      ctx.uniforms->alloc(&ubo, static_cast<int>(sizeof(ubo)));

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
    dc.uniformSize = static_cast<uint16_t>(sizeof(MsdfUbo));
    dc.textureBinding = 0;
    dc.texture = static_cast<TextureHandle>(buf.textureId);
    dc.vertexBuffers.push_back({0, buf.positionBuf});
    dc.vertexBuffers.push_back({1, buf.colorBuf});
    dc.vertexBuffers.push_back({2, buf.coordBuf});
    dc.vertexCount = buf.pointCount;
    pass.drawCalls.push_back(std::move(dc));
  }
}

}  // namespace fgfx
