#ifndef FGFX_NORMAL_MAPPED_SPRITE_BATCH_HPP
#define FGFX_NORMAL_MAPPED_SPRITE_BATCH_HPP

#include "Sprite.hpp"
#include "backend/BackendTypes.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace fgfx {

class Engine;
struct FrameContext;
struct RenderPass;

class NormalMappedSpriteBatch {
 public:
  explicit NormalMappedSpriteBatch(Engine* engine);
  ~NormalMappedSpriteBatch();

  void bufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4& mat,
                    glm::vec4 color);
  void rawBufferSprite(const std::shared_ptr<Sprite>& sprite, const glm::mat4& mat,
                       glm::vec4 color);

  void reset();
  void record(RenderPass& pass, FrameContext& ctx);

 private:
  struct Buffer {
    int textureId = -1;
    std::vector<glm::vec2> positions;
    std::vector<glm::vec2> normals;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec2> colorCoords;
    std::vector<glm::vec2> normalCoords;
    int pointCount = 0;
    BufferHandle positionBuf = kNullBuffer;
    BufferHandle normalBuf = kNullBuffer;
    BufferHandle colorBuf = kNullBuffer;
    BufferHandle colorCoordBuf = kNullBuffer;
    BufferHandle normalCoordBuf = kNullBuffer;
    int positionBufSize = 0;
    int normalBufSize = 0;
    int colorBufSize = 0;
    int colorCoordBufSize = 0;
    int normalCoordBufSize = 0;
    bool gpuBuffersCreated = false;
  };

  Engine* engine_;
  PipelineHandle pipeline_ = kNullPipeline;
  std::vector<Buffer> buffers_;

  Buffer& getBuffer(int textureId);
  void ensurePipeline();
  void bufferPoint(Buffer& buf, glm::vec2 pos, glm::vec2 normal, glm::vec4 color,
                   glm::vec2 colorCoord, glm::vec2 normalCoord);
};

}  // namespace fgfx

#endif
