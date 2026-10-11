# Web runtime

On Emscripten builds, FastGFX splits work between C++ (render loop, layer logic) and JavaScript (WebGL texture upload, asset fetching, BMFont parsing).

## runtime-js-compiled.js

Built from `js/` via Rollup (`npm run build`). Loaded by each example HTML before the wasm module:

```html
<script src="runtime-js-compiled.js"></script>
<script src="logo.js"></script>
```

Responsibilities:

- WebGL context on `#fgfx_canvas`
- `TextureManager` — sprite atlas packing, font parsing, texture uploads. JS-created WebGL textures must go through `ModuleGL.getNewId(GL.textures)` (not `textures.length`); otherwise the next C++ `glGenTextures` overwrites the MSDF atlas.
- Animation loop calling `Module._fgfx_render`

Initialization hooks exported to wasm:

- `Module.initializeFastGfxRuntime()` — bind GL context, start animator
- `Module.initializeFastGfxTextureManager()` — create TextureManager

## fgfxLoader

Default implementation in `js/runtime.js` (overridable before runtime init):

| Method | Returns | Used for |
|--------|---------|----------|
| `loadBinary(path)` | `Uint8Array` | Generic binary files |
| `loadSpriteImage(path)` | `HTMLImageElement` | Sprites |
| `loadTextureImage(path)` | `HTMLImageElement` | Textures |
| `loadFontData(name)` | text (`.fnt`) | Sprite fonts |
| `loadFontImage(name)` | image (`.png`) | Sprite font atlas |
| `loadMSDFFontData(name)` | text (`.fnt`) | MSDF fonts |
| `loadMSDFFontImage(name)` | image (`.png`) | MSDF atlas |
| `loadText(path)` | text | Misc text files |

All paths are resolved relative to the served example directory (typically under `assets/`).

## fgfx::io (C++ bridge)

On wasm, `fgfx::io::loadFile` does not read the filesystem directly. It calls `window.fgfxLoader.loadBinary` via EM_ASM and completes a C++ promise when data arrives:

```
loadFile(path) → EM_ASM → fgfxLoader.loadBinary
  → fgfx_io_fileLoaded(id, buffer, len)  // resolve promise
  → fgfx_io_fileFailed(id)               // reject promise
```

Native builds use a background thread pool + `std::ifstream` instead. `fgfx::init()` starts I/O workers; `fgfx::destroy()` joins them.

### API summary

```cpp
#include <io/FileLoader.hpp>

fgfx::io::init(1);   // called automatically by fgfx::init() on native
auto p = fgfx::io::loadFile("assets/level.json");
p->on_resolved([](std::string& data) { /* ... */ });
p->on_rejected([](std::exception_ptr err) { /* FileNotFoundError */ });
fgfx::io::destroy(); // called automatically by fgfx::destroy() on native
```

Depends on [cpp-rpc](../../../cpp-rpc) `serialization::promise`.

## emscripten-glue.js

Post-js linked into wasm examples (`--post-js=js/emscripten-glue.js`). Extends exported runtime methods alongside CMake `EXPORTED_FUNCTIONS` (includes `_fgfx_io_fileLoaded`, `_fgfx_io_fileFailed`).

## Asset manager on web

`asset-manager` examples compile fgfx sources directly into the wasm binary and reuse the same `fgfxLoader` + runtime bundle. See [asset-manager README](../../asset-manager/README.md).
