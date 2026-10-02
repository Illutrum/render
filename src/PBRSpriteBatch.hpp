#ifndef FGFX_PBR_SPRITE_BATCH_HPP
#define FGFX_PBR_SPRITE_BATCH_HPP

#include "Sprite.hpp"
#include "backend/BackendTypes.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace fgfx {

class Engine;
struct FrameContext;
struct RenderPass;

class PBRSpriteBatch {
 public:
  explicit PBRSpriteBatch(Engine* engine);
  ~PBRSpriteBatch();

  void bufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4& mat,
                    glm::vec4 color);
  void rawBufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4& mat,
                       glm::vec4 color);

  void setLightDir(glm::vec3 dir);
  void setEyeDir(glm::vec3 dir);

  void reset();
  void record(RenderPass& pass, FrameContext& ctx);

 private:
  struct Buffer {
    int textureId = -1;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> normals;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec2> colorCoords;
    std::vector<glm::vec2> normalCoords;
    std::vector<glm::vec2> materialCoords;
    int pointCount = 0;
    BufferHandle positionBuf = kNullBuffer;
    BufferHandle normalBuf = kNullBuffer;
    BufferHandle colorBuf = kNullBuffer;
    BufferHandle colorCoordBuf = kNullBuffer;
    BufferHandle normalCoordBuf = kNullBuffer;
    BufferHandle materialCoordBuf = kNullBuffer;
    int positionBufSize = 0;
    int normalBufSize = 0;
    int colorBufSize = 0;
    int colorCoordBufSize = 0;
    int normalCoordBufSize = 0;
    int materialCoordBufSize = 0;
    bool gpuBuffersCreated = false;
  };

  Engine* engine_;
  PipelineHandle pipeline_ = kNullPipeline;
  std::vector<Buffer> buffers_;
  glm::vec3 lightDir_{1.0f, 5.0f, 4.0f};
  glm::vec3 eyeDir_{0.0f, 0.0f, 1.0f};

  Buffer& getBuffer(int textureId);
  void ensurePipeline();
  void bufferPoint(Buffer& buf, glm::vec3 pos, glm::vec2 normal, glm::vec4 color,
                   glm::vec2 colorCoord, glm::vec2 normalCoord,
                   glm::vec2 materialCoord);
};

}  // namespace fgfx

#endif
