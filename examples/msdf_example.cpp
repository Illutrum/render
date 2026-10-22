#include <render.hpp>
#include <cmath>
#include <memory>

int main() {
  render::init();

  auto glyphBatch = std::make_shared<render::GlyphBatch>(render::engine.get());
  auto font = render::engine->getMSDFFont("assets/ui");

  render::engine->setSceneRenderFunction([glyphBatch, font](render::FrameBuilder& builder, double time, float delta) {
    (void)delta;
    float aspectRatio = 1.0f * render::engine->width / render::engine->height;
    render::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();
    builder.context().mvp = mvp;

    render::ClearValue clear;
    clear.r = 0.12f;
    clear.g = 0.14f;
    clear.b = 0.18f;
    clear.a = 1.0f;
    render::RenderPass& pass = builder.beginPass("scene", builder.surfaceColor(),
                                               render::LoadOp::Clear, clear);

    glyphBatch->reset();

    float pulse = 0.85f + 0.15f * sinf(time * 0.002f);
    float baseScale = 0.35f * pulse;

    font->renderString(
      glyphBatch.get(),
      glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-120, 40, 0)), glm::vec3(baseScale, baseScale, 1)),
      glm::vec4(1, 1, 1, 1),
      "FastGFX MSDF"
    );

    font->renderString(
      glyphBatch.get(),
      glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-90, -20, 0)), glm::vec3(baseScale * 0.7f, baseScale * 0.7f, 1)),
      glm::vec4(0.4f, 0.85f, 1.0f, 1),
      "Vector UI Text"
    );

    font->renderString(
      glyphBatch.get(),
      glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-70, -70, 0)), glm::vec3(baseScale * 1.4f, baseScale * 1.4f, 1)),
      glm::vec4(1.0f, 0.55f, 0.2f, 1),
      "Scale Test"
    );

    glyphBatch->record(pass, builder.context());
    builder.submitPass();
  });

#ifndef EMSCRIPTEN
#ifndef __ANDROID
  render::engine->mainLoop();
  render_log("Out of main loop! exiting");
#endif
#endif
}
