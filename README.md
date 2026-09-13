# FastGFX

Graphics rendering library. **WebGL2** (native OpenGL 3.3 / wasm) and **WebGPU** (Dawn Vulkan native / wasm) share one `RenderBackend` and frame graph.

Includes async file I/O (`fgfx::io`), sprite/MSDF batches, texture management, and a JavaScript runtime for web asset loading.

## Platforms

| Target | Status |
|--------|--------|
| Web (Emscripten + WebGL2) | Supported |
| Web (Emscripten + WebGPU) | Supported |
| Native Linux desktop (GLFW + GLEW / OpenGL) | Supported |
| Native Linux desktop (GLFW + Dawn / Vulkan) | Supported |
| Android / iOS / SDL | Not active (legacy code may remain) |

## Quick start

Build from the **repository root**:

```bash
cmake --build cmake-build-debug --target logo
./cmake-build-debug/examples/logo
```

WebGPU native:

```bash
cmake --build cmake-build-webgpu --target example_visualization
./cmake-build-webgpu/examples/example_visualization
```

Web example:

```bash
cmake --build cmake-build-emscripten --target msdf_example
cd cmake-build-emscripten/examples/msdf_example
python -m http.server 8080
```

## Examples

| Example | Description |
|---------|-------------|
| `logo` | Rotating logo sprite |
| `logo_blur` | Sprite + bitmap font (`cat`) |
| `lines` | Animated line layer |
| `msdf_example` | MSDF text rendering (`ui` font) |
| `example_visualization` | ThorVG + sprites + MSDF debug text (GL and WebGPU) |
| `example_game` | Reference game + loading UI |

## File I/O

Async binary loading lives in `fgfx::io` ([include/io/FileLoader.hpp](include/io/FileLoader.hpp)):

```cpp
#include <io/FileLoader.hpp>

auto data = fgfx::io::loadFile("assets/logo.png");
data->on_resolved([](std::string& bytes) { /* ... */ });
```

On native builds, `fgfx::init()` starts the I/O thread pool; `fgfx::destroy()` shuts it down.

## Documentation

- [Architecture](docs/architecture.md) — backends, frame graph, uniforms, features
- [WebGPU TODO](../web-gpu-todo.md) — phase checklist (as-built vs deferred)
- [game-dev overview](../README.md) — build presets (GL / WebGPU / wasm)

## Tests (doctest)

Headless tests cover file I/O, PNG decode, shader preprocessing, and BMFont XML parsing.

**Native — run binary:**
```bash
cmake --build cmake-build-debug --target test_fgfx
./cmake-build-debug/examples/test_fgfx
```

**Native — CTest:**
```bash
cd cmake-build-debug/game-dev/fgfx
ctest -R FGFXTests --output-on-failure
```

Filter a single test case:
```bash
./cmake-build-debug/examples/test_fgfx -tc="io load existing file"
```

## Related projects

- [asset-manager](../asset-manager/README.md) — ref-counted asset loading on top of FGFX
- [game-dev overview](../README.md)
