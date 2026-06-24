# All #includes in test programs have to be #include <mplot/header.h>
include_directories(BEFORE ${PROJECT_SOURCE_DIR})

# A macro that cleans up the precompiled module files (.pcm files for clang and .gcm for gcc) after building. Enabled by default.
macro (my_add_executable my_target my_sourcefile)
  add_executable(${my_target} ${my_sourcefile})
  if (NOT SAVE_PCM)
  add_custom_command(
    TARGET ${my_target}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E remove ./CMakeFiles/${my_target}.dir/*.pcm
    ARGS -E remove ./CMakeFiles/${my_target}.dir/*.gcm
    COMMENT "Clean pcm/gcm files for ${my_target}"
  )
  endif()
endmacro()

my_add_executable(helloworld helloworld.cpp)
target_sources(helloworld PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_CORE_MODULES})
target_link_libraries(helloworld OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(rod rod.cpp)
target_sources(rod PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${PROJECT_SOURCE_DIR}/mplot/RodVisual.cppm
)
target_link_libraries(rod OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(rod_with_normals rod_with_normals.cpp)
target_sources(rod_with_normals PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${PROJECT_SOURCE_DIR}/mplot/RodVisual.cppm ${PROJECT_SOURCE_DIR}/mplot/NormalsVisual.cppm
  )
target_link_libraries(rod_with_normals OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(ellipsoid ellipsoid.cpp)
target_sources(ellipsoid PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/NormalsVisual.cppm
)
target_link_libraries(ellipsoid OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(geodesic geodesic.cpp)
target_sources(geodesic PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/GeodesicVisual.cppm
)
target_link_libraries(geodesic OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(vectorvis vectorvis.cpp)
target_sources(vectorvis PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
)
target_link_libraries(vectorvis OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(cray_eye cray_eye.cpp)
target_sources(cray_eye PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/SphereVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
  ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
  ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
  ${PROJECT_SOURCE_DIR}/mplot/compoundray/EyeVisual.cppm
)
target_link_libraries(cray_eye OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(graph1 graph1.cpp)
target_sources(graph1 PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
)
target_link_libraries(graph1 OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

# sm::grid visualization example
my_add_executable(grid_simple grid_simple.cpp)
target_sources(grid_simple PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
)
target_link_libraries(grid_simple OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

# grid_simple, but orthographic view is the default
my_add_executable(grid_ortho grid_simple.cpp)
target_compile_definitions(grid_ortho PUBLIC ORTHOGRAPHIC=1)
target_sources(grid_ortho PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
)
target_link_libraries(grid_ortho OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

# similar to grid_simple but with a random value for the pixel which highlight better the different perspective
# mplot::Grid is runtime-configured
my_add_executable(grid_simple_rand grid_simple_rand.cpp)
target_sources(grid_simple_rand PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
)
target_link_libraries(grid_simple_rand OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

# grid_simple_rand, but orthographic view is the default
my_add_executable(grid_ortho_rand grid_simple_rand.cpp)
target_compile_definitions(grid_ortho_rand PUBLIC ORTHOGRAPHIC=1)
target_sources(grid_ortho_rand PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
)
target_link_libraries(grid_ortho_rand OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(grid_image grid_image.cpp)
target_sources(grid_image PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
)
target_link_libraries(grid_image OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(grid_border grid_border.cpp)
target_sources(grid_border PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
)
target_link_libraries(grid_border OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(grid_border2 grid_border2.cpp)
target_sources(grid_border2 PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/NormalsVisual.cppm
)
target_link_libraries(grid_border2 OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(grid_layout grid_layout.cpp)
target_sources(grid_layout PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
)

if(NOT APPLE)
  # Instancing also not available on Apple (limited to OpenGL 4.1)
  my_add_executable(breadcrumbs breadcrumbs.cpp)
  target_sources(breadcrumbs PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/GeodesicVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/InstancedScatterVisual.cppm
  )
  target_link_libraries(breadcrumbs OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
endif()

my_add_executable(showcase showcase.cpp)
target_sources(showcase PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  ${MPLOT_MATHS_HEXGRID_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
  ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
  ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/TriaxesVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/ScatterVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
)
target_link_libraries(showcase OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(hexgrid hexgrid.cpp)
target_sources(hexgrid PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
)
target_link_libraries(hexgrid OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(model_crawler model_crawler.cpp)
target_sources(model_crawler PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/InstancedScatterVisual.cppm
  ${PROJECT_SOURCE_DIR}/mplot/GeodesicVisual.cppm
)
target_link_libraries(model_crawler OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(voronoi_random voronoi_random.cpp)
target_sources(voronoi_random PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES
  ${MPLOT_MATHS_CORE_MODULES}
  ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
  ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
  ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
)
target_link_libraries(voronoi_random OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

my_add_executable(voronoi_random_orbit voronoi_random_orbit.cpp)
target_sources(voronoi_random_orbit PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES
  ${MPLOT_MATHS_CORE_MODULES}
  ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
  ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
  ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
)
target_link_libraries(voronoi_random_orbit OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

if(BUILD_EXAMPLES_EXTRA)
  # Basic mplot::Visual class example
  my_add_executable(visual visual.cpp)
  target_sources(visual PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  )
  target_link_libraries(visual OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(rosenbrock rosenbrock.cpp)
  target_sources(rosenbrock PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/TriFrameVisual.cppm
  )
  target_compile_definitions(rosenbrock PUBLIC FLT=float)
  target_link_libraries(rosenbrock OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  if(HDF5_FOUND) # anneal.cppm uses HDF5
    my_add_executable(rosenbrock_asa rosenbrock_asa.cpp)
    target_sources(rosenbrock_asa PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
      FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
      ${PROJECT_SOURCE_DIR}/maths/sm/anneal.cppm
      ${PROJECT_SOURCE_DIR}/maths/sm/hdfdata.cppm
      ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
      ${PROJECT_SOURCE_DIR}/mplot/PolygonVisual.cppm
      ${PROJECT_SOURCE_DIR}/mplot/TriFrameVisual.cppm
    )
    target_compile_definitions(rosenbrock_asa PUBLIC FLT=float VISUALISE)
    target_link_libraries(rosenbrock_asa ${HDF5_LIBRARIES} OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  endif()

  my_add_executable(convolve_rect convolve_rect.cpp)
  target_sources(convolve_rect PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_CARTGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/CartGridVisual.cppm
  )
  target_link_libraries(convolve_rect OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(convolve convolve.cpp)
  target_sources(convolve PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  )
  target_link_libraries(convolve OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # Plots a surface, counts (and shows) the frames per second
  my_add_executable(fps fps.cpp)
  target_sources(fps PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  )
  if(OpenMP_CXX_FOUND)
    target_link_libraries(fps OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  else()
    target_link_libraries(fps OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  endif()

  my_add_executable(fps_with_profiler fps_with_profiler.cpp)
  target_sources(fps_with_profiler PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/fps/profiler.cppm
  )
  if(OpenMP_CXX_FOUND)
    target_link_libraries(fps_with_profiler OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  else()
    target_link_libraries(fps_with_profiler OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  endif()

  my_add_executable(cartgrid cartgrid.cpp)
  target_sources(cartgrid PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_CARTGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/CartGridVisual.cppm
  )
  target_link_libraries(cartgrid OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # Repeated removal and re-addition of VisualModels to check memory stability
  my_add_executable(cartgrid_colourbar_repeat cartgrid_colourbar_repeat.cpp)
  target_sources(cartgrid_colourbar_repeat PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_CARTGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/CartGridVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
  )
  target_link_libraries(cartgrid_colourbar_repeat OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  if(HDF5_FOUND) # anneal.cppm uses HDF5
    set(ANNEAL_ASA_MODULES
      ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_VISUALMODEL_MODULES}
      ${PROJECT_SOURCE_DIR}/maths/sm/anneal.cppm
      ${PROJECT_SOURCE_DIR}/maths/sm/hdfdata.cppm
      ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
      ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
      ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
      ${PROJECT_SOURCE_DIR}/mplot/PolygonVisual.cppm
      ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
    )
    list(REMOVE_DUPLICATES ANNEAL_ASA_MODULES)

    # Adaptive Simulated Annealing algo on a made up objective
    my_add_executable(anneal_asa anneal_asa.cpp)
    target_compile_definitions(anneal_asa PUBLIC VISUALISE)
    target_sources(anneal_asa PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
      FILES ${ANNEAL_ASA_MODULES}
    )
    target_link_libraries(anneal_asa ${HDF5_LIBRARIES} OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

    my_add_executable(anneal_asa_novis anneal_asa.cpp)
    target_sources(anneal_asa_novis PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
      FILES ${MPLOT_MATHS_CORE_MODULES}
      ${MPLOT_MATHS_HEXGRID_MODULES}
      ${PROJECT_SOURCE_DIR}/maths/sm/anneal.cppm
      ${PROJECT_SOURCE_DIR}/maths/sm/hdfdata.cppm
      ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
      ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
    )
    target_link_libraries(anneal_asa_novis ${HDF5_LIBRARIES})

    # Adaptive Simulated Annealing algo on objective from Bohachevsky
    my_add_executable(anneal_boha anneal_asa.cpp)
    target_compile_definitions(anneal_boha PUBLIC USE_BOHACHEVSKY_FUNCTION VISUALISE)
    target_sources(anneal_boha PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
      FILES ${ANNEAL_ASA_MODULES}
    )
    target_link_libraries(anneal_boha ${HDF5_LIBRARIES} OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  endif(HDF5_FOUND)

  # Like HexGrid::resampleImage, this one uses an omp call that only works on Mac with
  # libomp, so avoid compiling this example on a Mac.
  if(NOT APPLE)
    my_add_executable(grid_resampled_image grid_resampled_image.cpp)
    target_sources(grid_resampled_image PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
      FILES  ${MPLOT_MATHS_CORE_MODULES}
      ${MPLOT_MATHS_VISUALMODEL_MODULES}
      ${MPLOT_CORE_MODULES}
      ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
      ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
    )
    if(OpenMP_CXX_FOUND)
      target_link_libraries(grid_resampled_image OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
    else()
      target_link_libraries(grid_resampled_image OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
    endif()
  endif()

  my_add_executable(unicode_coordaxes unicode_coordaxes.cpp)
  target_sources(unicode_coordaxes PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  )
  target_link_libraries(unicode_coordaxes OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourbar colourbar.cpp)
  target_sources(colourbar PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
  )
  target_link_libraries(colourbar OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourbar_interactive colourbar_interactive.cpp)
  target_sources(colourbar_interactive PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
  )
  target_link_libraries(colourbar_interactive OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(scatter_hex_mercator scatter_hex_mercator.cpp)
  target_sources(scatter_hex_mercator PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ScatterVisual.cppm
  )
  target_link_libraries(scatter_hex_mercator OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # Utility prog to test your .svg boundary files
  my_add_executable(show_svg_boundary show_svg_boundary.cpp)
  target_sources(show_svg_boundary PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  )
  target_link_libraries(show_svg_boundary OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_coords graph_coords.cpp)
  target_sources(graph_coords PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_coords OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_twinax graph_twinax.cpp)
  target_sources(graph_twinax PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_twinax OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_spline graph_spline.cpp)
  target_sources(graph_spline PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/spline.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_spline OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_incoming_data graph_incoming_data.cpp)
  target_sources(graph_incoming_data PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_incoming_data OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_incoming_data_rescale graph_incoming_data_rescale.cpp)
  target_sources(graph_incoming_data_rescale PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_incoming_data_rescale OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_line graph_line.cpp)
  target_sources(graph_line PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_line OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_line_xcross graph_line_xcross.cpp)
  target_sources(graph_line_xcross PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_line_xcross OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_histo graph_histo.cpp)
  target_sources(graph_histo PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_histo OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_distributions graph_distributions.cpp)
  target_sources(graph_distributions PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_distributions OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_dynamic_x2 graph_dynamic_x2.cpp)
  target_sources(graph_dynamic_x2 PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_dynamic_x2 OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_fouraxes graph_fouraxes.cpp)
  target_sources(graph_fouraxes PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_fouraxes OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_rightaxis graph_rightaxis.cpp)
  target_sources(graph_rightaxis PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_rightaxis OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_dynamic_sine graph_dynamic_sine.cpp)
  target_sources(graph_dynamic_sine PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_dynamic_sine OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_dynamic_sine_rescale graph_dynamic_sine_rescale.cpp)
  target_sources(graph_dynamic_sine_rescale PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_dynamic_sine_rescale OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # Next one to do:
  my_add_executable(graph_change_xaxis graph_change_xaxis.cpp)
  target_sources(graph_change_xaxis PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_change_xaxis OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_bar graph_bar.cpp)
  target_sources(graph_bar PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_bar OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_logist graph_logist.cpp)
  target_sources(graph_logist PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(graph_logist OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_logist2 graph_logist2.cpp)
  target_sources(graph_logist2 PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
    ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
  )
  target_link_libraries(graph_logist2 OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(graph_randomwalk graph_randomwalk.cpp)
  target_sources(graph_randomwalk PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/spline.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/random_walk.cppm
  )
  target_link_libraries(graph_randomwalk OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(linregr linregr.cpp)
  target_sources(linregr PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(linregr OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(randvec randvec.cpp)
  target_sources(randvec PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(randvec OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  ##### End graph examples

  my_add_executable(cone cone.cpp)
  target_sources(cone PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ConeVisual.cppm
  )
  target_link_libraries(cone OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(healpix healpix.cpp)
  target_sources(healpix PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
    ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
    ${PROJECT_SOURCE_DIR}/mplot/SphericalProjectionVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/healpix/healpix_bare.cppm
    ${PROJECT_SOURCE_DIR}/mplot/HealpixVisual.cppm
  )
  target_link_libraries(healpix OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(jsonconfig jsonconfig.cpp)
  target_sources(jsonconfig PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
  )
  target_link_libraries(jsonconfig)

  my_add_executable(ring ring.cpp)
  target_sources(ring PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/RingVisual.cppm
  )
  target_link_libraries(ring OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(pointrows pointrows.cpp)
  target_sources(pointrows PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/PointRowsVisual.cppm
  )
  target_link_libraries(pointrows OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(pointrows_mesh pointrows.cpp)
  target_compile_definitions(pointrows_mesh PUBLIC MESH=1)
  target_sources(pointrows_mesh PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/PointRowsMeshVisual.cppm
  )
  target_link_libraries(pointrows_mesh OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(txt txt.cpp)
  target_sources(txt PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/TxtVisual.cppm
  )
  target_link_libraries(txt OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(tri tri.cpp)
  target_sources(tri PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/TriangleVisual.cppm
  )
  target_link_libraries(tri OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(geodesic_ce geodesic_ce.cpp)
  target_sources(geodesic_ce PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/GeodesicVisualCE.cppm
  )
  target_link_libraries(geodesic_ce OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(quads quads.cpp)
  target_sources(quads PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/QuadsVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/QuadsMeshVisual.cppm
  )
  target_link_libraries(quads OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(quadsmesh quads.cpp)
  target_compile_definitions(quadsmesh PUBLIC MESH=1)
  target_sources(quadsmesh PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/QuadsVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/QuadsMeshVisual.cppm
  )
  target_link_libraries(quadsmesh OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(hexgrid_image hexgrid_image.cpp)
  target_sources(hexgrid_image PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  )
  if(OpenMP_CXX_FOUND)
    target_link_libraries(hexgrid_image OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  else()
    target_link_libraries(hexgrid_image OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  endif()

  my_add_executable(hexgrid_image_rect hexgrid_image_rect.cpp)
  target_sources(hexgrid_image_rect PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  )
  if(OpenMP_CXX_FOUND)
    target_link_libraries(hexgrid_image_rect OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  else()
    target_link_libraries(hexgrid_image_rect OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  endif()

  my_add_executable(hexgrid_image_onsphere hexgrid_image_onsphere.cpp)
  target_sources(hexgrid_image_onsphere PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HexGridVisual.cppm
  )
  if(OpenMP_CXX_FOUND)
    target_link_libraries(hexgrid_image_onsphere OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  else()
    target_link_libraries(hexgrid_image_onsphere OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  endif()

  my_add_executable(helloversion helloversion.cpp)
  target_sources(helloversion PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_CORE_MODULES})
  target_link_libraries(helloversion OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(myvisual myvisual.cpp)
  target_sources(myvisual PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_CORE_MODULES})
  target_link_libraries(myvisual OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(grid_flat_dynamic grid_flat_dynamic.cpp)
  target_sources(grid_flat_dynamic PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES  ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
  )
  target_link_libraries(grid_flat_dynamic OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmap_test colourmap_test.cpp)
  target_sources(colourmap_test PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
  )
  target_link_libraries(colourmap_test OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmap_browser colourmap_browser.cpp)
  target_sources(colourmap_browser PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/CyclicColourVisual.cppm
  )
  target_link_libraries(colourmap_browser OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(hsvwheel hsvwheel.cpp)
  target_sources(hsvwheel PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HSVWheelVisual.cppm
  )
  target_link_libraries(hsvwheel OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(cyclic_colour cyclic_colour.cpp)
  target_sources(cyclic_colour PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/CyclicColourVisual.cppm
  )
  target_link_libraries(cyclic_colour OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmaps_desaturating colourmaps_desaturating.cpp)
  target_sources(colourmaps_desaturating PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/HSVWheelVisual.cppm
  )
  target_link_libraries(colourmaps_desaturating OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmaps_other colourmaps_other.cpp)
  target_sources(colourmaps_other PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
  )
  target_link_libraries(colourmaps_other OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmaps_crameri colourmaps_crameri.cpp)
  target_sources(colourmaps_crameri PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
  )
  target_link_libraries(colourmaps_crameri OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmaps_cet colourmaps_cet.cpp)
  target_sources(colourmaps_cet PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
  )
  target_link_libraries(colourmaps_cet OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmaps_lenthe colourmaps_lenthe.cpp)
  target_sources(colourmaps_lenthe PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
  )
  target_link_libraries(colourmaps_lenthe OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmaps_matplotlib colourmaps_matplotlib.cpp)
  target_sources(colourmaps_matplotlib PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
  )
  target_link_libraries(colourmaps_matplotlib OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmaps_mono colourmaps_mono.cpp)
  target_sources(colourmaps_mono PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
  )
  target_link_libraries(colourmaps_mono OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(colourmaps_hsv1d colourmaps_hsv1d.cpp)
  target_sources(colourmaps_hsv1d PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_MATHS_VISUALMODEL_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ColourBarVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/HSVWheelVisual.cppm
  )
  target_link_libraries(colourmaps_hsv1d OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(logisticmap logisticmap.cpp)
  target_sources(logisticmap PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(logisticmap OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(quiver quiver.cpp)
  target_sources(quiver PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/QuiverVisual.cppm
  )
  target_link_libraries(quiver OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(rotating_models rotating_models.cpp)
  target_sources(rotating_models PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GeodesicVisual.cppm
  )
  target_link_libraries(rotating_models OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(scatter scatter.cpp)
  target_sources(scatter PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ScatterVisual.cppm
  )
  target_link_libraries(scatter OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(scatter_ico scatter_ico.cpp)
  target_sources(scatter_ico PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ScatterVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/TriangleVisual.cppm
  )
  target_link_libraries(scatter_ico OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(scatter_geodesic scatter_geodesic.cpp)
  target_sources(scatter_geodesic PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ScatterVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/TriangleVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/QuiverVisual.cppm
  )
  target_link_libraries(scatter_geodesic OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # Instancing also not available on Apple (limited to OpenGL 4.1)
  if(NOT APPLE)
    my_add_executable(scatter_instanced scatter_instanced.cpp)
    target_sources(scatter_instanced PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
      FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
      ${PROJECT_SOURCE_DIR}/mplot/InstancedScatterVisual.cppm
    )
    target_link_libraries(scatter_instanced OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  endif()

  my_add_executable(scatter_dynamic scatter_dynamic.cpp)
  target_sources(scatter_dynamic PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ScatterVisual.cppm
  )
  target_link_libraries(scatter_dynamic OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(duochrome duochrome.cpp)
  target_sources(duochrome PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/ScatterVisual.cppm
  )
  target_link_libraries(duochrome OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(twowindows twowindows.cpp)
  target_sources(twowindows PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/QuiverVisual.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(twowindows OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(twowindows_mutex twowindows_mutex.cpp)
  target_sources(twowindows_mutex PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/QuiverVisual.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(twowindows_mutex OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(threewindows threewindows.cpp)
  target_sources(threewindows PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/QuiverVisual.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(threewindows OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(rod_pan rod_pan.cpp)
  target_sources(rod_pan PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/RodVisual.cppm
  )
  target_link_libraries(rod_pan OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(grating grating.cpp)
  target_sources(grating PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/GratingVisual.cppm
  )
  target_link_libraries(grating OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(sphere sphere.cpp)
  target_sources(sphere PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  )
  target_link_libraries(sphere OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(icosahedron icosahedron.cpp)
  target_sources(icosahedron PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/IcosaVisual.cppm
  )
  target_link_libraries(icosahedron OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(testce testce.cpp)
  target_sources(testce PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${PROJECT_SOURCE_DIR}/maths/sm/mathconst.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/constexpr_math.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/bessel_i0.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/polysolve.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/trait_tests.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/random.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/interval.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/vec.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/vvec.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/algo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/quaternion.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/geometry_polyhedra.cppm
  )

  my_add_executable(geodesic_with_normals geodesic_with_normals.cpp)
  target_sources(geodesic_with_normals PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/GeodesicVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/NormalsVisual.cppm
  )
  target_link_libraries(geodesic_with_normals OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(draw_triangles draw_triangles.cpp)
  target_sources(draw_triangles PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  )
  target_link_libraries(draw_triangles OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(draw_triangles_intersections draw_triangles_intersections.cpp)
  target_sources(draw_triangles_intersections PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/SphereVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
  )
  target_link_libraries(draw_triangles_intersections OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(triangle_intersect triangle_intersect.cpp)
  target_sources(triangle_intersect PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
    ${PROJECT_SOURCE_DIR}/mplot/SphereVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/TriangleVisual.cppm
  )
  target_link_libraries(triangle_intersect OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(voronoi_boundary voronoi_boundary.cpp)
  target_sources(voronoi_boundary PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
    ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
  )
  target_link_libraries(voronoi_boundary OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(voronoi_rectangular voronoi_rectangular.cpp)
  target_sources(voronoi_rectangular PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
    ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
  )
  target_link_libraries(voronoi_rectangular OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(voronoi_vectordata voronoi_vectordata.cpp)
  target_sources(voronoi_vectordata PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
    ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
  )
  target_link_libraries(voronoi_vectordata OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(voronoi_function voronoi_function.cpp)
  target_sources(voronoi_function PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
    ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
  )
  target_link_libraries(voronoi_function OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(voronoi_function_flat voronoi_function_flat.cpp)
  target_sources(voronoi_function_flat PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
    ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
  )
  target_link_libraries(voronoi_function_flat OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(voronoi_fixed voronoi_fixed.cpp)
  target_sources(voronoi_fixed PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
    ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
  )
  target_link_libraries(voronoi_fixed OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(voronoi_fixed_xz voronoi_fixed_xz.cpp)
  target_sources(voronoi_fixed_xz PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
    ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
  )
  target_link_libraries(voronoi_fixed_xz OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(voronoi_fixed_nearlyz voronoi_fixed_nearlyz.cpp)
  target_sources(voronoi_fixed_nearlyz PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${MPLOT_MATHS_CORE_MODULES}
    ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/winder.cppm
    ${PROJECT_SOURCE_DIR}/mplot/jcvoronoi/jc_voronoi.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VoronoiVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
  )
  target_link_libraries(voronoi_fixed_nearlyz OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(rectangle rectangle.cpp)
  target_sources(rectangle PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/RectangleVisual.cppm
  )
  target_link_libraries(rectangle OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(rhombo rhombo.cpp)
  target_sources(rhombo PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/RhomboVisual.cppm
  )
  target_link_libraries(rhombo OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(rhombo_scene rhombo_scene.cpp)
  target_sources(rhombo_scene PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/RhomboVisual.cppm
  )
  target_link_libraries(rhombo_scene OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # This one requires a modular mplot::compoundray::Visual class (FIXME)
  my_add_executable(rhombo_scene2 rhombo_scene2.cpp)
  target_sources(rhombo_scene2 PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/compoundray/Visual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/RhomboVisual.cppm
  )
  target_link_libraries(rhombo_scene2 OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(cube cube.cpp)
  target_sources(cube PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/compoundray/Visual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/RhomboVisual.cppm
  )
  target_link_libraries(cube OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # Eigen finding is not reliable on Github Mac OS runners
  if(NOT APPLE)
    if(Eigen3_FOUND)
      my_add_executable(cubetrans cubetrans.cpp)
      target_sources(cubetrans PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
        FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
        ${PROJECT_SOURCE_DIR}/mplot/compoundray/Visual.cppm
        ${PROJECT_SOURCE_DIR}/mplot/RhomboVisual.cppm
        ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
        ${PROJECT_SOURCE_DIR}/mplot/SphereVisual.cppm
      )
      target_include_directories (cubetrans BEFORE PRIVATE ${EIGEN3_INCLUDE_DIR})
      target_link_libraries(cubetrans OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
    endif(Eigen3_FOUND)
  endif()

  my_add_executable(cubetrans2 cubetrans2.cpp)
  target_sources(cubetrans2 PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES} ${MPLOT_MATHS_HEXGRID_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/compoundray/Visual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/RhomboVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/VectorVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/SphereVisual.cppm
  )
  target_link_libraries(cubetrans2 OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  # Demo of sm::math's bootstrap implementation
  my_add_executable(bootstrap bootstrap.cpp)
  target_sources(bootstrap PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
    ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/bootstrap.cppm
  )
  target_link_libraries(bootstrap OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(vvec_convolve vvec_convolve.cpp)
  target_sources(vvec_convolve PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(vvec_convolve OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(vvec_gauss vvec_gauss.cpp)
  target_sources(vvec_gauss PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(vvec_gauss OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(vvec_rgb_gaussians vvec_rgb_gaussians.cpp)
  target_sources(vvec_rgb_gaussians PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(vvec_rgb_gaussians OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(vvec_smoothgauss vvec_smoothgauss.cpp)
  target_sources(vvec_smoothgauss PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(vvec_smoothgauss OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(quit_callback quit_callback.cpp)
  target_sources(quit_callback PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(quit_callback OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(izhikevich izhikevich.cpp)
  target_sources(izhikevich PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
    ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
  )
  target_link_libraries(izhikevich OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(bools bools.cpp)
  target_sources(bools PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/BoolVisual.cppm
  )
  target_link_libraries(bools OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(izhikevich_alt izhikevich_alt.cpp)
  target_sources(izhikevich_alt PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
    ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
  )
  target_link_libraries(izhikevich_alt OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(curvytelly curvytelly.cpp)
  target_sources(curvytelly  PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/CurvyTellyVisual.cppm
  )
  target_link_libraries(curvytelly OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(curvytelly_pipe curvytelly_pipe.cpp)
  target_sources(curvytelly_pipe  PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/compoundray/Visual.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/CurvyTellyVisual.cppm
  )
  target_link_libraries(curvytelly_pipe OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(curvytelly_chequered_pipe curvytelly_chequered_pipe.cpp)
  target_sources(curvytelly_chequered_pipe  PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/compoundray/Visual.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GridVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/CurvyTellyVisual.cppm
  )
  target_link_libraries(curvytelly_chequered_pipe OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(lighting_test lighting_test.cpp)
  target_sources(lighting_test PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  )
  target_link_libraries(lighting_test OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(lines lines.cpp)
  target_sources(lines PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  )
  target_link_libraries(lines OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(line line.cpp)
  target_sources(line PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  )
  target_link_libraries(line OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(maketicks maketicks.cpp)
  set(MAKETICKS_MODULES
    ${SM_INTERVAL_MODULES}
    ${SM_ALGO_MODULES}
    ${SM_VVEC_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/graphing.cppm
  )
  list(REMOVE_DUPLICATES MAKETICKS_MODULES)
  target_sources(maketicks PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MAKETICKS_MODULES}
  )

  my_add_executable(coordarrows coordarrows.cpp)
  target_sources(coordarrows PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  )
  target_link_libraries(coordarrows OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(frames frames.cpp)
  target_sources(frames PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
  )
  target_link_libraries(frames OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(sph_to_cart sph_to_cart.cpp)
  target_sources(sph_to_cart PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/QuiverVisual.cppm
  )
  target_link_libraries(sph_to_cart OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(polar polar.cpp)
  target_sources(polar PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm #
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm  # reqd by GraphVisual
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm # reqd by PolarVisual
    ${PROJECT_SOURCE_DIR}/mplot/PolarVisual.cppm
  )
  target_link_libraries(polar OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(zernike_radial zernike_radial.cpp)
  target_sources(zernike_radial PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(zernike_radial OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(zernike zernike.cpp)
  target_sources(zernike PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/PolarVisual.cppm
  )
  target_link_libraries(zernike OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(vonmises vonmises.cpp)
  target_sources(vonmises PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(vonmises OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(show_boundingboxes show_boundingboxes.cpp)
  target_sources(show_boundingboxes PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/mplot/RodVisual.cppm
    ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
  )
  target_link_libraries(show_boundingboxes OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(trace_boundary trace_boundary.cpp)
  target_sources(trace_boundary PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(trace_boundary OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(pca1 pca1.cpp)
  target_sources(pca1 PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/maths/tests/sklearn_iris.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/pca.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/ScatterVisual.cppm
    ${PROJECT_SOURCE_DIR}/mplot/TriaxesVisual.cppm
  )
  target_link_libraries(pca1 OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  my_add_executable(pca2 pca2.cpp)
  target_sources(pca2 PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
    ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
    ${PROJECT_SOURCE_DIR}/maths/tests/sklearn_iris.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/pca.cppm
    ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
  )
  target_link_libraries(pca2 OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

  if(ARMADILLO_FOUND)
    # I made use of principle component analysis from arma in this example.
    my_add_executable(ellipse_pca ellipse_pca.cpp)
    target_include_directories(ellipse_pca PUBLIC ${ARMADILLO_INCLUDE_DIR} ${ARMADILLO_INCLUDE_DIRS})
    target_sources(ellipse_pca PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
      FILES ${MPLOT_MATHS_CORE_MODULES} ${MPLOT_CORE_MODULES}
      ${PROJECT_SOURCE_DIR}/maths/sm/histo.cppm
      ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
      ${PROJECT_SOURCE_DIR}/maths/sm/pca.cppm
      ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
    )
    target_link_libraries(ellipse_pca ${ARMADILLO_LIBRARY} ${ARMADILLO_LIBRARIES} OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  endif()

endif()

if (0) # During development

  # Shader compute examples. Not supported on Apple, which is limited to
  # OpenGL 4.1 or Windows because I want to keep the GL header inclusion
  # simple on these examples.
  if(NOT APPLE AND NOT WIN32)
    add_subdirectory(gl_compute)
  endif()

  # Examples showing how to use mathplot with Qt. Compiled only if
  # find_package found Qt5
  if(Qt5_FOUND)
    add_subdirectory(qt)
  endif()

  # Disabled as haven't figured out a GL multi-context scheme for wx windows
  #if(wxWidgets_FOUND)
  #  add_subdirectory(wx)
  #endif()

  if (OpenGL_EGL_FOUND)
    add_subdirectory(pi)
  endif()


  # Screenshots used in the documentation/reference website
  if(BUILD_DOC_SCREENSHOTS)
    add_subdirectory(docs)
  endif()

endif(0)
