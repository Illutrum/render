import { TextureManager } from './TextureManager.js'
import { Animator } from './Animator.js'

const canvas = document.getElementById('fgfx_canvas')
if (canvas) {
  canvas.tabIndex = 0
}
const useWebGPU = !!window.FGFX_BACKEND_WEBGPU
const glAttrs = { alpha: true }
const gl = useWebGPU ? null : canvas.getContext('webgl2', glAttrs)
if (!useWebGPU && !gl) {
  throw new Error('WebGL2 is required')
}

const animator = new Animator(false)
let textureManager

const loadImage = (url) =>
  new Promise((resolve, reject) => {
    const image = new Image()
    image.crossOrigin = 'anonymous'
    image.src = url
    if (image.complete) return resolve(image)
    image.onload = () => resolve(image)
    image.onerror = () => reject(new Error(`Could not load image ${url}`))
  })

const loadText = (url) =>
  fetch(url)
    .then((r) => (r.ok ? r.text() : Promise.reject(new Error(`HTTP ${r.status}: ${r.statusText}`))))

const loadBinary = (url) =>
  fetch(url)
    .then((r) => (r.ok ? r.arrayBuffer() : Promise.reject(new Error(`HTTP ${r.status}: ${r.statusText}`))))
    .then((buf) => new Uint8Array(buf))

window.fgfxLoader = window.fgfxLoader ?? {
  loadSpriteImage: loadImage,
  loadTextureImage: loadImage,
  loadFontData: (name) => loadText(`${name}.fnt`),
  loadFontImage: (name) => loadImage(`${name}.png`),
  loadMSDFFontData: (name) => loadText(`${name}.fnt`),
  loadMSDFFontImage: (name) => loadImage(`${name}.png`),
  loadText: (name) => loadText(name),
  loadBinary: (name) => loadBinary(name)
}

let glContextHandle = 0

function bindFgfxGlContext() {
  if (glContextHandle) {
    ModuleGL.makeContextCurrent(glContextHandle)
    return
  }
  GLctx = gl
}

window.Module = window.Module ?? {}
window.Module.initializeFastGfxRuntime = function () {
  console.log('INIT!', useWebGPU ? 'WebGPU' : 'WebGL2')
  if (!useWebGPU) {
    if (!glContextHandle) {
      glContextHandle = ModuleGL.registerContext(gl, {
        majorVersion: 2,
        minorVersion: 0,
        alpha: true
      })
    }
    if (!glContextHandle) {
      throw new Error('Failed to register WebGL2 context with Emscripten')
    }
    bindFgfxGlContext()
  }
}

window.Module.initializeFastGfxTextureManager = function () {
  textureManager = new TextureManager(gl, window.fgfxLoader)
  animator.addAnimation(animateFgfx)
}

const pixelRatio = window.devicePixelRatio ?? 1
const animateFgfx = (time, delta) => {
  if (!textureManager || !window.Module._fgfx_render) return
  textureManager.reload()
  if (!useWebGPU) bindFgfxGlContext()

  const pixelWidth = (canvas.clientWidth * pixelRatio) | 0
  const pixelHeight = (canvas.clientHeight * pixelRatio) | 0
  if (canvas.width !== pixelWidth || canvas.height !== pixelHeight || window.devicePixelRatio !== pixelRatio) {
    canvas.width = pixelWidth
    canvas.height = pixelHeight
    if (gl) gl.viewport(0, 0, canvas.width, canvas.height)
    console.log('CANVAS RESIZE', canvas.width, canvas.height, pixelRatio)
  }
  window.Module._fgfx_render(time, delta, canvas.width, canvas.height, canvas.clientWidth * 0.264, canvas.clientHeight * 0.264)
}

function glfwKeyFromEvent(e) {
  if (e.code === 'ArrowRight') return 262
  if (e.code === 'ArrowLeft') return 263
  if (e.code === 'ArrowDown') return 264
  if (e.code === 'ArrowUp') return 265
  if (e.code === 'Escape') return 256
  if (e.key && e.key.length === 1) {
    return e.key.toUpperCase().charCodeAt(0)
  }
  return -1
}

window.addEventListener('keydown', (e) => {
  if (e.repeat) return
  const fn = window.Module && window.Module._fgfx_key_event
  if (!fn) return
  const key = glfwKeyFromEvent(e)
  if (key < 0) return
  fn(key, 1)
})

window.addEventListener('keyup', (e) => {
  const fn = window.Module && window.Module._fgfx_key_event
  if (!fn) return
  const key = glfwKeyFromEvent(e)
  if (key < 0) return
  fn(key, 0)
})

window.addEventListener('mousemove', (e) => {
  const fn = window.Module && window.Module._fgfx_mouse_move
  if (!fn || !canvas) return
  const rect = canvas.getBoundingClientRect()
  fn(e.clientX - rect.left, e.clientY - rect.top)
})

window.addEventListener('mousedown', (e) => {
  const fn = window.Module && window.Module._fgfx_mouse_button
  if (!fn) return
  fn(e.button, 1)
})

window.addEventListener('mouseup', (e) => {
  const fn = window.Module && window.Module._fgfx_mouse_button
  if (!fn) return
  fn(e.button, 0)
})

