#include "MSDFLayer.hpp"
#include "Engine.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "config.hpp"

namespace fgfx {

  MSDFLayer::MSDFLayer(Engine* enginep) : TypedLayer<MSDFLayer>(enginep) {
    composition = 1;
    pxRange = 4.0f;
  }

  void MSDFLayer::setPxRange(float range) {
    pxRange = range;
  }

  MSDFBuffer* MSDFLayer::getBuffer(int textureId) {
    for (auto it = buffers.begin(); it != buffers.end(); ++it) {
      if ((*it)->textureId == textureId) return *it;
    }
    MSDFBuffer* buf = new MSDFBuffer(textureId);
    buffers.push_back(buf);
    return buf;
  }

  void MSDFLayer::bufferSprite(const std::shared_ptr<Sprite>& sprite, glm::vec2 pos, glm::vec4 color, float size, float rotation) {
    if (sprite->unloaded) engine->reloadSprite(sprite);
    if (sprite->textureId == -1) return;
    getBuffer(sprite->textureId)->bufferSprite(sprite, pos, color, size, rotation);
  }

  void MSDFLayer::bufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4 &mat, glm::vec4 color) {
    if (sprite->unloaded) engine->reloadSprite(sprite);
    if (sprite->textureId == -1) return;
    getBuffer(sprite->textureId)->bufferSprite(sprite, mat, color);
  }

  void MSDFLayer::rawBufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4 &mat, glm::vec4 color) {
    if (sprite->unloaded) engine->reloadSprite(sprite);
    if (sprite->textureId == -1) return;
    getBuffer(sprite->textureId)->rawBufferSprite(sprite, mat, color);
  }

  GLuint MSDFLayer::msdfProgram = 0;
  GLuint MSDFLayer::msdfProgramAttribPosition = 0;
  GLuint MSDFLayer::msdfProgramAttribColor = 0;
  GLuint MSDFLayer::msdfProgramAttribCoord = 0;
  GLuint MSDFLayer::msdfProgramUniformCameraMatrix = 0;
  GLuint MSDFLayer::msdfProgramUniformSampler = 0;
  GLuint MSDFLayer::msdfProgramUniformPxRange = 0;
  GLuint MSDFLayer::msdfProgramUniformAtlasSize = 0;

  void MSDFLayer::initializeMSDFProgram() {
#ifndef __USE_OPENGL
    const char vShaderStr[] =
      "#version 300 es\n"
      "in vec3 aVertexPosition;\n"
      "in vec4 aVertexColor;\n"
      "in vec2 aTextureCoord;\n"
      "uniform mat4 uCameraMatrix;\n"
      "out vec4 vColor;\n"
      "out highp vec2 vTextureCoord;\n"
      "void main() {\n"
      "  gl_Position = uCameraMatrix * vec4(aVertexPosition, 1.0);\n"
      "  vColor = aVertexColor;\n"
      "  vTextureCoord = aTextureCoord;\n"
      "}\n";

    const char fShaderStr[] =
      "#version 300 es\n"
      "precision highp float;\n"
      "in vec4 vColor;\n"
      "uniform sampler2D uSampler;\n"
      "uniform float uPxRange;\n"
      "uniform vec2 uAtlasSize;\n"
      "in highp vec2 vTextureCoord;\n"
      "out vec4 fragColor;\n"
      "float median(float r, float g, float b) {\n"
      "  return max(min(r, g), min(max(r, g), b));\n"
      "}\n"
      "float screenPxRange() {\n"
      "  vec2 unitRange = vec2(uPxRange) / max(uAtlasSize, vec2(1.0));\n"
      "  vec2 screenTexSize = vec2(1.0) / fwidth(vTextureCoord);\n"
      "  return max(0.5 * dot(unitRange, screenTexSize), 1.0);\n"
      "}\n"
      "void main() {\n"
      "  vec3 msd = texture(uSampler, vTextureCoord).rgb;\n"
      "  float sd = median(msd.r, msd.g, msd.b);\n"
      "  float screenPxDistance = screenPxRange() * (sd - 0.5);\n"
      "  float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);\n"
      "  fragColor = vec4(vColor.rgb, vColor.a * opacity);\n"
      "}\n";
#endif

#ifdef __USE_OPENGL
    const char vShaderStr[] =
      "attribute vec3 aVertexPosition;\n"
      "attribute vec4 aVertexColor;\n"
      "attribute vec2 aTextureCoord;\n"
      "uniform mat4 uCameraMatrix;\n"
      "varying vec4 vColor;\n"
      "varying vec2 vTextureCoord;\n"
      "void main() {\n"
      "  gl_Position = uCameraMatrix * vec4(aVertexPosition, 1.0);\n"
      "  vColor = aVertexColor;\n"
      "  vTextureCoord = aTextureCoord;\n"
      "}\n";

    const char fShaderStr[] =
      "varying vec4 vColor;\n"
      "uniform sampler2D uSampler;\n"
      "uniform float uPxRange;\n"
      "uniform vec2 uAtlasSize;\n"
      "varying vec2 vTextureCoord;\n"
      "float median(float r, float g, float b) {\n"
      "  return max(min(r, g), min(max(r, g), b));\n"
      "}\n"
      "float screenPxRange() {\n"
      "  vec2 unitRange = vec2(uPxRange) / max(uAtlasSize, vec2(1.0));\n"
      "  vec2 screenTexSize = vec2(1.0) / fwidth(vTextureCoord);\n"
      "  return max(0.5 * dot(unitRange, screenTexSize), 1.0);\n"
      "}\n"
      "void main() {\n"
      "  vec3 msd = texture2D(uSampler, vTextureCoord).rgb;\n"
      "  float sd = median(msd.r, msd.g, msd.b);\n"
      "  float screenPxDistance = screenPxRange() * (sd - 0.5);\n"
      "  float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);\n"
      "  gl_FragColor = vec4(vColor.rgb, vColor.a * opacity);\n"
      "}\n";
#endif

    auto vertexShader = fgfx::loadShader(GL_VERTEX_SHADER, vShaderStr);
    auto fragmentShader = fgfx::loadShader(GL_FRAGMENT_SHADER, fShaderStr);
    auto program = fgfx::createProgram(vertexShader, fragmentShader);
    if (!program) throw "MSDF shader program failed";

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    msdfProgram = program;
    msdfProgramAttribPosition = glGetAttribLocation(program, "aVertexPosition");
    msdfProgramAttribColor = glGetAttribLocation(program, "aVertexColor");
    msdfProgramAttribCoord = glGetAttribLocation(program, "aTextureCoord");
    msdfProgramUniformCameraMatrix = glGetUniformLocation(program, "uCameraMatrix");
    msdfProgramUniformSampler = glGetUniformLocation(program, "uSampler");
    msdfProgramUniformPxRange = glGetUniformLocation(program, "uPxRange");
    msdfProgramUniformAtlasSize = glGetUniformLocation(program, "uAtlasSize");
  }

  void MSDFLayer::beginDraw(const ModelViewProjection& mvp) {
    glUseProgram(msdfProgram);
    glEnableVertexAttribArray(msdfProgramAttribPosition);
    glEnableVertexAttribArray(msdfProgramAttribColor);
    glEnableVertexAttribArray(msdfProgramAttribCoord);
    glUniformMatrix4fv(msdfProgramUniformCameraMatrix, 1, GL_FALSE,
                       (GLfloat*)glm::value_ptr(mvp.multipliedMatrix));
    glUniform1f(msdfProgramUniformPxRange, pxRange);
  }

  void MSDFLayer::endDraw() {
    glDisableVertexAttribArray(msdfProgramAttribPosition);
    glDisableVertexAttribArray(msdfProgramAttribColor);
    glDisableVertexAttribArray(msdfProgramAttribCoord);
  }

  void MSDFLayer::reset() {
    for (auto it = buffers.begin(); it != buffers.end(); ++it) {
      (*it)->reset();
    }
  }

  void MSDFLayer::upload() {
    if (!visible) return;
    for (auto it = buffers.begin(); it != buffers.end(); ++it) {
      (*it)->upload();
    }
  }

  void MSDFLayer::render(const ModelViewProjection& mvp) {
    if (!visible) return;
    beginDraw(mvp);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_CULL_FACE);
    for (auto it = buffers.begin(); it != buffers.end(); ++it) {
      MSDFBuffer* buffer = *it;
      glBindTexture(GL_TEXTURE_2D, buffer->textureId);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      GLint atlasWidth = 1;
      GLint atlasHeight = 1;
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &atlasWidth);
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &atlasHeight);
      glUniform2f(msdfProgramUniformAtlasSize, static_cast<float>(atlasWidth), static_cast<float>(atlasHeight));
      glUniform1i(msdfProgramUniformSampler, 0);
      buffer->render();
    }
    endDraw();
  }

  void MSDFLayer::destroyMSDFProgram() {
    glDeleteProgram(msdfProgram);
  }

}
