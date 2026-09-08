#
# Define variables of module groups for use by the sebsjames/mathplot
# build process itself, and by client projects.
#

# Sets up variables that define list of the modules used building
# mathplot. Pass in the path to the mathplot root, the sebsjames/maths
# root and to nlohmann/json
macro(setup_module_variables_for_mathplot base_directory maths_directory json_directory)

  # These are set assuming a submoduled maths.
  set(MPLOT_MATHS_CORE_MODULES
    ${maths_directory}/sm/mathconst.cppm
    ${maths_directory}/sm/constexpr_math.cppm
    ${maths_directory}/sm/interval.cppm
    ${maths_directory}/sm/polysolve.cppm
    ${maths_directory}/sm/bessel_i0.cppm
    ${maths_directory}/sm/random.cppm
    ${maths_directory}/sm/vec.cppm
    ${maths_directory}/sm/quaternion.cppm
    ${maths_directory}/sm/mat.cppm
    ${maths_directory}/sm/trait_tests.cppm
    ${maths_directory}/sm/util.cppm
    ${maths_directory}/sm/base64.cppm
    ${maths_directory}/sm/crc32.cppm
    ${maths_directory}/sm/flags.cppm
    ${maths_directory}/sm/algo.cppm
    ${maths_directory}/sm/geometry.cppm
    ${maths_directory}/sm/geometry_polyhedra.cppm
    ${maths_directory}/sm/vvec.cppm
    ${maths_directory}/sm/scale.cppm
    ${maths_directory}/sm/centroid.cppm
    ${maths_directory}/sm/spline.cppm
    ${maths_directory}/sm/winder.cppm
  )

  # The modules required for hexgrids
  set(MPLOT_MATHS_HEXGRID_MODULES
    ${maths_directory}/sm/nm_simplex.cppm
    ${maths_directory}/sm/binomial.cppm
    ${maths_directory}/sm/bezcoord.cppm
    ${maths_directory}/sm/bezcurve.cppm
    ${maths_directory}/sm/bezcurvepath.cppm
    ${maths_directory}/sm/hex.cppm
    ${maths_directory}/sm/hexgrid.cppm
    ${maths_directory}/sm/algo_hexgrid.cppm
  )

  # The modules required for cartgrids
  set(MPLOT_MATHS_CARTGRID_MODULES
    ${maths_directory}/sm/nm_simplex.cppm
    ${maths_directory}/sm/binomial.cppm
    ${maths_directory}/sm/bezcoord.cppm
    ${maths_directory}/sm/bezcurve.cppm
    ${maths_directory}/sm/bezcurvepath.cppm
    ${maths_directory}/sm/boxfilter.cppm
    ${maths_directory}/sm/grid.cppm
    ${maths_directory}/sm/rect.cppm
    ${maths_directory}/sm/cartgrid.cppm
  )

  # Modules in addition to core required for ReadCurves.cppm
  set(MPLOT_MATHS_READCURVES_MODULES
    ${maths_directory}/sm/nm_simplex.cppm
    ${maths_directory}/sm/binomial.cppm
    ${maths_directory}/sm/bezcoord.cppm
    ${maths_directory}/sm/bezcurve.cppm
    ${maths_directory}/sm/bezcurvepath.cppm
  )

  # Maths used in individual VisualModels, but not the mathplot core
  set(MPLOT_MATHS_VISUALMODEL_MODULES
    ${maths_directory}/sm/histo.cppm
    ${maths_directory}/sm/grid.cppm
    ${maths_directory}/sm/nm_simplex.cppm
    ${maths_directory}/sm/bezcoord.cppm
    ${maths_directory}/sm/binomial.cppm
    ${maths_directory}/sm/bezcurve.cppm
    ${maths_directory}/sm/bezcurvepath.cppm
    ${maths_directory}/sm/hex.cppm
    ${maths_directory}/sm/hexgrid.cppm
    ${maths_directory}/sm/jc_voronoi.cppm
  )

  # Base modules for mathplot. With these you can build helloworld and programs containing (most) VisualModels.
  set(MPLOT_CORE_MODULES
    ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_MATHS_CARTGRID_MODULES}
    ${MPLOT_MATHS_HEXGRID_MODULES}
    ${base_directory}/mplot/keys.cppm
    ${base_directory}/mplot/version.cppm
    ${base_directory}/mplot/tools.cppm
    ${base_directory}/mplot/unicode.cppm
    ${base_directory}/mplot/loadpng.cppm
    ${base_directory}/mplot/gl/version.cppm
    ${base_directory}/mplot/gl/util_mx.cppm
    ${base_directory}/mplot/colour.cppm
    ${base_directory}/mplot/win_t.cppm
    ${base_directory}/mplot/CoordArrows.cppm
    ${base_directory}/mplot/VisualOwnable.cppm
    ${base_directory}/mplot/Visual.cppm
    ${base_directory}/mplot/VisualGlfw.cppm
    ${base_directory}/mplot/VisualFace.cppm
    ${base_directory}/mplot/TextFeatures.cppm
    ${base_directory}/mplot/TextGeometry.cppm
    ${base_directory}/mplot/VisualResources.cppm
    ${base_directory}/mplot/VisualCommon.cppm
    ${base_directory}/mplot/VisualFont.cppm
    ${base_directory}/mplot/VisualTextModel.cppm
    ${base_directory}/mplot/VisualModel.cppm
    ${base_directory}/mplot/NavMesh.cppm
    ${base_directory}/mplot/ColourMap.cppm
    ${base_directory}/mplot/colourmaps_cet.cppm
    ${base_directory}/mplot/colourmaps_crameri.cppm
    ${base_directory}/mplot/ColourMap_Lists.cppm
    ${base_directory}/mplot/graphing.cppm
    ${base_directory}/mplot/graphstyles.cppm
    ${base_directory}/mplot/DatasetStyle.cppm
    ${base_directory}/mplot/VisualDataModel.cppm
    ${base_directory}/mplot/healpix/healpix_bare.cppm
    ${base_directory}/mplot/fps/profiler.cppm
    ${json_directory}/src/modules/json.cppm
  )
  list(REMOVE_DUPLICATES MPLOT_CORE_MODULES)

  set(MPLOT_READCURVES_MODULES
    ${base_directory}/mplot/tools.cppm
    ${base_directory}/include/rapidxml-1.13/rapidxml/rapidxml.cppm
    ${base_directory}/mplot/ReadCurves.cppm
  )

  # (Possibly) all the VisualModel modules. Usually, you will make
  # your own list of just the ones you want to compile.
  set(MPLOT_ALL_VISUALMODEL_MODULES
    ${base_directory}/mplot/GraphVisual.cppm
    ${base_directory}/mplot/GridVisual.cppm
    ${base_directory}/mplot/RodVisual.cppm
    ${base_directory}/mplot/RhomboVisual.cppm
    ${base_directory}/mplot/VectorVisual.cppm
    ${base_directory}/mplot/PlaneVisual.cppm
    ${base_directory}/mplot/SphereVisual.cppm
    ${base_directory}/mplot/ConeVisual.cppm
    ${base_directory}/mplot/QuiverVisual.cppm
    ${base_directory}/mplot/NormalsVisual.cppm
    ${base_directory}/mplot/GeodesicVisual.cppm
    ${base_directory}/mplot/InstancedScatterVisual.cppm
    ${base_directory}/mplot/HexGridVisual.cppm
    ${base_directory}/mplot/CartGridVisual.cppm
    ${base_directory}/mplot/TriaxesVisual.cppm
    ${base_directory}/mplot/TxtVisual.cppm
    ${base_directory}/mplot/ScatterVisual.cppm
    ${base_directory}/mplot/CurvyTellyVisual.cppm
    ${base_directory}/mplot/BoolVisual.cppm
    ${base_directory}/mplot/RectangleVisual.cppm
    ${base_directory}/mplot/TriangleVisual.cppm
    ${base_directory}/mplot/VoronoiVisual.cppm
    ${base_directory}/mplot/IcosaVisual.cppm
    ${base_directory}/mplot/GratingVisual.cppm
    ${base_directory}/mplot/ColourBarVisual.cppm
    ${base_directory}/mplot/HSVWheelVisual.cppm
    ${base_directory}/mplot/CyclicColourVisual.cppm
    ${base_directory}/mplot/QuadsVisual.cppm
    ${base_directory}/mplot/QuadsMeshVisual.cppm
    ${base_directory}/mplot/PointRowsVisual.cppm
    ${base_directory}/mplot/PointRowsMeshVisual.cppm
    ${base_directory}/mplot/RingVisual.cppm
    ${base_directory}/mplot/SphericalProjectionVisual.cppm
    ${base_directory}/mplot/HealpixVisual.cppm
    ${base_directory}/mplot/PolygonVisual.cppm
    ${base_directory}/mplot/TriFrameVisual.cppm
    ${base_directory}/mplot/PolarVisual.cppm
  )

endmacro()
