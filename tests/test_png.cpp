#include <doctest/doctest.h>
#include <io/FileLoader.hpp>
#include <Image.hpp>
#include <future>
#include <string>

namespace {

template<typename T>
T waitPromise(std::shared_ptr<serialization::promise<T>> p) {
  std::promise<T> sync;
  p->on_resolved([&](T& value) { sync.set_value(std::move(value)); });
  p->on_rejected([&](std::exception_ptr err) { sync.set_exception(err); });
  return sync.get_future().get();
}

}

TEST_CASE("loadPng decodes logo.png") {
  fgfx::io::init(1);
  std::string path = std::string(FGFX_SOURCE_DIR) + "/examples/assets/logo.png";
  auto image = waitPromise(fgfx::loadPng(path));
  CHECK(image != nullptr);
  CHECK(image->width > 0);
  CHECK(image->height > 0);
  fgfx::io::destroy();
}
