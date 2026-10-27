cmake_minimum_required(VERSION 3.17)
project(render LANGUAGES CXX)

include(FetchContent)

if(NOT TARGET rpc_cpp)
  FetchContent_Declare(rpc_cpp
    GIT_REPOSITORY https://github.com/Daemonarium/rpc-cpp.git
    GIT_TAG v0.1.0
    GIT_SHALLOW TRUE
  )
  FetchContent_GetProperties(rpc_cpp)
  if(NOT rpc_cpp_POPULATED)
    FetchContent_Populate(rpc_cpp)
  endif()
  add_library(rpc_cpp INTERFACE)
  target_include_directories(rpc_cpp INTERFACE ${rpc_cpp_SOURCE_DIR}/src)
endif()

FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11
)
FetchContent_GetProperties(doctest)
if(NOT doctest_POPULATED)
  FetchContent_Populate(doctest)
  add_library(doctest INTERFACE)
  add_library(doctest::doctest ALIAS doctest)
  target_include_directories(doctest INTERFACE ${doctest_SOURCE_DIR})
endif()

enable_testing()

set(RENDER_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# GL (default) or WebGPU. The cache name stays FGFX_BACKEND.
set(FGFX_BACKEND "GL" CACHE STRING "Render backend: GL | WebGPU")

aux_source_directory(${RENDER_ROOT}/src RENDER_SOURCES)
list(APPEND RENDER_SOURCES ${RENDER_ROOT}/src/io/FileLoader.cpp)

if(FGFX_BACKEND STREQUAL "WebGPU")
  list(APPEND RENDER_SOURCES
    ${RENDER_ROOT}/src/backend/WebGLBackend.cpp
    ${RENDER_ROOT}/src/backend/WebGPUBackend.cpp
  )
  if(EMSCRIPTEN)
    list(APPEND RENDER_SOURCES ${RENDER_ROOT}/src/backend/WebGPUBackendWasm.cpp)
  else()
    list(APPEND RENDER_SOURCES ${RENDER_ROOT}/src/backend/WebGPUBackendNative.cpp)
  endif()
else()
  list(APPEND RENDER_SOURCES ${RENDER_ROOT}/src/backend/WebGLBackend.cpp)
endif()

add_library(render STATIC ${RENDER_SOURCES})
set_target_properties(render PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)
target_compile_features(render PUBLIC cxx_std_20)
target_include_directories(render PUBLIC
  ${RENDER_ROOT}/include
  ${RENDER_ROOT}/src
  ${RENDER_ROOT}/glm
)
target_link_libraries(render PUBLIC rpc_cpp GL png z glfw GLEW pthread)
if(NOT EMSCRIPTEN)
  target_compile_definitions(render PUBLIC __USE_OPENGL)
endif()
if(FGFX_BACKEND STREQUAL "WebGPU")
  include(${RENDER_ROOT}/cmake/Dawn.cmake)
  target_link_libraries(render PUBLIC webgpu)
  target_compile_definitions(render PUBLIC FGFX_BACKEND_WEBGPU)
endif()
if(COMMAND apply_p2996_target)
  apply_p2996_target(render)
endif()

if(NOT EMSCRIPTEN)
  add_executable(test_render
    ${RENDER_ROOT}/tests/test_fgfx.cpp
    ${RENDER_ROOT}/tests/test_file_io.cpp
    ${RENDER_ROOT}/tests/test_shaders.cpp
    ${RENDER_ROOT}/tests/test_bmfont_parse.cpp
    ${RENDER_ROOT}/tests/test_msdf_ink.cpp
    ${RENDER_ROOT}/tests/test_png.cpp
  )
  target_compile_definitions(test_render PRIVATE FGFX_SOURCE_DIR="${RENDER_ROOT}")
  target_link_libraries(test_render PRIVATE render doctest::doctest)
  if(COMMAND apply_p2996_target)
    apply_p2996_target(test_render)
  endif()
  if(COMMAND apply_p2996_rpath_origin)
    apply_p2996_rpath_origin(test_render)
  endif()
  set_target_properties(test_render PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  )
  add_test(NAME RenderTests COMMAND $<TARGET_FILE:test_render>)

  file(GLOB RENDER_EXAMPLE_FILES "${RENDER_ROOT}/examples/*.cpp")
  foreach(EXAMPLE_FILE ${RENDER_EXAMPLE_FILES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    set(EXAMPLE_OUT_DIR ${CMAKE_BINARY_DIR}/examples/${EXAMPLE_NAME})
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE render)
    if(COMMAND apply_p2996_target)
      apply_p2996_target(${EXAMPLE_NAME})
    endif()
    if(COMMAND apply_p2996_rpath_origin)
      apply_p2996_rpath_origin(${EXAMPLE_NAME})
    endif()
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${EXAMPLE_OUT_DIR}
    )
    if(FGFX_BACKEND STREQUAL "WebGPU" AND COMMAND target_copy_webgpu_binaries)
      target_copy_webgpu_binaries(${EXAMPLE_NAME})
    endif()
    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RENDER_ROOT}/examples/assets
        ${EXAMPLE_OUT_DIR}/assets
    )
  endforeach()
endif()
