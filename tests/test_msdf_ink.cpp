#include <doctest/doctest.h>

#include "MSDFFont.hpp"
#include "TextInk.hpp"

TEST_CASE("measureInk uses glyph quads not advance") {
  fgfx::MSDFFont font("ink");
  font.setHeight(50.0f, 32.0f);
  font.setCharacter('A', nullptr, glm::vec2(10.0f, 20.0f), glm::vec2(4.0f, 8.0f), 30.0f);

  CHECK(font.measureString("A").x == doctest::Approx(30.0f));

  fgfx::TextInk ink = font.measureInk("A");
  CHECK_FALSE(ink.empty);
  CHECK(ink.advance == doctest::Approx(30.0f));
  CHECK(ink.minX == doctest::Approx(4.0f));
  CHECK(ink.maxX == doctest::Approx(14.0f));
  CHECK(ink.maxY == doctest::Approx(-8.0f));
  CHECK(ink.minY == doctest::Approx(-28.0f));
  CHECK(ink.cx() == doctest::Approx(9.0f));
  CHECK(ink.cy() == doctest::Approx(-18.0f));
  CHECK(ink.width() == doctest::Approx(10.0f));
  CHECK(ink.height() == doctest::Approx(20.0f));
}

TEST_CASE("measureInk skips empty glyphs and unions runs") {
  fgfx::MSDFFont font("ink");
  font.setHeight(50.0f, 32.0f);
  font.setCharacter('A', nullptr, glm::vec2(10.0f, 20.0f), glm::vec2(4.0f, 8.0f), 30.0f);
  font.setCharacter(' ', nullptr, glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), 12.0f);
  font.setCharacter('B', nullptr, glm::vec2(8.0f, 8.0f), glm::vec2(0.0f, 0.0f), 10.0f);

  fgfx::TextInk ink = font.measureInk("A B");
  CHECK_FALSE(ink.empty);
  CHECK(ink.advance == doctest::Approx(52.0f));
  CHECK(ink.minX == doctest::Approx(4.0f));
  CHECK(ink.maxX == doctest::Approx(50.0f));
  CHECK(ink.maxY == doctest::Approx(0.0f));
  CHECK(ink.minY == doctest::Approx(-28.0f));
}

TEST_CASE("textYDownMat places ink center on a Y-down world point") {
  fgfx::TextInk ink;
  ink.empty = false;
  ink.minX = 4.0f;
  ink.maxX = 14.0f;
  ink.minY = -28.0f;
  ink.maxY = -8.0f;
  glm::mat4 mat = fgfx::textYDownMat(glm::vec2(100.0f, 200.0f), 2.0f, ink, fgfx::TextAlignH::Center);
  glm::vec4 world = mat * glm::vec4(ink.cx(), ink.cy(), 0.0f, 1.0f);
  CHECK(world.x == doctest::Approx(100.0f));
  CHECK(world.y == doctest::Approx(200.0f));
}

TEST_CASE("fitInkScale fills the shorter box axis") {
  fgfx::TextInk ink;
  ink.empty = false;
  ink.minX = 0.0f;
  ink.maxX = 10.0f;
  ink.minY = 0.0f;
  ink.maxY = 20.0f;
  CHECK(fgfx::fitInkScale(ink, 100.0f, 100.0f, 0.5f) == doctest::Approx(2.5f));
}
