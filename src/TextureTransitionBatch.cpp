#include "TextureTransitionBatch.hpp"
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
in vec3 aCoord1;
in vec3 aCoord2;
layout(std140) uniform Camera {
  mat4 matrix;
};
out vec4 vColor;
out vec3 vCoord1;
out vec3 vCoord2;
void main() {
  gl_Position = matrix * vec4(aPosition, 1.0);
  vColor = aColor;
  vCoord1 = aCoord1;
  vCoord2 = aCoord2;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 330
in vec4 vColor;
in vec3 vCoord1;
in vec3 vCoord2;
uniform sampler2D uSampler;
out vec4 fragColor;
void main() {
  vec4 textureColor1 = texture(uSampler, vCoord1.xy);
  vec4 textureColor2 = texture(uSampler, vCoord2.xy);
  fragColor = vColor * (textureColor1 * vCoord1.z + textureColor2 * vCoord2.z);
}
)";
#else
constexpr const char* kVertexGLSL = R"(#version 300 es
in vec3 aPosition;
in vec4 aColor;
in vec3 aCoord1;
in vec3 aCoord2;
layout(std140) uniform Camera {
  mat4 matrix;
};
out vec4 vColor;
out highp vec3 vCoord1;
out highp vec3 vCoord2;
void main() {
  gl_Position = matrix * vec4(aPosition, 1.0);
  vColor = aColor;
  vCoord1 = aCoord1;
  vCoord2 = aCoord2;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 300 es
precision mediump float;
in vec4 vColor;
in highp vec3 vCoord1;
in highp vec3 vCoord2;
uniform sampler2D uSampler;
out vec4 fragColor;
void main() {
  vec4 textureColor1 = texture(uSampler, vCoord1.xy);
  vec4 textureColor2 = texture(uSampler, vCoord2.xy);
  fragColor = vColor * (textureColor1 * vCoord1.z + textureColor2 * vCoord2.z);
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
  @location(2) coord1 : vec3<f32>,
  @location(3) coord2 : vec3<f32>,
}

struct VertexOutput {
  @builtin(position) position : vec4<f32>,
  @location(0) vColor : vec4<f32>,
  @location(1) vCoord1 : vec3<f32>,
  @location(2) vCoord2 : vec3<f32>,
}

@vertex
fn vs_main(input : VertexInput) -> VertexOutput {
  var output : VertexOutput;
  output.position = camera.matrix * vec4<f32>(input.position, 1.0);
  output.vColor = input.color;
  output.vCoord1 = input.coord1;
  output.vCoord2 = input.coord2;
  return output;
}

@fragment
fn fs_main(input : VertexOutput) -> @location(0) vec4<f32> {
  let textureColor1 = textureSample(uTexture, uSampler, input.vCoord1.xy);
  let textureColor2 = textureSample(uTexture, uSampler, input.vCoord2.xy);
  return input.vColor * (textureColor1 * input.vCoord1.z + textureColor2 * input.vCoord2.z);
}
)";
}  // namespace

TextureTransitionBatch::TextureTransitionBatch(Engine* engine) : engine_(engine) {}

TextureTransitionBatch::~TextureTransitionBatch() {
  RenderBackend* backend = engine_ ? engine_->getBackend() : nullptr;
  if (pipeline_ != kNullPipeline && backend) {
    backend->destroyPipeline(pipeline_);
  }
  for (auto& buf : buffers_) {
    if (buf.gpuBuffersCreated && backend) {
      backend->destroyBuffer(buf.positionBuf);
      backend->destroyBuffer(buf.colorBuf);
      backend->destroyBuffer(buf.coord1Buf);
      backend->destroyBuffer(buf.coord2Buf);
    }
  }
}

TextureTransitionBatch::Buffer& TextureTransitionBatch::getBuffer(int textureId) {
  for (auto& buf : buffers_) {
    if (buf.textureId == textureId) return buf;
  }
  buffers_.emplace_back();
  buffers_.back().textureId = textureId;
  return buffers_.back();
}

void TextureTransitionBatch::ensurePipeline() {
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
  VertexBinding coord1Binding;
  coord1Binding.binding = 2;
  coord1Binding.stride = sizeof(glm::vec3);
  coord1Binding.attributes.push_back({2, 0, PixelFormat::RGB32Float, "aCoord1"});
  desc.vertexBindings.push_back(coord1Binding);
  VertexBinding coord2Binding;
  coord2Binding.binding = 3;
  coord2Binding.stride = sizeof(glm::vec3);
  coord2Binding.attributes.push_back({3, 0, PixelFormat::RGB32Float, "aCoord2"});
  desc.vertexBindings.push_back(coord2Binding);
  pipeline_ = engine_->getBackend()->createPipeline(desc);
}

void TextureTransitionBatch::bufferTextureTransitionPoint(
    const std::shared_ptr<Texture>& texture, glm::vec2 point, glm::vec4 color,
    glm::vec3 coord1, glm::vec3 coord2) {
  bufferTextureTransitionPoint(texture, glm::vec3(point.x, point.y, 0), color,
                               coord1, coord2);
}

void TextureTransitionBatch::bufferTextureTransitionPoint(
    const std::shared_ptr<Texture>& texture, glm::vec3 point, glm::vec4 color,
    glm::vec3 coord1, glm::vec3 coord2) {
  if (texture->unloaded) engine_->reloadTexture(texture);
  if (texture->texture == -1) return;
  Buffer& buf = getBuffer(texture->texture);
  buf.positions.push_back(point);
  buf.colors.push_back(color);
  buf.coords1.push_back(coord1);
  buf.coords2.push_back(coord2);
  buf.pointCount++;
}

void TextureTransitionBatch::reset() {
  for (auto& buf : buffers_) {
    buf.positions.clear();
    buf.colors.clear();
    buf.coords1.clear();
    buf.coords2.clear();
    buf.pointCount = 0;
  }
}

void TextureTransitionBatch::record(RenderPass& pass, FrameContext& ctx) {
  ensurePipeline();
  RenderBackend* backend = engine_->getBackend();
  glm::mat4 mvp = ctx.mvp.multipliedMatrix;
  uint32_t uniformOffset =
      ctx.uniforms->alloc(&mvp, static_cast<int>(sizeof(mvp)));
  for (auto& buf : buffers_) {
    if (buf.pointCount == 0) continue;

    int posSize = static_cast<int>(buf.pointCount * sizeof(glm::vec3));
    int colSize = static_cast<int>(buf.pointCount * sizeof(glm::vec4));
    int c1Size = static_cast<int>(buf.pointCount * sizeof(glm::vec3));
    int c2Size = static_cast<int>(buf.pointCount * sizeof(glm::vec3));

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
    if (!buf.gpuBuffersCreated || c1Size > buf.coord1BufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.coord1Buf);
      buf.coord1Buf = backend->createBuffer({c1Size, BufferUsage::Vertex});
      buf.coord1BufSize = c1Size;
    }
    if (!buf.gpuBuffersCreated || c2Size > buf.coord2BufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.coord2Buf);
      buf.coord2Buf = backend->createBuffer({c2Size, BufferUsage::Vertex});
      buf.coord2BufSize = c2Size;
    }
    buf.gpuBuffersCreated = true;

    backend->updateBuffer(buf.positionBuf, buf.positions.data(), posSize);
    backend->updateBuffer(buf.colorBuf, buf.colors.data(), colSize);
    backend->updateBuffer(buf.coord1Buf, buf.coords1.data(), c1Size);
    backend->updateBuffer(buf.coord2Buf, buf.coords2.data(), c2Size);

    DrawCall dc;
    dc.pipeline = pipeline_;
    dc.uniformBinding = 0;
    dc.uniformOffset = uniformOffset;
    dc.uniformSize = static_cast<uint16_t>(sizeof(glm::mat4));
    dc.textureBinding = 0;
    dc.texture = static_cast<TextureHandle>(buf.textureId);
    dc.vertexBuffers.push_back({0, buf.positionBuf});
    dc.vertexBuffers.push_back({1, buf.colorBuf});
    dc.vertexBuffers.push_back({2, buf.coord1Buf});
    dc.vertexBuffers.push_back({3, buf.coord2Buf});
    dc.vertexCount = buf.pointCount;
    pass.drawCalls.push_back(std::move(dc));
  }
}

}  // namespace render
