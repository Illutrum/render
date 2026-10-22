//
// Created by Michał Łaszczewski on 22/12/16.
//

#include "Texture.hpp"
#include <render.hpp>
#include "backend/RenderBackend.hpp"

namespace render {

  Texture::Texture(std::string namep) : name(namep) {
    texture = -1;
    unloaded = false;
    initialized = false;
    loadPromise = nullptr;
  }

  Texture::Texture(std::string namep, TextureHandle tex) : name(namep) {
    texture = static_cast<int>(tex);
    unloaded = false;
    initialized = tex != kNullTexture;
    loadPromise = nullptr;
  }

  Texture::Texture() {
    texture = -1;
    unloaded = false;
    initialized = false;
    loadPromise = nullptr;
  }

  void Texture::recreate(int widthp, int heightp, PixelFormat formatp) {
    if (!engine || !engine->getBackend()) {
      return;
    }
    if (texture >= 0) {
      engine->getBackend()->destroyTexture(handle());
      texture = -1;
    }
    TextureDesc desc;
    desc.width = widthp;
    desc.height = heightp;
    desc.format = formatp;
    desc.generateMipmaps = false;
    texture = static_cast<int>(engine->getBackend()->createTexture(desc));
    width = widthp;
    height = heightp;
    format = formatp;
    initialized = texture > 0;
  }

  Texture::~Texture() {
    if (texture >= 0 && engine && engine->getBackend()) {
      engine->getBackend()->destroyTexture(handle());
      texture = -1;
    }
  }

}
