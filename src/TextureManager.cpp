//
// Created by Michał Łaszczewski on 12/12/16.
//
#ifndef EMSCRIPTEN

#include "TextureManager.hpp"
#include "Image.hpp"
#include "BmFontParse.hpp"
#include <fastgfx.hpp>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <string>
#include <sstream>

namespace fgfx {

  TextureManager::TextureManager() {
    maxSize = 512;
    spacing = 2;
  }

  TextureManager::~TextureManager() {

  }

  void TextureManager::loadSprite(std::shared_ptr<Sprite> sprite, std::shared_ptr<Image> image) {
    SpriteInfo info;

    info.image = image;
    info.sprite = sprite;

    bool bigger = false;
    bool added = false;
    while(image->width + spacing * 2 > maxSize || image->height + spacing * 2 > maxSize) {
      maxSize *= 2;
      bigger = true;
    }
    if(!bigger) {
      for (auto & spriteTexture : spriteTextures) {
        if (spriteTexture->tryAddSprite(info)) {
          added = true;
          break;
        }
      }
    }
    if(!added) { // new texture
      auto ntex = std::make_shared<SpritesTexture>(maxSize, spacing);
      spriteTextures.push_back(ntex);
      ntex->tryAddSprite(info);
    }
  }
  static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    [](int c) {return !std::isspace(c);}));
    return s;
  }
  std::istringstream& readAttrib(std::istringstream& in, std::string& name, std::string& value) {
    std::getline(in, name, '=');
    name = ltrim(name);
    in >> value;
    return in;
  }
  template <typename T>
  std::string to_string(T value)
  {
    std::ostringstream os ;
    os << value ;
    return os.str() ;
  }

  static void uploadMSDFFontGlyph(std::shared_ptr<MSDFFont> font, std::shared_ptr<Image> image, GLuint texture,
                                  unsigned short character, int x, int y, int width, int height,
                                  int xoffset, int yoffset, int xadvance) {
    std::string spriteName = "msdf_" + font->name + "_" + to_string(character);
    std::shared_ptr<Sprite> sprite = engine->getEmptySprite(spriteName);
    sprite->setTextureFragment(texture,
                               1.0f * (x + 0.5f) / image->width,
                               1.0f * (y + 0.5f) / image->height,
                               1.0f * (x + width + 0.5f) / image->width,
                               1.0f * (y + height + 0.5f) / image->height,
                               width, height, true);
    font->setCharacter(character, sprite, glm::vec2(width, height), glm::vec2(xoffset, yoffset), xadvance);
  }

  void uploadSpriteFont(std::shared_ptr<SpriteFont> font, std::shared_ptr<Image> image, std::string data) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image->data);
    glGenerateMipmap(GL_TEXTURE_2D);

    std::istringstream in(data);
    std::string line;
    while(std::getline(in,line)) {
      std::istringstream lineIn(line);
      std::string command;
      std::string key;
      std::string value;
      lineIn >> command;
      if(command == "info") {
        /// Not needed
      } else if(command == "common") {
        while(readAttrib(lineIn, key, value)) {
          if(key=="lineHeight") font->lineHeight = atof(value.c_str());
          else if(key=="base") font->baseLine = atof(value.c_str());
          else if(key=="scaleW") font->scale.x = atof(value.c_str());
          else if(key=="scaleH") font->scale.y = atof(value.c_str());
        }
      } else if(command == "info") {
        /// Not needed
      } else if(command == "page") {
        /// Not needed
      } else if(command == "chars") {
        /// Not needed
      } else if(command == "char") {
        unsigned short character;
        int x,y,width,height,xoffset,yoffset,xadvance;
        while(readAttrib(lineIn, key, value)) {
          if(key=="id") character = atoi(value.c_str());
          else if(key=="x") x = atoi(value.c_str());
          else if(key=="y") y = atoi(value.c_str());
          else if(key=="width") width = atoi(value.c_str());
          else if(key=="height") height = atoi(value.c_str());
          else if(key=="xoffset") xoffset = atoi(value.c_str());
          else if(key=="yoffset") yoffset = atoi(value.c_str());
          else if(key=="xadvance") xadvance = atoi(value.c_str());
        }
        std::string spriteName = "font_"+font->name+"_"+to_string(character);
        std::shared_ptr<Sprite> sprite = engine->getEmptySprite(spriteName);
        sprite->setTextureFragment(texture,
                                   1.0f * (x + 0.5f) / image->width,
                                   1.0f * (y + 0.5f) / image->height,
                                   1.0f * (x + width + 0.5f) / image->width,
                                   1.0f * (y + height + 0.5f) / image->height,
                                   width, height, true);
        font->setCharacter(character, sprite, glm::vec2(width, height), glm::vec2(xoffset, yoffset), xadvance);
      }
    }
  }


  std::string loadBuffer(std::string path);

  void TextureManager::loadSpriteFont(std::shared_ptr<SpriteFont> spriteFont, std::shared_ptr<Image> image, std::string& data) {
    if(spriteFont->name.size() == 0 ) throw ImageNotFoundError();
    fgfx::engine->uploadQueue.enqueue([=]{
      uploadSpriteFont(spriteFont, image, data);
    });
  }

  void uploadMSDFFont(std::shared_ptr<MSDFFont> font, std::shared_ptr<Image> image, std::string data) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->data);

    float pxRange = 4.0f;
    const bool isXml = data.find("<font") != std::string::npos || data.find("<?xml") != std::string::npos;

    if (isXml) {
      size_t dfPos = data.find("<distanceField");
      if (dfPos != std::string::npos) {
        size_t dfEnd = data.find("/>", dfPos);
        if (dfEnd != std::string::npos) {
          std::string dfTag = data.substr(dfPos, dfEnd - dfPos + 2);
          float range = parseXmlAttr(dfTag, "distanceRange");
          if (range > 0) pxRange = range;
        }
      }
      size_t commonPos = data.find("<common");
      if (commonPos != std::string::npos) {
        size_t commonEnd = data.find("/>", commonPos);
        if (commonEnd != std::string::npos) {
          std::string commonTag = data.substr(commonPos, commonEnd - commonPos + 2);
          font->lineHeight = parseXmlAttr(commonTag, "lineHeight");
          font->baseLine = parseXmlAttr(commonTag, "base");
          font->scale.x = parseXmlAttr(commonTag, "scaleW");
          font->scale.y = parseXmlAttr(commonTag, "scaleH");
        }
      }
      size_t pos = 0;
      while ((pos = data.find("<char ", pos)) != std::string::npos) {
        size_t end = data.find("/>", pos);
        if (end == std::string::npos) break;
        std::string charTag = data.substr(pos, end - pos + 2);
        uploadMSDFFontGlyph(font, image, texture,
                            static_cast<unsigned short>(parseXmlAttrInt(charTag, "id")),
                            parseXmlAttrInt(charTag, "x"),
                            parseXmlAttrInt(charTag, "y"),
                            parseXmlAttrInt(charTag, "width"),
                            parseXmlAttrInt(charTag, "height"),
                            parseXmlAttrInt(charTag, "xoffset"),
                            parseXmlAttrInt(charTag, "yoffset"),
                            parseXmlAttrInt(charTag, "xadvance"));
        pos = end + 2;
      }
    } else {
      std::istringstream in(data);
      std::string line;
      while (std::getline(in, line)) {
        std::istringstream lineIn(line);
        std::string command;
        std::string key;
        std::string value;
        lineIn >> command;
        if (command == "common") {
          while (readAttrib(lineIn, key, value)) {
            if (key == "lineHeight") font->lineHeight = atof(value.c_str());
            else if (key == "base") font->baseLine = atof(value.c_str());
            else if (key == "scaleW") font->scale.x = atof(value.c_str());
            else if (key == "scaleH") font->scale.y = atof(value.c_str());
          }
        } else if (command == "char") {
          unsigned short character;
          int x, y, width, height, xoffset, yoffset, xadvance;
          while (readAttrib(lineIn, key, value)) {
            if (key == "id") character = atoi(value.c_str());
            else if (key == "x") x = atoi(value.c_str());
            else if (key == "y") y = atoi(value.c_str());
            else if (key == "width") width = atoi(value.c_str());
            else if (key == "height") height = atoi(value.c_str());
            else if (key == "xoffset") xoffset = atoi(value.c_str());
            else if (key == "yoffset") yoffset = atoi(value.c_str());
            else if (key == "xadvance") xadvance = atoi(value.c_str());
          }
          uploadMSDFFontGlyph(font, image, texture, character, x, y, width, height, xoffset, yoffset, xadvance);
        }
      }
    }
    font->setPxRange(pxRange);
  }

  void TextureManager::loadMSDFFont(std::shared_ptr<MSDFFont> msdfFont, std::shared_ptr<Image> image, std::string& data) {
    if (msdfFont->name.size() == 0) throw ImageNotFoundError();
    fgfx::engine->uploadQueue.enqueue([=] {
      uploadMSDFFont(msdfFont, image, data);
    });
  }

  void TextureManager::loadTexture(std::shared_ptr<Texture> texture, std::shared_ptr<Image> image) {
    if(texture->name.size() == 0 ) throw ImageNotFoundError();
    fgfx::engine->uploadQueue.enqueue([=]{
      texture->upload(image->width, image->height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, image->data);
    });
  }

  void TextureManager::clean() {
    for (auto & spriteTexture : spriteTextures) {
      spriteTexture->clean();
    }
  }

}

#endif

