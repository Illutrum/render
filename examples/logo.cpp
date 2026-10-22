#include <render.hpp>
#include <memory>

float rotation = 0;
#define PI 3.141592

int main() {
  render::init();

  auto logoSprite = render::engine->getSprite("assets/logo.png");
  auto spriteBatch = std::make_shared<render::SpriteBatch>(render::engine.get());

  render::engine->setSceneRenderFunction([logoSprite, spriteBatch](render::FrameBuilder& builder, double time, float delta) {
    (void)time;
    float aspectRatio = 1.0 * render::engine->width / render::engine->height;
    render::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();
    builder.context().mvp = mvp;

    render::ClearValue clear;
    clear.r = 0.9f;
    clear.g = 0.9f;
    clear.b = 0.95f;
    clear.a = 1.0f;
    render::RenderPass& pass = builder.beginPass("scene", builder.surfaceColor(),
                                               render::LoadOp::Clear, clear);

    spriteBatch->reset();
    spriteBatch->bufferSprite(logoSprite, glm::vec2(0, 0), glm::vec4(1, 1, 1, 1), 10.0, rotation);
    spriteBatch->record(pass, builder.context());
    builder.submitPass();

    rotation += 0.001 * delta;
    if (rotation > 2 * PI) rotation -= 2 * PI;
  });

#ifndef EMSCRIPTEN
#ifndef __ANDROID
  render::engine->mainLoop();
  render_log("Out of main loop! exiting");
#endif
#endif
}
