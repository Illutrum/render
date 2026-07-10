#ifndef FGFX_MSDFLAYER_H
#define FGFX_MSDFLAYER_H

#include "MSDFBuffer.hpp"
#include "Layer.hpp"
#include "Sprite.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace fgfx {

  class Engine;

  class MSDFLayer : public TypedLayer<MSDFLayer> {
  public:
    std::vector<MSDFBuffer*> buffers;
    float pxRange;

    MSDFBuffer* getBuffer(int textureId);

    friend class Engine;
    friend class MSDFBuffer;

    static GLuint msdfProgram;
    static GLuint msdfProgramAttribPosition;
    static GLuint msdfProgramAttribColor;
    static GLuint msdfProgramAttribCoord;
    static GLuint msdfProgramUniformCameraMatrix;
    static GLuint msdfProgramUniformSampler;
    static GLuint msdfProgramUniformPxRange;

    static void initializeMSDFProgram();
    static void destroyMSDFProgram();

  public:
    MSDFLayer(Engine* enginep);

    void setPxRange(float range);

    void bufferSprite(const std::shared_ptr<Sprite>& sprite, glm::vec2 pos, glm::vec4 color, float size, float rotation);
    void bufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4 &mat, glm::vec4 color);
    void rawBufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4 &mat, glm::vec4 color);

    virtual void reset() override;
    virtual void upload() override;
    virtual void render(const ModelViewProjection& mvp) override;

    void beginDraw(const ModelViewProjection& mvp);
    void endDraw();
  };

}

#endif
