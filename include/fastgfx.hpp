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

namespace fgfx {

  class Engine;
  class LineLayer;
  class SpriteLayer;
  class Layer;
  class Sprite;
  class PolygonLayer;
  class SpriteFont;
  class Texture;
  class TexturedLayer;
  class TextureTransitionLayer;
  class UniversalLayer;
  class NormalMappedSpriteLayer;
  class PBRSpriteLayer;
  class FrameBuffer;

  class InitializationFailed : public std::exception {};

  void init();
  void destroy();
  extern std::shared_ptr<Engine> engine;

#ifdef __USE_GLFW
  extern GLFWwindow* glfwWindow;
#endif
}

#include "../src/Sprite.hpp"
#include "../src/Engine.hpp"
#include "../src/LineLayer.hpp"
#include "../src/SpriteLayer.hpp"
#include "../src/Layer.hpp"
#include "../src/PolygonLayer.hpp"
#include "../src/SpriteFont.hpp"
#include "../src/Texture.hpp"
#include "../src/FrameBuffer.hpp"

#include "../src/TexturedBuffer.hpp"
#include "../src/TexturedLayer.hpp"
#include "../src/TextureTransitionLayer.hpp"
#include "../src/Image.hpp"

#include "../src/UniversalLayer.hpp"
#include "../src/NormalMappedSpriteLayer.hpp"
#include "../src/PBRSpriteLayer.hpp"

#include "../src/BuffersLayer.hpp"
#include "../src/PBRSpriteShaderBuffer.hpp"
#include "../src/PBRPolygonBuffer.hpp"
#include "../src/PBRTexturedBuffer.hpp"

#include "../src/GeneratorBuffer.hpp"

#endif //FGFX_H
