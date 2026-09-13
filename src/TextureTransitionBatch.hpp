#ifndef FGFX_TEXTURE_TRANSITION_BATCH_HPP
#define FGFX_TEXTURE_TRANSITION_BATCH_HPP

#include "Texture.hpp"
#include "backend/BackendTypes.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace fgfx {

class Engine;
struct FrameContext;
struct RenderPass;

class TextureTransitionBatch {
 public:
  explicit TextureTransitionBatch(Engine* engine);
  ~TextureTransitionBatch();

  void bufferTextureTransitionPoint(const std::shared_ptr<Texture>& texture,
                                    glm::vec2 point, glm::vec4 color,
                                    glm::vec3 coord1, glm::vec3 coord2);
  void bufferTextureTransitionPoint(const std::shared_ptr<Texture>& texture,
                                    glm::vec3 point, glm::vec4 color,
                                    glm::vec3 coord1, glm::vec3 coord2);

  void reset();
  void record(RenderPass& pass, FrameContext& ctx);

 private:
  struct Buffer {
    int textureId = -1;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> coords1;
    std::vector<glm::vec3> coords2;
    int pointCount = 0;
    BufferHandle positionBuf = kNullBuffer;
    BufferHandle colorBuf = kNullBuffer;
    BufferHandle coord1Buf = kNullBuffer;
    BufferHandle coord2Buf = kNullBuffer;
    int positionBufSize = 0;
    int colorBufSize = 0;
    int coord1BufSize = 0;
    int coord2BufSize = 0;
    bool gpuBuffersCreated = false;
  };

  Engine* engine_;
  PipelineHandle pipeline_ = kNullPipeline;
  std::vector<Buffer> buffers_;

  Buffer& getBuffer(int textureId);
  void ensurePipeline();
};

}  // namespace fgfx

#endif
