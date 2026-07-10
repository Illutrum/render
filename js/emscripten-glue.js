// Expose Emscripten's internal GL registry to the external FastGFX runtime.
if (typeof window !== 'undefined') {
  window.ModuleGL = GL
}
