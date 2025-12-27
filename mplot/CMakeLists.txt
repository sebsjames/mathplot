#
# This file controls the installation of the header files.
#

# Header installation (list from ls *.h* -1, then re-ordered into functionality groups)
install(
  FILES
  colour.h
  ColourMap.h
  ColourMap_Lists.h
  colourmaps_cet.h
  colourmaps_crameri.h
  GridFeatures.h
  keys.h
  lenthe_colormap.hpp
  loadpng.h
  lodepng.h
  ReadCurves.h
  tools.h
  unicode.h
  version.h

  DatasetStyle.h
  graphing.h
  graphstyles.h
  NavMesh.h
  TextFeatures.h
  TextGeometry.h

  VisualCommon.h
  VisualFont.h
  VisualDefaultShaders.h

  VisualFaceBase.h
  VisualFaceNoMX.h
  VisualFaceMX.h

  VisualResourcesBase.h
  VisualResourcesNoMX.h
  VisualResourcesMX.h

  VisualGlfw.h

  VisualBase.h
  VisualOwnableNoMX.h
  VisualOwnableMX.h
  VisualNoMX.h
  VisualMX.h
  Visual.h

  VisualModelBase.h
  VisualModelImplNoMX.h
  VisualModelImplMX.h
  VisualModel.h

  VisualDataModel.h

  VisualTextModelBase.h
  VisualTextModelImplNoMX.h
  VisualTextModelImplMX.h
  VisualTextModel.h

  CartGridVisual.h
  ColourBarVisual.h
  ConeVisual.h
  ConfigVisual.h
  CoordArrows.h
  CurvyTellyVisual.h
  CyclicColourVisual.h
  GeodesicVisualCE.h
  GeodesicVisual.h
  GraphVisual.h
  GratingVisual.h
  GridVisual.h
  HealpixVisual.h
  HexGridVisual.h
  HSVWheelVisual.h
  IcosaVisual.h
  InstancedScatterVisual.h
  LengthscaleVisual.h
  NormalsVisual.h
  PointRowsMeshVisual.h
  PointRowsVisual.h
  PolarVisual.h
  PolygonVisual.h
  QuadsMeshVisual.h
  QuadsVisual.h
  QuiverVisual.h
  RectangleVisual.h
  RhomboVisual.h
  RingVisual.h
  RodVisual.h
  ScatterVisual.h
  Scatter3Visual.h
  SphereVisual.h
  SphericalProjectionVisual.h
  TriangleVisual.h
  TriaxesVisual.h
  TriFrameVisual.h
  TxtVisual.h
  VectorVisual.h
  VerticesVisual.h
  VoronoiVisual.h

  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mplot
)

if(WIN32)
  # On windows we need mplot/fonts/verafonts.h
  add_subdirectory(fonts)
endif()

# There are also headers in sub directories
add_subdirectory(gl) # GL common and compute code
add_subdirectory(glad) # GLAD header library
add_subdirectory(healpix) # Public, but slightly modified HEALPix headers

# And the jcvoronoi code
add_subdirectory(jcvoronoi)

# The Visual-in-a-Qt-Widget code
add_subdirectory(qt)
# wxWindows
add_subdirectory(wx)

# Compound ray glue code
add_subdirectory(compoundray)

# Install the EXPORT so that mathplot has its own .cmake file and find_package(mathplot) should work
install(FILES mathplot-config.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/mathplot)
#install(EXPORT mathplot DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/mathplot)
