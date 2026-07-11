#ifndef FGFX_IO_FILE_LOADER_H
#define FGFX_IO_FILE_LOADER_H

#include <string>
#include <memory>

#include <serialization/promise.hpp>

#ifndef _NOEXCEPT
#define _NOEXCEPT _GLIBCXX_USE_NOEXCEPT _GLIBCXX_TXN_SAFE_DYN
#endif

#ifdef __ANDROID
#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

namespace fgfx {
namespace io {

  class FileNotFoundError : public std::exception {
  public:
    std::string message;
    FileNotFoundError(std::string name) {
      message = "File " + name + " not found";
    }
    virtual const char* what() const _NOEXCEPT override { return message.c_str(); }
  };

  std::shared_ptr<serialization::promise<std::string>> loadFile(std::string path);

  void init(int loadThreads = 1);
  void destroy();

#ifdef __ANDROID
  extern AAssetManager* assetManager;
#endif

}
}

#endif
