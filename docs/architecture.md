# Architecture

One frame graph. WebGL2 / OpenGL is the default backend. WebGPU is the other one.

## Backends

`RenderBackend` is a virtual interface (`createTexture` / `createBuffer` / `createPipeline`, frame/pass/draw, uniform ring). Two implementations:

| Class | Target |
|-------|--------|
| `WebGLBackend` | Native OpenGL 3.3 (GLFW + GLEW) and wasm WebGL2 |
| `WebGPUBackend` | Native Dawn/Vulkan and wasm WebGPU (one `.cpp`, init differs) |

`Engine` owns `std::unique_ptr<RenderBackend>`. Native WebGPU init is `WebGPUBackendNative.cpp` (GLFW `NO_API` + X11 surface). Wasm init is `WebGPUBackendWasm.cpp` (`navigator.gpu`).

## Frame graph

Recording is immediate, not a DAG:

1. `Engine::render` builds a `FrameBuilder` and calls the scene function.
2. The scene records `RenderPass`es (`beginPass` / `submitPass`).
3. `FrameGraph::build()` copies the pass list. There is no topological sort, barrier insertion, or transient allocator yet.
4. `FrameGraph::execute(backend)` runs passes in record order.

`PassKind::Render` maps to `beginRenderPass` … draw calls … `endRenderPass`. `PassKind::External` runs a callback (ThorVG rasterization). Passes with `requiredFeatures` the backend does not `supportsAll` are skipped (one log per pass name).

Surface clear is part of the graph: a pass targeting `kNullTexture` with `LoadOp::Clear` writes the backbuffer. If no executed pass targets the surface, `execute` calls `backend.clearSurface(surfaceClear)` (default olive `{0.31, 0.52, 0.16, 1}`). `FrameBuilder::setSurfaceClear` overrides the fallback. WebGPU does **not** clear in `beginFrame`.

`RenderPass::opaque` (default false) stable-sorts draw calls by `(pipeline, texture)` before execute. Alpha-blended scene passes leave this off.

## Draw calls and uniforms

`DrawCall` stores pipeline, texture, vertex buffers, vertex count, and a **ring offset** (`uniformOffset` / `uniformSize`). Batches copy MVP (or MSDF UBO) once per `record()` via `ctx.uniforms->alloc()`. `UniformAllocator` is a frame-scoped ring: align, wrap-with-log if full, `uploadToUniformRing`. GL uses one 1 MiB UBO (`glBufferSubData` / `glBindBufferRange`); WebGPU uses one 1 MiB buffer (`queue.writeBuffer`) plus bind-group offset.

## Bind groups and state cache

WebGPU `draw()` looks up `(layout, uniformOffset, texture)` in a frame-scoped `bindGroupCache_` (cap 256, cleared in `beginFrame`). `destroyTexture` also drops cache entries that reference that handle.

GL skips redundant `bindPipeline` / `bindTexture` within a pass. WebGPU skips redundant `setPipeline`. `currentPipeline_` / `currentProgram_` reset at pass boundaries.

Handles are indices into grow-only vectors. `destroy*` nulls the slot (RAII releases the GPU object); indices are not recycled.

## Features

```cpp
enum class Feature : uint32_t {
  Compute, StorageTexture, RasterizeToTexture
};
```

GL: Compute and StorageTexture false, RasterizeToTexture true. WebGPU: all true. `createPipeline` returns `kNullPipeline` if `!supportsAll(desc.requiredFeatures)`.

## Pipelines and color format

`PipelineDesc.targetFormat` defaults to `BGRA8Unorm`, which WebGPU treats as **swapchain format**. The swapchain itself prefers `BGRA8Unorm` / `RGBA8Unorm` over `*Srgb` so 8-bit clears and sampled PNG values match GL `GL_RGBA8` (Dawn lists `BGRA8UnormSrgb` first). Offscreen targets set an explicit format (e.g. `RGBA8Unorm`). GL ignores `targetFormat`; FBO completeness is runtime.

## Batches

Sprite/glyph/polygon/line/textured/transition/normal-mapped/PBR batches implement `record(RenderPass&, FrameContext&)`. They are ordinary objects, not created by `Engine`. One uniform `alloc` per `record()`, then one `DrawCall` per texture (or one for untextured batches).

## SVG

This library does not rasterize SVG. A host can record an external pass for that. `PassKind::External` is the hook.

## Resize

`Engine::mainLoop` polls `glfwGetFramebufferSize` each frame. `WebGPUBackend::beginFrame` reconfigures the surface when `engine->width/height` differ from the last configure. Same path for native and wasm. No GLFW framebuffer callback.

## Deferred

- Topological sort, automatic barriers, transient resource allocation
- Compute encoder / `executeComputePass` / post-process blur
- Dawn built from source (`WEBGPU_BUILD_FROM_SOURCE=ON` is unused; prebuilt distribution is enough)
- Handle recycling
