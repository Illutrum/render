cmake_minimum_required(VERSION 3.17)
project(FastGFX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FGFX_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(FGFX_INCLUDE ${FGFX_ROOT}/include)
set(FGFX_SRC ${FGFX_ROOT}/src)

include_directories(../glm include src ${CMAKE_SOURCE_DIR}/cpp-rpc/src)

aux_source_directory(${FGFX_ROOT}/src FGFX_SRC_LIST)
list(APPEND FGFX_SRC_LIST ${FGFX_ROOT}/src/io/FileLoader.cpp)

# Render backend selection: GL (default) or WebGPU (Phase 7).
set(FGFX_BACKEND "GL" CACHE STRING "Render backend: GL | WebGPU")

# Backend sources: only compile the selected backend's .cpp files.
# Native Dawn init (GLFW/X11) must not be compiled for Emscripten.
if(FGFX_BACKEND STREQUAL "WebGPU")
  set(FGFX_BACKEND_SOURCES
    "${FGFX_ROOT}/src/backend/WebGLBackend.cpp"
    "${FGFX_ROOT}/src/backend/WebGPUBackend.cpp")
  if(EMSCRIPTEN)
    list(APPEND FGFX_BACKEND_SOURCES
      "${FGFX_ROOT}/src/backend/WebGPUBackendWasm.cpp")
  else()
    list(APPEND FGFX_BACKEND_SOURCES
      "${FGFX_ROOT}/src/backend/WebGPUBackendNative.cpp")
  endif()
else()
  set(FGFX_BACKEND_SOURCES "${FGFX_ROOT}/src/backend/WebGLBackend.cpp")
endif()
list(APPEND FGFX_SRC_LIST ${FGFX_BACKEND_SOURCES})
set(FGFX_SOURCES ${FGFX_SRC_LIST})
set(FGFX_SOURCES ${FGFX_SOURCES} PARENT_SCOPE)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -stdlib=libc++ -O3 -Os")

if(EMSCRIPTEN)
  include(${CMAKE_SOURCE_DIR}/game-dev/engine/cmake/EngineHelpers.cmake)
  file(GLOB EXAMPLE_FILES "examples/*.cpp")
  foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE} ${FGFX_SOURCES})
    target_include_directories(${EXAMPLE_NAME} PRIVATE include src)
    set(EXAMPLE_OUT_DIR ${CMAKE_BINARY_DIR}/examples/${EXAMPLE_NAME})
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${EXAMPLE_OUT_DIR}
    )
    apply_emscripten_backend_target(${EXAMPLE_NAME})
    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${FGFX_ROOT}/examples/${EXAMPLE_NAME}.html
        ${EXAMPLE_OUT_DIR}/index.html
      COMMAND ${CMAKE_COMMAND} -E copy
        ${FGFX_ROOT}/runtime-js-compiled.js
        ${EXAMPLE_OUT_DIR}/runtime-js-compiled.js
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${FGFX_ROOT}/examples/assets
        ${EXAMPLE_OUT_DIR}/assets
    )
  endforeach()
endif()
