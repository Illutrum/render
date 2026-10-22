//
// Created by Michał Łaszczewski on 04/05/16.
//

#ifndef ENGINE_SPRITEFONT_H
#define ENGINE_SPRITEFONT_H

#include <glm/glm.hpp>
#include "Sprite.hpp"
#include <map>
#include <memory>
#include <string>
#include <serialization/promise.hpp>

namespace render {

  class SpriteBatch;

  class SpriteCharacter {
  public:
    std::shared_ptr<Sprite> sprite;
    glm::vec2 size;
    glm::vec2 offset;
    float advance;
  };

  // Visual ink AABB in the same font space as renderString (origin = pen,
  // +X right, +Y up). Empty when the string has no drawable glyphs.
  struct TextInk {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float advance = 0.0f;
    bool empty = true;

    float cx() const {
      return (minX + maxX) * 0.5f;
    }
    float cy() const {
      return (minY + maxY) * 0.5f;
    }
    float width() const {
      return maxX - minX;
    }
    float height() const {
      return maxY - minY;
    }
  };

  enum class TextAlignH {
    Left,
    Center,
    Right
  };

  class SpriteFont : public std::enable_shared_from_this<SpriteFont> {
  public:
    std::string name;
    std::map<unsigned short, SpriteCharacter> characters;

    float lineHeight;
    float baseLine;
    glm::vec2 scale;

    SpriteFont(std::string namep);
    ~SpriteFont();

    void setHeight(float linep, float basep);
    void setCharacter(unsigned short character, std::shared_ptr<Sprite> sprite, glm::vec2 size, glm::vec2 offset,
                      float advance);

    void renderString(const std::shared_ptr<SpriteBatch>& layer, const glm::mat4& mat, const glm::vec4& color,
                      const std::string& text);
    void renderSubString(const std::shared_ptr<SpriteBatch>& layer, const glm::mat4& mat, const glm::vec4& color,
                         const std::string& text, int from, int count);

    void renderString(SpriteBatch* layer, const glm::mat4& mat, const glm::vec4& color,
                      const std::string& text);
    void renderSubString(SpriteBatch* layer, const glm::mat4& mat, const glm::vec4& color,
                         const std::string& text, int from, int count);

    glm::vec2 measureString(const std::string& text);
    glm::vec2 measureCharacter(char c);
    TextInk measureInk(const std::string& text) const;

    std::shared_ptr<serialization::promise<std::shared_ptr<SpriteFont>>> loadPromise;
  };

}

#endif //ENGINE_SPRITEFONT_H
