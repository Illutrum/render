// Expose Emscripten's internal GL registry to the external FastGFX runtime.
// On the WebGPU wasm build there is no GL library, so this is a no-op.
if (typeof window !== 'undefined' && typeof GL !== 'undefined') {
  window.ModuleGL = GL
}
