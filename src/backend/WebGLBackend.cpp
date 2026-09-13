// WebGLBackend implementation. Wraps existing GL calls behind the
// RenderBackend interface. Only the sprite path is implemented for the spike.

#include "WebGLBackend.hpp"
#include "../shaders.hpp"

#include <glm/glm.hpp>

namespace fgfx {

namespace {
// Map PixelFormat to (component count, GL type).
struct GlFormat { int components; GLenum type; };
GlFormat glFormatOf(PixelFormat f) {
  switch (f) {
    case PixelFormat::RGBA32Float: return {4, GL_FLOAT};
    case PixelFormat::RGBA8Unorm:  return {4, GL_UNSIGNED_BYTE};
    case PixelFormat::RGB32Float:  return {3, GL_FLOAT};
    case PixelFormat::RG32Float:   return {2, GL_FLOAT};
    case PixelFormat::R32Float:    return {1, GL_FLOAT};
    case PixelFormat::BGRA8Unorm:  return {4, GL_UNSIGNED_BYTE};
  }
  return {4, GL_FLOAT};
}

GLenum glUsageOf(BufferUsage u) {
  switch (u) {
    case BufferUsage::Vertex:  return GL_ARRAY_BUFFER;
    case BufferUsage::Index:   return GL_ELEMENT_ARRAY_BUFFER;
    case BufferUsage::Uniform: return GL_UNIFORM_BUFFER;
    case BufferUsage::Storage:
      // SSBO is desktop GL 4.3 / GLES 3.1. WebGL2 is GLES 3.0 and has no
      // GL_SHADER_STORAGE_BUFFER. Sprite path never uses Storage; keep a
      // harmless fallback so the wasm build compiles.
#ifdef GL_SHADER_STORAGE_BUFFER
      return GL_SHADER_STORAGE_BUFFER;
#else
      return GL_ARRAY_BUFFER;
#endif
  }
  return GL_ARRAY_BUFFER;
}
}  // namespace

WebGLBackend::~WebGLBackend() {
  for (auto& [_, info] : pipelines_) {
    if (info.program) glDeleteProgram(info.program);
  }
  pipelines_.clear();
  for (auto& [_, fbo] : fboCache_) {
    if (fbo) glDeleteFramebuffers(1, &fbo);
  }
  fboCache_.clear();
  if (stencilRbo_) {
    glDeleteRenderbuffers(1, &stencilRbo_);
    stencilRbo_ = 0;
  }
  if (scratchUbo_) {
    glDeleteBuffers(1, &scratchUbo_);
    scratchUbo_ = 0;
  }
}

PipelineHandle WebGLBackend::createPipeline(const PipelineDesc& desc) {
  GLuint vs = loadShader(GL_VERTEX_SHADER, desc.vertexGLSL);
  GLuint fs = loadShader(GL_FRAGMENT_SHADER, desc.fragmentGLSL);
  GLuint prog = createProgram(vs, fs);
  glDeleteShader(vs);
  glDeleteShader(fs);

  PipelineInfo info;
  info.program = prog;
  switch (desc.topology) {
    case PrimitiveTopology::LineList:  info.topology = GL_LINES; break;
    case PrimitiveTopology::LineStrip: info.topology = GL_LINE_STRIP; break;
    case PrimitiveTopology::PointList: info.topology = GL_POINTS; break;
    case PrimitiveTopology::TriangleList:
    default:                           info.topology = GL_TRIANGLES; break;
  }
  for (const auto& vb : desc.vertexBindings) {
    std::vector<PipelineInfo::AttrInfo> attrs;
    for (const auto& a : vb.attributes) {
      GlFormat gf = glFormatOf(a.format);
      // Query the actual GL attribute location by name (the linker
      // assigns locations; our PipelineDesc location is just a hint).
      GLint loc = a.location;
      if (!a.name.empty()) {
        loc = glGetAttribLocation(prog, a.name.c_str());
        if (loc < 0) continue;  // attribute not found / optimized out
      }
      attrs.push_back({loc, gf.components, a.offset});
    }
    info.bindingAttrs[vb.binding] = std::move(attrs);
  }
  info.samplerLocation = glGetUniformLocation(prog, "uSampler");
  GLint numBlocks = 0;
  glGetProgramiv(prog, GL_ACTIVE_UNIFORM_BLOCKS, &numBlocks);
  if (numBlocks > 0) {
    glUniformBlockBinding(prog, 0, 0);
  }

  PipelineHandle handle = static_cast<PipelineHandle>(prog);
  pipelines_[prog] = std::move(info);
  return handle;
}

void WebGLBackend::destroyPipeline(PipelineHandle pipeline) {
  auto it = pipelines_.find(static_cast<GLuint>(pipeline));
  if (it == pipelines_.end()) return;
  if (it->second.program) glDeleteProgram(it->second.program);
  pipelines_.erase(it);
}

BufferHandle WebGLBackend::createBuffer(const BufferDesc& desc) {
  GLuint buf = 0;
  glGenBuffers(1, &buf);
  GLenum target = glUsageOf(desc.usage);
  glBindBuffer(target, buf);
  glBufferData(target, desc.size, nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(target, 0);
  bufferTargets_[buf] = target;
  return static_cast<BufferHandle>(buf);
}

void WebGLBackend::updateBuffer(BufferHandle buffer, const void* data,
                                 int size) {
  GLuint buf = static_cast<GLuint>(buffer);
  GLenum target = GL_ARRAY_BUFFER;
  auto it = bufferTargets_.find(buf);
  if (it != bufferTargets_.end()) target = it->second;
  glBindBuffer(target, buf);
  glBufferData(target, size, data, GL_DYNAMIC_DRAW);
  glBindBuffer(target, 0);
}

void WebGLBackend::destroyBuffer(BufferHandle buffer) {
  GLuint buf = static_cast<GLuint>(buffer);
  bufferTargets_.erase(buf);
  glDeleteBuffers(1, &buf);
}

TextureHandle WebGLBackend::createTexture(const TextureDesc& desc) {
  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  desc.generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, desc.width, desc.height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glBindTexture(GL_TEXTURE_2D, 0);
  return static_cast<TextureHandle>(tex);
}

void WebGLBackend::updateTexture(TextureHandle texture, int width, int height,
                                  const void* data) {
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
              GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void WebGLBackend::destroyTexture(TextureHandle texture) {
  GLuint tex = static_cast<GLuint>(texture);
  auto it = fboCache_.find(tex);
  if (it != fboCache_.end()) {
    glDeleteFramebuffers(1, &it->second);
    fboCache_.erase(it);
  }
  glDeleteTextures(1, &tex);
}

GLuint WebGLBackend::ensureFramebuffer(TextureHandle texture) {
  GLuint tex = static_cast<GLuint>(texture);
  auto it = fboCache_.find(tex);
  if (it != fboCache_.end()) {
    return it->second;
  }
  GLuint fbo = 0;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  fboCache_[tex] = fbo;
  return fbo;
}

void WebGLBackend::bindColorTarget(TextureHandle texture, int viewportW, int viewportH) {
  if (texture == kNullTexture) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, ensureFramebuffer(texture));
  }
  glViewport(0, 0, viewportW, viewportH);
}

void WebGLBackend::attachDepthStencil(int width, int height) {
  if (stencilRbo_ == 0) {
    glGenRenderbuffers(1, &stencilRbo_);
  }
  if (stencilW_ != width || stencilH_ != height) {
    glBindRenderbuffer(GL_RENDERBUFFER, stencilRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    stencilW_ = width;
    stencilH_ = height;
  }
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, stencilRbo_);
}

void WebGLBackend::unbindColorTarget() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WebGLBackend::beginFrame() {
  // GL is immediate-mode; nothing to do.
}

void WebGLBackend::beginRenderPass(TextureHandle colorTarget, LoadOp loadOp,
                                    const ClearValue& clear, int viewportW,
                                    int viewportH) {
  bindColorTarget(colorTarget, viewportW, viewportH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  if (loadOp == LoadOp::Clear) {
    glClearColor(clear.r, clear.g, clear.b, clear.a);
    glClear(GL_COLOR_BUFFER_BIT);
  }
}

void WebGLBackend::bindPipeline(PipelineHandle pipeline) {
  GLuint prog = static_cast<GLuint>(pipeline);
  glUseProgram(prog);
  currentProgram_ = prog;
  auto it = pipelines_.find(prog);
  if (it == pipelines_.end()) return;
  // Enable all vertex attrib arrays for this pipeline.
  for (auto& [binding, attrs] : it->second.bindingAttrs) {
    for (auto& a : attrs) {
      glEnableVertexAttribArray(a.location);
    }
  }
  // Bind sampler to unit 0 for the spike.
  if (it->second.samplerLocation >= 0) {
    glUniform1i(it->second.samplerLocation, 0);
  }
}

void WebGLBackend::bindUniformData(int binding, const void* data, int size) {
  if (binding != 0 || !data || size <= 0) return;
  if (!scratchUbo_) {
    glGenBuffers(1, &scratchUbo_);
  }
  glBindBuffer(GL_UNIFORM_BUFFER, scratchUbo_);
  glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, scratchUbo_);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void WebGLBackend::bindTexture(int binding, TextureHandle texture) {
  glActiveTexture(GL_TEXTURE0 + binding);
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
}

void WebGLBackend::bindVertexBuffer(int binding, BufferHandle buffer) {
  glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(buffer));
  auto it = pipelines_.find(currentProgram_);
  if (it == pipelines_.end()) return;
  auto bit = it->second.bindingAttrs.find(binding);
  if (bit == it->second.bindingAttrs.end()) return;
  for (auto& a : bit->second) {
    glVertexAttribPointer(a.location, a.size, GL_FLOAT, GL_FALSE, 0,
                          reinterpret_cast<void*>(a.offset));
  }
}

void WebGLBackend::draw(int vertexCount) {
  GLenum topo = GL_TRIANGLES;
  auto it = pipelines_.find(currentProgram_);
  if (it != pipelines_.end()) topo = it->second.topology;
  glDrawArrays(topo, 0, vertexCount);
}

void WebGLBackend::endRenderPass() {
  // Disable vertex attrib arrays to keep GL state clean for the old path.
  auto it = pipelines_.find(currentProgram_);
  if (it != pipelines_.end()) {
    for (auto& [binding, attrs] : it->second.bindingAttrs) {
      for (auto& a : attrs) {
        glDisableVertexAttribArray(a.location);
      }
    }
  }
}

void WebGLBackend::endFrame() {
  // Present is handled by the caller (glfwSwapBuffers / emscripten GL context).
}

}  // namespace fgfx
