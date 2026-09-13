#include <fastgfx.hpp>
#include <memory>

float rotation = 0;
#define PI 3.141592

int main() {
  fgfx::init();

  auto logoSprite = fgfx::engine->getSprite("assets/logo.png");
  auto spriteBatch = std::make_shared<fgfx::SpriteBatch>(fgfx::engine.get());

  fgfx::engine->setSceneRenderFunction([logoSprite, spriteBatch](fgfx::FrameBuilder& builder, double time, float delta) {
    (void)time;
    float aspectRatio = 1.0 * fgfx::engine->width / fgfx::engine->height;
    fgfx::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)),
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
    spriteBatch->bufferSprite(logoSprite, glm::vec2(0, 0), glm::vec4(1, 1, 1, 1), 10.0, rotation);
    spriteBatch->record(pass, builder.context());
    builder.submitPass();

    rotation += 0.001 * delta;
    if (rotation > 2 * PI) rotation -= 2 * PI;
  });

#ifndef EMSCRIPTEN
#ifndef __ANDROID
  fgfx::engine->mainLoop();
  fgfx_log("Out of main loop! exiting");
#endif
#endif
}
