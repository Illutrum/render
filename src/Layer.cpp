#include "Layer.hpp"


namespace fgfx {

  Layer::Layer(Engine *enginep) :  engine(enginep), composition(2),
    frozen(false), visible(true), camera(false) {
  }

  Layer::~Layer() {
  }
};
