# All #includes in test programs have to be #include <mplot/header.h>
include_directories(BEFORE ${PROJECT_SOURCE_DIR})

set(MATHPLOT_EXAMPLES_MODULES
  ${MPLOT_CORE_MODULES}
  ${MPLOT_READCURVES_MODULES}
  ${MPLOT_ALL_VISUALMODEL_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/compoundray/Visual.cppm
  ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
  ${PROJECT_SOURCE_DIR}/maths/sm/bootstrap.cppm
  ${PROJECT_SOURCE_DIR}/maths/sm/rungekutta4.cppm
  ${SM_HEXFFT_MODULES}
)
list(REMOVE_DUPLICATES MATHPLOT_EXAMPLES_MODULES)
add_library(mathplot_examples STATIC)
target_sources(mathplot_examples PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MATHPLOT_EXAMPLES_MODULES}
)
target_link_libraries(mathplot_examples OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

add_executable(helloworld helloworld.cpp)
target_link_libraries(helloworld mathplot_examples)

add_executable(rod rod.cpp)
target_link_libraries(rod mathplot_examples)

add_executable(rod_with_normals rod_with_normals.cpp)
target_link_libraries(rod_with_normals mathplot_examples)

add_executable(ellipsoid ellipsoid.cpp)
target_link_libraries(ellipsoid mathplot_examples)

add_executable(geodesic geodesic.cpp)
target_link_libraries(geodesic mathplot_examples)

add_executable(vectorvis vectorvis.cpp)
target_link_libraries(vectorvis mathplot_examples)

add_executable(planevis planevis.cpp)
target_link_libraries(planevis mathplot_examples)

add_executable(obb_collisions obb_collisions.cpp)
target_link_libraries(obb_collisions mathplot_examples)

# Demo of sm::rungekutta4, the 4th order Runge-Kutta ODE solver
add_executable(rungekutta rungekutta.cpp)
target_link_libraries(rungekutta mathplot_examples)

# Note that for graph1, we DON'T use the static library mathplot_examples, just as a standalone example
add_executable(graph1 graph1.cpp)
target_sources(graph1 PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/mplot/GraphVisual.cppm
)
target_link_libraries(graph1 OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)

# sm::grid visualization example
add_executable(grid_simple grid_simple.cpp)
target_link_libraries(grid_simple mathplot_examples)

# grid_simple, but orthographic view is the default
add_executable(grid_ortho grid_simple.cpp)
target_compile_definitions(grid_ortho PUBLIC ORTHOGRAPHIC=1)
target_link_libraries(grid_ortho mathplot_examples)

# similar to grid_simple but with a random value for the pixel which highlight better the different perspective
# mplot::Grid is runtime-configured
add_executable(grid_simple_rand grid_simple_rand.cpp)
target_link_libraries(grid_simple_rand mathplot_examples)

# grid_simple_rand, but orthographic view is the default
add_executable(grid_ortho_rand grid_simple_rand.cpp)
target_compile_definitions(grid_ortho_rand PUBLIC ORTHOGRAPHIC=1)
target_link_libraries(grid_ortho_rand mathplot_examples)

add_executable(grid_image grid_image.cpp)
target_link_libraries(grid_image mathplot_examples)

add_executable(grid_border grid_border.cpp)
target_link_libraries(grid_border mathplot_examples)

add_executable(grid_border2 grid_border2.cpp)
target_link_libraries(grid_border2 mathplot_examples)

add_executable(grid_layout grid_layout.cpp)
target_sources(grid_layout PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/maths/sm/grid.cppm
)

if(NOT APPLE)
  # Instancing also not available on Apple (limited to OpenGL 4.1)
  add_executable(breadcrumbs breadcrumbs.cpp)
  target_link_libraries(breadcrumbs mathplot_examples)
endif()

add_executable(showcase showcase.cpp)
target_link_libraries(showcase mathplot_examples)

add_executable(hexgrid hexgrid.cpp)
target_link_libraries(hexgrid mathplot_examples)

add_executable(model_crawler model_crawler.cpp)
target_link_libraries(model_crawler mathplot_examples)

add_executable(voronoi_random voronoi_random.cpp)
target_link_libraries(voronoi_random mathplot_examples)

add_executable(voronoi_random_orbit voronoi_random_orbit.cpp)
target_link_libraries(voronoi_random_orbit mathplot_examples)

# Basic mplot::Visual class example
add_executable(visual visual.cpp)
target_link_libraries(visual mathplot_examples)

add_executable(rosenbrock rosenbrock.cpp)
target_compile_definitions(rosenbrock PUBLIC FLT=float)
target_link_libraries(rosenbrock mathplot_examples)

if(HDF5_FOUND) # anneal.cppm uses HDF5
  add_executable(rosenbrock_asa rosenbrock_asa.cpp)
  target_sources(rosenbrock_asa PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${PROJECT_SOURCE_DIR}/maths/sm/anneal.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/hdfdata.cppm
  )
  target_compile_definitions(rosenbrock_asa PUBLIC FLT=float VISUALISE)
  target_link_libraries(rosenbrock_asa ${HDF5_LIBRARIES} mathplot_examples)
endif()

add_executable(convolve_rect convolve_rect.cpp)
target_link_libraries(convolve_rect mathplot_examples)

add_executable(convolve convolve.cpp)
target_link_libraries(convolve mathplot_examples)

# Plots a surface, counts (and shows) the frames per second
add_executable(fps fps.cpp)
if(OpenMP_CXX_FOUND)
  target_compile_options(fps PRIVATE "${OpenMP_CXX_FLAGS}")
  target_link_libraries(fps OpenMP::OpenMP_CXX mathplot_examples)
else()
  target_link_libraries(fps mathplot_examples)
endif()

add_executable(fps_with_profiler fps_with_profiler.cpp)
if(OpenMP_CXX_FOUND)
  target_compile_options(fps_with_profiler PRIVATE "${OpenMP_CXX_FLAGS}")
  target_link_libraries(fps_with_profiler OpenMP::OpenMP_CXX mathplot_examples)
else()
  target_link_libraries(fps_with_profiler mathplot_examples)
endif()

add_executable(cartgrid cartgrid.cpp)
target_link_libraries(cartgrid mathplot_examples)

# Repeated removal and re-addition of VisualModels to check memory stability
add_executable(cartgrid_colourbar_repeat cartgrid_colourbar_repeat.cpp)
target_link_libraries(cartgrid_colourbar_repeat mathplot_examples)

if(HDF5_FOUND) # anneal.cppm uses HDF5
  set(ANNEAL_ASA_MODULES
    ${PROJECT_SOURCE_DIR}/maths/sm/anneal.cppm
    ${PROJECT_SOURCE_DIR}/maths/sm/hdfdata.cppm
  )
  list(REMOVE_DUPLICATES ANNEAL_ASA_MODULES)

  # Adaptive Simulated Annealing algo on a made up objective
  add_executable(anneal_asa anneal_asa.cpp)
  target_compile_definitions(anneal_asa PUBLIC VISUALISE)
  target_sources(anneal_asa PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${ANNEAL_ASA_MODULES}
  )
  target_link_libraries(anneal_asa ${HDF5_LIBRARIES} mathplot_examples)

  add_executable(anneal_asa_novis anneal_asa.cpp)
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
  add_executable(anneal_boha anneal_asa.cpp)
  target_compile_definitions(anneal_boha PUBLIC USE_BOHACHEVSKY_FUNCTION VISUALISE)
  target_sources(anneal_boha PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${ANNEAL_ASA_MODULES}
  )
  target_link_libraries(anneal_boha ${HDF5_LIBRARIES} mathplot_examples)

endif(HDF5_FOUND)

# Like HexGrid::resampleImage, this one uses an omp call that only works on Mac with
# libomp, so avoid compiling this example on a Mac.
if(NOT APPLE)
  add_executable(grid_resampled_image grid_resampled_image.cpp)
  if(OpenMP_CXX_FOUND)
    target_link_libraries(grid_resampled_image OpenMP::OpenMP_CXX mathplot_examples)
  else()
    target_link_libraries(grid_resampled_image mathplot_examples)
  endif()
endif()

add_executable(unicode_coordaxes unicode_coordaxes.cpp)
target_link_libraries(unicode_coordaxes mathplot_examples)

add_executable(colourbar colourbar.cpp)
target_link_libraries(colourbar mathplot_examples)

add_executable(colourbar_interactive colourbar_interactive.cpp)
target_link_libraries(colourbar_interactive mathplot_examples)

add_executable(scatter_hex_mercator scatter_hex_mercator.cpp)
target_link_libraries(scatter_hex_mercator mathplot_examples)

# Utility prog to test your .svg boundary files
add_executable(show_svg_boundary show_svg_boundary.cpp)
target_link_libraries(show_svg_boundary mathplot_examples)

add_executable(graph_coords graph_coords.cpp)
target_link_libraries(graph_coords mathplot_examples)

add_executable(graph_twinax graph_twinax.cpp)
target_link_libraries(graph_twinax mathplot_examples)

add_executable(graph_spline graph_spline.cpp)
target_link_libraries(graph_spline mathplot_examples)

add_executable(graph_incoming_data graph_incoming_data.cpp)
target_link_libraries(graph_incoming_data mathplot_examples)

add_executable(graph_incoming_data_rescale graph_incoming_data_rescale.cpp)
target_link_libraries(graph_incoming_data_rescale mathplot_examples)

add_executable(graph_line graph_line.cpp)
target_link_libraries(graph_line mathplot_examples)

add_executable(graph_line_xcross graph_line_xcross.cpp)
target_link_libraries(graph_line_xcross mathplot_examples)

add_executable(graph_histo graph_histo.cpp)
target_link_libraries(graph_histo mathplot_examples)

add_executable(graph_distributions graph_distributions.cpp)
target_link_libraries(graph_distributions mathplot_examples)

add_executable(graph_dynamic_x2 graph_dynamic_x2.cpp)
target_link_libraries(graph_dynamic_x2 mathplot_examples)

add_executable(graph_fouraxes graph_fouraxes.cpp)
target_link_libraries(graph_fouraxes mathplot_examples)

add_executable(graph_rightaxis graph_rightaxis.cpp)
target_link_libraries(graph_rightaxis mathplot_examples)

add_executable(graph_dynamic_sine graph_dynamic_sine.cpp)
target_link_libraries(graph_dynamic_sine mathplot_examples)

add_executable(graph_dynamic_sine_rescale graph_dynamic_sine_rescale.cpp)
target_link_libraries(graph_dynamic_sine_rescale mathplot_examples)

add_executable(graph_change_xaxis graph_change_xaxis.cpp)
target_link_libraries(graph_change_xaxis mathplot_examples)

add_executable(graph_bar graph_bar.cpp)
target_link_libraries(graph_bar mathplot_examples)

add_executable(graph_logist graph_logist.cpp)
target_link_libraries(graph_logist mathplot_examples)

add_executable(graph_logist2 graph_logist2.cpp)
target_link_libraries(graph_logist2 mathplot_examples)

add_executable(graph_randomwalk graph_randomwalk.cpp)
target_sources(graph_randomwalk PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES
  ${PROJECT_SOURCE_DIR}/maths/sm/random_walk.cppm
)
target_link_libraries(graph_randomwalk mathplot_examples)

add_executable(linregr linregr.cpp)
target_link_libraries(linregr mathplot_examples)

add_executable(randvec randvec.cpp)
target_link_libraries(randvec mathplot_examples)

##### End graph examples

add_executable(cone cone.cpp)
target_link_libraries(cone mathplot_examples)

add_executable(healpix healpix.cpp)
target_link_libraries(healpix mathplot_examples)

add_executable(jsonconfig jsonconfig.cpp)
target_sources(jsonconfig PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${MPLOT_MATHS_CORE_MODULES}
  ${PROJECT_SOURCE_DIR}/json/src/modules/json.cppm
  ${PROJECT_SOURCE_DIR}/maths/sm/config.cppm
)
target_link_libraries(jsonconfig)

add_executable(ring ring.cpp)
target_link_libraries(ring mathplot_examples)

add_executable(pointrows pointrows.cpp)
target_link_libraries(pointrows mathplot_examples)

add_executable(pointrows_mesh pointrows.cpp)
target_compile_definitions(pointrows_mesh PUBLIC MESH=1)
target_link_libraries(pointrows_mesh mathplot_examples)

add_executable(txt txt.cpp)
target_link_libraries(txt mathplot_examples)

add_executable(tri tri.cpp)
target_link_libraries(tri mathplot_examples)

add_executable(geodesic_ce geodesic_ce.cpp)
target_sources(geodesic_ce PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}
  FILES ${PROJECT_SOURCE_DIR}/mplot/GeodesicVisualCE.cppm
)
target_link_libraries(geodesic_ce mathplot_examples)

add_executable(quads quads.cpp)
target_link_libraries(quads mathplot_examples)

add_executable(quadsmesh quads.cpp)
target_compile_definitions(quadsmesh PUBLIC MESH=1)
target_link_libraries(quadsmesh mathplot_examples)

add_executable(hexgrid_image hexgrid_image.cpp)
if(OpenMP_CXX_FOUND)
  target_link_libraries(hexgrid_image OpenMP::OpenMP_CXX mathplot_examples)
else()
  target_link_libraries(hexgrid_image mathplot_examples)
endif()

add_executable(hexgrid_fft_image hexgrid_fft_image.cpp)
if(OpenMP_CXX_FOUND)
  target_link_libraries(hexgrid_fft_image OpenMP::OpenMP_CXX mathplot_examples)
else()
  target_link_libraries(hexgrid_fft_image mathplot_examples)
endif()

add_executable(hexgrid_image_rect hexgrid_image_rect.cpp)
if(OpenMP_CXX_FOUND)
  target_link_libraries(hexgrid_image_rect OpenMP::OpenMP_CXX mathplot_examples)
else()
  target_link_libraries(hexgrid_image_rect mathplot_examples)
endif()

add_executable(hexgrid_fft_image_rect hexgrid_fft_image_rect.cpp)
if(OpenMP_CXX_FOUND)
  target_link_libraries(hexgrid_fft_image_rect OpenMP::OpenMP_CXX mathplot_examples)
else()
  target_link_libraries(hexgrid_fft_image_rect mathplot_examples)
endif()

add_executable(hexgrid_image_onsphere hexgrid_image_onsphere.cpp)
if(OpenMP_CXX_FOUND)
  target_link_libraries(hexgrid_image_onsphere OpenMP::OpenMP_CXX mathplot_examples)
else()
  target_link_libraries(hexgrid_image_onsphere mathplot_examples)
endif()

add_executable(helloversion helloversion.cpp)
target_link_libraries(helloversion mathplot_examples)

add_executable(myvisual myvisual.cpp)
target_link_libraries(myvisual mathplot_examples)

add_executable(grid_flat_dynamic grid_flat_dynamic.cpp)
target_link_libraries(grid_flat_dynamic mathplot_examples)

add_executable(colourmap_test colourmap_test.cpp)
target_link_libraries(colourmap_test mathplot_examples)

add_executable(colourmap_browser colourmap_browser.cpp)
target_link_libraries(colourmap_browser mathplot_examples)

add_executable(hsvwheel hsvwheel.cpp)
target_link_libraries(hsvwheel mathplot_examples)

add_executable(cyclic_colour cyclic_colour.cpp)
target_link_libraries(cyclic_colour mathplot_examples)

add_executable(colourmaps_desaturating colourmaps_desaturating.cpp)
target_link_libraries(colourmaps_desaturating mathplot_examples)

add_executable(colourmaps_other colourmaps_other.cpp)
target_link_libraries(colourmaps_other mathplot_examples)

add_executable(colourmaps_crameri colourmaps_crameri.cpp)
target_link_libraries(colourmaps_crameri mathplot_examples)

add_executable(colourmaps_cet colourmaps_cet.cpp)
target_link_libraries(colourmaps_cet mathplot_examples)

add_executable(colourmaps_lenthe colourmaps_lenthe.cpp)
target_link_libraries(colourmaps_lenthe mathplot_examples)

add_executable(colourmaps_matplotlib colourmaps_matplotlib.cpp)
target_link_libraries(colourmaps_matplotlib mathplot_examples)

add_executable(colourmaps_mono colourmaps_mono.cpp)
target_link_libraries(colourmaps_mono mathplot_examples)

add_executable(colourmaps_hsv1d colourmaps_hsv1d.cpp)
target_link_libraries(colourmaps_hsv1d mathplot_examples)

add_executable(logisticmap logisticmap.cpp)
target_link_libraries(logisticmap mathplot_examples)

add_executable(quiver quiver.cpp)
target_link_libraries(quiver mathplot_examples)

add_executable(rotating_models rotating_models.cpp)
target_link_libraries(rotating_models mathplot_examples)

add_executable(scatter scatter.cpp)
target_link_libraries(scatter mathplot_examples)

add_executable(scatter_ico scatter_ico.cpp)
target_link_libraries(scatter_ico mathplot_examples)

add_executable(scatter_geodesic scatter_geodesic.cpp)
target_link_libraries(scatter_geodesic mathplot_examples)

# Instancing also not available on Apple (limited to OpenGL 4.1)
if(NOT APPLE)
  add_executable(scatter_instanced scatter_instanced.cpp)
  target_link_libraries(scatter_instanced mathplot_examples)
endif()

add_executable(scatter_dynamic scatter_dynamic.cpp)
target_link_libraries(scatter_dynamic mathplot_examples)

add_executable(duochrome duochrome.cpp)
target_link_libraries(duochrome mathplot_examples)

add_executable(twowindows twowindows.cpp)
target_link_libraries(twowindows mathplot_examples)

add_executable(twowindows_mutex twowindows_mutex.cpp)
target_link_libraries(twowindows_mutex mathplot_examples)

add_executable(threewindows threewindows.cpp)
target_link_libraries(threewindows mathplot_examples)

add_executable(rod_pan rod_pan.cpp)
target_link_libraries(rod_pan mathplot_examples)

add_executable(grating grating.cpp)
target_link_libraries(grating mathplot_examples)

add_executable(sphere sphere.cpp)
target_link_libraries(sphere mathplot_examples)

add_executable(icosahedron icosahedron.cpp)
target_link_libraries(icosahedron mathplot_examples)

add_executable(testce testce.cpp)
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

add_executable(geodesic_with_normals geodesic_with_normals.cpp)
target_link_libraries(geodesic_with_normals mathplot_examples)

add_executable(draw_triangles draw_triangles.cpp)
target_link_libraries(draw_triangles mathplot_examples)

add_executable(draw_triangles_intersections draw_triangles_intersections.cpp)
target_link_libraries(draw_triangles_intersections mathplot_examples)

add_executable(triangle_intersect triangle_intersect.cpp)
target_link_libraries(triangle_intersect mathplot_examples)

add_executable(voronoi_boundary voronoi_boundary.cpp)
target_link_libraries(voronoi_boundary mathplot_examples)

add_executable(voronoi_rectangular voronoi_rectangular.cpp)
target_link_libraries(voronoi_rectangular mathplot_examples)

add_executable(voronoi_vectordata voronoi_vectordata.cpp)
target_link_libraries(voronoi_vectordata mathplot_examples)

add_executable(voronoi_function voronoi_function.cpp)
target_link_libraries(voronoi_function mathplot_examples)

add_executable(voronoi_function_flat voronoi_function_flat.cpp)
target_link_libraries(voronoi_function_flat mathplot_examples)

add_executable(voronoi_delaunay voronoi_delaunay.cpp)
target_link_libraries(voronoi_delaunay mathplot_examples)

add_executable(voronoi_fixed voronoi_fixed.cpp)
target_link_libraries(voronoi_fixed mathplot_examples)

add_executable(voronoi_fixed_xz voronoi_fixed_xz.cpp)
target_link_libraries(voronoi_fixed_xz mathplot_examples)

add_executable(voronoi_fixed_nearlyz voronoi_fixed_nearlyz.cpp)
target_link_libraries(voronoi_fixed_nearlyz mathplot_examples)

add_executable(rectangle rectangle.cpp)
target_link_libraries(rectangle mathplot_examples)

add_executable(rhombo rhombo.cpp)
target_link_libraries(rhombo mathplot_examples)

add_executable(rhombo_scene rhombo_scene.cpp)
target_link_libraries(rhombo_scene mathplot_examples)

add_executable(rhombo_scene2 rhombo_scene2.cpp)
target_link_libraries(rhombo_scene2 mathplot_examples)

add_executable(cube cube.cpp)
target_link_libraries(cube mathplot_examples)

# Eigen finding is not reliable on Github Mac OS runners
if(NOT APPLE)
  if(Eigen3_FOUND)
    add_executable(cubetrans cubetrans.cpp)
    target_include_directories (cubetrans BEFORE PRIVATE ${EIGEN3_INCLUDE_DIR})
    target_link_libraries(cubetrans mathplot_examples)
  endif(Eigen3_FOUND)
endif()

add_executable(cubetrans2 cubetrans2.cpp)
target_link_libraries(cubetrans2 mathplot_examples)

# Demo of sm::math's bootstrap implementation
add_executable(bootstrap bootstrap.cpp)
target_link_libraries(bootstrap mathplot_examples)

add_executable(vvec_convolve vvec_convolve.cpp)
target_link_libraries(vvec_convolve mathplot_examples)

add_executable(vvec_gauss vvec_gauss.cpp)
target_link_libraries(vvec_gauss mathplot_examples)

add_executable(vvec_rgb_gaussians vvec_rgb_gaussians.cpp)
target_link_libraries(vvec_rgb_gaussians mathplot_examples)

add_executable(vvec_smoothgauss vvec_smoothgauss.cpp)
target_link_libraries(vvec_smoothgauss mathplot_examples)

add_executable(quit_callback quit_callback.cpp)
target_link_libraries(quit_callback mathplot_examples)

add_executable(izhikevich izhikevich.cpp)
target_link_libraries(izhikevich mathplot_examples)

add_executable(bools bools.cpp)
target_link_libraries(bools mathplot_examples)

add_executable(izhikevich_alt izhikevich_alt.cpp)
target_link_libraries(izhikevich_alt mathplot_examples)

add_executable(curvytelly curvytelly.cpp)
target_link_libraries(curvytelly mathplot_examples)

add_executable(curvytelly_pipe curvytelly_pipe.cpp)
target_link_libraries(curvytelly_pipe mathplot_examples)

add_executable(curvytelly_chequered_pipe curvytelly_chequered_pipe.cpp)
target_link_libraries(curvytelly_chequered_pipe mathplot_examples)

add_executable(lighting_test lighting_test.cpp)
target_link_libraries(lighting_test mathplot_examples)

add_executable(lines lines.cpp)
target_link_libraries(lines mathplot_examples)

add_executable(line line.cpp)
target_link_libraries(line mathplot_examples)

add_executable(maketicks maketicks.cpp)
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

add_executable(coordarrows coordarrows.cpp)
target_link_libraries(coordarrows mathplot_examples)

add_executable(frames frames.cpp)
target_link_libraries(frames mathplot_examples)

add_executable(sph_to_cart sph_to_cart.cpp)
target_link_libraries(sph_to_cart mathplot_examples)

add_executable(polar polar.cpp)
target_link_libraries(polar mathplot_examples)

add_executable(zernike_radial zernike_radial.cpp)
target_link_libraries(zernike_radial mathplot_examples)

add_executable(zernike zernike.cpp)
target_link_libraries(zernike mathplot_examples)

add_executable(vonmises vonmises.cpp)
target_link_libraries(vonmises mathplot_examples)

add_executable(show_boundingboxes show_boundingboxes.cpp)
target_link_libraries(show_boundingboxes mathplot_examples)

add_executable(trace_boundary trace_boundary.cpp)
target_link_libraries(trace_boundary mathplot_examples)

if (0) # During development



  if(0)
    # I made use of principle component analysis from arma in this example.
    add_executable(ellipse_pca ellipse_pca.cpp)
    target_link_libraries(ellipse_pca ${ARMADILLO_LIBRARY} ${ARMADILLO_LIBRARIES} OpenGL::GL glfw Freetype::Freetype glad mplot_fontdata)
  endif()

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
