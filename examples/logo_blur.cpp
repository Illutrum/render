#include <fastgfx.hpp>
#include <memory>

#define PI 3.141592

int main() {
  fgfx::init();

  auto logoSprite = fgfx::engine->getSprite("assets/logo.png");
  auto spriteBatch = std::make_shared<fgfx::SpriteBatch>(fgfx::engine.get());
  auto font = fgfx::engine->getSpriteFont("assets/cat");

  fgfx::engine->setSceneRenderFunction([logoSprite, spriteBatch, font](fgfx::FrameBuilder& builder, double time, float delta) {
    (void)delta;
    float aspectRatio = 1.0 * fgfx::engine->width / fgfx::engine->height;

    fgfx::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();
    builder.context().mvp = mvp;

    fgfx::ClearValue clear;
    clear.r = 0.9f;
    clear.g = 0.9f;
    clear.b = 0.95f;
    clear.a = 1.0f;
    fgfx::RenderPass& pass = builder.beginPass("scene", builder.surfaceColor(),
                                               fgfx::LoadOp::Clear, clear);

    spriteBatch->reset();

    for (float i = 100; i > 0; i -= 0.2) {
      float cycleTime = fmod((time - i) / 10000, 1.0) * 2 * PI;
      float rotation = cycleTime * sin(cycleTime) * 12.3;
      spriteBatch->bufferSprite(logoSprite,
                                glm::scale(
                                    glm::rotate(glm::mat4(1.0), rotation, glm::vec3(0, 0, 1)), glm::vec3(10, 10, 10)),
                                glm::vec4(1, 1, 1, 0.1 / (i * 0.23)));
    }

    float fontScale = 0.23;
    font->renderString(spriteBatch.get(),
                       glm::scale(glm::translate(glm::mat4(1.0), glm::vec3(-80, 80, 0)), glm::vec3(fontScale, fontScale, fontScale)),
                       glm::vec4(0.6, 0, 0.7, 1), "FastGFX FTW!");

    spriteBatch->record(pass, builder.context());
    builder.submitPass();
  });

#ifndef EMSCRIPTEN
#ifndef __ANDROID
  fgfx::engine->mainLoop();
  fgfx_log("Out of main loop! exiting");
#endif
#endif
}
