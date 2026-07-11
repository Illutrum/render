#include <doctest/doctest.h>
#include <io/FileLoader.hpp>
#include <fstream>
#include <future>
#include <thread>
#include <vector>

namespace {

template<typename T>
T waitPromise(std::shared_ptr<serialization::promise<T>> p) {
  std::promise<T> sync;
  p->on_resolved([&](T& value) { sync.set_value(std::move(value)); });
  p->on_rejected([&](std::exception_ptr err) { sync.set_exception(err); });
  return sync.get_future().get();
}

std::string writeTempFile(const std::string& name, const std::string& content) {
  std::string path = std::string("/tmp/fgfx_test_") + name;
  std::ofstream out(path, std::ios::binary);
  out << content;
  return path;
}

struct IoFixture {
  IoFixture() { fgfx::io::init(2); }
  ~IoFixture() { fgfx::io::destroy(); }
};

}

TEST_CASE("io load existing file") {
  IoFixture fixture;
  auto path = writeTempFile("load_ok.txt", "hello fgfx");
  auto data = waitPromise(fgfx::io::loadFile(path));
  CHECK(data == "hello fgfx");
}

TEST_CASE("io load missing file") {
  IoFixture fixture;
  CHECK_THROWS_AS(
    waitPromise(fgfx::io::loadFile("/tmp/fgfx_test_does_not_exist_12345.bin")),
    fgfx::io::FileNotFoundError
  );
}

TEST_CASE("io init destroy") {
  fgfx::io::init(1);
  fgfx::io::destroy();
  fgfx::io::init(1);
  auto path = writeTempFile("after_reinit.txt", "ok");
  auto data = waitPromise(fgfx::io::loadFile(path));
  CHECK(data == "ok");
  fgfx::io::destroy();
}

TEST_CASE("io concurrent loads") {
  IoFixture fixture;
  auto pathA = writeTempFile("concurrent_a.txt", "aaa");
  auto pathB = writeTempFile("concurrent_b.txt", "bbb");
  auto promiseA = fgfx::io::loadFile(pathA);
  auto promiseB = fgfx::io::loadFile(pathB);
  CHECK(waitPromise(promiseA) == "aaa");
  CHECK(waitPromise(promiseB) == "bbb");
}
