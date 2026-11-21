install(FILES gl.h gl_mx.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mplot/glad)

# Build glad library here
add_library(glad STATIC gl.c)
target_include_directories(glad AFTER PRIVATE ${PROJECT_SOURCE_DIR})
