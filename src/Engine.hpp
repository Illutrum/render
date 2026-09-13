#ifndef ENGINE_H
#define ENGINE_H

#include "SpriteFont.hpp"
#include "MSDFFont.hpp"
#include "Sprite.hpp"
#include "Texture.hpp"
#include "shaders.hpp"
#include "backend/BackendTypes.hpp"

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#ifndef EMSCRIPTEN
#include "concurrentqueue.hpp"
#include "Image.hpp"
#include "SpritesTexture.hpp"
#include "TextureManager.hpp"

#endif

namespace fgfx {

  class RenderBackend;
  class UniformAllocator;
  class FrameBuilder;

  class Engine : public std::enable_shared_from_this<Engine> {
  protected:
    std::map<std::string, std::shared_ptr<Sprite>> sprites;
    std::map<std::string, std::shared_ptr<SpriteFont>> spriteFonts;
    std::map<std::string, std::shared_ptr<MSDFFont>> msdfFonts;
    std::map<std::string, std::shared_ptr<Texture>> textures;

#ifdef EMSCRIPTEN
    std::vector<std::shared_ptr<Sprite>> spritesToLoad;
    std::vector<std::shared_ptr<SpriteFont>> spriteFontsToLoad;
    std::vector<std::shared_ptr<MSDFFont>> msdfFontsToLoad;
    std::vector<std::shared_ptr<Texture>> texturesToLoad;
#endif

#ifndef EMSCRIPTEN
  public:
    TextureManager textureManager;
    using UploadTask = std::function<void()>;
    moodycamel::ConcurrentQueue<UploadTask> uploadQueue;
  protected:
#endif

    friend class Sprite;
    friend class SpriteFont;
    friend class MSDFFont;
    friend class SpriteBatch;

    std::function<void(double,float)> renderFunction;
    std::function<void(FrameBuilder&, double, float)> sceneRenderFunction;

    // Render backend abstraction (GL or WebGPU).
    BackendType backendType = BackendType::GL;
    std::unique_ptr<RenderBackend> backend;
    std::unique_ptr<UniformAllocator> uniforms;

  public:
    double renderTime;
    float renderDelta;

    // Public access to the render backend (for native init).
    RenderBackend* getBackend() { return backend.get(); }

    int width;
    int height;
    GLint boundFramebuffer;
    int viewportX;
    int viewportY;
    int viewportW;
    int viewportH;

    double realWidth;
    double realHeight;

    Engine();
    ~Engine();

    std::shared_ptr<Sprite> getSprite(std::string spriteName);
    std::shared_ptr<Sprite> getEmptySprite(std::string spriteName);
    std::shared_ptr<SpriteFont> getSpriteFont(std::string spriteFontName);
    std::shared_ptr<MSDFFont> getMSDFFont(std::string msdfFontName);
    std::shared_ptr<Texture> getTexture(std::string texture);
    std::shared_ptr<Texture> createEmptyTexture(int width, int height, GLint internalFormat, GLenum format, GLenum type);

#ifdef EMSCRIPTEN
    int getSpritesToLoadCount();
    std::shared_ptr<Sprite> getSpriteToLoad(int n);
    void clearSpritesToLoad();
    int getSpriteFontsToLoadCount();
    std::shared_ptr<SpriteFont> getSpriteFontToLoad(int n);
    void clearSpriteFontsToLoad();
    int getMSDFFontsToLoadCount();
    std::shared_ptr<MSDFFont> getMSDFFontToLoad(int n);
    void clearMSDFFontsToLoad();
    int getTexturesToLoadCount();
    std::shared_ptr<Texture> getTextureToLoad(int n);
    void clearTexturesToLoad();
#endif

    void reloadSprite(std::shared_ptr<Sprite> spritep);
    void reloadTexture(std::shared_ptr<Texture> texturep);

    void setRenderFunction(std::function<void(float,float)> renderFunctionp);
    void setSceneRenderFunction(std::function<void(FrameBuilder&, double, float)> fn);
    void render(double time, float delta, int widthp, int heightp);
    void uploadTextures();
    void seedDrawTarget(int widthp, int heightp);
    void bindDrawTarget(GLint fbo, int x, int y, int w, int h);

    void upload(std::function<void()> uploadFunction);

#ifndef EMSCRIPTEN
    void mainLoop();
#endif
  };

  extern std::shared_ptr<Engine> engine;

};

#endif // ENGINE_H
