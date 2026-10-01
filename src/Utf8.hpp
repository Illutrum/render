#ifndef FGFX_UTF8_HPP
#define FGFX_UTF8_HPP

#include <string>

namespace fgfx {

inline unsigned nextUtf8(const std::string& text, int& i) {
  if (i < 0 || i >= static_cast<int>(text.size())) {
    return 0;
  }
  unsigned char c0 = static_cast<unsigned char>(text[static_cast<std::size_t>(i)]);
  if (c0 < 0x80) {
    ++i;
    return c0;
  }
  if ((c0 & 0xE0) == 0xC0 && i + 1 < static_cast<int>(text.size())) {
    unsigned char c1 = static_cast<unsigned char>(text[static_cast<std::size_t>(i) + 1]);
    if ((c1 & 0xC0) == 0x80) {
      i += 2;
      return ((c0 & 0x1Fu) << 6) | (c1 & 0x3Fu);
    }
  }
  if ((c0 & 0xF0) == 0xE0 && i + 2 < static_cast<int>(text.size())) {
    unsigned char c1 = static_cast<unsigned char>(text[static_cast<std::size_t>(i) + 1]);
    unsigned char c2 = static_cast<unsigned char>(text[static_cast<std::size_t>(i) + 2]);
    if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80) {
      i += 3;
      return ((c0 & 0x0Fu) << 12) | ((c1 & 0x3Fu) << 6) | (c2 & 0x3Fu);
    }
  }
  if ((c0 & 0xF8) == 0xF0 && i + 3 < static_cast<int>(text.size())) {
    i += 4;
    return 0xFFFD;
  }
  ++i;
  return c0;
}

inline unsigned toUpperCodepoint(unsigned cp) {
  if (cp >= 'a' && cp <= 'z') {
    return cp - ('a' - 'A');
  }
  switch (cp) {
    case 0x0105: return 0x0104; // ą Ą
    case 0x0107: return 0x0106; // ć Ć
    case 0x0119: return 0x0118; // ę Ę
    case 0x0142: return 0x0141; // ł Ł
    case 0x0144: return 0x0143; // ń Ń
    case 0x00F3: return 0x00D3; // ó Ó
    case 0x015B: return 0x015A; // ś Ś
    case 0x017A: return 0x0179; // ź Ź
    case 0x017C: return 0x017B; // ż Ż
    default: return cp;
  }
}

inline void appendUtf8(std::string& out, unsigned cp) {
  if (cp < 0x80) {
    out.push_back(static_cast<char>(cp));
    return;
  }
  if (cp < 0x800) {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    return;
  }
  out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
  out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
  out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
}

inline std::string utf8Upper(const char* text) {
  std::string in = text ? text : "";
  std::string out;
  out.reserve(in.size());
  int i = 0;
  while (i < static_cast<int>(in.size())) {
    appendUtf8(out, toUpperCodepoint(nextUtf8(in, i)));
  }
  return out;
}

} // namespace fgfx

#endif
