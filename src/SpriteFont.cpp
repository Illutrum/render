//
// Created by Michał Łaszczewski on 04/05/16.
//

#include "SpriteFont.hpp"
#include "SpriteBatch.hpp"
#include "TextInk.hpp"
#include "Utf8.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "config.hpp"

namespace fgfx {

  SpriteFont::SpriteFont(std::string namep) : name(namep) {
    scale = glm::vec2(1,1);
    loadPromise = nullptr;
  }
  SpriteFont::~SpriteFont() {

  }

  void SpriteFont::setHeight(float linep, float basep) {
    lineHeight = linep;
    baseLine = basep;
  }
  void SpriteFont::setCharacter(unsigned short character, std::shared_ptr<Sprite> sprite, glm::vec2 size,
                                glm::vec2 offset, float advance) {
    //emscripten_log(EM_LOG_ERROR,"SETUP %s CHAR: %c %d\n",name.c_str(),character,character);
    SpriteCharacter characterStruct;
    characterStruct.size = size;
    characterStruct.offset = offset;
    characterStruct.sprite = sprite;
    characterStruct.advance = advance;
    characters[character] = characterStruct;
  }
  void SpriteFont::renderString(SpriteBatch* layer, const glm::mat4& mat, const glm::vec4& color,
                                const std::string& text){
    //emscripten_log(EM_LOG_ERROR,"RENDER %s STRING: %s\n",name.c_str(),text.c_str());
    float x = 0;
    int i = 0;
    while (i < static_cast<int>(text.size())) {
      unsigned cp = nextUtf8(text, i);
      if (cp > 0xFFFFu) {
        continue;
      }
      auto it=characters.find(static_cast<unsigned short>(cp));
      if(it!=characters.end()) {
        SpriteCharacter charStruct = it->second;
        glm::vec2 pos(x + charStruct.size.x * 0.5 + charStruct.offset.x,-charStruct.size.y*0.5-charStruct.offset.y);
        auto translate = glm::translate(glm::mat4(1.0),glm::vec3(pos,0));
        auto scale = glm::scale(glm::mat4(1.0),glm::vec3(charStruct.size.x*0.5,charStruct.size.y*0.5,1));
        layer->rawBufferSprite(charStruct.sprite,mat*translate*scale,color);
        x += charStruct.advance;
      }
    }
  }

  void SpriteFont::renderSubString(SpriteBatch* layer, const glm::mat4& mat, const glm::vec4& color,
                                   const std::string& text, int from, int count) {
    //emscripten_log(EM_LOG_ERROR,"RENDER %s STRING: %s\n",name.c_str(),text.c_str());
    float x = 0;
    int i = from;
    int to = from + count;
    if (to > static_cast<int>(text.size())) {
      to = static_cast<int>(text.size());
    }
    while (i < to) {
      unsigned cp = nextUtf8(text, i);
      if (cp > 0xFFFFu) {
        continue;
      }
      auto it=characters.find(static_cast<unsigned short>(cp));
      if(it!=characters.end()) {
        SpriteCharacter charStruct = it->second;
        glm::vec2 pos(x + charStruct.size.x * 0.5 + charStruct.offset.x,
                      -charStruct.size.y * 0.5 - charStruct.offset.y);
        auto translate = glm::translate(glm::mat4(1.0), glm::vec3(pos, 0));
        auto scale = glm::scale(glm::mat4(1.0), glm::vec3(charStruct.size.x * 0.5, charStruct.size.y * 0.5, 1));
        layer->rawBufferSprite(charStruct.sprite, mat*translate*scale, color);
        x += charStruct.advance;
      }
    }
  }

  void SpriteFont::renderString(const std::shared_ptr<SpriteBatch>& layer, const glm::mat4& mat, const glm::vec4& color,
                                const std::string& text) {
    renderString(layer.get(), mat, color, text);
  }
  void SpriteFont::renderSubString(const std::shared_ptr<SpriteBatch>& layer, const glm::mat4& mat, const glm::vec4& color,
                                   const std::string& text, int from, int count) {
    renderSubString(layer.get(), mat, color, text, from, count);
  }

  glm::vec2 SpriteFont::measureString(const std::string& text) {
    float height=0;
    float width=0;
    int i = 0;
    while (i < static_cast<int>(text.size())) {
      unsigned cp = nextUtf8(text, i);
      if (cp > 0xFFFFu) {
        continue;
      }
      auto it=characters.find(static_cast<unsigned short>(cp));
      if(it!=characters.end()) {
        SpriteCharacter charStruct = it->second;
        float h = charStruct.size.y+charStruct.offset.y;
        if(h>height) height = h;
        width+=charStruct.advance;
      }
    }
    return glm::vec2(width,lineHeight);
  }

  glm::vec2 SpriteFont::measureCharacter(char c) {
    auto it = characters.find(c);
    if(it != characters.end()) {
      SpriteCharacter charStruct = it->second;
      return glm::vec2(charStruct.advance, lineHeight);
    }
    return glm::vec2(0, lineHeight);
  }

  TextInk SpriteFont::measureInk(const std::string& text) const {
    return measureInkFromMap(characters, text);
  }

}

