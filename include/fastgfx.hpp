//
// Created by Michał Łaszczewski on 01/07/16.
//

#ifndef FGFX_H
#define FGFX_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <functional>
#include <serialization/promise.hpp>

#include "../src/config.hpp"

namespace fgfx {

  class Engine;
  class Sprite;
  class SpriteFont;
  class MSDFFont;
  class Texture;

  class InitializationFailed : public std::exception {};

  void init();
  void destroy();
  void addShutdownHook(void (*fn)());
  extern std::shared_ptr<Engine> engine;

#ifdef __USE_GLFW
  extern GLFWwindow* glfwWindow;
#ifdef FGFX_BACKEND_WEBGPU
  extern GLFWwindow* glContextWindow;  // hidden window for GL texture loading
#endif
#endif
}

#include "../src/Sprite.hpp"
#include "../src/Engine.hpp"
#include "../src/SpriteBatch.hpp"
#include "../src/GlyphBatch.hpp"
#include "../src/PolygonBatch.hpp"
#include "../src/LineBatch.hpp"
#include "../src/TexturedBatch.hpp"
#include "../src/TextureTransitionBatch.hpp"
#include "../src/NormalMappedSpriteBatch.hpp"
#include "../src/PBRSpriteBatch.hpp"
#include "../src/backend/FrameGraph.hpp"
#include "../src/SpriteFont.hpp"
#include "../src/MSDFFont.hpp"
#include "../src/Texture.hpp"
#include "../src/Image.hpp"

#endif //FGFX_H
