# Building

```bash
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Stock C++20 is enough for this library. The tests fetch doctest. `glm` is a submodule. `daemonarium/rpc-cpp` `v0.1.0` is fetched for `serialization/promise.hpp`.

Native dependencies: OpenGL, GLFW, GLEW, libpng, zlib.

Examples (`logo`, `logo_blur`, `lines`, `msdf_example`) are built next to the test binary. They open a window. `ctest` does not run them.

WebGPU, after `git submodule update --init`:

```bash
cmake -B build-webgpu -DFGFX_BACKEND=WebGPU
cmake --build build-webgpu
```

That links the Dawn backend from the `webgpu` submodule.

## JavaScript loader

The browser host loads `runtime-js-compiled.js`. Rebuild it from `js/` when the loader changes:

```bash
cd js
npm ci
npm run build
```

That writes `runtime-js-compiled.js` at the repository root. This package is not published to npm. This repository does not link the wasm executable. The host does.
