#include "MSDFBuffer.hpp"
#include "MSDFLayer.hpp"
#include <cmath>
#include "config.hpp"

namespace fgfx {

  MSDFBuffer::MSDFBuffer(int textureIdp) : textureId(textureIdp) {
    glGenBuffers(1, &positionBuffer);
    glGenBuffers(1, &colorBuffer);
    glGenBuffers(1, &coordBuffer);
    spritePointsCount = 0;
  }

  MSDFBuffer::~MSDFBuffer() {
    glDeleteBuffers(1, &positionBuffer);
    glDeleteBuffers(1, &colorBuffer);
    glDeleteBuffers(1, &coordBuffer);
  }

  void MSDFBuffer::reserve(int pointCount) {
    spriteVertices.reserve(pointCount);
    spriteColors.reserve(pointCount);
    spriteCoords.reserve(pointCount);
  }

  void MSDFBuffer::bufferPoint(glm::vec3 pos, glm::vec4 color, glm::vec2 coord) {
    spriteVertices.push_back(pos);
    spriteColors.push_back(color);
    spriteCoords.push_back(coord);
    spritePointsCount++;
  }

  void MSDFBuffer::bufferPoint(glm::vec2 pos, glm::vec4 color, glm::vec2 coord) {
    bufferPoint(glm::vec3(pos.x, pos.y, 0), color, coord);
  }

  void MSDFBuffer::bufferSprite(const std::shared_ptr<Sprite>& sprite, glm::vec2 pos, glm::vec4 color, double size, double rotation) {
    double sv = sin(rotation);
    double cv = cos(rotation);
    glm::vec2 up(sv * size, -cv * size);
    glm::vec2 right(cv * size * sprite->ratio, sv * size * sprite->ratio);
    glm::vec2 m1 = up - right;
    glm::vec2 m2 = up + right;

    bufferPoint(pos + m1, color, sprite->coords[0]);
    bufferPoint(pos + m2, color, sprite->coords[1]);
    bufferPoint(pos - m1, color, sprite->coords[2]);
    bufferPoint(pos - m2, color, sprite->coords[3]);
    bufferPoint(pos + m1, color, sprite->coords[0]);
    bufferPoint(pos - m1, color, sprite->coords[2]);
  }

  void MSDFBuffer::bufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4 &mat, glm::vec4 color) {
    auto r = sprite->ratio;
    glm::vec3 p1 = glm::vec3(mat * glm::vec4(-1 * r, -1, 0, 1));
    glm::vec3 p2 = glm::vec3(mat * glm::vec4(1 * r, -1, 0, 1));
    glm::vec3 p3 = glm::vec3(mat * glm::vec4(1 * r, 1, 0, 1));
    glm::vec3 p4 = glm::vec3(mat * glm::vec4(-1 * r, 1, 0, 1));

    bufferPoint(p1, color, sprite->coords[0]);
    bufferPoint(p2, color, sprite->coords[1]);
    bufferPoint(p3, color, sprite->coords[2]);
    bufferPoint(p4, color, sprite->coords[3]);
    bufferPoint(p1, color, sprite->coords[0]);
    bufferPoint(p3, color, sprite->coords[2]);
  }

  void MSDFBuffer::rawBufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4 &mat, glm::vec4 color) {
    glm::vec3 p1 = glm::vec3(mat * glm::vec4(-1, -1, 0, 1));
    glm::vec3 p2 = glm::vec3(mat * glm::vec4(1, -1, 0, 1));
    glm::vec3 p3 = glm::vec3(mat * glm::vec4(1, 1, 0, 1));
    glm::vec3 p4 = glm::vec3(mat * glm::vec4(-1, 1, 0, 1));

    bufferPoint(p1, color, sprite->coords[0]);
    bufferPoint(p2, color, sprite->coords[1]);
    bufferPoint(p3, color, sprite->coords[2]);
    bufferPoint(p4, color, sprite->coords[3]);
    bufferPoint(p1, color, sprite->coords[0]);
    bufferPoint(p3, color, sprite->coords[2]);
  }

  void MSDFBuffer::reset() {
    spriteVertices.clear();
    spriteColors.clear();
    spriteCoords.clear();
    spritePointsCount = 0;
  }

  void MSDFBuffer::upload() {
    if (!spritePointsCount) return;

    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, spritePointsCount * sizeof(glm::vec3), spriteVertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, spritePointsCount * sizeof(glm::vec4), spriteColors.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
    glBufferData(GL_ARRAY_BUFFER, spritePointsCount * sizeof(glm::vec2), spriteCoords.data(), GL_DYNAMIC_DRAW);
  }

  void MSDFBuffer::render() {
    if (!spritePointsCount) return;

    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glVertexAttribPointer(MSDFLayer::msdfProgramAttribPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
    glVertexAttribPointer(MSDFLayer::msdfProgramAttribCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glVertexAttribPointer(MSDFLayer::msdfProgramAttribColor, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, spritePointsCount);
  }

}
