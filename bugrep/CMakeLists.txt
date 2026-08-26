include_directories(BEFORE ${PROJECT_SOURCE_DIR})

add_executable(a a.cpp)
target_sources(a PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES
  ${PROJECT_SOURCE_DIR}/bugrep/B.cppm
)

set(CMAP_MODULES
  ${SM_VEC_MODULES}
  ${SM_FLAGS_MODULES}
  ${SM_CRC32_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/tools.cppm
  ${PROJECT_SOURCE_DIR}/mplot/ColourMap.cppm
  ${PROJECT_SOURCE_DIR}/mplot/colourmaps_cet.cppm
  ${PROJECT_SOURCE_DIR}/mplot/colourmaps_crameri.cppm
  ${PROJECT_SOURCE_DIR}/mplot/ColourMap_Lists.cppm
)
list(REMOVE_DUPLICATES CMAP_MODULES)

add_executable(cmap cmap.cpp)
target_sources(cmap PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR} FILES ${CMAP_MODULES})

set(HEXGRIDBUG_MODULES
${MPLOT_MATHS_CORE_MODULES}
${MPLOT_MATHS_HEXGRID_MODULES}
${MPLOT_MATHS_VISUALMODEL_MODULES}
${MPLOT_CORE_MODULES}
${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
)
list(REMOVE_DUPLICATES HEXGRIDBUG_MODULES)

add_executable(hexgridbug hexgridbug.cpp)
target_sources(hexgridbug PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR} FILES ${HEXGRIDBUG_MODULES})
target_link_libraries(hexgridbug OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

add_executable(124888_visual 124888_main.cpp)
target_sources(124888_visual PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES
  ${PROJECT_SOURCE_DIR}/bugrep/124888_resources.cpp
  ${PROJECT_SOURCE_DIR}/bugrep/124888_specific.cpp
  ${PROJECT_SOURCE_DIR}/bugrep/124888_visual.cpp
)

add_executable(nostrong nostrong_main.cpp)
target_sources(nostrong PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES
  ${SM_MAT_MODULES}
  ${PROJECT_SOURCE_DIR}/bugrep/nostrong_a.cpp
  ${PROJECT_SOURCE_DIR}/bugrep/nostrong_b.cpp
)
