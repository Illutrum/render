#include "NormalMappedSpriteBatch.hpp"
#include "Engine.hpp"
#include "backend/RenderBackend.hpp"
#include "backend/FrameGraph.hpp"

#include <cstring>

namespace fgfx {

namespace {
#ifdef __USE_OPENGL
constexpr const char* kVertexGLSL = R"(#version 330
in vec2 aPosition;
in vec2 aNormal;
in vec4 aColor;
in vec2 aColorCoord;
in vec2 aNormalCoord;
layout(std140) uniform Camera {
  mat4 matrix;
};
out vec2 vNormal;
out vec4 vColor;
out vec2 vColorCoord;
out vec2 vNormalCoord;
void main() {
  gl_Position = matrix * vec4(aPosition, 0.0, 1.0);
  vNormal = aNormal;
  vColor = aColor;
  vColorCoord = aColorCoord;
  vNormalCoord = aNormalCoord;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 330
in vec2 vNormal;
in vec4 vColor;
in vec2 vColorCoord;
in vec2 vNormalCoord;
uniform sampler2D uSampler;
out vec4 fragColor;
void main() {
  vec4 textureColor = texture(uSampler, vColorCoord);
  vec4 color = textureColor * vColor;
  vec4 textureNormalPixel = texture(uSampler, vNormalCoord);
  vec3 normal = textureNormalPixel.xyz * 2.0 - vec3(1.0, 1.0, 1.0);
  vec3 normalYAxis = vec3(vNormal.x, vNormal.y, 0);
  vec3 normalXAxis = vec3(normalYAxis.y, -normalYAxis.x, 0);
  normal = normalXAxis * normal.x + normalYAxis * normal.y + vec3(0, 0, normal.z);
  normal.xy = normal.xy * -1.0;
  vec3 lightDir = normalize(vec3(-1, -3, 6));
  vec3 eyeDir = normalize(vec3(0, 0, 1));
  vec3 reflection = normalize(-reflect(lightDir, normal));
  float shininess = 23.0;
  vec3 diffuse = color.xyz * max(dot(normal, lightDir), 0.0);
  diffuse = clamp(diffuse, 0.0, 1.0);
  vec3 specular = color.xyz * pow(max(dot(reflection, eyeDir), 0.0), 0.3 * shininess);
  fragColor = vec4(diffuse + specular, color.a * textureNormalPixel.a);
}
)";
#else
constexpr const char* kVertexGLSL = R"(#version 300 es
in vec2 aPosition;
in vec2 aNormal;
in vec4 aColor;
in vec2 aColorCoord;
in vec2 aNormalCoord;
layout(std140) uniform Camera {
  mat4 matrix;
};
out vec2 vNormal;
out vec4 vColor;
out highp vec2 vColorCoord;
out highp vec2 vNormalCoord;
void main() {
  gl_Position = matrix * vec4(aPosition, 0.0, 1.0);
  vNormal = aNormal;
  vColor = aColor;
  vColorCoord = aColorCoord;
  vNormalCoord = aNormalCoord;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 300 es
precision mediump float;
in vec2 vNormal;
in vec4 vColor;
in highp vec2 vColorCoord;
in highp vec2 vNormalCoord;
uniform sampler2D uSampler;
out vec4 fragColor;
void main() {
  vec4 textureColor = texture(uSampler, vColorCoord);
  vec4 color = textureColor * vColor;
  vec4 textureNormalPixel = texture(uSampler, vNormalCoord);
  vec3 normal = textureNormalPixel.xyz * 2.0 - vec3(1.0, 1.0, 1.0);
  vec3 normalYAxis = vec3(vNormal.x, vNormal.y, 0);
  vec3 normalXAxis = vec3(normalYAxis.y, -normalYAxis.x, 0);
  normal = normalXAxis * normal.x + normalYAxis * normal.y + vec3(0, 0, normal.z);
  normal.xy = normal.xy * -1.0;
  vec3 lightDir = normalize(vec3(-1, -3, 6));
  vec3 eyeDir = normalize(vec3(0, 0, 1));
  vec3 reflection = normalize(-reflect(lightDir, normal));
  float shininess = 23.0;
  vec3 diffuse = color.xyz * max(dot(normal, lightDir), 0.0);
  diffuse = clamp(diffuse, 0.0, 1.0);
  vec3 specular = color.xyz * pow(max(dot(reflection, eyeDir), 0.0), 0.3 * shininess);
  fragColor = vec4(diffuse + specular, color.a * textureNormalPixel.a);
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
  @location(0) position : vec2<f32>,
  @location(1) normal : vec2<f32>,
  @location(2) color : vec4<f32>,
  @location(3) colorCoord : vec2<f32>,
  @location(4) normalCoord : vec2<f32>,
}

struct VertexOutput {
  @builtin(position) position : vec4<f32>,
  @location(0) vNormal : vec2<f32>,
  @location(1) vColor : vec4<f32>,
  @location(2) vColorCoord : vec2<f32>,
  @location(3) vNormalCoord : vec2<f32>,
}

@vertex
fn vs_main(input : VertexInput) -> VertexOutput {
  var output : VertexOutput;
  output.position = camera.matrix * vec4<f32>(input.position, 0.0, 1.0);
  output.vNormal = input.normal;
  output.vColor = input.color;
  output.vColorCoord = input.colorCoord;
  output.vNormalCoord = input.normalCoord;
  return output;
}

@fragment
fn fs_main(input : VertexOutput) -> @location(0) vec4<f32> {
  let textureColor = textureSample(uTexture, uSampler, input.vColorCoord);
  let color = textureColor * input.vColor;
  let textureNormalPixel = textureSample(uTexture, uSampler, input.vNormalCoord);
  var normal = textureNormalPixel.xyz * 2.0 - vec3<f32>(1.0, 1.0, 1.0);
  let normalYAxis = vec3<f32>(input.vNormal.x, input.vNormal.y, 0.0);
  let normalXAxis = vec3<f32>(normalYAxis.y, -normalYAxis.x, 0.0);
  normal = normalXAxis * normal.x + normalYAxis * normal.y + vec3<f32>(0.0, 0.0, normal.z);
  normal = vec3<f32>(normal.xy * -1.0, normal.z);
  let lightDir = normalize(vec3<f32>(-1.0, -3.0, 6.0));
  let eyeDir = normalize(vec3<f32>(0.0, 0.0, 1.0));
  let reflection = normalize(-reflect(lightDir, normal));
  let shininess = 23.0;
  var diffuse = color.xyz * max(dot(normal, lightDir), 0.0);
  diffuse = clamp(diffuse, vec3<f32>(0.0), vec3<f32>(1.0));
  let specular = color.xyz * pow(max(dot(reflection, eyeDir), 0.0), 0.3 * shininess);
  return vec4<f32>(diffuse + specular, color.a * textureNormalPixel.a);
}
)";
}  // namespace

NormalMappedSpriteBatch::NormalMappedSpriteBatch(Engine* engine) : engine_(engine) {}

NormalMappedSpriteBatch::~NormalMappedSpriteBatch() {
  RenderBackend* backend = engine_ ? engine_->getBackend() : nullptr;
  if (pipeline_ != kNullPipeline && backend) {
    backend->destroyPipeline(pipeline_);
  }
  for (auto& buf : buffers_) {
    if (buf.gpuBuffersCreated && backend) {
      backend->destroyBuffer(buf.positionBuf);
      backend->destroyBuffer(buf.normalBuf);
      backend->destroyBuffer(buf.colorBuf);
      backend->destroyBuffer(buf.colorCoordBuf);
      backend->destroyBuffer(buf.normalCoordBuf);
    }
  }
}

NormalMappedSpriteBatch::Buffer& NormalMappedSpriteBatch::getBuffer(int textureId) {
  for (auto& buf : buffers_) {
    if (buf.textureId == textureId) return buf;
  }
  buffers_.emplace_back();
  buffers_.back().textureId = textureId;
  return buffers_.back();
}

void NormalMappedSpriteBatch::ensurePipeline() {
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
  posBinding.stride = sizeof(glm::vec2);
  posBinding.attributes.push_back({0, 0, PixelFormat::RG32Float, "aPosition"});
  desc.vertexBindings.push_back(posBinding);
  VertexBinding normalBinding;
  normalBinding.binding = 1;
  normalBinding.stride = sizeof(glm::vec2);
  normalBinding.attributes.push_back({1, 0, PixelFormat::RG32Float, "aNormal"});
  desc.vertexBindings.push_back(normalBinding);
  VertexBinding colorBinding;
  colorBinding.binding = 2;
  colorBinding.stride = sizeof(glm::vec4);
  colorBinding.attributes.push_back({2, 0, PixelFormat::RGBA32Float, "aColor"});
  desc.vertexBindings.push_back(colorBinding);
  VertexBinding colorCoordBinding;
  colorCoordBinding.binding = 3;
  colorCoordBinding.stride = sizeof(glm::vec2);
  colorCoordBinding.attributes.push_back({3, 0, PixelFormat::RG32Float, "aColorCoord"});
  desc.vertexBindings.push_back(colorCoordBinding);
  VertexBinding normalCoordBinding;
  normalCoordBinding.binding = 4;
  normalCoordBinding.stride = sizeof(glm::vec2);
  normalCoordBinding.attributes.push_back({4, 0, PixelFormat::RG32Float, "aNormalCoord"});
  desc.vertexBindings.push_back(normalCoordBinding);
  pipeline_ = engine_->getBackend()->createPipeline(desc);
}

void NormalMappedSpriteBatch::bufferPoint(Buffer& buf, glm::vec2 pos,
                                          glm::vec2 normal, glm::vec4 color,
                                          glm::vec2 colorCoord,
                                          glm::vec2 normalCoord) {
  buf.positions.push_back(pos);
  buf.normals.push_back(normal);
  buf.colors.push_back(color);
  buf.colorCoords.push_back(colorCoord);
  buf.normalCoords.push_back(normalCoord);
  buf.pointCount++;
}

void NormalMappedSpriteBatch::bufferSprite(const std::shared_ptr<Sprite>& sprite,
                                           const glm::mat4& mat, glm::vec4 color) {
  if (sprite->unloaded) engine_->reloadSprite(sprite);
  if (sprite->textureId == -1) return;
  Buffer& buf = getBuffer(sprite->textureId);

  auto r = sprite->ratio * 2;
  glm::vec2 normal = glm::normalize(glm::vec2(glm::mat3(mat) * glm::vec3(0, 1, 0)));
  glm::vec2 p1 = glm::vec2(mat * glm::vec4(-1 * r, -1, 0, 1));
  glm::vec2 p2 = glm::vec2(mat * glm::vec4(1 * r, -1, 0, 1));
  glm::vec2 p3 = glm::vec2(mat * glm::vec4(1 * r, 1, 0, 1));
  glm::vec2 p4 = glm::vec2(mat * glm::vec4(-1 * r, 1, 0, 1));

  glm::vec2 colorCoords[4] = {
      sprite->coords[0],
      sprite->coords[1],
      (sprite->coords[2] + sprite->coords[1]) * 0.5f,
      (sprite->coords[3] + sprite->coords[0]) * 0.5f
  };
  glm::vec2 normalCoords[4] = {
      (sprite->coords[0] + sprite->coords[3]) * 0.5f,
      (sprite->coords[1] + sprite->coords[2]) * 0.5f,
      sprite->coords[2],
      sprite->coords[3]
  };

  bufferPoint(buf, p1, normal, color, colorCoords[0], normalCoords[0]);
  bufferPoint(buf, p2, normal, color, colorCoords[1], normalCoords[1]);
  bufferPoint(buf, p3, normal, color, colorCoords[2], normalCoords[2]);
  bufferPoint(buf, p4, normal, color, colorCoords[3], normalCoords[3]);
  bufferPoint(buf, p1, normal, color, colorCoords[0], normalCoords[0]);
  bufferPoint(buf, p3, normal, color, colorCoords[2], normalCoords[2]);
}

void NormalMappedSpriteBatch::rawBufferSprite(const std::shared_ptr<Sprite>& sprite,
                                              const glm::mat4& mat, glm::vec4 color) {
  if (sprite->unloaded) engine_->reloadSprite(sprite);
  if (sprite->textureId == -1) return;
  Buffer& buf = getBuffer(sprite->textureId);

  glm::vec2 normal = glm::normalize(glm::vec2(mat * glm::vec4(1, 0, 0, 0)));
  glm::vec2 p1 = glm::vec2(mat * glm::vec4(-1, -1, 0, 1));
  glm::vec2 p2 = glm::vec2(mat * glm::vec4(1, -1, 0, 1));
  glm::vec2 p3 = glm::vec2(mat * glm::vec4(1, 1, 0, 1));
  glm::vec2 p4 = glm::vec2(mat * glm::vec4(-1, 1, 0, 1));

  glm::vec2 colorCoords[4] = {
      sprite->coords[0] + sprite->coords[3] * 0.5f,
      sprite->coords[1] + sprite->coords[2] * 0.5f,
      sprite->coords[2],
      sprite->coords[3]
  };
  glm::vec2* normalCoords = sprite->coords;

  bufferPoint(buf, p1, normal, color, colorCoords[0], normalCoords[0]);
  bufferPoint(buf, p2, normal, color, colorCoords[1], normalCoords[1]);
  bufferPoint(buf, p3, normal, color, colorCoords[2], normalCoords[2]);
  bufferPoint(buf, p4, normal, color, colorCoords[3], normalCoords[3]);
  bufferPoint(buf, p1, normal, color, colorCoords[0], normalCoords[0]);
  bufferPoint(buf, p3, normal, color, colorCoords[2], normalCoords[2]);
}

void NormalMappedSpriteBatch::reset() {
  for (auto& buf : buffers_) {
    buf.positions.clear();
    buf.normals.clear();
    buf.colors.clear();
    buf.colorCoords.clear();
    buf.normalCoords.clear();
    buf.pointCount = 0;
  }
}

void NormalMappedSpriteBatch::record(RenderPass& pass, FrameContext& ctx) {
  ensurePipeline();
  RenderBackend* backend = engine_->getBackend();
  glm::mat4 mvp = ctx.mvp.multipliedMatrix;
  uint32_t uniformOffset =
      ctx.uniforms->alloc(&mvp, static_cast<int>(sizeof(mvp)));
  for (auto& buf : buffers_) {
    if (buf.pointCount == 0) continue;

    int posSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));
    int nrmSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));
    int colSize = static_cast<int>(buf.pointCount * sizeof(glm::vec4));
    int ccSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));
    int ncSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));

    if (!buf.gpuBuffersCreated || posSize > buf.positionBufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.positionBuf);
      buf.positionBuf = backend->createBuffer({posSize, BufferUsage::Vertex});
      buf.positionBufSize = posSize;
    }
    if (!buf.gpuBuffersCreated || nrmSize > buf.normalBufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.normalBuf);
      buf.normalBuf = backend->createBuffer({nrmSize, BufferUsage::Vertex});
      buf.normalBufSize = nrmSize;
    }
    if (!buf.gpuBuffersCreated || colSize > buf.colorBufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.colorBuf);
      buf.colorBuf = backend->createBuffer({colSize, BufferUsage::Vertex});
      buf.colorBufSize = colSize;
    }
    if (!buf.gpuBuffersCreated || ccSize > buf.colorCoordBufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.colorCoordBuf);
      buf.colorCoordBuf = backend->createBuffer({ccSize, BufferUsage::Vertex});
      buf.colorCoordBufSize = ccSize;
    }
    if (!buf.gpuBuffersCreated || ncSize > buf.normalCoordBufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.normalCoordBuf);
      buf.normalCoordBuf = backend->createBuffer({ncSize, BufferUsage::Vertex});
      buf.normalCoordBufSize = ncSize;
    }
    buf.gpuBuffersCreated = true;

    backend->updateBuffer(buf.positionBuf, buf.positions.data(), posSize);
    backend->updateBuffer(buf.normalBuf, buf.normals.data(), nrmSize);
    backend->updateBuffer(buf.colorBuf, buf.colors.data(), colSize);
    backend->updateBuffer(buf.colorCoordBuf, buf.colorCoords.data(), ccSize);
    backend->updateBuffer(buf.normalCoordBuf, buf.normalCoords.data(), ncSize);

    DrawCall dc;
    dc.pipeline = pipeline_;
    dc.uniformBinding = 0;
    dc.uniformOffset = uniformOffset;
    dc.uniformSize = static_cast<uint16_t>(sizeof(glm::mat4));
    dc.textureBinding = 0;
    dc.texture = static_cast<TextureHandle>(buf.textureId);
    dc.vertexBuffers.push_back({0, buf.positionBuf});
    dc.vertexBuffers.push_back({1, buf.normalBuf});
    dc.vertexBuffers.push_back({2, buf.colorBuf});
    dc.vertexBuffers.push_back({3, buf.colorCoordBuf});
    dc.vertexBuffers.push_back({4, buf.normalCoordBuf});
    dc.vertexCount = buf.pointCount;
    pass.drawCalls.push_back(std::move(dc));
  }
}

}  // namespace fgfx
