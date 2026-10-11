/** Register a JS-created WebGLTexture in Emscripten's GL.textures via getNewId.
 *  textures.length is wrong: C++ glGenTextures uses the shared GL.counter, so a
 *  length-based id is overwritten by the next gen and MSDF/sprites go missing. */
export function registerEmscriptenTexture(webglTexture) {
  const table = window.ModuleGL
  if (!table || typeof table.getNewId !== 'function') {
    throw new Error('ModuleGL.getNewId is required to register WebGL textures')
  }
  const textureId = table.getNewId(table.textures)
  webglTexture.name = textureId
  table.textures[textureId] = webglTexture
  return textureId
}

export function applyRawUnpack(glCtx) {
  glCtx.pixelStorei(glCtx.UNPACK_COLORSPACE_CONVERSION_WEBGL, glCtx.NONE)
  glCtx.pixelStorei(glCtx.UNPACK_FLIP_Y_WEBGL, 0)
  glCtx.pixelStorei(glCtx.UNPACK_PREMULTIPLY_ALPHA_WEBGL, 0)
}

function STNode(x, y, width, height) {
  this.left = null
  this.right = null
  this.image = null
  this.x = x
  this.y = y
  this.width = width
  this.height = height
}

STNode.prototype.insert = function (image, spacing) {
  const boxWidth = image.width + spacing
  const boxHeight = image.height + spacing

  if (this.left && this.right) {
    return this.left.insert(image, spacing) ?? this.right.insert(image, spacing)
  }
  if (this.image) return null
  if (boxWidth > this.width || boxHeight > this.height) return null
  if (boxWidth === this.width && boxHeight === this.height) {
    this.image = image
    return this
  }
  const dw = this.width - boxWidth
  const dh = this.height - boxHeight
  if (dw > dh) {
    this.left = new STNode(this.x, this.y, boxWidth, this.height)
    this.right = new STNode(this.x + boxWidth, this.y, this.width - boxWidth, this.height)
  } else {
    this.left = new STNode(this.x, this.y, this.width, boxHeight)
    this.right = new STNode(this.x, this.y + boxHeight, this.width, this.height - boxHeight)
  }
  return this.left.insert(image, spacing)
}

function SpritesTexture(gl, size, spacing) {
  this.gl = gl
  this.size = size
  this.canvas = document.createElement('canvas')
  this.canvas.width = this.size
  this.canvas.height = this.size
  this.ctx = this.canvas.getContext('2d')
  this.sprites = []
  this.spritesByName = {}
  this.spacing = spacing
  this.needUpload = false

  const glCtx = this.gl
  this.texture = glCtx.createTexture()
  glCtx.bindTexture(glCtx.TEXTURE_2D, this.texture)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MAG_FILTER, glCtx.LINEAR)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MIN_FILTER, glCtx.LINEAR_MIPMAP_LINEAR)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_WRAP_S, glCtx.CLAMP_TO_EDGE)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_WRAP_T, glCtx.CLAMP_TO_EDGE)
  this.textureId = registerEmscriptenTexture(this.texture)
}

SpritesTexture.prototype.tryPack = function (sprites) {
  const nodes = new Array(sprites.length)
  const root = new STNode(0, 0, this.size, this.size)

  sprites = [...sprites].sort((a, b) => Math.max(b.width, b.height) - Math.max(a.width, a.height))

  for (let i = 0; i < sprites.length; i++) {
    const sprite = sprites[i]
    const node = root.insert(sprite.image, this.spacing * 2)
    if (!node) return false
    nodes[i] = node
  }

  const fix = -2
  for (let i = 0; i < sprites.length; i++) {
    const node = nodes[i]
    const sprite = sprites[i]
    sprite.coords = {
      width: node.width,
      height: node.height,
      x: node.x,
      y: node.y,
      xmin: (node.x + fix) / this.size,
      ymin: (node.y + fix) / this.size,
      xmax: (node.x + node.width - this.spacing + fix) / this.size,
      ymax: (node.y + node.height - this.spacing + fix) / this.size
    }
  }
  this.sprites = sprites
  this.needUpload = true
  return true
}

SpritesTexture.prototype.tryAddSprite = function (sprite) {
  return this.tryPack(this.sprites.concat([sprite]))
}

SpritesTexture.prototype.isBigEnough = function (w, h) {
  return w < this.size && h < this.size
}

SpritesTexture.prototype.upload = function () {
  if (!this.needUpload) return

  const { ctx, canvas, gl } = this
  ctx.clearRect(0, 0, canvas.width, canvas.height)

  for (let i = 0; i < this.sprites.length; i++) {
    const sprite = this.sprites[i]
    ctx.drawImage(sprite.image, sprite.coords.x, sprite.coords.y)
  }

  gl.bindTexture(gl.TEXTURE_2D, this.texture)
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, canvas)
  gl.generateMipmap(gl.TEXTURE_2D)

  for (let i = 0; i < this.sprites.length; i++) {
    const sprite = this.sprites[i]
    sprite.texture = this.textureId
    Module._fgfx_Sprite_setTextureFragment(
      sprite.sprite,
      sprite.texture,
      sprite.coords.xmin,
      sprite.coords.ymin,
      sprite.coords.xmax,
      sprite.coords.ymax,
      sprite.coords.width,
      sprite.coords.height,
      false
    )
  }
  this.needUpload = false
}

SpritesTexture.prototype.clean = function (engineTime) {
  for (let i = 0; i < this.sprites.length; i++) {
    const sprite = this.sprites[i].sprite
    if (sprite && engineTime - sprite.lastUseTime > 30) {
      console.log('REMOVE SPRITE', this.sprites[i].name)
      sprite.unload()
      this.sprites.splice(i, 1)
      i--
    }
  }
}

export { SpritesTexture }
export default SpritesTexture
