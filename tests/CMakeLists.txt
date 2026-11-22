# CTests

# Compile the modules that we use into a library
set(MATHPLOT_TESTS_MODULES
  ${MPLOT_MATHS_CORE_MODULES}
  ${MPLOT_MATHS_READCURVES_MODULES}
  ${MPLOT_READCURVES_MODULES}
  ${MPLOT_MATHS_VISUALMODEL_MODULES}
  ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/QuiverVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/ScatterVisual.cppm
)
list(REMOVE_DUPLICATES MATHPLOT_TESTS_MODULES)
add_library(mathplot_tests STATIC)
target_sources(mathplot_tests PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MATHPLOT_TESTS_MODULES}
)
set(MATHPLOT_EXAMPLES_INCLUDES
  ${PROJECT_SOURCE_DIR}
  ${OPENGL_INCLUDE_DIR} ${GLFW3_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/json/include # for maths_config
)
target_include_directories(mathplot_tests AFTER PRIVATE ${MATHPLOT_EXAMPLES_INCLUDES})
if (NOT HAVE_RAPIDXML)
  target_include_directories(mathplot_tests AFTER PRIVATE ${PROJECT_SOURCE_DIR}/include/rapidxml-1.13)
endif() # else we have it in a directory included by default in the path

# Test reading trial.svg
set(TARGETTEST3 testreadcurves)
set(SOURCETEST3 testreadcurves.cpp)
add_executable(${TARGETTEST3} ${SOURCETEST3})
target_link_libraries(testreadcurves mathplot_tests)
# No longer add this test, because it contains linear curves, which sm::bezcurvepath no longer supports
add_test(testreadcurves ${TARGETTEST3})

# Test reading whiskerbarrels_withcentres.svg
set(TARGETTEST31 testreadcurves_circles)
set(SOURCETEST31 testreadcurves_circles.cpp)
add_executable(${TARGETTEST31} ${SOURCETEST31})
target_link_libraries(testreadcurves_circles mathplot_tests)
add_test(testreadcurves_circles ${TARGETTEST31})

if(${glfw3_FOUND})
  # Test hexgrid3 (hexgrid2 with visualisation)
  add_executable(testhexgrid3 testhexgrid3.cpp)
  target_link_libraries(testhexgrid3 mathplot_tests OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  add_executable(testbighexgrid testbighexgrid.cpp)
  target_link_libraries(testbighexgrid mathplot_tests OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  add_executable(testbezderiv3 testbezderiv3.cpp)
  target_compile_definitions(testbezderiv3 PUBLIC FLT=double)
  target_link_libraries(testbezderiv3 mathplot_tests OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  add_test(testbezderiv3 testbezderiv3)

  add_executable(testVisCoordArrows testVisCoordArrows.cpp)
  target_link_libraries(testVisCoordArrows mathplot_tests OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  add_executable(testVisRemoveModel testVisRemoveModel.cpp)
  target_link_libraries(testVisRemoveModel mathplot_tests OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # Test elliptical HexGrid code (visualized with mplot::Visual)
  add_executable(test_ellipseboundary test_ellipseboundary.cpp)
  target_link_libraries(test_ellipseboundary mathplot_tests OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # Test circular HexGrid code
  add_executable(test_circleboundary test_circleboundary.cpp)
  target_link_libraries(test_circleboundary mathplot_tests OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
endif()

# Test the colour mapping
add_executable(testColourMap testColourMap.cpp)
target_link_libraries(testColourMap mathplot_tests)
add_test(testColourMap testColourMap)

add_executable(testrgbhsv testrgbhsv.cpp)
target_link_libraries(testrgbhsv mathplot_tests)
add_test(testrgbhsv testrgbhsv)

# mplot::tools
add_executable(testTools testTools.cpp)
target_link_libraries(testTools mathplot_tests)
add_test(testTools testTools)

add_executable(testloadpng testloadpng.cpp)
target_link_libraries(testloadpng mathplot_tests)
add_test(testloadpng testloadpng)

add_executable(testgraphnumberformat testgraphnumberformat.cpp)
target_link_libraries(testgraphnumberformat mathplot_tests)
add_test(testgraphnumberformat testgraphnumberformat)

add_executable(testmaketicks testmaketicks.cpp)
target_link_libraries(testmaketicks mathplot_tests)
add_test(testmaketicks testmaketicks)

add_executable(testmakeformatticks testmakeformatticks.cpp)
target_link_libraries(testmakeformatticks mathplot_tests)
add_test(testmakeformatticks testmakeformatticks)

add_executable(unordered_map_vec_set unordered_map_vec_set.cpp)
target_link_libraries(unordered_map_vec_set mathplot_tests)
add_test(unordered_map_vec_set unordered_map_vec_set)
