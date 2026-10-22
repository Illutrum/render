// UniformAllocator: frame-scoped uniform ring buffer.
// Batches write into the backend ring at record time; DrawCall stores offset.

#ifndef FGFX_BACKEND_UNIFORM_ALLOCATOR_HPP
#define FGFX_BACKEND_UNIFORM_ALLOCATOR_HPP

#include "RenderBackend.hpp"
#include "../config.hpp"

#include <cstdint>

namespace render {

class UniformAllocator {
 public:
  explicit UniformAllocator(RenderBackend& backend)
      : backend_(&backend),
        alignment_(backend.uniformRingAlignment()),
        capacity_(backend.uniformRingCapacity()) {
    if (alignment_ == 0) {
      alignment_ = 256;
    }
  }

  uint32_t alloc(const void* data, int size) {
    if (size <= 0 || data == nullptr) {
      return 0;
    }
    auto bytes = static_cast<uint32_t>(size);
    if (bytes > capacity_) {
      render_log("UniformAllocator: alloc size %d exceeds ring capacity %u",
               size, capacity_);
      return 0;
    }
    cursor_ = alignUp(cursor_, alignment_);
    if (cursor_ + bytes > capacity_) {
      render_log("UniformAllocator: ring wrap (capacity %u)", capacity_);
      cursor_ = 0;
    }
    uint32_t offset = cursor_;
    cursor_ += bytes;
    backend_->uploadToUniformRing(offset, data, size);
    return offset;
  }

  void reset() { cursor_ = 0; }

 private:
  static uint32_t alignUp(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1u) / alignment * alignment;
  }

  RenderBackend* backend_;
  uint32_t cursor_ = 0;
  uint32_t alignment_ = 256;
  uint32_t capacity_ = 0;
};

}  // namespace render

#endif  // FGFX_BACKEND_UNIFORM_ALLOCATOR_HPP
