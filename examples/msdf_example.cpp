//
// MSDF font rendering example
//
#include <fastgfx.hpp>
#include <cmath>

int main() {
  fgfx::init();

  auto msdfLayer = fgfx::engine->createMSDFLayer();
  auto font = fgfx::engine->getMSDFFont("assets/ui");

  fgfx::engine->setRenderFunction([msdfLayer, font](double time, float delta) {
    float aspectRatio = 1.0f * fgfx::engine->width / fgfx::engine->height;
    fgfx::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();

    glClearColor(0.12f, 0.14f, 0.18f, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    msdfLayer->reset();

    float pulse = 0.85f + 0.15f * sinf(time * 0.002f);
    float baseScale = 0.35f * pulse;

    font->renderString(
      msdfLayer,
      glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-120, 40, 0)), glm::vec3(baseScale, baseScale, 1)),
      glm::vec4(1, 1, 1, 1),
      "FastGFX MSDF"
    );

    font->renderString(
      msdfLayer,
      glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-90, -20, 0)), glm::vec3(baseScale * 0.7f, baseScale * 0.7f, 1)),
      glm::vec4(0.4f, 0.85f, 1.0f, 1),
      "Vector UI Text"
    );

    font->renderString(
      msdfLayer,
      glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-70, -70, 0)), glm::vec3(baseScale * 1.4f, baseScale * 1.4f, 1)),
      glm::vec4(1.0f, 0.55f, 0.2f, 1),
      "Scale Test"
    );

    msdfLayer->upload();
    msdfLayer->render(mvp);
  });

#ifndef EMSCRIPTEN
#ifndef __ANDROID
  fgfx::engine->mainLoop();
  fgfx_log("Out of main loop! exiting");
#endif
#endif
}
