# Dawn.cmake — integrates the prebuilt Dawn WebGPU backend.
# The WebGPU distribution is the webgpu/ submodule. It fetches prebuilt
# Dawn binaries and provides the `webgpu` interface target plus webgpu.hpp.
#
# Included from the top-level list only when FGFX_BACKEND is WebGPU.

set(WEBGPU_DIR ${CMAKE_CURRENT_SOURCE_DIR}/webgpu)

# Use Dawn as the backend (not wgpu-native, not emscripten).
set(WEBGPU_BACKEND "DAWN" CACHE STRING "" FORCE)
# Shared linking so a host can hot-swap its .so without re-linking Dawn.
set(WEBGPU_LINK_TYPE "SHARED" CACHE STRING "" FORCE)

# webgpu/CMakeLists.txt calls cmake_minimum_required(3.0) + project(webgpu),
# which re-inits and CLEARS CMAKE_EXE_LINKER_FLAGS (drops p2996 -lc++).
add_subdirectory(${WEBGPU_DIR} ${CMAKE_BINARY_DIR}/_deps/webgpu-build)
if(P2996_LIB_DIR)
  set(CMAKE_EXE_LINKER_FLAGS
    "-L${P2996_LIB_DIR} -Wl,-rpath,${P2996_LIB_DIR} -lc++ -lc++abi -lunwind -stdlib=libc++"
    CACHE STRING "p2996 exe linker flags" FORCE)
endif()

if(NOT TARGET dawn)
  add_library(dawn INTERFACE)
  target_link_libraries(dawn INTERFACE webgpu)
endif()
