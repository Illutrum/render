#ifndef FGFX_TEXT_INK_HPP
#define FGFX_TEXT_INK_HPP

#include "SpriteFont.hpp"
#include "Utf8.hpp"

#include <algorithm>
#include <map>
#include <string>

#include <glm/gtc/matrix_transform.hpp>

namespace render {

inline void accumulateInk(TextInk& ink, float x, const SpriteCharacter& ch) {
  if (ch.size.x <= 0.0f || ch.size.y <= 0.0f) {
    return;
  }
  float left = x + ch.offset.x;
  float right = left + ch.size.x;
  float top = -ch.offset.y;
  float bottom = top - ch.size.y;
  if (ink.empty) {
    ink.minX = left;
    ink.maxX = right;
    ink.minY = bottom;
    ink.maxY = top;
    ink.empty = false;
    return;
  }
  ink.minX = std::min(ink.minX, left);
  ink.maxX = std::max(ink.maxX, right);
  ink.minY = std::min(ink.minY, bottom);
  ink.maxY = std::max(ink.maxY, top);
}

inline TextInk measureInkFromMap(const std::map<unsigned short, SpriteCharacter>& characters,
                                 const std::string& text) {
  TextInk ink;
  float x = 0.0f;
  int i = 0;
  while (i < static_cast<int>(text.size())) {
    unsigned cp = nextUtf8(text, i);
    if (cp > 0xFFFFu) {
      continue;
    }
    auto it = characters.find(static_cast<unsigned short>(cp));
    if (it == characters.end()) {
      continue;
    }
    accumulateInk(ink, x, it->second);
    x += it->second.advance;
  }
  ink.advance = x;
  return ink;
}

// World space is Y-down (astro/fgfx ortho). Font space is Y-up, matching renderString.
inline glm::mat4 textYDownMat(glm::vec2 pos, float scale, const TextInk& ink, TextAlignH align) {
  float localX = 0.0f;
  float localY = 0.0f;
  if (!ink.empty) {
    if (align == TextAlignH::Left) {
      localX = ink.minX;
    } else if (align == TextAlignH::Right) {
      localX = ink.maxX;
    } else {
      localX = ink.cx();
    }
    localY = ink.cy();
  }
  return glm::scale(
    glm::translate(glm::mat4(1.0f),
                   glm::vec3(pos.x - localX * scale, pos.y + localY * scale, 0.0f)),
    glm::vec3(scale, -scale, 1.0f));
}

inline float fitInkScale(const TextInk& ink, float boxW, float boxH, float fill = 0.7f) {
  if (ink.empty) {
    return 1.0f;
  }
  float w = ink.width();
  float h = ink.height();
  if (w < 1e-4f) {
    w = 1e-4f;
  }
  if (h < 1e-4f) {
    h = 1e-4f;
  }
  float sx = boxW * fill / w;
  float sy = boxH * fill / h;
  return sx < sy ? sx : sy;
}

} // namespace render

#endif
