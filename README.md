# FastGFX

Graphics rendering library optimized for speed. Primary target is **WebGL via Emscripten**; **native desktop (GLFW + GLEW)** is supported for development and testing.

Includes async file I/O (`fgfx::io`), sprite/MSDF layers, texture management, and a JavaScript runtime for web asset loading.

## Platforms

| Target | Status |
|--------|--------|
| Web (Emscripten + WebGL) | Primary |
| Native Linux desktop (GLFW + GLEW + libpng) | Dev / testing |
| Android / iOS / SDL | Not active (legacy code may remain) |

## Quick start

Build from the **repository root**:

```bash
cmake --build cmake-build-debug --target logo
./cmake-build-debug/examples/logo
```

Web example (requires JS bundle — see [docs/building.md](docs/building.md)):

```bash
cmake --build cmake-build-emscripten --target msdf_example
cd cmake-build-emscripten/game-dev/fgfx/examples/msdf_example
python -m http.server 8080
```

## Examples

| Example | Description |
|---------|-------------|
| `logo` | Rotating logo sprite |
| `logo_blur` | Sprite + bitmap font (`cat`) |
| `lines` | Animated line layer |
| `msdf_example` | MSDF text rendering (`ui` font) |

Each web example has a matching `.html` in `examples/`.

## File I/O

Async binary loading lives in `fgfx::io` ([include/io/FileLoader.hpp](include/io/FileLoader.hpp)):

```cpp
#include <io/FileLoader.hpp>

auto data = fgfx::io::loadFile("assets/logo.png");
data->on_resolved([](std::string& bytes) { /* ... */ });
```

On native builds, `fgfx::init()` starts the I/O thread pool; `fgfx::destroy()` shuts it down. See [docs/web-runtime.md](docs/web-runtime.md) for the web bridge.

## Documentation

- [Building](docs/building.md) — native / wasm, JS bundle, dependencies
- [Fonts](docs/fonts.md) — MSDF and bitmap font generation
- [Web runtime](docs/web-runtime.md) — `fgfxLoader`, Emscripten glue
- [Architecture](docs/architecture.md) — layers, C++ vs JS split

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
