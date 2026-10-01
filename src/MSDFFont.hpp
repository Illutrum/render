#ifndef FGFX_MSDFFONT_H
#define FGFX_MSDFFONT_H

#include "SpriteFont.hpp"
#include "TextInk.hpp"
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <memory>
#include <serialization/promise.hpp>

namespace fgfx {

  class GlyphBatch;

  class MSDFFont : public std::enable_shared_from_this<MSDFFont> {
  public:
    std::string name;
    std::map<unsigned short, SpriteCharacter> characters;

    float lineHeight;
    float baseLine;
    glm::vec2 scale;
    float pxRange;

    MSDFFont(std::string namep);
    ~MSDFFont();

    void setHeight(float linep, float basep);
    void setPxRange(float range);
    void setScale(float scaleW, float scaleH);
    void setCharacter(unsigned short character, std::shared_ptr<Sprite> sprite, glm::vec2 size, glm::vec2 offset, float advance);

    void renderString(GlyphBatch* layer, const glm::mat4& mat, const glm::vec4& color, const std::string& text);
    void renderSubString(GlyphBatch* layer, const glm::mat4& mat, const glm::vec4& color,
                         const std::string& text, int from, int count);
    void renderString(const std::shared_ptr<GlyphBatch>& layer, const glm::mat4& mat, const glm::vec4& color,
                      const std::string& text);
    void renderSubString(const std::shared_ptr<GlyphBatch>& layer, const glm::mat4& mat, const glm::vec4& color,
                         const std::string& text, int from, int count);

    glm::vec2 measureString(const std::string& text);
    glm::vec2 measureCharacter(char c);
    TextInk measureInk(const std::string& text) const;

    std::shared_ptr<serialization::promise<std::shared_ptr<MSDFFont>>> loadPromise;
  };

}

#endif
