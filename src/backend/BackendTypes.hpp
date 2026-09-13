// Backend-agnostic resource types for the render graph.
// Handles are uint32_t: on GL they alias GLuint names; on WebGPU they index
// into the backend's internal resource tables. 0 is the null handle.

#ifndef FGFX_BACKEND_BACKEND_TYPES_HPP
#define FGFX_BACKEND_BACKEND_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace fgfx {

enum class BackendType { GL, WebGPU };

enum class Feature : uint32_t {
  Compute            = 1u << 0,
  StorageTexture     = 1u << 1,
  RasterizeToTexture = 1u << 2,
};
using FeatureSet = uint32_t;

constexpr FeatureSet featureBit(Feature f) { return static_cast<uint32_t>(f); }
constexpr bool hasFeature(FeatureSet set, Feature f) {
  return (set & featureBit(f)) != 0;
}
inline const char* featureName(Feature f) {
  switch (f) {
    case Feature::Compute: return "Compute";
    case Feature::StorageTexture: return "StorageTexture";
    case Feature::RasterizeToTexture: return "RasterizeToTexture";
  }
  return "Unknown";
}

// Opaque resource handles. 0 == null.
using TextureHandle = uint32_t;
using BufferHandle = uint32_t;
using PipelineHandle = uint32_t;

constexpr TextureHandle kNullTexture = 0;
constexpr BufferHandle kNullBuffer = 0;
constexpr PipelineHandle kNullPipeline = 0;

enum class PixelFormat {
  RGBA8Unorm,
  BGRA8Unorm,
  R32Float,
  RG32Float,
  RGB32Float,
  RGBA32Float,
};

enum class BufferUsage {
  Vertex,
  Index,
  Uniform,
  Storage,
};

enum class PrimitiveTopology {
  TriangleList,
  LineList,
  LineStrip,
  PointList,
};

enum class BlendFactor {
  One,
  Zero,
  SrcAlpha,
  OneMinusSrcAlpha,
};

enum class LoadOp { Load, Clear, DontCare };
enum class StoreOp { Store, DontCare };

struct VertexAttribute {
  int location;
  int offset;
  PixelFormat format;
  std::string name;  // GLSL attribute name (used by WebGLBackend to query location)
};

struct VertexBinding {
  int binding;
  int stride;
  std::vector<VertexAttribute> attributes;
};

struct BlendState {
  bool enabled = false;
  BlendFactor srcColor = BlendFactor::SrcAlpha;
  BlendFactor dstColor = BlendFactor::OneMinusSrcAlpha;
};

struct PipelineDesc {
  // GLSL sources (used by WebGLBackend) and WGSL sources (used by WebGPUBackend).
  // The backend picks the pair it understands.
  std::string vertexGLSL;
  std::string fragmentGLSL;
  std::string vertexWGSL;
  std::string fragmentWGSL;
  std::vector<VertexBinding> vertexBindings;
  BlendState blend;
  PrimitiveTopology topology = PrimitiveTopology::TriangleList;
  // Packed UBO size in bytes (std140). Sprite/polygon = 64, MSDF = 80.
  int uniformSize = 64;
  // When false, bind group is uniform-only (no sampled texture).
  bool hasTexture = true;
  FeatureSet requiredFeatures = 0;
};

struct TextureDesc {
  int width;
  int height;
  PixelFormat format = PixelFormat::RGBA8Unorm;
  bool generateMipmaps = true;
};

struct BufferDesc {
  int size;
  BufferUsage usage = BufferUsage::Vertex;
};

struct ClearValue {
  float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
};

}  // namespace fgfx

#endif  // FGFX_BACKEND_BACKEND_TYPES_HPP
