# Render

The Illutrum frame graph. WebGL2 / OpenGL is the default backend. WebGPU is the other one. A picture needs one include.

```cpp
#include <render.hpp>
```

Repository: [Illutrum/render](https://github.com/Illutrum/render).

## What it draws

`render::init` opens the window and starts file loading. `render::engine` owns the backend. A scene records passes on a `render::FrameBuilder`. Batches (sprites, MSDF and bitmap text, lines, polygons) implement `record`. Paint reads the frame graph. This library does not lay out a page and it does not rasterize SVG.

`FGFX_BACKEND` selects the backend: `GL` (default) or `WebGPU`.

## Build

```bash
git submodule update --init
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

WebGPU:

```bash
cmake -B build-webgpu -DFGFX_BACKEND=WebGPU
cmake --build build-webgpu
```

The wasm loader is `js/`. It is not an npm package.

```bash
cd js && npm ci && npm run build
```

That writes `runtime-js-compiled.js`. The host links the wasm module. This repository does not.

## Use

```cmake
add_subdirectory(render)
target_link_libraries(your_target PRIVATE render)
```

Public headers are `render.hpp`, `io/FileLoader.hpp`, and `BmFontParse.hpp`. The namespace is `render`.

## License

MIT. Copyright (c) 2026 Magowie Sp. z o.o. GLM, the WebGPU distribution, and the DejaVu atlas are named in `NOTICE`.
