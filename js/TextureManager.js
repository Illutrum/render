import parseBmFont from 'parse-bmfont-ascii'

function parseBmFontXml(fntData) {
  const doc = new DOMParser().parseFromString(fntData, 'application/xml')
  const commonEl = doc.querySelector('common')
  const common = {
    lineHeight: parseFloat(commonEl.getAttribute('lineHeight')),
    base: parseFloat(commonEl.getAttribute('base')),
    scaleW: parseFloat(commonEl.getAttribute('scaleW')),
    scaleH: parseFloat(commonEl.getAttribute('scaleH'))
  }
  const chars = [...doc.querySelectorAll('char')].map((el) => ({
    id: parseInt(el.getAttribute('id'), 10),
    x: parseInt(el.getAttribute('x'), 10),
    y: parseInt(el.getAttribute('y'), 10),
    width: parseInt(el.getAttribute('width'), 10),
    height: parseInt(el.getAttribute('height'), 10),
    xoffset: parseInt(el.getAttribute('xoffset'), 10),
    yoffset: parseInt(el.getAttribute('yoffset'), 10),
    xadvance: parseInt(el.getAttribute('xadvance'), 10)
  }))
  const df = doc.querySelector('distanceField')
  const distanceRange = df ? parseFloat(df.getAttribute('distanceRange')) : 4
  return { common, chars, distanceRange }
}

function parseBmFontData(fntData) {
  const trimmed = fntData.trimStart()
  if (trimmed.startsWith('<')) return parseBmFontXml(fntData)
  return parseBmFont(fntData)
}

import { SpritesTexture } from './SpritesTexture.js'

function TextureManager(gl, loader) {
  this.gl = gl
  this.loader = loader
  this.loadingSprites = new Map()
  this.loadedSprites = new Map()
  this.spriteTextures = []
  this.maxSize = 512
  this.spacing = 2
}

TextureManager.prototype.loadSprite = function (sprite) {
  const name = Module.UTF8ToString(Module._fgfx_Sprite_getName(sprite))
  console.debug('SPRITE REQUESTED', name, sprite)
  const loaded = this.loadedSprites.get(name)
  if (loaded) {
    console.debug('SPRITE LOAD AND PUSH', name)
    Module._fgfx_Sprite_setTextureFragment(
      loaded.sprite,
      loaded.texture,
      loaded.coords.xmin,
      loaded.coords.ymin,
      loaded.coords.xmax,
      loaded.coords.ymax,
      loaded.coords.width,
      loaded.coords.height,
      loaded.preloaded ?? false
    )
    return
  }
  let loading = this.loadingSprites.get(name)
  if (loading) {
    loading.sprite = sprite
    return
  }
  const spriteEntry = {
    coords: null,
    sprite,
    texture: 0,
    preloaded: false,
    name,
    image: null
  }
  this.loadingSprites.set(name, spriteEntry)
  this.loader.loadSpriteImage(name).then((image) => {
    spriteEntry.image = image
    let bigger = false
    let added = false
    while (image.width + this.spacing * 2 > this.maxSize || image.height + this.spacing * 2 > this.maxSize) {
      this.maxSize *= 2
      bigger = true
    }
    if (!bigger) {
      for (let i = 0; i < this.spriteTextures.length; i++) {
        if (this.spriteTextures[i].tryAddSprite(spriteEntry)) {
          added = true
          break
        }
      }
    }
    if (!added) {
      const ntex = new SpritesTexture(this.gl, this.maxSize, this.spacing)
      this.spriteTextures.push(ntex)
      ntex.tryAddSprite(spriteEntry)
    }
  })
}

TextureManager.prototype.loadSpriteFont = function (font) {
  const name = Module.UTF8ToString(Module._fgfx_SpriteFont_getName(font))
  console.info('LOADING FONT', name)
  const imagePromise = this.loader.loadFontImage(name)
  const dataPromise = this.loader.loadFontData(name)
  imagePromise.then((image) =>
    dataPromise.then((data) => this.uploadPackedSpriteFont(font, data, image, name))
  )
}

TextureManager.prototype.loadMSDFFont = function (font) {
  const name = Module.UTF8ToString(Module._fgfx_MSDFFont_getName(font))
  console.info('LOADING MSDF FONT', name)
  const imagePromise = this.loader.loadMSDFFontImage(name)
  const dataPromise = this.loader.loadMSDFFontData(name)
  imagePromise.then((image) =>
    dataPromise.then((data) => this.uploadPackedMSDFFont(font, data, image, name))
  )
}

TextureManager.prototype.loadTexture = function (texture) {
  const name = Module.UTF8ToString(Module._fgfx_Texture_getName(texture))
  const imagePromise = this.loader.loadTextureImage(name)

  imagePromise.then((image) => {
    let textureId = Module._fgfx_Texture_getTexture(texture)
    console.error('LOADED TEX IMAGE', name, textureId)
    let tex
    if (textureId === -1) {
      tex = this.gl.createTexture()
      while (ModuleGL.textures.length === 0) ModuleGL.textures.push(null)
      textureId = ModuleGL.textures.length
      ModuleGL.textures[textureId] = tex
    } else {
      tex = ModuleGL.textures[textureId]
    }
    const glCtx = this.gl
    glCtx.bindTexture(glCtx.TEXTURE_2D, tex)
    glCtx.texImage2D(glCtx.TEXTURE_2D, 0, glCtx.RGBA, glCtx.RGBA, glCtx.UNSIGNED_BYTE, image)
    glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MAG_FILTER, glCtx.LINEAR)
    glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MIN_FILTER, glCtx.LINEAR_MIPMAP_LINEAR)
    glCtx.generateMipmap(glCtx.TEXTURE_2D)
    Module._fgfx_Texture_setTexture(texture, textureId)
  })
}

TextureManager.prototype.reload = function () {
  let cnt = Module._fgfx_getSpritesToLoadCount()
  for (let i = 0; i < cnt; i++) {
    const sprite = Module._fgfx_getSpriteToLoad(i)
    this.loadSprite(sprite)
  }
  Module._fgfx_clearSpritesToLoad()

  cnt = Module._fgfx_getSpriteFontsToLoadCount()
  for (let i = 0; i < cnt; i++) {
    const font = Module._fgfx_getSpriteFontToLoad(i)
    this.loadSpriteFont(font)
  }
  Module._fgfx_clearSpriteFontsToLoad()

  cnt = Module._fgfx_getMSDFFontsToLoadCount()
  for (let i = 0; i < cnt; i++) {
    const font = Module._fgfx_getMSDFFontToLoad(i)
    this.loadMSDFFont(font)
  }
  Module._fgfx_clearMSDFFontsToLoad()

  for (let i = 0; i < this.spriteTextures.length; i++) {
    this.spriteTextures[i].upload()
  }

  cnt = Module._fgfx_getTexturesToLoadCount()
  for (let i = 0; i < cnt; i++) {
    const texture = Module._fgfx_getTextureToLoad(i)
    this.loadTexture(texture)
  }
  Module._fgfx_clearTexturesToLoad()
}

TextureManager.prototype.clean = function (engineTime) {
  for (let i = 0; i < this.spriteTextures.length; i++) {
    this.spriteTextures[i].clean(engineTime)
  }
}

TextureManager.prototype.uploadPackedSpriteFont = function (spriteFont, fntData, image, fontName) {
  const data = parseBmFontData(fntData)
  const name = fontName ?? Module.UTF8ToString(Module._fgfx_SpriteFont_getName(spriteFont))
  const w = image.width
  const h = image.height
  const glCtx = this.gl
  const texture = glCtx.createTexture()
  glCtx.bindTexture(glCtx.TEXTURE_2D, texture)
  glCtx.texImage2D(glCtx.TEXTURE_2D, 0, glCtx.RGBA, glCtx.RGBA, glCtx.UNSIGNED_BYTE, image)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MAG_FILTER, glCtx.LINEAR)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MIN_FILTER, glCtx.LINEAR_MIPMAP_LINEAR)
  glCtx.generateMipmap(glCtx.TEXTURE_2D)

  while (ModuleGL.textures.length === 0) ModuleGL.textures.push(null)
  const textureId = ModuleGL.textures.length
  ModuleGL.textures.push(texture)

  for (let i = 0, l = data.chars.length; i < l; i++) {
    const char = data.chars[i]
    const c = String.fromCharCode(char.id)
    const spriteName = 'font_' + name + '_' + c
    const cppSpriteName = Module.allocate(Module.intArrayFromString(spriteName), 0)
    const sprite = Module._fgfx_getEmptySprite(cppSpriteName)
    Module._free(cppSpriteName)
    Module._fgfx_Sprite_setTextureFragment(
      sprite,
      textureId,
      char.x / w,
      char.y / h,
      (char.x + char.width) / w,
      (char.y + char.height) / h,
      char.width,
      char.height,
      true
    )
    Module._fgfx_SpriteFont_setCharacter(
      spriteFont,
      char.id,
      sprite,
      char.width,
      char.height,
      char.xoffset,
      char.yoffset,
      char.xadvance
    )
  }
  Module._fgfx_SpriteFont_setHeight(spriteFont, data.common.lineHeight, data.common.base)
}

TextureManager.prototype.uploadPackedMSDFFont = function (msdfFont, fntData, image, fontName) {
  const data = parseBmFontData(fntData)
  const name = fontName ?? Module.UTF8ToString(Module._fgfx_MSDFFont_getName(msdfFont))
  const w = image.width
  const h = image.height
  const glCtx = this.gl
  const texture = glCtx.createTexture()
  glCtx.bindTexture(glCtx.TEXTURE_2D, texture)
  glCtx.texImage2D(glCtx.TEXTURE_2D, 0, glCtx.RGBA, glCtx.RGBA, glCtx.UNSIGNED_BYTE, image)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MAG_FILTER, glCtx.LINEAR)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MIN_FILTER, glCtx.LINEAR)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_WRAP_S, glCtx.CLAMP_TO_EDGE)
  glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_WRAP_T, glCtx.CLAMP_TO_EDGE)

  while (ModuleGL.textures.length === 0) ModuleGL.textures.push(null)
  const textureId = ModuleGL.textures.length
  ModuleGL.textures.push(texture)

  for (let i = 0, l = data.chars.length; i < l; i++) {
    const char = data.chars[i]
    const c = String.fromCharCode(char.id)
    const spriteName = 'msdf_' + name + '_' + c
    const cppSpriteName = Module.allocate(Module.intArrayFromString(spriteName), 0)
    const sprite = Module._fgfx_getEmptySprite(cppSpriteName)
    Module._free(cppSpriteName)
    Module._fgfx_Sprite_setTextureFragment(
      sprite,
      textureId,
      char.x / w,
      char.y / h,
      (char.x + char.width) / w,
      (char.y + char.height) / h,
      char.width,
      char.height,
      true
    )
    Module._fgfx_MSDFFont_setCharacter(
      msdfFont,
      char.id,
      sprite,
      char.width,
      char.height,
      char.xoffset,
      char.yoffset,
      char.xadvance
    )
  }
  Module._fgfx_MSDFFont_setHeight(msdfFont, data.common.lineHeight, data.common.base)
  Module._fgfx_MSDFFont_setPxRange(msdfFont, data.distanceRange ?? 4.0)
  Module._fgfx_MSDFFont_setScale(
    msdfFont,
    data.common.scaleW || w,
    data.common.scaleH || h
  )
  Module._fgfx_MSDFFont_finishLoad(msdfFont)
}

TextureManager.prototype.loadPackedSprites = function (sprites, imageUrl) {
  const preloadImageTask = preload.preloadImageTask(imageUrl)
  console.debug('PRELOAD PACKED SPRITES', sprites)
  for (const k in sprites) {
    const coords = sprites[k]
    this.loadingSprites.set(k, {
      name: k,
      coords,
      sprite: null,
      texture: 0,
      preloaded: true
    })
  }
  preloadImageTask.start()
  preloadImageTask.resultPromise.then(
    (function (image) {
      const glCtx = this.gl
      const texture = glCtx.createTexture()
      glCtx.bindTexture(glCtx.TEXTURE_2D, texture)
      glCtx.texImage2D(glCtx.TEXTURE_2D, 0, glCtx.RGBA, glCtx.RGBA, glCtx.UNSIGNED_BYTE, image)
      glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MAG_FILTER, glCtx.LINEAR)
      glCtx.texParameteri(glCtx.TEXTURE_2D, glCtx.TEXTURE_MIN_FILTER, glCtx.LINEAR_MIPMAP_LINEAR)
      glCtx.generateMipmap(glCtx.TEXTURE_2D)

      while (ModuleGL.textures.length === 0) ModuleGL.textures.push(null)
      const textureId = ModuleGL.textures.length
      ModuleGL.textures.push(texture)
      console.debug('LOADED SPRITES TEXTURE', textureId)
      for (const k in sprites) {
        console.debug('SPRITE LOADED', k)
        const loaded = this.loadingSprites.get(k)
        loaded.texture = textureId
        this.loadingSprites.delete(k)
        this.loadedSprites.set(k, loaded)
        if (loaded.sprite) {
          console.debug('SPRITE PUSH', k)
          Module._api_fgfx_Sprite_setTextureFragment(
            loaded.sprite,
            loaded.texture,
            loaded.coords.xmin,
            loaded.coords.ymin,
            loaded.coords.xmax,
            loaded.coords.ymax,
            loaded.coords.width,
            loaded.coords.height
          )
          loaded.sprite.preloaded = loaded.preloaded ?? false
        }
      }
    }.bind(this))
  )
}

export { TextureManager }
export default TextureManager
