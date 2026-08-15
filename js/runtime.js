import { TextureManager } from './TextureManager.js'
import { Animator } from './Animator.js'

const canvas = document.getElementById('fgfx_canvas')
const glAttrs = { alpha: true }
const gl = canvas.getContext('webgl2', glAttrs)
if (!gl) {
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

window.Module = window.Module ?? {}
window.Module.initializeFastGfxRuntime = function () {
  console.log('INIT!')
  GLctx = gl
  animator.addAnimation(animateFgfx)
}
window.Module.initializeFastGfxTextureManager = function () {
  textureManager = new TextureManager(gl, window.fgfxLoader)
}

const pixelRatio = window.devicePixelRatio ?? 1
const animateFgfx = (time, delta) => {
  textureManager.reload()
  GLctx = gl

  const pixelWidth = (canvas.clientWidth * pixelRatio) | 0
  const pixelHeight = (canvas.clientHeight * pixelRatio) | 0
  if (canvas.width !== pixelWidth || canvas.height !== pixelHeight || window.devicePixelRatio !== pixelRatio) {
    canvas.width = pixelWidth
    canvas.height = pixelHeight
    gl.viewport(0, 0, canvas.width, canvas.height)
    console.log('CANVAS RESIZE', canvas.width, canvas.height, pixelRatio)
  }
  window.Module._fgfx_render(time, delta, canvas.width, canvas.height, canvas.clientWidth * 0.264, canvas.clientHeight * 0.264)
}
