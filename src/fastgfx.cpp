//
// Created by Michał Łaszczewski on 09/07/16.
//

#include <render.hpp>

#include <vector>
#include <memory>
#include <cstdint>
#include <render.hpp>
#include "config.hpp"
#include <io/FileLoader.hpp>
#include "backend/RenderBackend.hpp"

#ifdef FGFX_BACKEND_WEBGPU
#include "backend/WebGPUBackend.hpp"
#endif

#ifndef EMSCRIPTEN
#include <thread>
#endif

#ifdef __USE_GLFW
void glfwErrorCallback(int error, const char* description) {
  render_log("GLFW error %d : %s", error, description)
}
#endif

namespace render {

#ifdef __ANDROID
  AAssetManager* assetManager;
#endif

#ifndef EMSCRIPTEN
  std::thread loaderThread;
  bool finished;
  void imageLoader();
#endif

#ifdef __USE_GLFW
  GLFWwindow* glfwWindow;
#endif

  namespace {
    std::vector<void(*)()> shutdownHooks;
  }

  void addShutdownHook(void (*fn)()) {
    if (!fn) {
      return;
    }
    shutdownHooks.push_back(fn);
  }

  void destroy() {
    for (auto it = shutdownHooks.rbegin(); it != shutdownHooks.rend(); ++it) {
      (*it)();
    }
    shutdownHooks.clear();
#ifdef __USE_GLFW
    //glfwDestroyWindow(render::glfwWindow);
    glfwTerminate();
#endif
#ifndef EMSCRIPTEN
    finished = true;
    if(loaderThread.joinable()) loaderThread.join();
    render::io::destroy();
#endif
  }

  void init() {
#ifndef EMSCRIPTEN
    finished = false;
    render::io::init();
#endif
#ifdef EMSCRIPTEN
    EM_ASM(Module.initializeFastGfxRuntime());
    render_log("ENGINE INITIALIZATION\n");
    render::engine = std::make_shared<render::Engine>();
#ifdef FGFX_BACKEND_WEBGPU
    if (auto* wgpuBackend = dynamic_cast<WebGPUBackend*>(engine->getBackend())) {
      wgpuBackend->initWasm("#fgfx_canvas", 1280, 1024);
    }
#endif
    EM_ASM(Module.initializeFastGfxTextureManager());
#endif
#ifdef __USE_GLFW
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
      render_log("could not init glfw")
      throw InitializationFailed();
    }
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_SAMPLES, 16);
#ifdef FGFX_BACKEND_WEBGPU
    // WebGPU: main window has no GL context; Dawn creates its own Vulkan surface.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

    glfwWindow = glfwCreateWindow(1280, 1024, "FGFX", NULL, NULL);
    if (!glfwWindow) {
      render_log("could not create window")
      throw InitializationFailed();
    }

#ifndef FGFX_BACKEND_WEBGPU
    glfwMakeContextCurrent(glfwWindow);
    glEnable(GL_MULTISAMPLE);
#ifdef __linux__
    GLenum err = glewInit();
    if (err != GLEW_OK) {
      render_log("could not init glew %d : %s", err, glewGetErrorString(err));
      throw InitializationFailed();
    }
#endif
#endif

    render::engine = std::make_shared<render::Engine>();

#ifdef FGFX_BACKEND_WEBGPU
    // Initialize the WebGPU backend with Dawn + the GLFW window.
    auto* wgpuBackend = dynamic_cast<WebGPUBackend*>(engine->getBackend());
    if (wgpuBackend) {
      wgpuBackend->initNative(glfwWindow);
    }
#endif
#endif
#ifdef __ANDROID
    render::engine = std::make_shared<render::Engine>();
#endif
#ifndef EMSCRIPTEN
    //loaderThread = std::thread(imageLoader);
#endif
  }

  const char* glErrorString(GLenum errCode) {
    switch(errCode) {
      case GL_INVALID_ENUM : return "GL_INVALID_ENUM";
      case GL_INVALID_VALUE : return "GL_INVALID_VALUE";
      case GL_INVALID_OPERATION : return "GL_INVALID_OPERATION";
      case GL_INVALID_FRAMEBUFFER_OPERATION : return "GL_INVALID_FRAMEBUFFER_OPERATION";
      case GL_OUT_OF_MEMORY : return "GL_OUT_OF_MEMORY";
#ifdef __USE_OPENGL
      case GL_STACK_UNDERFLOW : return "GL_STACK_UNDERFLOW";
      case GL_STACK_OVERFLOW : return "GL_STACK_OVERRFLOW";
#endif
      default: return "UNKNOWN_ERROR";
    }
  }

  void checkGLError() {
    GLenum errCode;
    if ((errCode = glGetError()) != GL_NO_ERROR) {
      render_log("GL Error: %d %s", errCode, glErrorString(errCode));
      throw std::exception();
    }
  }

}

#ifdef EMSCRIPTEN
using namespace render;

extern "C" {

FGFX_API
void fgfx_render(double time, float delta, int widthp, int heightp, double rw, double rh) {
  render::engine->realWidth = rw;
  render::engine->realHeight = rh;
  render::engine->render(time, delta, widthp, heightp);
}

FGFX_API
int fgfx_getSpritesToLoadCount() {
  return render::engine->getSpritesToLoadCount();
}

FGFX_API
render::Sprite* fgfx_getSpriteToLoad(int id) {
  return render::engine->getSpriteToLoad(id).get();
}

FGFX_API
void fgfx_clearSpritesToLoad(int id) {
  render::engine->clearSpritesToLoad();
}

FGFX_API
render::Sprite* fgfx_getSprite(const char* spriteName) {
  return render::engine->getSprite(spriteName).get();
}

FGFX_API
render::Sprite* fgfx_getEmptySprite(const char* spriteName) {
  return render::engine->getEmptySprite(spriteName).get();
}

FGFX_API
const char* fgfx_Sprite_getName(render::Sprite* sprite) {
  return sprite->name.c_str();
}

FGFX_API
void fgfx_Sprite_setTextureFragment(render::Sprite* sprite, int textureIdp, float left, float top, float right, float bottom,
                                    int widthp, int heightp, bool preloaded) {
  sprite->setTextureFragment(textureIdp, left, top, right, bottom, widthp, heightp, preloaded);
  if (sprite->loadPromise) {
    sprite->loadPromise->resolve(sprite->shared_from_this());
  }
}

FGFX_API
int fgfx_getSpriteFontsToLoadCount() {
  return render::engine->getSpriteFontsToLoadCount();
}

FGFX_API
render::SpriteFont* fgfx_getSpriteFontToLoad(int id) {
  return render::engine->getSpriteFontToLoad(id).get();
}

FGFX_API
void fgfx_clearSpriteFontsToLoad(int id) {
  render::engine->clearSpriteFontsToLoad();
}

FGFX_API
const char* fgfx_SpriteFont_getName(render::SpriteFont* font) {
  return font->name.c_str();
}

FGFX_API
void fgfx_SpriteFont_setHeight(render::SpriteFont* font, float linep, float basep) {
  font -> setHeight(linep, basep);
  font->loadPromise->resolve(font->shared_from_this());
}

FGFX_API
void fgfx_SpriteFont_setCharacter(render::SpriteFont* font, unsigned short character, render::Sprite* sprite,
                            float xSize, float ySize, float xOffset, float yOffset, float advance) {
  font -> setCharacter(character,sprite->shared_from_this(),glm::vec2(xSize,ySize),glm::vec2(xOffset,yOffset),advance);
}

FGFX_API
int fgfx_getMSDFFontsToLoadCount() {
  return render::engine->getMSDFFontsToLoadCount();
}

FGFX_API
render::MSDFFont* fgfx_getMSDFFontToLoad(int id) {
  return render::engine->getMSDFFontToLoad(id).get();
}

FGFX_API
void fgfx_clearMSDFFontsToLoad(int id) {
  render::engine->clearMSDFFontsToLoad();
}

FGFX_API
const char* fgfx_MSDFFont_getName(render::MSDFFont* font) {
  return font->name.c_str();
}

FGFX_API
void fgfx_MSDFFont_setHeight(render::MSDFFont* font, float linep, float basep) {
  font->setHeight(linep, basep);
}

FGFX_API
void fgfx_MSDFFont_setPxRange(render::MSDFFont* font, float range) {
  font->setPxRange(range);
}

FGFX_API
void fgfx_MSDFFont_setScale(render::MSDFFont* font, float scaleW, float scaleH) {
  font->setScale(scaleW, scaleH);
}

FGFX_API
void fgfx_MSDFFont_setCharacter(render::MSDFFont* font, unsigned short character, render::Sprite* sprite,
                                float xSize, float ySize, float xOffset, float yOffset, float advance) {
  font->setCharacter(character, sprite->shared_from_this(), glm::vec2(xSize, ySize), glm::vec2(xOffset, yOffset), advance);
}

FGFX_API
void fgfx_MSDFFont_finishLoad(render::MSDFFont* font) {
  font->loadPromise->resolve(font->shared_from_this());
}


FGFX_API
int fgfx_getTexturesToLoadCount() {
  return render::engine->getTexturesToLoadCount();
}

FGFX_API
render::Texture* fgfx_getTextureToLoad(int id) {
  return render::engine->getTextureToLoad(id).get();
}

FGFX_API
void fgfx_clearTexturesToLoad(int id) {
  render::engine->clearTexturesToLoad();
}

FGFX_API
render::Texture* fgfx_getTexture(const char* textureName) {
  return render::engine->getTexture(textureName).get();
}

FGFX_API
const char* fgfx_Texture_getName(render::Texture* texture) {
  return texture->name.c_str();
}

FGFX_API
void fgfx_Texture_setTexture(render::Texture* texture, int textureId) {
  texture->texture = textureId;
  texture->initialized = true;
  texture->loadPromise->resolve(texture->shared_from_this());
}

FGFX_API
int fgfx_Texture_getTexture(render::Texture* texture) {
  if(!texture->initialized) return -1;
  return texture->texture;
}

FGFX_API
int fgfx_createTextureRGBA(int width, int height, const uint8_t* data) {
  if (!render::engine || !render::engine->getBackend() || !data || width <= 0 ||
      height <= 0) {
    return 0;
  }
  render::TextureDesc desc;
  desc.width = width;
  desc.height = height;
  desc.format = render::PixelFormat::RGBA8Unorm;
  desc.generateMipmaps = false;
  render::TextureHandle handle = render::engine->getBackend()->createTexture(desc);
  render::engine->getBackend()->updateTexture(handle, width, height, data);
  return static_cast<int>(handle);
}

};

#endif


