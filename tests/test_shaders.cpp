#include <doctest/doctest.h>
#include "shaders.hpp"

TEST_CASE("stripGlesQualifiers removes precision and qualifiers") {
  std::string src =
    "precision mediump float;\n"
    "highp vec4 color;\n"
    "mediump float x;\n"
    "lowp int i;\n";
  render::stripGlesQualifiers(src);
  CHECK(src.find("precision") == std::string::npos);
  CHECK(src.find("highp") == std::string::npos);
  CHECK(src.find("mediump") == std::string::npos);
  CHECK(src.find("lowp") == std::string::npos);
  CHECK(src.find("vec4 color") != std::string::npos);
}
