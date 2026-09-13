// UniformAllocator: minimal frame-allocated uniform buffer helper.
// For the spike: one persistent UBO per allocation, updated via the backend.
// Phase 10 replaces this with a proper ring buffer (mappedAtCreation /
// glBufferSubData streaming).

#ifndef FGFX_BACKEND_UNIFORM_ALLOCATOR_HPP
#define FGFX_BACKEND_UNIFORM_ALLOCATOR_HPP

#include "BackendTypes.hpp"
#include "RenderBackend.hpp"

namespace fgfx {

class UniformAllocator {
 public:
  explicit UniformAllocator(RenderBackend& backend) : backend_(&backend) {}

  // Allocate a uniform buffer of `size` bytes and upload `data` into it.
  // Returns the buffer handle. For the spike each allocation creates a new
  // buffer; the caller is responsible for destroying it (or it lives for the
  // frame and is reused).
  BufferHandle allocate(const void* data, int size) {
    BufferDesc desc{size, BufferUsage::Uniform};
    BufferHandle buf = backend_->createBuffer(desc);
    backend_->updateBuffer(buf, data, size);
    return buf;
  }

  void destroy(BufferHandle buf) { backend_->destroyBuffer(buf); }

 private:
  RenderBackend* backend_;
};

}  // namespace fgfx

#endif  // FGFX_BACKEND_UNIFORM_ALLOCATOR_HPP
