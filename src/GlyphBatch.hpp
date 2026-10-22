#ifndef FGFX_GLYPH_BATCH_HPP
#define FGFX_GLYPH_BATCH_HPP

#include "Sprite.hpp"
#include "backend/BackendTypes.hpp"
#include "BatchGpu.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace render {

class Engine;
struct FrameContext;
struct RenderPass;

class GlyphBatch {
 public:
  explicit GlyphBatch(Engine* engine);
  ~GlyphBatch();

  void setPxRange(float range);
  void setAtlasSize(glm::vec2 size);

  void bufferSprite(const std::shared_ptr<Sprite>& sprite, glm::vec2 pos,
                    glm::vec4 color, float size, float rotation);
  void bufferSprite(const std::shared_ptr<Sprite>& sprite,
                    const glm::mat4& mat, glm::vec4 color);
  void rawBufferSprite(const std::shared_ptr<Sprite>& sprite,
                       const glm::mat4& mat, glm::vec4 color);

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
  GpuUploadEpoch uploads_;
  float pxRange_ = 4.0f;
  glm::vec2 atlasSize_ = glm::vec2(1.0f, 1.0f);

  Buffer& getBuffer(int textureId);
  void ensurePipeline();
  void bufferPoint(glm::vec3 pos, glm::vec4 color, glm::vec2 coord, Buffer& buf);
};

}  // namespace render

#endif
