#ifndef FGFX_BATCH_GPU_HPP
#define FGFX_BATCH_GPU_HPP

#include "backend/BackendTypes.hpp"
#include "backend/RenderBackend.hpp"

#include <cstdint>
#include <vector>

namespace fgfx {

// FrameGraph runs after the scene has recorded every pass. Reusing one GPU
// vertex buffer across record() calls would leave earlier DrawCalls pointing
// at the last upload. Keep each record's buffers until the next frame.
struct RetiredGpuMesh {
  BufferHandle position = kNullBuffer;
  BufferHandle color = kNullBuffer;
  BufferHandle coord = kNullBuffer;
};

inline void destroyRetiredGpuMesh(RenderBackend* backend, RetiredGpuMesh& mesh) {
  if (!backend) {
    return;
  }
  if (mesh.position != kNullBuffer) {
    backend->destroyBuffer(mesh.position);
  }
  if (mesh.color != kNullBuffer) {
    backend->destroyBuffer(mesh.color);
  }
  if (mesh.coord != kNullBuffer) {
    backend->destroyBuffer(mesh.coord);
  }
  mesh = {};
}

struct GpuUploadEpoch {
  uint64_t gcFrame = 0;
  std::vector<RetiredGpuMesh> retired;

  void gc(uint64_t frame, RenderBackend* backend) {
    if (frame == gcFrame) {
      return;
    }
    gcFrame = frame;
    for (auto& mesh : retired) {
      destroyRetiredGpuMesh(backend, mesh);
    }
    retired.clear();
  }

  void retire(BufferHandle position, BufferHandle color,
              BufferHandle coord = kNullBuffer) {
    retired.push_back({position, color, coord});
  }

  void destroyAll(RenderBackend* backend) {
    for (auto& mesh : retired) {
      destroyRetiredGpuMesh(backend, mesh);
    }
    retired.clear();
  }
};

}  // namespace fgfx

#endif
