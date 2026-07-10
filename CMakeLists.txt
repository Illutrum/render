cmake_minimum_required(VERSION 3.17)
project(FastGFX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(../glm include)
include_directories(src)
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/cpp-rpc/src)

set(SRC_ROOT ./)
aux_source_directory(${SRC_ROOT}src SRC_LIST)
set(SOURCE_FILES ${SRC_LIST})

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -stdlib=libc++ -O3 -Os")

set(FSXX_ROOT ${CMAKE_SOURCE_DIR}/game-dev/fsxx)
include_directories(${FSXX_ROOT}/include)
aux_source_directory(${FSXX_ROOT}/src FSXX_SOURCES)

if(NOT EMSCRIPTEN) 
  add_definitions(-D__USE_OPENGL)
  add_library(fgfx STATIC ${SOURCE_FILES})
endif()


file(GLOB EXAMPLE_FILES "examples/*.cpp")

foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
  get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
  add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE} ${FSXX_SOURCES} ${SOURCE_FILES})
  target_include_directories(${EXAMPLE_NAME} PRIVATE include src)
  # Non-Emscripten builds need desktop OpenGL, libpng, GLFW, GLEW
  if(NOT EMSCRIPTEN)
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
    target_link_libraries(${EXAMPLE_NAME} PRIVATE
      GL
      png
      z
      glfw
      GLEW)
  else()
    set(EXAMPLE_OUT_DIR ${CMAKE_BINARY_DIR}/examples/${EXAMPLE_NAME})
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${EXAMPLE_OUT_DIR}
    )
    target_link_options(${EXAMPLE_NAME} PRIVATE
      "-sEXPORTED_RUNTIME_METHODS=UTF8ToString,allocate,intArrayFromString"
      "-sEXPORTED_FUNCTIONS=_main,_malloc,_free"
      "--post-js=${CMAKE_CURRENT_SOURCE_DIR}/js/emscripten-glue.js"
    )
    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/${EXAMPLE_NAME}.html
        ${EXAMPLE_OUT_DIR}/index.html
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime-js-compiled.js
        ${EXAMPLE_OUT_DIR}/runtime-js-compiled.js
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/assets
        ${EXAMPLE_OUT_DIR}/assets
    )
  endif()
endforeach()