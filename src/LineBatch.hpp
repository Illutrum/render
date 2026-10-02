#ifndef FGFX_LINE_BATCH_HPP
#define FGFX_LINE_BATCH_HPP

#include "backend/BackendTypes.hpp"
#include "BatchGpu.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace fgfx {

class Engine;
struct FrameContext;
struct RenderPass;

class LineBatch {
 public:
  explicit LineBatch(Engine* engine);
  ~LineBatch();

  void bufferPoint(glm::vec3 pos, glm::vec4 color);
  void bufferLine(glm::vec3 p1, glm::vec3 p2, glm::vec4 color);

  void reset();
  void record(RenderPass& pass, FrameContext& ctx);

 private:
  Engine* engine_;
  PipelineHandle pipeline_ = kNullPipeline;
  std::vector<glm::vec3> positions_;
  std::vector<glm::vec4> colors_;
  int pointCount_ = 0;
  BufferHandle positionBuf_ = kNullBuffer;
  BufferHandle colorBuf_ = kNullBuffer;
  int positionBufSize_ = 0;
  int colorBufSize_ = 0;
  bool gpuBuffersCreated_ = false;
  GpuUploadEpoch uploads_;

  void ensurePipeline();
};

}  // namespace fgfx

#endif
