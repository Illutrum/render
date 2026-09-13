// WebGPUBackend implementation using the webgpu.hpp C++ API.
// One file for native (Dawn) and wasm (Emscripten) — only init differs.

// Define WEBGPU_CPP_IMPLEMENTATION in exactly one translation unit to
// generate the wgpu:: C++ wrapper method implementations.
#define WEBGPU_CPP_IMPLEMENTATION

#include "WebGPUBackend.hpp"
#include "config.hpp"
#include "../Engine.hpp"

#include <cstring>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace fgfx {

namespace {
// WGSL shader for the sprite batch.
const char* kSpriteWGSL = R"WGSL(
struct Camera {
  matrix : mat4x4<f32>,
}
@group(0) @binding(0) var<uniform> camera : Camera;

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

@group(0) @binding(1) var uTexture : texture_2d<f32>;
@group(0) @binding(2) var uSampler : sampler;

@fragment
fn fs_main(input : VertexOutput) -> @location(0) vec4<f32> {
  let texColor = textureSample(uTexture, uSampler, input.vCoord);
  return input.vColor * texColor;
}
)WGSL";
}  // namespace

WGPUVertexFormat WebGPUBackend::vertexFormatOf(PixelFormat f) {
  switch (f) {
    case PixelFormat::RGBA32Float: return WGPUVertexFormat_Float32x4;
    case PixelFormat::RGB32Float:  return WGPUVertexFormat_Float32x3;
    case PixelFormat::RG32Float:   return WGPUVertexFormat_Float32x2;
    case PixelFormat::R32Float:    return WGPUVertexFormat_Float32;
    default: return WGPUVertexFormat_Float32x4;
  }
}

WGPUBufferUsage WebGPUBackend::bufferUsageOf(BufferUsage u) {
  switch (u) {
    case BufferUsage::Vertex:  return WGPUBufferUsage_Vertex;
    case BufferUsage::Index:   return WGPUBufferUsage_Index;
    case BufferUsage::Uniform: return WGPUBufferUsage_Uniform;
    case BufferUsage::Storage: return WGPUBufferUsage_Storage;
  }
  return WGPUBufferUsage_Vertex;
}

WGPUTextureFormat WebGPUBackend::textureFormatOf(PixelFormat f) {
  switch (f) {
    case PixelFormat::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
    case PixelFormat::BGRA8Unorm: return WGPUTextureFormat_BGRA8Unorm;
    default: return WGPUTextureFormat_RGBA8Unorm;
  }
}

WebGPUBackend::WebGPUBackend() {
  // Handle 0 is kNull* — reserve the first slot so real resources start at 1.
  pipelines_.emplace_back();
  buffers_.emplace_back();
  textures_.emplace_back();
}

WebGPUBackend::~WebGPUBackend() {
  // RAII wrappers release all resources automatically.
  pipelines_.clear();
  buffers_.clear();
  textures_.clear();
  scratchUniformBuffer_ = {};
  defaultSampler_ = {};
  commandEncoder_ = {};
  passEncoder_ = {};
  currentBackbufferView_ = {};
  currentBackbufferTexture_ = {};
  surface_ = {};
}

void WebGPUBackend::initWithDevice(wgpu::Instance instance,
                                     wgpu::Device device,
                                     wgpu::Surface surface,
                                     int width, int height,
                                     wgpu::Adapter adapter) {
  instance_ = instance;
  device_ = device;
  adapter_ = adapter;
  queue_ = device.getQueue();
  surface_ = wgpu::raii::Surface(std::move(surface));
  surfaceWidth_ = width;
  surfaceHeight_ = height;

  // Prefer 8-bit UNORM (not *Srgb) so shader/clear 0–1 values match GL's
  // GL_RGBA8 path. Dawn lists BGRA8UnormSrgb first; writing linear clears
  // into sRGB lightens the whole frame vs OpenGL.
  auto isLinearUnorm8 = [](WGPUTextureFormat f) {
    return f == WGPUTextureFormat_BGRA8Unorm ||
           f == WGPUTextureFormat_RGBA8Unorm;
  };
  surfaceFormat_ = WGPUTextureFormat_BGRA8Unorm;
  wgpu::SurfaceCapabilities caps = wgpu::Default;
  if (adapter_ &&
      surface_->getCapabilities(adapter_, &caps) == wgpu::Status::Success &&
      caps.formatCount > 0) {
    surfaceFormat_ = caps.formats[0];
    for (size_t i = 0; i < caps.formatCount; ++i) {
      if (isLinearUnorm8(caps.formats[i])) {
        surfaceFormat_ = caps.formats[i];
        break;
      }
    }
    fgfx_log("WebGPU: surface format=%d (%zu formats)\n",
             static_cast<int>(surfaceFormat_),
             static_cast<size_t>(caps.formatCount));
  } else {
#ifdef EMSCRIPTEN
    int isRgba = EM_ASM_INT({
      try {
        return (navigator.gpu.getPreferredCanvasFormat() === 'rgba8unorm')
                   ? 1
                   : 0;
      } catch (e) {
        return 0;
      }
    });
    surfaceFormat_ = isRgba ? WGPUTextureFormat_RGBA8Unorm
                            : WGPUTextureFormat_BGRA8Unorm;
    fgfx_log("WebGPU: using preferred canvas format=%d\n",
             static_cast<int>(surfaceFormat_));
#else
    fgfx_log("WebGPU: getCapabilities failed, using BGRA8Unorm\n");
#endif
  }
  caps.freeMembers();

  reconfigureSurface(width, height);

  // Default sampler: linear filtering, no mipmapping (we upload mip 0 only).
  // Use INIT macro — = {} zero-inits maxAnisotropy to 0, which Dawn rejects.
  WGPUSamplerDescriptor samplerDesc = WGPU_SAMPLER_DESCRIPTOR_INIT;
  samplerDesc.magFilter = WGPUFilterMode_Linear;
  samplerDesc.minFilter = WGPUFilterMode_Linear;
  samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
  samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
  samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
  samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
  defaultSampler_ = device.createSampler(samplerDesc);

  // Uniform ring buffer (frame-allocated UBO).
  WGPUBufferDescriptor uboDesc = {};
  uboDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  uboDesc.size = uniformRingCapacity_;
  uboDesc.mappedAtCreation = false;
  scratchUniformBuffer_ = device.createBuffer(uboDesc);
}

void WebGPUBackend::reconfigureSurface(int width, int height) {
  surfaceWidth_ = width;
  surfaceHeight_ = height;
  WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
  config.device = device_;
  config.format = surfaceFormat_;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.width = static_cast<uint32_t>(width);
  config.height = static_cast<uint32_t>(height);
  config.presentMode = WGPUPresentMode_Fifo;
  config.alphaMode = WGPUCompositeAlphaMode_Opaque;
  if (surfaceConfigured_) surface_->unconfigure();
  surface_->configure(config);
  surfaceConfigured_ = true;
}

PipelineHandle WebGPUBackend::createPipeline(const PipelineDesc& desc) {
  if (!supportsAll(desc.requiredFeatures)) {
    fgfx_log("createPipeline rejected: missing required features");
    return kNullPipeline;
  }
  PipelineInfo info;
  info.uniformSize = desc.uniformSize > 0 ? desc.uniformSize : 64;
  info.hasTexture = desc.hasTexture;

  std::string wgsl;
  if (!desc.vertexWGSL.empty()) {
    wgsl = desc.vertexWGSL;
    if (!desc.fragmentWGSL.empty()) {
      wgsl += "\n";
      wgsl += desc.fragmentWGSL;
    }
  } else {
    wgsl = kSpriteWGSL;
  }

  // Shader module from WGSL.
  // Use wgpu:: Default init — it calls setDefault() which sets chain.sType.
  wgpu::ShaderSourceWGSL wgslDesc = wgpu::Default;
  wgslDesc.code = WGPUStringView{wgsl.c_str(), wgsl.size()};
  wgpu::ShaderModuleDescriptor shaderDesc = wgpu::Default;
  shaderDesc.nextInChain = &wgslDesc.chain;
  info.shaderModule = device_.createShaderModule(shaderDesc);

  // Bind group layout: 0=uniform, optionally 1=texture + 2=sampler.
  WGPUBindGroupLayoutEntry bgEntries[3] = {
      WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
      WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
      WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT,
  };
  bgEntries[0].binding = 0;
  bgEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
  bgEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
  bgEntries[0].buffer.minBindingSize = static_cast<uint64_t>(info.uniformSize);
  uint32_t entryCount = 1;
  if (info.hasTexture) {
    bgEntries[1].binding = 1;
    bgEntries[1].visibility = WGPUShaderStage_Fragment;
    bgEntries[1].texture.sampleType = WGPUTextureSampleType_Float;
    bgEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    bgEntries[2].binding = 2;
    bgEntries[2].visibility = WGPUShaderStage_Fragment;
    bgEntries[2].sampler.type = WGPUSamplerBindingType_Filtering;
    entryCount = 3;
  }

  WGPUBindGroupLayoutDescriptor bglDesc =
      WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
  bglDesc.entryCount = entryCount;
  bglDesc.entries = bgEntries;
  info.bindGroupLayout = device_.createBindGroupLayout(bglDesc);

  WGPUBindGroupLayout bgl = *info.bindGroupLayout;
  WGPUPipelineLayoutDescriptor plDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  plDesc.bindGroupLayoutCount = 1;
  plDesc.bindGroupLayouts = &bgl;
  info.pipelineLayout = device_.createPipelineLayout(plDesc);

  // Vertex buffer layouts.
  std::vector<WGPUVertexBufferLayout> vertexLayouts(
      desc.vertexBindings.size());
  std::vector<std::vector<WGPUVertexAttribute>> vertexAttrs(
      desc.vertexBindings.size());
  for (size_t i = 0; i < desc.vertexBindings.size(); i++) {
    const auto& vb = desc.vertexBindings[i];
    vertexAttrs[i].resize(vb.attributes.size());
    for (size_t j = 0; j < vb.attributes.size(); j++) {
      vertexAttrs[i][j].shaderLocation =
          static_cast<uint32_t>(vb.attributes[j].location);
      vertexAttrs[i][j].offset =
          static_cast<uint64_t>(vb.attributes[j].offset);
      vertexAttrs[i][j].format = vertexFormatOf(vb.attributes[j].format);
    }
    vertexLayouts[i].arrayStride = static_cast<uint64_t>(vb.stride);
    vertexLayouts[i].stepMode = WGPUVertexStepMode_Vertex;
    vertexLayouts[i].attributeCount =
        static_cast<uint32_t>(vertexAttrs[i].size());
    vertexLayouts[i].attributes = vertexAttrs[i].data();
  }

  // Blend state.
  WGPUBlendComponent blendComponent = WGPU_BLEND_COMPONENT_INIT;
  blendComponent.operation = WGPUBlendOperation_Add;
  blendComponent.srcFactor = WGPUBlendFactor_SrcAlpha;
  blendComponent.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  WGPUBlendState blendState = WGPU_BLEND_STATE_INIT;
  blendState.color = blendComponent;
  blendState.alpha = blendComponent;

  // Zero-init leaves writeMask=None, so the shader never writes color.
  WGPUColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_INIT;
  colorTarget.format = (desc.targetFormat == PixelFormat::BGRA8Unorm)
      ? surfaceFormat_
      : textureFormatOf(desc.targetFormat);
  colorTarget.blend = &blendState;

  WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
  WGPUShaderModule sm = *info.shaderModule;
  fragmentState.module = sm;
  fragmentState.targetCount = 1;
  fragmentState.targets = &colorTarget;
  fragmentState.entryPoint = wgpu::StringView("fs_main");

  WGPURenderPipelineDescriptor pd = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
  WGPUPipelineLayout pl = *info.pipelineLayout;
  pd.layout = pl;
  pd.vertex.module = sm;
  pd.vertex.entryPoint = wgpu::StringView("vs_main");
  pd.vertex.bufferCount = static_cast<uint32_t>(vertexLayouts.size());
  pd.vertex.buffers = vertexLayouts.data();
  pd.fragment = &fragmentState;
  switch (desc.topology) {
    case PrimitiveTopology::LineList:
      pd.primitive.topology = WGPUPrimitiveTopology_LineList;
      break;
    case PrimitiveTopology::LineStrip:
      pd.primitive.topology = WGPUPrimitiveTopology_LineStrip;
      break;
    case PrimitiveTopology::PointList:
      pd.primitive.topology = WGPUPrimitiveTopology_PointList;
      break;
    case PrimitiveTopology::TriangleList:
    default:
      pd.primitive.topology = WGPUPrimitiveTopology_TriangleList;
      break;
  }
  pd.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  pd.primitive.frontFace = WGPUFrontFace_CCW;
  pd.primitive.cullMode = WGPUCullMode_None;
  pd.depthStencil = nullptr;
  pd.multisample.count = 1;
  pd.multisample.mask = 0xFFFFFFFF;
  pd.multisample.alphaToCoverageEnabled = false;

  info.pipeline = device_.createRenderPipeline(pd);
  pipelines_.push_back(std::move(info));
  return static_cast<PipelineHandle>(pipelines_.size() - 1);
}

void WebGPUBackend::destroyPipeline(PipelineHandle pipeline) {
  auto idx = static_cast<size_t>(pipeline);
  if (idx >= pipelines_.size()) return;
  pipelines_[idx] = {};
}

BufferHandle WebGPUBackend::createBuffer(const BufferDesc& desc) {
  WGPUBufferDescriptor bd = WGPU_BUFFER_DESCRIPTOR_INIT;
  bd.usage = bufferUsageOf(desc.usage) | WGPUBufferUsage_CopyDst;
  bd.size = static_cast<uint64_t>(desc.size);
  bd.mappedAtCreation = false;
  wgpu::Buffer buf = device_.createBuffer(bd);
  buffers_.push_back({std::move(buf), desc.size, desc.usage});
  return static_cast<BufferHandle>(buffers_.size() - 1);
}

void WebGPUBackend::updateBuffer(BufferHandle buffer, const void* data,
                                   int size) {
  auto idx = static_cast<size_t>(buffer);
  if (idx >= buffers_.size()) return;
  queue_.writeBuffer(*buffers_[idx].buffer, 0, data,
                      static_cast<size_t>(size));
}

void WebGPUBackend::destroyBuffer(BufferHandle buffer) {
  auto idx = static_cast<size_t>(buffer);
  if (idx >= buffers_.size()) return;
  buffers_[idx] = {};
}

TextureHandle WebGPUBackend::createTexture(const TextureDesc& desc) {
  WGPUTextureDescriptor td = WGPU_TEXTURE_DESCRIPTOR_INIT;
  td.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst |
            WGPUTextureUsage_RenderAttachment;
  td.dimension = WGPUTextureDimension_2D;
  td.size.width = static_cast<uint32_t>(desc.width);
  td.size.height = static_cast<uint32_t>(desc.height);
  td.size.depthOrArrayLayers = 1;
  td.format = textureFormatOf(desc.format);
  // WebGPU path: no mipmap generation yet. Always use 1 mip level.
  td.mipLevelCount = 1;
  td.sampleCount = 1;
  td.viewFormatCount = 0;
  td.viewFormats = nullptr;
  wgpu::Texture tex = device_.createTexture(td);

  WGPUTextureViewDescriptor vd = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
  vd.format = td.format;
  vd.dimension = WGPUTextureViewDimension_2D;
  vd.mipLevelCount = 1;
  vd.baseMipLevel = 0;
  vd.baseArrayLayer = 0;
  vd.arrayLayerCount = 1;
  vd.aspect = WGPUTextureAspect_All;
  wgpu::TextureView view = tex.createView(vd);

  textures_.push_back({std::move(tex), std::move(view),
                        desc.width, desc.height});
  return static_cast<TextureHandle>(textures_.size() - 1);
}

void WebGPUBackend::updateTexture(TextureHandle texture, int width,
                                     int height, const void* data) {
  auto idx = static_cast<size_t>(texture);
  if (idx >= textures_.size()) return;

  WGPUTexelCopyTextureInfo copy = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
  copy.texture = *textures_[idx].texture;
  copy.mipLevel = 0;
  copy.origin = {0, 0, 0};
  copy.aspect = WGPUTextureAspect_All;

  WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
  layout.offset = 0;
  layout.bytesPerRow = static_cast<uint32_t>(width * 4);
  layout.rowsPerImage = static_cast<uint32_t>(height);

  WGPUExtent3D writeSize = WGPU_EXTENT_3D_INIT;
  writeSize.width = static_cast<uint32_t>(width);
  writeSize.height = static_cast<uint32_t>(height);
  writeSize.depthOrArrayLayers = 1;

  queue_.writeTexture(copy, data,
                       static_cast<size_t>(width * height * 4),
                       layout, writeSize);
}

void WebGPUBackend::destroyTexture(TextureHandle texture) {
  auto idx = static_cast<size_t>(texture);
  if (idx >= textures_.size()) return;
  textures_[idx] = {};
  for (auto it = bindGroupCache_.begin(); it != bindGroupCache_.end();) {
    if (it->first.texture == texture) {
      it = bindGroupCache_.erase(it);
    } else {
      ++it;
    }
  }
}

void WebGPUBackend::beginFrame() {
  bindGroupCache_.clear();
  resetUniformRing();
  currentPipeline_ = nullptr;
  if (engine && engine->width > 0 && engine->height > 0 &&
      (engine->width != surfaceWidth_ || engine->height != surfaceHeight_)) {
    reconfigureSurface(engine->width, engine->height);
  }
  if (!surface_ || !device_) return;
  currentBackbufferView_ = {};
  currentBackbufferTexture_ = {};

  wgpu::SurfaceTexture surfaceTexture = wgpu::Default;
  surface_->getCurrentTexture(&surfaceTexture);

  if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
      surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
    fgfx_log("WebGPU: getCurrentTexture failed (status=%d), retrying configure\n",
             static_cast<int>(surfaceTexture.status));
    reconfigureSurface(surfaceWidth_, surfaceHeight_);
    surfaceTexture = wgpu::Default;
    surface_->getCurrentTexture(&surfaceTexture);
  }

  if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
      surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
    fgfx_log("WebGPU: getCurrentTexture failed (status=%d), skipping frame\n",
             static_cast<int>(surfaceTexture.status));
    commandEncoder_ = device_.createCommandEncoder();
    return;
  }

  currentBackbufferTexture_ = wgpu::Texture(surfaceTexture.texture);
  currentBackbufferView_ = currentBackbufferTexture_->createView();
  commandEncoder_ = device_.createCommandEncoder();
}

void WebGPUBackend::clearSurface(const ClearValue& clear) {
  if (!commandEncoder_ || !currentBackbufferView_) {
    return;
  }
  WGPURenderPassColorAttachment colorAttachment =
      WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
  colorAttachment.view = *currentBackbufferView_;
  colorAttachment.loadOp = WGPULoadOp_Clear;
  colorAttachment.storeOp = WGPUStoreOp_Store;
  colorAttachment.clearValue = {clear.r, clear.g, clear.b, clear.a};

  WGPURenderPassDescriptor rpDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
  rpDesc.colorAttachmentCount = 1;
  rpDesc.colorAttachments = &colorAttachment;
  wgpu::RenderPassEncoder pass = commandEncoder_->beginRenderPass(rpDesc);
  pass.end();
}

void WebGPUBackend::beginRenderPass(TextureHandle colorTarget, LoadOp loadOp,
                                      const ClearValue& clear, int viewportW,
                                      int viewportH,
                                      TextureHandle depthTarget) {
  WGPURenderPassColorAttachment colorAttachment =
      WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
  colorAttachment.view = (colorTarget == kNullTexture)
      ? static_cast<WGPUTextureView>(*currentBackbufferView_)
      : static_cast<WGPUTextureView>(
            *textures_[static_cast<size_t>(colorTarget)].view);
  if (!colorAttachment.view) {
    passEncoder_ = {};
    return;
  }
  colorAttachment.loadOp = (loadOp == LoadOp::Clear)
      ? WGPULoadOp_Clear : WGPULoadOp_Load;
  colorAttachment.storeOp = WGPUStoreOp_Store;
  colorAttachment.clearValue = {clear.r, clear.g, clear.b, clear.a};

  WGPURenderPassDepthStencilAttachment depthAttachment =
      WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
  bool hasDepth = false;
  if (depthTarget != kNullTexture) {
    auto didx = static_cast<size_t>(depthTarget);
    if (didx < textures_.size() && *textures_[didx].view) {
      depthAttachment.view = *textures_[didx].view;
      depthAttachment.depthLoadOp = WGPULoadOp_Clear;
      depthAttachment.depthStoreOp = WGPUStoreOp_Store;
      depthAttachment.depthClearValue = 1.0f;
      depthAttachment.depthReadOnly = false;
      depthAttachment.stencilLoadOp = WGPULoadOp_Undefined;
      depthAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
      depthAttachment.stencilReadOnly = true;
      hasDepth = true;
    }
  }

  WGPURenderPassDescriptor rpDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
  rpDesc.colorAttachmentCount = 1;
  rpDesc.colorAttachments = &colorAttachment;
  if (hasDepth) {
    rpDesc.depthStencilAttachment = &depthAttachment;
  }

  passEncoder_ = commandEncoder_->beginRenderPass(rpDesc);
  passEncoder_->setViewport(0, 0, static_cast<float>(viewportW),
                              static_cast<float>(viewportH), 0.0f, 1.0f);
  currentPipeline_ = nullptr;
}

void WebGPUBackend::bindPipeline(PipelineHandle pipeline) {
  if (!passEncoder_) return;
  auto idx = static_cast<size_t>(pipeline);
  PipelineInfo* next = &pipelines_[idx];
  if (currentPipeline_ == next) {
    return;
  }
  currentPipeline_ = next;
  WGPURenderPipeline p = *currentPipeline_->pipeline;
  passEncoder_->setPipeline(p);
}

uint32_t WebGPUBackend::uniformRingCapacity() const {
  return uniformRingCapacity_;
}

uint32_t WebGPUBackend::uniformRingAlignment() const { return 256; }

void WebGPUBackend::uploadToUniformRing(uint32_t offset, const void* data,
                                        int size) {
  if (!data || size <= 0 || !scratchUniformBuffer_) {
    return;
  }
  queue_.writeBuffer(*scratchUniformBuffer_, offset, data,
                     static_cast<size_t>(size));
}

void WebGPUBackend::resetUniformRing() {}

void WebGPUBackend::bindUniformData(int binding, uint32_t offset, int size) {
  (void)binding;
  (void)size;
  pendingUniformOffset_ = offset;
}

void WebGPUBackend::bindTexture(int binding, TextureHandle texture) {
  (void)binding;
  pendingTexture_ = static_cast<size_t>(texture);
}

void WebGPUBackend::bindVertexBuffer(int binding, BufferHandle buffer) {
  if (!passEncoder_) return;
  auto idx = static_cast<size_t>(buffer);
  WGPUBuffer b = *buffers_[idx].buffer;
  // Whole-buffer bind: batches currently issue one draw per buffer. If a
  // buffer is ever sliced across draw calls, pass explicit offset+size.
  passEncoder_->setVertexBuffer(static_cast<uint32_t>(binding), b, 0,
                                WGPU_WHOLE_SIZE);
}

void WebGPUBackend::draw(int vertexCount) {
  if (!passEncoder_ || !currentPipeline_) return;
  WGPUBindGroupEntry bgEntries[3] = {
      WGPU_BIND_GROUP_ENTRY_INIT,
      WGPU_BIND_GROUP_ENTRY_INIT,
      WGPU_BIND_GROUP_ENTRY_INIT,
  };
  bgEntries[0].binding = 0;
  bgEntries[0].buffer = *scratchUniformBuffer_;
  bgEntries[0].offset = pendingUniformOffset_;
  bgEntries[0].size = static_cast<uint64_t>(currentPipeline_->uniformSize);
  uint32_t entryCount = 1;
  TextureHandle textureKey = kNullTexture;
  if (currentPipeline_->hasTexture) {
    if (pendingTexture_ >= textures_.size() ||
        !*textures_[pendingTexture_].view) {
      return;
    }
    textureKey = static_cast<TextureHandle>(pendingTexture_);
    bgEntries[1].binding = 1;
    bgEntries[1].textureView = *textures_[pendingTexture_].view;
    bgEntries[2].binding = 2;
    bgEntries[2].sampler = *defaultSampler_;
    entryCount = 3;
  }

  BindGroupKey key;
  key.layout = *currentPipeline_->bindGroupLayout;
  key.uniformOffset = pendingUniformOffset_;
  key.texture = textureKey;
  if (bindGroupCache_.size() > kMaxBindGroupCache) {
    fgfx_log("WebGPU: bind group cache overflow, clearing");
    bindGroupCache_.clear();
  }
  wgpu::BindGroup bindGroup;
  auto it = bindGroupCache_.find(key);
  if (it != bindGroupCache_.end()) {
    bindGroup = *it->second;
  } else {
    WGPUBindGroupDescriptor bgDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    bgDesc.layout = key.layout;
    bgDesc.entryCount = entryCount;
    bgDesc.entries = bgEntries;
    wgpu::raii::BindGroup created = device_.createBindGroup(bgDesc);
    bindGroup = *created;
    bindGroupCache_.emplace(key, std::move(created));
  }

  passEncoder_->setBindGroup(0, bindGroup, 0, nullptr);
  passEncoder_->draw(static_cast<uint32_t>(vertexCount), 1, 0, 0);
}

void WebGPUBackend::endRenderPass() {
  if (!passEncoder_) return;
  passEncoder_->end();
  passEncoder_ = {};
  currentPipeline_ = nullptr;
}

void WebGPUBackend::endFrame() {
  if (commandEncoder_) {
    WGPUCommandBuffer commands = commandEncoder_->finish();
    commandEncoder_ = {};
    queue_.submit(commands);
  }
  if (currentBackbufferView_) {
#ifndef EMSCRIPTEN
    // Browser swapchain presents on rAF; wgpuSurfacePresent aborts on wasm.
    surface_->present();
#endif
  }
  currentBackbufferView_ = {};
  currentBackbufferTexture_ = {};
#ifndef EMSCRIPTEN
  if (device_) device_.tick();
#endif
  if (instance_) instance_.processEvents();
}

}  // namespace fgfx
