//
// Created by Michał Łaszczewski on 09/07/16.
//
#include <fastgfx.hpp>

float rotation = 0;
#define PI 3.141592

int main() {
  fgfx::init();

  auto logoSprite = fgfx::engine->getSprite("assets/logo.png");
  auto spriteLayer = fgfx::engine->createSpriteLayer();

  fgfx::engine->setRenderFunction([logoSprite,spriteLayer](double time, float delta) {
   // fgfx_log("FGFX Render! %d %d %f\n",fgfx::engine->width, fgfx::engine->height,delta);
    float aspectRatio = 1.0 * fgfx::engine->width / fgfx::engine->height;
    fgfx::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0),glm::vec3(0, 1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();

    glClearColor(0.9, 0.9, 0.95, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    spriteLayer->reset();
    spriteLayer->bufferSprite(logoSprite, glm::vec2(0,0), glm::vec4(1,1,1,1), 10.0, rotation);
    spriteLayer->upload();
    spriteLayer->render(mvp);

    rotation += 0.001 * delta;
    if(rotation > 2 * PI) rotation -= 2 * PI;
  });

  #ifndef EMSCRIPTEN
  #ifndef __ANDROID
    fgfx::engine->mainLoop();
    fgfx_log("Out of main loop! exiting");
  #endif
  #endif
}
