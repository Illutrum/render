#include "MSDFFont.hpp"
#include "GlyphBatch.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "config.hpp"

namespace fgfx {

  namespace {
    template <typename Batch>
    void bufferString(MSDFFont& font, Batch* layer, const glm::mat4& mat,
                      const glm::vec4& color, const std::string& text,
                      int from, int count) {
      layer->setPxRange(font.pxRange);
      layer->setAtlasSize(font.scale);
      float x = 0;
      int to = from + count;
      for (int i = from; i < to; i++) {
        char c = text[i];
        auto it = font.characters.find(c);
        if (it != font.characters.end()) {
          SpriteCharacter charStruct = it->second;
          glm::vec2 pos(x + charStruct.size.x * 0.5f + charStruct.offset.x,
                        -charStruct.size.y * 0.5f - charStruct.offset.y);
          auto translate = glm::translate(glm::mat4(1.0), glm::vec3(pos, 0));
          auto scaleMat = glm::scale(glm::mat4(1.0),
                                     glm::vec3(charStruct.size.x * 0.5f,
                                               charStruct.size.y * 0.5f, 1));
          layer->rawBufferSprite(charStruct.sprite, mat * translate * scaleMat, color);
          x += charStruct.advance;
        }
      }
    }
  }

  MSDFFont::MSDFFont(std::string namep) : name(namep) {
    scale = glm::vec2(1, 1);
    pxRange = 4.0f;
    loadPromise = nullptr;
  }

  MSDFFont::~MSDFFont() {
  }

  void MSDFFont::setHeight(float linep, float basep) {
    lineHeight = linep;
    baseLine = basep;
  }

  void MSDFFont::setPxRange(float range) {
    pxRange = range;
  }

  void MSDFFont::setScale(float scaleW, float scaleH) {
    scale = glm::vec2(scaleW, scaleH);
  }

  void MSDFFont::setCharacter(unsigned short character, std::shared_ptr<Sprite> sprite, glm::vec2 size,
                              glm::vec2 offset, float advance) {
    SpriteCharacter characterStruct;
    characterStruct.size = size;
    characterStruct.offset = offset;
    characterStruct.sprite = sprite;
    characterStruct.advance = advance;
    characters[character] = characterStruct;
  }

  void MSDFFont::renderString(GlyphBatch* layer, const glm::mat4& mat, const glm::vec4& color,
                              const std::string& text) {
    bufferString(*this, layer, mat, color, text, 0, (int)text.size());
  }

  void MSDFFont::renderSubString(GlyphBatch* layer, const glm::mat4& mat, const glm::vec4& color,
                                 const std::string& text, int from, int count) {
    bufferString(*this, layer, mat, color, text, from, count);
  }

  void MSDFFont::renderString(const std::shared_ptr<GlyphBatch>& layer, const glm::mat4& mat, const glm::vec4& color,
                              const std::string& text) {
    renderString(layer.get(), mat, color, text);
  }

  void MSDFFont::renderSubString(const std::shared_ptr<GlyphBatch>& layer, const glm::mat4& mat, const glm::vec4& color,
                                 const std::string& text, int from, int count) {
    renderSubString(layer.get(), mat, color, text, from, count);
  }

  glm::vec2 MSDFFont::measureString(const std::string& text) {
    float width = 0;
    for (int i = 0; i < (int)text.size(); i++) {
      char c = text[i];
      auto it = characters.find(c);
      if (it != characters.end()) {
        width += it->second.advance;
      }
    }
    return glm::vec2(width, lineHeight);
  }

  glm::vec2 MSDFFont::measureCharacter(char c) {
    auto it = characters.find(c);
    if (it != characters.end()) {
      return glm::vec2(it->second.advance, lineHeight);
    }
    return glm::vec2(0, lineHeight);
  }

}
