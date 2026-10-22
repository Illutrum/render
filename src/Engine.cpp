#include "Engine.hpp"
#include "config.hpp"
#include "render.hpp"
#include "backend/RenderBackend.hpp"
#include "backend/WebGLBackend.hpp"
#include "backend/FrameGraph.hpp"
#include "backend/UniformAllocator.hpp"
#ifdef FGFX_BACKEND_WEBGPU
#include "backend/WebGPUBackend.hpp"
#endif
#include <chrono>
#include <io/FileLoader.hpp>

namespace render {

  std::shared_ptr<Engine> engine;

  Engine::Engine() {
    // Create the render backend.
#ifdef FGFX_BACKEND_WEBGPU
    backendType = BackendType::WebGPU;
    backend = std::make_unique<WebGPUBackend>();
#else
    backendType = BackendType::GL;
    backend = std::make_unique<WebGLBackend>();
#endif
    uniforms = std::make_unique<UniformAllocator>(*backend);

    width = 0;
    height = 0;

    renderTime = 0;
  }

  Engine::~Engine() {
    textures.clear();
    sprites.clear();
    spriteFonts.clear();
    msdfFonts.clear();
    uniforms.reset();
    backend.reset();
  }

  std::shared_ptr<Sprite> Engine::getEmptySprite(std::string spriteName) {
    auto it = sprites.find(spriteName);
    if (it != sprites.end()) {
      std::shared_ptr<Sprite> sprite = it->second;
      if(sprite->unloaded) reloadSprite(sprite);
      return sprite;
    }
    std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>(spriteName);
    sprites[spriteName] = sprite;
    return sprite;
  }

  std::shared_ptr<Sprite> Engine::getSprite(std::string spriteName) {
    auto it = sprites.find(spriteName);
    if (it != sprites.end()) {
      std::shared_ptr<Sprite> sprite = it->second;
      if(sprite->unloaded) reloadSprite(sprite);
      return sprite;
    }
    std::shared_ptr<Sprite> sprite = std::make_shared<Sprite>(spriteName);
    sprite->loadPromise = std::make_shared<serialization::promise<std::shared_ptr<Sprite>>>();
    render_log("LOADING SPRITE!!! %s\n",spriteName.c_str());
#ifdef EMSCRIPTEN
    spritesToLoad.push_back(sprite);
#else
    loadPng(spriteName)->on_resolved([this, sprite](std::shared_ptr<Image> image) {
      textureManager.loadSprite(sprite, image);
      sprite->loadPromise->resolve(sprite);
    });
#endif
    sprites[spriteName] = sprite;
    return sprite;
  }

  void Engine::reloadSprite(std::shared_ptr<Sprite> spritep) {
#ifdef EMSCRIPTEN
    spritesToLoad.push_back(spritep);
#else
    loadPng(spritep->name)->on_resolved([this, spritep](std::shared_ptr<Image> image) {
      textureManager.loadSprite(spritep, image);
      spritep->loadPromise->resolve(spritep);
    });
#endif
    spritep->unloaded=false;
  }

  void Engine::reloadTexture(std::shared_ptr<Texture> texturep) {
    if(texturep->name.size() == 0) throw "WTF?!";
#ifdef EMSCRIPTEN
    texturesToLoad.push_back(texturep);
#else
    loadPng(texturep->name)->on_resolved([this, texturep](std::shared_ptr<Image> image) {
      textureManager.loadTexture(texturep, image);
      texturep->loadPromise->resolve(texturep);
    });
#endif
    texturep->unloaded=false;
  }
  
  std::shared_ptr<Texture> Engine::getTexture(std::string textureName) {
    auto it = textures.find(textureName);
    if (it != textures.end()) {
      std::shared_ptr<Texture> texture = it->second;
      if(texture->unloaded) reloadTexture(texture);
      return texture;
    }
    render_log("LOADING TEXTURE!!! %s\n",textureName.c_str());
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(textureName);
    texture->loadPromise = std::make_shared<serialization::promise<std::shared_ptr<Texture>>>();
#ifdef EMSCRIPTEN
    texturesToLoad.push_back(texture);
#else
    loadPng(textureName)->on_resolved([this, texture](std::shared_ptr<Image> image) {
      textureManager.loadTexture(texture, image);
      texture->loadPromise->resolve(texture);
    });
#endif
    textures[textureName] = texture;
    return texture;
  }
  std::shared_ptr<Texture> Engine::createEmptyTexture(int width, int height, PixelFormat format) {
    auto texture = std::make_shared<Texture>();
    texture->recreate(width, height, format);
    return texture;
  }

  std::shared_ptr<SpriteFont> Engine::getSpriteFont(std::string spriteFontName) {
    auto it = spriteFonts.find(spriteFontName);
    if (it != spriteFonts.end()) return it->second;
    std::shared_ptr<SpriteFont> spriteFont = std::make_shared<SpriteFont>(spriteFontName);
    spriteFont->loadPromise = std::make_shared<serialization::promise<std::shared_ptr<SpriteFont>>>();
#ifdef EMSCRIPTEN
    spriteFontsToLoad.push_back(spriteFont);
#else
    auto pngp = loadPng(spriteFontName + ".png");
    auto datap = render::io::loadFile(spriteFontName + ".fnt");
    pngp->on_resolved([this, spriteFont, datap](std::shared_ptr<Image> image) {
      datap->on_resolved([this, spriteFont, image](std::string data){
        textureManager.loadSpriteFont(spriteFont, image, data);
        render::engine->uploadQueue.enqueue([=] {
          spriteFont->loadPromise->resolve(spriteFont);
        });
      });
    });
    pngp->on_rejected([/*this,*/ spriteFont](std::exception_ptr error) {
      spriteFont->loadPromise->reject(error);
    });
    datap->on_rejected([/*this,*/ spriteFont](std::exception_ptr error) {
      spriteFont->loadPromise->reject(error);
    });
#endif
    spriteFonts[spriteFontName] = spriteFont;
    return spriteFont;
  }

  std::shared_ptr<MSDFFont> Engine::getMSDFFont(std::string msdfFontName) {
    auto it = msdfFonts.find(msdfFontName);
    if (it != msdfFonts.end()) return it->second;
    std::shared_ptr<MSDFFont> msdfFont = std::make_shared<MSDFFont>(msdfFontName);
    msdfFont->loadPromise = std::make_shared<serialization::promise<std::shared_ptr<MSDFFont>>>();
#ifdef EMSCRIPTEN
    msdfFontsToLoad.push_back(msdfFont);
#else
    auto pngp = loadPng(msdfFontName + ".png");
    auto datap = render::io::loadFile(msdfFontName + ".fnt");
    pngp->on_resolved([this, msdfFont, datap](std::shared_ptr<Image> image) {
      datap->on_resolved([this, msdfFont, image](std::string data) {
        textureManager.loadMSDFFont(msdfFont, image, data);
        render::engine->uploadQueue.enqueue([=] {
          msdfFont->loadPromise->resolve(msdfFont);
        });
      });
    });
    pngp->on_rejected([msdfFont](std::exception_ptr error) {
      msdfFont->loadPromise->reject(error);
    });
    datap->on_rejected([msdfFont](std::exception_ptr error) {
      msdfFont->loadPromise->reject(error);
    });
#endif
    msdfFonts[msdfFontName] = msdfFont;
    return msdfFont;
  }

#ifdef EMSCRIPTEN
  int Engine::getSpritesToLoadCount() {
    return spritesToLoad.size();
  }
  std::shared_ptr<Sprite> Engine::getSpriteToLoad(int n) {
    return spritesToLoad[n];
  }
  void Engine::clearSpritesToLoad() {
    spritesToLoad.clear();
  }
  int Engine::getSpriteFontsToLoadCount() {
    return spriteFontsToLoad.size();
  }
  std::shared_ptr<SpriteFont> Engine::getSpriteFontToLoad(int n) {
    return spriteFontsToLoad[n];
  }
  void Engine::clearSpriteFontsToLoad() {
    spriteFontsToLoad.clear();
  }
  int Engine::getMSDFFontsToLoadCount() {
    return msdfFontsToLoad.size();
  }
  std::shared_ptr<MSDFFont> Engine::getMSDFFontToLoad(int n) {
    return msdfFontsToLoad[n];
  }
  void Engine::clearMSDFFontsToLoad() {
    msdfFontsToLoad.clear();
  }
  int Engine::getTexturesToLoadCount() {
    return texturesToLoad.size();
  }
  std::shared_ptr<Texture> Engine::getTextureToLoad(int n) {
    return texturesToLoad[n];
  }
  void Engine::clearTexturesToLoad() {
    texturesToLoad.clear();
  }
#endif

  void Engine::setRenderFunction(std::function<void(float,float)> renderFunctionp) {
    renderFunction = renderFunctionp;
  }

  void Engine::setSceneRenderFunction(std::function<void(FrameBuilder&, double, float)> fn) {
    sceneRenderFunction = std::move(fn);
  }

  void Engine::uploadTextures() {
#ifndef EMSCRIPTEN
    Engine::UploadTask task;
    while (render::engine->uploadQueue.try_dequeue(task)) {
      task();
#ifndef FGFX_BACKEND_WEBGPU
      checkGLError();
#endif
    }
#endif
  }

  void Engine::render(double time, float delta, int widthp, int heightp) {
    ++frameIndex;
    width = widthp;
    height = heightp;
#ifndef FGFX_BACKEND_WEBGPU
    checkGLError();
#endif
    uploadTextures();

    renderTime = time;
    renderDelta = delta;

    // Tick (asset manager / input) then scene through the render graph.
    if (renderFunction) renderFunction(time, delta);

    // New path: scene renders through the render graph.
    if (sceneRenderFunction) {
      uniforms->reset();
      FrameBuilder builder(*backend, *uniforms, kNullTexture, widthp, heightp);
      builder.context().time = time;
      builder.context().delta = delta;
      builder.context().backend = backend.get();
      sceneRenderFunction(builder, time, delta);
      FrameGraph graph = builder.build();
      graph.execute(*backend);
    }
  }

  void Engine::upload(std::function<void()> uploadFunction) {
#ifdef EMSCRIPTEN
    uploadFunction();
#else
    uploadQueue.enqueue(uploadFunction);
#endif
  }

#ifdef __USE_GLFW
  void Engine::mainLoop() {
#ifndef FGFX_BACKEND_WEBGPU
    glfwMakeContextCurrent(glfwWindow);
#endif

    double lastTime = glfwGetTime() * 1000.0;
    while (!glfwWindowShouldClose(glfwWindow)) {
      // Poll before render so keysJustPressed is visible this frame.
      // GameHost clears edge bits after the scene has read RawInput.
      glfwPollEvents();
      double now = glfwGetTime() * 1000.0;
      double delta = now - lastTime;
      glfwGetFramebufferSize(glfwWindow, &width, &height);
      int w, h;
      glfwGetWindowSize(glfwWindow, &w, &h);
      realWidth = 0.264 * w; // pixels to mm
      realHeight = 0.264 * h; // pixels to mm
      render(now, delta, width, height);
      lastTime = now;
#ifndef FGFX_BACKEND_WEBGPU
      // GL presents via swap; WebGPU presents via surface.present() in endFrame().
      glfwSwapBuffers(glfwWindow);
#endif
    }
  }
#endif

};
