#ifndef FGFX_MSDFBUFFER_H
#define FGFX_MSDFBUFFER_H

#include "Sprite.hpp"

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "config.hpp"

namespace fgfx {

  class MSDFBuffer {
  protected:
    std::vector<glm::vec3> spriteVertices;
    std::vector<glm::vec4> spriteColors;
    std::vector<glm::vec2> spriteCoords;
    int spritePointsCount;

    GLuint positionBuffer;
    GLuint colorBuffer;
    GLuint coordBuffer;

    friend class MSDFLayer;
  public:
    int textureId;

    MSDFBuffer(int textureIdp);
    ~MSDFBuffer();

    void reserve(int pointCount);
    void bufferPoint(glm::vec3 pos, glm::vec4 color, glm::vec2 coord);
    void bufferPoint(glm::vec2 pos, glm::vec4 color, glm::vec2 coord);
    void bufferSprite(const std::shared_ptr<Sprite>& sprite, glm::vec2 pos, glm::vec4 color, double size, double rotation);
    void bufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4 &mat, glm::vec4 color);
    void rawBufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4 &mat, glm::vec4 color);
    void reset();
    void upload();
    void render();
  };

}

#endif
