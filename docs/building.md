# Building FastGFX

Build from the **repository root**, not from `game-dev/fgfx/` directly.

## Native (Linux desktop)

Configure once (if not already done):

```bash
cmake -B cmake-build-debug -S .
```

Build examples or tests:

```bash
cmake --build cmake-build-debug --target logo msdf_example test_fgfx
```

Native binaries land in `cmake-build-debug/examples/`. Example assets are copied to `cmake-build-debug/examples/assets/` automatically (POST_BUILD).

Run:

```bash
cd cmake-build-debug/examples
./logo
```

### Native dependencies

- OpenGL
- GLFW
- GLEW
- libpng + zlib

## Web (Emscripten)

Use the Emscripten toolchain preset (e.g. `cmake-build-emscripten`):

```bash
cmake --build cmake-build-emscripten --target msdf_example
```

Output per example:

```
cmake-build-emscripten/game-dev/fgfx/examples/<name>/
  index.html
  <name>.js
  runtime-js-compiled.js
  assets/
```

Serve the output directory:

```bash
cd cmake-build-emscripten/game-dev/fgfx/examples/msdf_example
python -m http.server 8080
```

## JavaScript runtime bundle

Web examples require `runtime-js-compiled.js` in the fgfx source tree. It is gitignored and must be built before wasm POST_BUILD copy steps:

```bash
cd game-dev/fgfx/js
npm install
npm run build
```

This produces `game-dev/fgfx/runtime-js-compiled.js` from `js/runtime.js`, `TextureManager.js`, etc.

## Deprecated: compile.sh

`compile.sh` in this directory (`cmake . && make`) is **deprecated**. Use root CMake builds as described above.
