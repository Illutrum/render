#include <doctest/doctest.h>
#include <BmFontParse.hpp>

TEST_CASE("parseXmlAttr reads numeric attributes") {
  std::string charTag = "<char id=\"65\" x=\"10\" y=\"20\" width=\"30\" height=\"40\" xoffset=\"1\" yoffset=\"2\" xadvance=\"32\"/>";
  CHECK(fgfx::parseXmlAttrInt(charTag, "id") == 65);
  CHECK(fgfx::parseXmlAttrInt(charTag, "x") == 10);
  CHECK(fgfx::parseXmlAttrInt(charTag, "width") == 30);
}

TEST_CASE("parseXmlAttr reads distanceField range") {
  std::string dfTag = "<distanceField fieldType=\"msdf\" distanceRange=\"4\"/>";
  CHECK(fgfx::parseXmlAttr(dfTag, "distanceRange") == doctest::Approx(4.0f));
}

TEST_CASE("parseXmlAttr returns zero for missing attribute") {
  std::string tag = "<common lineHeight=\"42\"/>";
  CHECK(fgfx::parseXmlAttrInt(tag, "base") == 0);
}
