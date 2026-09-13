#ifndef FGFX_TEXTURED_BATCH_HPP
#define FGFX_TEXTURED_BATCH_HPP

#include "Texture.hpp"
#include "backend/BackendTypes.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace fgfx {

class Engine;
struct FrameContext;
struct RenderPass;

class TexturedBatch {
 public:
  explicit TexturedBatch(Engine* engine);
  ~TexturedBatch();

  void bufferTexturedPoint(const std::shared_ptr<Texture>& texture, glm::vec2 point,
                           glm::vec2 coord, glm::vec4 color);
  void bufferTexturedPoint(const std::shared_ptr<Texture>& texture, glm::vec3 point,
                           glm::vec2 coord, glm::vec4 color);

  void reset();
  void record(RenderPass& pass, FrameContext& ctx);

 private:
  struct Buffer {
    int textureId = -1;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec2> coords;
    int pointCount = 0;
    BufferHandle positionBuf = kNullBuffer;
    BufferHandle colorBuf = kNullBuffer;
    BufferHandle coordBuf = kNullBuffer;
    int positionBufSize = 0;
    int colorBufSize = 0;
    int coordBufSize = 0;
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
