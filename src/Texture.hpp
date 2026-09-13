//
// Created by Michał Łaszczewski on 22/12/16.
//

#ifndef FGFX_TEXTURE_H
#define FGFX_TEXTURE_H

#include <string>
#include "backend/BackendTypes.hpp"
#include <serialization/promise.hpp>

namespace fgfx {

  class Texture : public std::enable_shared_from_this<Texture> {
  public:
    std::string name;
    int texture = -1;
    bool initialized = false;
    bool unloaded = false;
    int width = 0;
    int height = 0;
    PixelFormat format = PixelFormat::RGBA8Unorm;

    Texture();
    Texture(std::string namep);
    Texture(std::string namep, TextureHandle tex);
    ~Texture();

    TextureHandle handle() const {
      return texture >= 0 ? static_cast<TextureHandle>(texture) : kNullTexture;
    }

    void recreate(int widthp, int heightp, PixelFormat formatp);

    std::shared_ptr<serialization::promise<std::shared_ptr<Texture>>> loadPromise;
  };

}

#endif //FGFX_TEXTURE_H
