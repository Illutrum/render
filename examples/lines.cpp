#include <render.hpp>
#include <vector>
#include <cstdlib>
#include <memory>

std::vector<std::pair<glm::vec2, glm::vec2>> points;
#define POINTS_COUNT 23
#define POINTS_SIZE 50.0f
#define POINTS_SPEED 2.3f

int main() {
  render::init();

  points.resize(POINTS_COUNT);
  for (int i = 0; i < POINTS_COUNT; i++) {
    points[i].first.x = 1.0 * (rand() % 2000) / 1000 - 1;
    points[i].first.y = 1.0 * (rand() % 2000) / 1000 - 1;
    points[i].second.x = 1.0 * (rand() % 2000) / 1000 - 1;
    points[i].second.y = 1.0 * (rand() % 2000) / 1000 - 1;
  }

  auto logoSprite = render::engine->getSprite("assets/logo.png");
  auto spriteBatch = std::make_shared<render::SpriteBatch>(render::engine.get());
  auto lineBatch = std::make_shared<render::LineBatch>(render::engine.get());

  render::engine->setSceneRenderFunction([=](render::FrameBuilder& builder, double time, float delta) {
    (void)time;
    float aspectRatio = 1.0 * render::engine->width / render::engine->height;
    render::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)),
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
    lineBatch->reset();

    for (int i = 0; i < POINTS_COUNT; i++) {
      std::pair<glm::vec2, glm::vec2>& p = points[i];
      p.first = p.first + p.second * POINTS_SPEED * (delta * 0.001f);
      if (p.first.x > 1) {
        p.second.x *= -1;
        p.first.x = 1;
      }
      if (p.first.x < -1) {
        p.second.x *= -1;
        p.first.x = -1;
      }
      if (p.first.y > 1) {
        p.second.y *= -1;
        p.first.y = 1;
      }
      if (p.first.y < -1) {
        p.second.y *= -1;
        p.first.y = -1;
      }
    }

    for (int i = 0; i < POINTS_COUNT; i++)
      for (int j = i + 1; j < POINTS_COUNT; j++) {
        const glm::vec2& p1 = points[i].first;
        const glm::vec2& p2 = points[j].first;
        lineBatch->bufferLine(
            glm::vec3(p1.x, p1.y, 0) * POINTS_SIZE,
            glm::vec3(p2.x, p2.y, 0) * POINTS_SIZE,
            glm::vec4(0, 0, 0.5, 0.7));
      }

    spriteBatch->bufferSprite(logoSprite, glm::vec2(0, 0), glm::vec4(1, 1, 1, 0.7), 10.0, 0);

    lineBatch->record(pass, builder.context());
    spriteBatch->record(pass, builder.context());
    builder.submitPass();
  });

#ifndef EMSCRIPTEN
#ifndef __ANDROID
  render::engine->mainLoop();
  render_log("Out of main loop! exiting");
#endif
#endif
}
