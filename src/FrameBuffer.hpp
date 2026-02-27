//
// Created by Michał Łaszczewski on 28/03/17.
//

#ifndef FGFX_FRAMEBUFFER_H
#define FGFX_FRAMEBUFFER_H

#include "config.hpp"
#include <memory>
#include "Image.hpp"

namespace fgfx {

  class FrameBuffer {
  public:
    int width;
    int height;
    bool hasZ;

    GLuint fbo;
    GLuint colorRbo;
    GLuint depthRbo;

    GLuint colorFormat;
    GLuint depthFormat;

    FrameBuffer(int widthp, int heightp, bool hasZp,
        GLuint colorFormatp /*= GL_RGBA16*/, GLuint depthFormatp /*= GL_DEPTH_COMPONENT16*/);
    ~FrameBuffer();

    void beginRender();
    void endRender();
    void readImage(std::shared_ptr<Image> image);
    void readRGB8(void* rgb8);
    void readRGB16(void* rgb16);
  private:
    GLint old_fbo;
    GLint old_viewport[4];
    void saveFBO();
    void loadFBO();
  };

}



#endif //FGFX_FRAMEBUFFER_H


