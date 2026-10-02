#include "PBRSpriteBatch.hpp"
#include "Engine.hpp"
#include "backend/RenderBackend.hpp"
#include "backend/FrameGraph.hpp"

#include <cstring>

namespace fgfx {

namespace {
#ifdef __USE_OPENGL
constexpr const char* kVertexGLSL = R"(#version 330
in vec3 aPosition;
in vec2 aNormal;
in vec4 aColor;
in vec2 aColorCoord;
in vec2 aNormalCoord;
in vec2 aMaterialCoord;
layout(std140) uniform Camera {
    mat4 matrix;
    vec4 lightDir;
    vec4 eyeDir;
};
out vec2 vNormal;
out vec4 vColor;
out vec2 vColorCoord;
out vec2 vNormalCoord;
out vec2 vMaterialCoord;
void main() {
  gl_Position = matrix * vec4(aPosition, 1.0);
  vNormal = aNormal;
  vColor = aColor;
  vColorCoord = aColorCoord;
  vNormalCoord = aNormalCoord;
  vMaterialCoord = aMaterialCoord;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 330
in vec2 vNormal;
in vec4 vColor;
in vec2 vColorCoord;
in vec2 vNormalCoord;
in vec2 vMaterialCoord;
uniform sampler2D uSampler;
layout(std140) uniform Camera {
    mat4 matrix;
    vec4 lightDir;
    vec4 eyeDir;
};
out vec4 fragColor;

vec3 environmentLight(vec3 dir, float roughness) {
  vec3 sunDir = normalize(lightDir.xyz);
  vec3 sunColor = vec3(1.0, 0.95, 0.88) * 6.5;
  vec3 sunHaloColor = vec3(1.0, 0.88, 0.65) * 0.18;
  float sunSize = 0.08;
  float sunBorder = 0.08;
  vec3 skyColor = vec3(0.82, 0.86, 0.92) * (dir.y * 0.35 + 0.65);
  vec3 groundColor = vec3(0.70, 0.62, 0.50) * (-dir.y * 0.25 + 0.75);
  float groundHeight = 0.0;
  float sunDistance = length(dir - sunDir);
  vec3 sunBeam = sunColor * (clamp((sunSize - sunDistance) / (sunBorder + roughness), -1.0, 1.0) * 0.5 + 0.5);
  vec3 sunHalo = sunHaloColor * (1.0 / (sunDistance + roughness));
  skyColor += sunBeam + sunHalo;
  return mix(groundColor, skyColor, clamp((dir.y + groundHeight) / roughness, -1.0, 1.0) * 0.5 + 0.5);
}

void main() {
  vec4 textureColor = texture(uSampler, vColorCoord);
  vec4 color = textureColor * vColor;
  vec4 textureNormalPixel = texture(uSampler, vNormalCoord);
  vec4 textureMaterialPixel = texture(uSampler, vMaterialCoord);
  vec3 normal = textureNormalPixel.xyz * 2.0 - vec3(1.0, 1.0, 1.0);
  vec3 normalYAxis = vec3(vNormal.x, vNormal.y, 0);
  vec3 normalXAxis = vec3(normalYAxis.y, -normalYAxis.x, 0);
  normal = -normalXAxis * normal.x - normalYAxis * normal.y - vec3(0, 0, normal.z);
  normal.z = normal.z * -1.0;
  normal = normalize(normal);
  vec3 eye = normalize(eyeDir.xyz);
  vec3 light = normalize(lightDir.xyz);
  vec3 halfV = normalize(light + eye);
  vec3 reflection = normalize(reflect(-eye, normal));
  float roughness = max(textureMaterialPixel.r, 0.04);
  float metallic = textureMaterialPixel.g;
  float occulsion = textureMaterialPixel.b;
  float ndl = max(dot(normal, light), 0.0);
  float spec = pow(max(dot(normal, halfV), 0.0), mix(12.0, 180.0, 1.0 - roughness)) * ndl;
  vec3 diffuse = color.rgb * (1.0 - metallic);
  vec3 specCol = mix(vec3(0.04), color.rgb, metallic);
  vec3 lit = diffuse * (0.38 + 0.62 * ndl) + specCol * spec * (1.0 + 3.0 * metallic);
  vec3 env = diffuse * environmentLight(normal, 2.0) * 0.75
           + specCol * environmentLight(reflection, roughness) * 1.2;
  fragColor = vec4(occulsion * (lit + env), color.a * textureMaterialPixel.a);
}
)";
#else
constexpr const char* kVertexGLSL = R"(#version 300 es
in vec3 aPosition;
in vec2 aNormal;
in vec4 aColor;
in vec2 aColorCoord;
in vec2 aNormalCoord;
in vec2 aMaterialCoord;
layout(std140) uniform Camera {
    mat4 matrix;
    vec4 lightDir;
    vec4 eyeDir;
};
out vec2 vNormal;
out vec4 vColor;
out highp vec2 vColorCoord;
out highp vec2 vNormalCoord;
out highp vec2 vMaterialCoord;
void main() {
  gl_Position = matrix * vec4(aPosition, 1.0);
  vNormal = aNormal;
  vColor = aColor;
  vColorCoord = aColorCoord;
  vNormalCoord = aNormalCoord;
  vMaterialCoord = aMaterialCoord;
}
)";

constexpr const char* kFragmentGLSL = R"(#version 300 es
precision mediump float;
in vec2 vNormal;
in vec4 vColor;
in highp vec2 vColorCoord;
in highp vec2 vNormalCoord;
in highp vec2 vMaterialCoord;
uniform sampler2D uSampler;
layout(std140) uniform Camera {
    mat4 matrix;
    vec4 lightDir;
    vec4 eyeDir;
};
out vec4 fragColor;

vec3 environmentLight(vec3 dir, float roughness) {
  vec3 sunDir = normalize(lightDir.xyz);
  vec3 sunColor = vec3(1.0, 0.95, 0.88) * 6.5;
  vec3 sunHaloColor = vec3(1.0, 0.88, 0.65) * 0.18;
  float sunSize = 0.08;
  float sunBorder = 0.08;
  vec3 skyColor = vec3(0.82, 0.86, 0.92) * (dir.y * 0.35 + 0.65);
  vec3 groundColor = vec3(0.70, 0.62, 0.50) * (-dir.y * 0.25 + 0.75);
  float groundHeight = 0.0;
  float sunDistance = length(dir - sunDir);
  vec3 sunBeam = sunColor * (clamp((sunSize - sunDistance) / (sunBorder + roughness), -1.0, 1.0) * 0.5 + 0.5);
  vec3 sunHalo = sunHaloColor * (1.0 / (sunDistance + roughness));
  skyColor += sunBeam + sunHalo;
  return mix(groundColor, skyColor, clamp((dir.y + groundHeight) / roughness, -1.0, 1.0) * 0.5 + 0.5);
}

void main() {
  vec4 textureColor = texture(uSampler, vColorCoord);
  vec4 color = textureColor * vColor;
  vec4 textureNormalPixel = texture(uSampler, vNormalCoord);
  vec4 textureMaterialPixel = texture(uSampler, vMaterialCoord);
  vec3 normal = textureNormalPixel.xyz * 2.0 - vec3(1.0, 1.0, 1.0);
  vec3 normalYAxis = vec3(vNormal.x, vNormal.y, 0);
  vec3 normalXAxis = vec3(normalYAxis.y, -normalYAxis.x, 0);
  normal = -normalXAxis * normal.x - normalYAxis * normal.y - vec3(0, 0, normal.z);
  normal.z = normal.z * -1.0;
  normal = normalize(normal);
  vec3 eye = normalize(eyeDir.xyz);
  vec3 light = normalize(lightDir.xyz);
  vec3 halfV = normalize(light + eye);
  vec3 reflection = normalize(reflect(-eye, normal));
  float roughness = max(textureMaterialPixel.r, 0.04);
  float metallic = textureMaterialPixel.g;
  float occulsion = textureMaterialPixel.b;
  float ndl = max(dot(normal, light), 0.0);
  float spec = pow(max(dot(normal, halfV), 0.0), mix(12.0, 180.0, 1.0 - roughness)) * ndl;
  vec3 diffuse = color.rgb * (1.0 - metallic);
  vec3 specCol = mix(vec3(0.04), color.rgb, metallic);
  vec3 lit = diffuse * (0.38 + 0.62 * ndl) + specCol * spec * (1.0 + 3.0 * metallic);
  vec3 env = diffuse * environmentLight(normal, 2.0) * 0.75
           + specCol * environmentLight(reflection, roughness) * 1.2;
  fragColor = vec4(occulsion * (lit + env), color.a * textureMaterialPixel.a);
}
)";
#endif

constexpr const char* kVertexWGSL = R"(
struct Camera {
  matrix : mat4x4<f32>,
  lightDir : vec4<f32>,
  eyeDir : vec4<f32>,
}
@group(0) @binding(0) var<uniform> camera : Camera;
@group(0) @binding(1) var uTexture : texture_2d<f32>;
@group(0) @binding(2) var uSampler : sampler;

struct VertexInput {
  @location(0) position : vec3<f32>,
  @location(1) normal : vec2<f32>,
  @location(2) color : vec4<f32>,
  @location(3) colorCoord : vec2<f32>,
  @location(4) normalCoord : vec2<f32>,
  @location(5) materialCoord : vec2<f32>,
}

struct VertexOutput {
  @builtin(position) position : vec4<f32>,
  @location(0) vNormal : vec2<f32>,
  @location(1) vColor : vec4<f32>,
  @location(2) vColorCoord : vec2<f32>,
  @location(3) vNormalCoord : vec2<f32>,
  @location(4) vMaterialCoord : vec2<f32>,
}

@vertex
fn vs_main(input : VertexInput) -> VertexOutput {
  var output : VertexOutput;
  output.position = camera.matrix * vec4<f32>(input.position, 1.0);
  output.vNormal = input.normal;
  output.vColor = input.color;
  output.vColorCoord = input.colorCoord;
  output.vNormalCoord = input.normalCoord;
  output.vMaterialCoord = input.materialCoord;
  return output;
}

fn environmentLight(dir : vec3<f32>, roughness : f32) -> vec3<f32> {
  let sunDir = normalize(camera.lightDir.xyz);
  let sunColor = vec3<f32>(1.0, 0.95, 0.88) * 6.5;
  let sunHaloColor = vec3<f32>(1.0, 0.88, 0.65) * 0.18;
  let sunSize = 0.08;
  let sunBorder = 0.08;
  var skyColor = vec3<f32>(0.82, 0.86, 0.92) * (dir.y * 0.35 + 0.65);
  let groundColor = vec3<f32>(0.70, 0.62, 0.50) * (-dir.y * 0.25 + 0.75);
  let groundHeight = 0.0;
  let sunDistance = length(dir - sunDir);
  let sunBeam = sunColor * (clamp((sunSize - sunDistance) / (sunBorder + roughness), -1.0, 1.0) * 0.5 + 0.5);
  let sunHalo = sunHaloColor * (1.0 / (sunDistance + roughness));
  skyColor = skyColor + sunBeam + sunHalo;
  return mix(groundColor, skyColor, clamp((dir.y + groundHeight) / roughness, -1.0, 1.0) * 0.5 + 0.5);
}

@fragment
fn fs_main(input : VertexOutput) -> @location(0) vec4<f32> {
  let textureColor = textureSample(uTexture, uSampler, input.vColorCoord);
  let color = textureColor * input.vColor;
  let textureNormalPixel = textureSample(uTexture, uSampler, input.vNormalCoord);
  let textureMaterialPixel = textureSample(uTexture, uSampler, input.vMaterialCoord);
  var normal = textureNormalPixel.xyz * 2.0 - vec3<f32>(1.0, 1.0, 1.0);
  let normalYAxis = vec3<f32>(input.vNormal.x, input.vNormal.y, 0.0);
  let normalXAxis = vec3<f32>(normalYAxis.y, -normalYAxis.x, 0.0);
  normal = -normalXAxis * normal.x - normalYAxis * normal.y - vec3<f32>(0.0, 0.0, normal.z);
  normal = normalize(vec3<f32>(normal.x, normal.y, normal.z * -1.0));
  let eye = normalize(camera.eyeDir.xyz);
  let light = normalize(camera.lightDir.xyz);
  let halfV = normalize(light + eye);
  let reflection = normalize(reflect(-eye, normal));
  let roughness = max(textureMaterialPixel.r, 0.04);
  let metallic = textureMaterialPixel.g;
  let occulsion = textureMaterialPixel.b;
  let ndl = max(dot(normal, light), 0.0);
  let spec = pow(max(dot(normal, halfV), 0.0), mix(12.0, 180.0, 1.0 - roughness)) * ndl;
  let diffuse = color.rgb * (1.0 - metallic);
  let specCol = mix(vec3<f32>(0.04), color.rgb, metallic);
  let lit = diffuse * (0.38 + 0.62 * ndl) + specCol * spec * (1.0 + 3.0 * metallic);
  let env = diffuse * environmentLight(normal, 2.0) * 0.75
          + specCol * environmentLight(reflection, roughness) * 1.2;
  return vec4<f32>(occulsion * (lit + env), color.a * textureMaterialPixel.a);
}
)";

struct PbrUbo {
  glm::mat4 matrix;
  glm::vec4 lightDir; // xyz, w unused
  glm::vec4 eyeDir;
};
static_assert(sizeof(PbrUbo) == 96, "PBR UBO must be 96 bytes (std140)");
}  // namespace

PBRSpriteBatch::PBRSpriteBatch(Engine* engine) : engine_(engine) {}

void PBRSpriteBatch::setLightDir(glm::vec3 dir) {
  if (glm::length(dir) < 1e-5f) {
    lightDir_ = glm::vec3(1.0f, 5.0f, 4.0f);
    return;
  }
  lightDir_ = glm::normalize(dir);
}

void PBRSpriteBatch::setEyeDir(glm::vec3 dir) {
  if (glm::length(dir) < 1e-5f) {
    eyeDir_ = glm::vec3(0.0f, 0.0f, 1.0f);
    return;
  }
  eyeDir_ = glm::normalize(dir);
}

PBRSpriteBatch::~PBRSpriteBatch() {
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
      backend->destroyBuffer(buf.materialCoordBuf);
    }
  }
}

PBRSpriteBatch::Buffer& PBRSpriteBatch::getBuffer(int textureId) {
  for (auto& buf : buffers_) {
    if (buf.textureId == textureId) return buf;
  }
  buffers_.emplace_back();
  buffers_.back().textureId = textureId;
  return buffers_.back();
}

void PBRSpriteBatch::ensurePipeline() {
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
  desc.uniformSize = 96;
  desc.hasTexture = true;
  VertexBinding posBinding;
  posBinding.binding = 0;
  posBinding.stride = sizeof(glm::vec3);
  posBinding.attributes.push_back({0, 0, PixelFormat::RGB32Float, "aPosition"});
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
  VertexBinding materialCoordBinding;
  materialCoordBinding.binding = 5;
  materialCoordBinding.stride = sizeof(glm::vec2);
  materialCoordBinding.attributes.push_back({5, 0, PixelFormat::RG32Float, "aMaterialCoord"});
  desc.vertexBindings.push_back(materialCoordBinding);
  pipeline_ = engine_->getBackend()->createPipeline(desc);
}

void PBRSpriteBatch::bufferPoint(Buffer& buf, glm::vec3 pos, glm::vec2 normal,
                                 glm::vec4 color, glm::vec2 colorCoord,
                                 glm::vec2 normalCoord, glm::vec2 materialCoord) {
  buf.positions.push_back(pos);
  buf.normals.push_back(normal);
  buf.colors.push_back(color);
  buf.colorCoords.push_back(colorCoord);
  buf.normalCoords.push_back(normalCoord);
  buf.materialCoords.push_back(materialCoord);
  buf.pointCount++;
}

void PBRSpriteBatch::bufferSprite(const std::shared_ptr<Sprite>& sprite,
                                  const glm::mat4& mat, glm::vec4 color) {
  if (sprite->unloaded) engine_->reloadSprite(sprite);
  if (sprite->textureId == -1) return;
  Buffer& buf = getBuffer(sprite->textureId);

  auto r = sprite->ratio * 3;
  glm::vec3 up = glm::mat3(mat) * glm::vec3(0, 1, 0);
  glm::vec2 normal = glm::vec2(up);
  if (glm::length(normal) < 1e-5f) {
    normal = glm::vec2(0.0f, 1.0f);
  } else {
    normal = glm::normalize(normal);
  }
  glm::vec3 p1 = glm::vec3(mat * glm::vec4(-1 * r, -1, 0, 1));
  glm::vec3 p2 = glm::vec3(mat * glm::vec4(1 * r, -1, 0, 1));
  glm::vec3 p3 = glm::vec3(mat * glm::vec4(1 * r, 1, 0, 1));
  glm::vec3 p4 = glm::vec3(mat * glm::vec4(-1 * r, 1, 0, 1));

  glm::vec2 colorCoords[4] = {
      sprite->coords[0],
      sprite->coords[1],
      sprite->coords[1] + (sprite->coords[2] - sprite->coords[1]) * (1.0f / 3.0f),
      sprite->coords[0] + (sprite->coords[3] - sprite->coords[0]) * (1.0f / 3.0f)
  };
  glm::vec2 normalCoords[4] = {
      sprite->coords[0] + (sprite->coords[3] - sprite->coords[0]) * (1.0f / 3.0f),
      sprite->coords[1] + (sprite->coords[2] - sprite->coords[1]) * (1.0f / 3.0f),
      sprite->coords[1] + (sprite->coords[2] - sprite->coords[1]) * (2.0f / 3.0f),
      sprite->coords[0] + (sprite->coords[3] - sprite->coords[0]) * (2.0f / 3.0f)
  };
  glm::vec2 materialCoords[4] = {
      sprite->coords[0] + (sprite->coords[3] - sprite->coords[0]) * (2.0f / 3.0f),
      sprite->coords[1] + (sprite->coords[2] - sprite->coords[1]) * (2.0f / 3.0f),
      sprite->coords[2],
      sprite->coords[3]
  };

  bufferPoint(buf, p1, normal, color, colorCoords[0], normalCoords[0], materialCoords[0]);
  bufferPoint(buf, p2, normal, color, colorCoords[1], normalCoords[1], materialCoords[1]);
  bufferPoint(buf, p3, normal, color, colorCoords[2], normalCoords[2], materialCoords[2]);
  bufferPoint(buf, p4, normal, color, colorCoords[3], normalCoords[3], materialCoords[3]);
  bufferPoint(buf, p1, normal, color, colorCoords[0], normalCoords[0], materialCoords[0]);
  bufferPoint(buf, p3, normal, color, colorCoords[2], normalCoords[2], materialCoords[2]);
}

void PBRSpriteBatch::rawBufferSprite(const std::shared_ptr<Sprite>& sprite,
                                     const glm::mat4& mat, glm::vec4 color) {
  if (sprite->unloaded) engine_->reloadSprite(sprite);
  if (sprite->textureId == -1) return;
  Buffer& buf = getBuffer(sprite->textureId);

  glm::vec2 normal = glm::normalize(glm::vec2(mat * glm::vec4(1, 0, 0, 0)));
  glm::vec3 p1 = glm::vec3(mat * glm::vec4(-1, -1, 0, 1));
  glm::vec3 p2 = glm::vec3(mat * glm::vec4(1, -1, 0, 1));
  glm::vec3 p3 = glm::vec3(mat * glm::vec4(1, 1, 0, 1));
  glm::vec3 p4 = glm::vec3(mat * glm::vec4(-1, 1, 0, 1));

  glm::vec2 colorCoords[4] = {
      sprite->coords[0],
      sprite->coords[1],
      sprite->coords[1] + (sprite->coords[2] - sprite->coords[1]) * (1.0f / 3.0f),
      sprite->coords[0] + (sprite->coords[3] - sprite->coords[0]) * (1.0f / 3.0f)
  };
  glm::vec2 normalCoords[4] = {
      sprite->coords[1] + (sprite->coords[3] - sprite->coords[0]) * (1.0f / 3.0f),
      sprite->coords[0] + (sprite->coords[2] - sprite->coords[1]) * (1.0f / 3.0f),
      sprite->coords[1] + (sprite->coords[2] - sprite->coords[1]) * (2.0f / 3.0f),
      sprite->coords[0] + (sprite->coords[3] - sprite->coords[0]) * (2.0f / 3.0f)
  };
  glm::vec2 materialCoords[4] = {
      sprite->coords[1] + (sprite->coords[3] - sprite->coords[0]) * (2.0f / 3.0f),
      sprite->coords[0] + (sprite->coords[2] - sprite->coords[1]) * (2.0f / 3.0f),
      sprite->coords[2],
      sprite->coords[3]
  };

  bufferPoint(buf, p1, normal, color, colorCoords[0], normalCoords[0], materialCoords[0]);
  bufferPoint(buf, p2, normal, color, colorCoords[1], normalCoords[1], materialCoords[1]);
  bufferPoint(buf, p3, normal, color, colorCoords[2], normalCoords[2], materialCoords[2]);
  bufferPoint(buf, p4, normal, color, colorCoords[3], normalCoords[3], materialCoords[3]);
  bufferPoint(buf, p1, normal, color, colorCoords[0], normalCoords[0], materialCoords[0]);
  bufferPoint(buf, p3, normal, color, colorCoords[2], normalCoords[2], materialCoords[2]);
}

void PBRSpriteBatch::reset() {
  for (auto& buf : buffers_) {
    buf.positions.clear();
    buf.normals.clear();
    buf.colors.clear();
    buf.colorCoords.clear();
    buf.normalCoords.clear();
    buf.materialCoords.clear();
    buf.pointCount = 0;
  }
}

void PBRSpriteBatch::record(RenderPass& pass, FrameContext& ctx) {
  ensurePipeline();
  RenderBackend* backend = engine_->getBackend();
  PbrUbo ubo;
  ubo.matrix = ctx.mvp.multipliedMatrix;
  ubo.lightDir = glm::vec4(lightDir_, 0.0f);
  ubo.eyeDir = glm::vec4(eyeDir_, 0.0f);
  uint32_t uniformOffset =
      ctx.uniforms->alloc(&ubo, static_cast<int>(sizeof(ubo)));
  for (auto& buf : buffers_) {
    if (buf.pointCount == 0) continue;

    int posSize = static_cast<int>(buf.pointCount * sizeof(glm::vec3));
    int nrmSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));
    int colSize = static_cast<int>(buf.pointCount * sizeof(glm::vec4));
    int ccSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));
    int ncSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));
    int mcSize = static_cast<int>(buf.pointCount * sizeof(glm::vec2));

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
    if (!buf.gpuBuffersCreated || mcSize > buf.materialCoordBufSize) {
      if (buf.gpuBuffersCreated) backend->destroyBuffer(buf.materialCoordBuf);
      buf.materialCoordBuf = backend->createBuffer({mcSize, BufferUsage::Vertex});
      buf.materialCoordBufSize = mcSize;
    }
    buf.gpuBuffersCreated = true;

    backend->updateBuffer(buf.positionBuf, buf.positions.data(), posSize);
    backend->updateBuffer(buf.normalBuf, buf.normals.data(), nrmSize);
    backend->updateBuffer(buf.colorBuf, buf.colors.data(), colSize);
    backend->updateBuffer(buf.colorCoordBuf, buf.colorCoords.data(), ccSize);
    backend->updateBuffer(buf.normalCoordBuf, buf.normalCoords.data(), ncSize);
    backend->updateBuffer(buf.materialCoordBuf, buf.materialCoords.data(), mcSize);

    DrawCall dc;
    dc.pipeline = pipeline_;
    dc.uniformBinding = 0;
    dc.uniformOffset = uniformOffset;
    dc.uniformSize = static_cast<uint16_t>(sizeof(PbrUbo));
    dc.textureBinding = 0;
    dc.texture = static_cast<TextureHandle>(buf.textureId);
    dc.vertexBuffers.push_back({0, buf.positionBuf});
    dc.vertexBuffers.push_back({1, buf.normalBuf});
    dc.vertexBuffers.push_back({2, buf.colorBuf});
    dc.vertexBuffers.push_back({3, buf.colorCoordBuf});
    dc.vertexBuffers.push_back({4, buf.normalCoordBuf});
    dc.vertexBuffers.push_back({5, buf.materialCoordBuf});
    dc.vertexCount = buf.pointCount;
    pass.drawCalls.push_back(std::move(dc));
  }
}

}  // namespace fgfx
