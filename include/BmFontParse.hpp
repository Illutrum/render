#ifndef FGFX_BMFONT_PARSE_H
#define FGFX_BMFONT_PARSE_H

#include <string>
#include <cstdlib>

namespace render {

  inline float parseXmlAttr(const std::string& tag, const char* name) {
    std::string key = std::string(name) + "=\"";
    size_t pos = tag.find(key);
    if (pos == std::string::npos) return 0;
    pos += key.size();
    size_t end = tag.find('"', pos);
    if (end == std::string::npos) return 0;
    return static_cast<float>(atof(tag.substr(pos, end - pos).c_str()));
  }

  inline int parseXmlAttrInt(const std::string& tag, const char* name) {
    return static_cast<int>(parseXmlAttr(tag, name));
  }

}

#endif
