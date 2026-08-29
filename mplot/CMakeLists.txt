#
# This file controls the installation of the header files.
#

# Header installation (list from ls *.h* -1, then re-ordered into functionality groups)
install(
  FILES
  BezCurvePathVisual.cppm
  BoolVisual.cppm
  CartGridVisual.cppm
  CMakeLists.txt
  ColourBarVisual.cppm
  colour.cppm
  ColourMap.cppm
  ColourMap_Lists.cppm
  colourmaps_cet.cppm
  colourmaps_crameri.cppm
  ConeVisual.cppm
  ConfigVisual.cppm
  CoordArrows.cppm
  CurvyTellyVisual.cppm
  CyclicColourVisual.cppm
  DatasetStyle.cppm
  DoubleHexGridVisual.cppm
  GeodesicVisualCE.cppm
  GeodesicVisual.cppm
  graphing.cppm
  graphstyles.cppm
  GraphVisual.cppm
  GratingVisual.cppm
  GridVisual.cppm
  HealpixVisual.cppm
  HexGridVisual.cppm
  HSVWheelVisual.cppm
  IcosaVisual.cppm
  InstancedScatterVisual.cppm
  keys.cppm
  LengthscaleVisual.cppm
  lenthe_colormap.hpp
  loadpng.cppm
  lodepng.h
  mathplot-config.cmake
  NavMesh.cppm
  NormalsVisual.cppm
  PointRowsMeshVisual.cppm
  PointRowsVisual.cppm
  PolarVisual.cppm
  PolygonVisual.cppm
  QuadsMeshVisual.cppm
  QuadsVisual.cppm
  QuiverVisual.cppm
  ReadCurves.cppm
  RectangleVisual.cppm
  RhomboVisual.cppm
  RingVisual.cppm
  RodVisual.cppm
  ScalarVisual.cppm
  ScatterVisual.cppm
  SphereVisual.cppm
  SphericalProjectionVisual.cppm
  TextFeatures.cppm
  TextGeometry.cppm
  tools.cppm
  TriangleVisual.cppm
  TriaxesVisual.cppm
  TriFrameVisual.cppm
  TxtVisual.cppm
  unicode.cppm
  VectorVisual.cppm
  version.cppm
  VerticesVisual.cppm
  VisualCommon.cppm
  Visual.cppm
  VisualDataModel.cppm
  VisualDefaultShaders.hpp
  VisualFaceAsm.cpp
  VisualFaceAsm.hpp
  VisualFace.cppm
  VisualFont.cppm
  VisualGlfw.cppm
  VisualModel.cppm
  VisualOwnable.cppm
  VisualResources.cppm
  VisualTextModel.cppm
  VoronoiVisual.cppm
  win_t.cppm

  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mplot
)

if(WIN32)
  # On windows we need mplot/fonts/verafonts.h
  add_subdirectory(fonts)
endif()

# Build a small library containing the mathplot font binaries
include_directories(BEFORE ${PROJECT_SOURCE_DIR})
add_library(mplot_fontdata VisualFaceAsm.cpp)

# There are also headers in sub directories
add_subdirectory(gl) # GL common and compute code
add_subdirectory(glad) # GLAD header library
add_subdirectory(healpix) # Public, but slightly modified HEALPix headers

# The Visual-in-a-Qt-Widget code
add_subdirectory(qt)
# wxWindows
add_subdirectory(wx)

# Compound ray glue code
add_subdirectory(compoundray)

# FPS profiling
add_subdirectory(fps)

# Install the EXPORT so that mathplot has its own .cmake file and find_package(mathplot) should work
install(FILES mathplot-config.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/mathplot)
#install(EXPORT mathplot DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/mathplot)
