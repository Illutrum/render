if(EMSCRIPTEN)
  return()
endif()

file(GLOB EXAMPLE_FILES "${FGFX_ROOT}/examples/*.cpp")

foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
  get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
  add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
  target_include_directories(${EXAMPLE_NAME} PRIVATE ${FGFX_INCLUDE} ${FGFX_SRC})
  target_compile_definitions(${EXAMPLE_NAME} PRIVATE __USE_OPENGL)
  set_target_properties(${EXAMPLE_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  )
  target_link_libraries(${EXAMPLE_NAME} PRIVATE
    engine_runtime
    GL png z glfw GLEW
  )
  apply_p2996_target(${EXAMPLE_NAME})
  apply_p2996_rpath_origin(${EXAMPLE_NAME})
  add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${FGFX_ROOT}/examples/assets
      ${CMAKE_BINARY_DIR}/examples/assets
  )
endforeach()

add_executable(test_fgfx
  ${FGFX_ROOT}/tests/test_fgfx.cpp
  ${FGFX_ROOT}/tests/test_file_io.cpp
  ${FGFX_ROOT}/tests/test_shaders.cpp
  ${FGFX_ROOT}/tests/test_bmfont_parse.cpp
  ${FGFX_ROOT}/tests/test_png.cpp
)
target_include_directories(test_fgfx PRIVATE ${FGFX_INCLUDE} ${FGFX_SRC})
target_compile_definitions(test_fgfx PRIVATE
  __USE_OPENGL
  FGFX_SOURCE_DIR="${FGFX_ROOT}"
)
target_link_libraries(test_fgfx PRIVATE engine_runtime png z GL GLEW doctest::doctest)
apply_p2996_target(test_fgfx)
apply_p2996_rpath_origin(test_fgfx)
set_target_properties(test_fgfx PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
add_test(NAME FGFXTests COMMAND $<TARGET_FILE:test_fgfx>)
