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

if(wxWidgets_FOUND)
  add_subdirectory(wx)
endif()

if (OpenGL_EGL_FOUND)
  add_subdirectory(pi)
endif()

# All #includes in test programs have to be #include <mplot/header.h>
include_directories(BEFORE ${PROJECT_SOURCE_DIR})

# Screenshots used in the documentation/reference website
if(BUILD_DOC_SCREENSHOTS)
  add_subdirectory(docs)
endif()

#
# Any example that uses sm::hexgrid or sm::cartgrid requires
# libarmadillo (because the classes use sm::BezCurve).
#
if(ARMADILLO_FOUND)
  # Basic mplot::Visual class example
  add_executable(visual visual.cpp)
  target_link_libraries(visual OpenGL::GL glfw Freetype::Freetype)

  add_executable(convolve_rect convolve_rect.cpp)
  target_link_libraries(convolve_rect OpenGL::GL glfw Freetype::Freetype)

  add_executable(convolve convolve.cpp)
  target_link_libraries(convolve OpenGL::GL glfw Freetype::Freetype)

  # Adaptive Simulated Annealing algo on a made up objective
  if(HDF5_FOUND)
    add_executable(anneal_asa anneal_asa.cpp)
    target_compile_definitions(anneal_asa PUBLIC VISUALISE)
    target_link_libraries(anneal_asa OpenGL::GL glfw Freetype::Freetype)

    add_executable(anneal_asa_novis anneal_asa.cpp)
    target_link_libraries(anneal_asa_novis)

    # Adaptive Simulated Annealing algo on objective from Bohachevsky
    add_executable(anneal_boha anneal_asa.cpp)
    target_compile_definitions(anneal_boha PUBLIC USE_BOHACHEVSKY_FUNCTION VISUALISE)
    target_link_libraries(anneal_boha OpenGL::GL glfw Freetype::Freetype)
  endif(HDF5_FOUND)

  # Plots a surface, counts (and shows) the frames per second
  add_executable(fps fps.cpp)
  if(OpenMP_CXX_FOUND)
    target_link_libraries(fps OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype)
  else()
    target_link_libraries(fps OpenGL::GL glfw Freetype::Freetype)
  endif()

  add_executable(fps_with_profiler fps_with_profiler.cpp)
  if(OpenMP_CXX_FOUND)
    target_link_libraries(fps_with_profiler OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype)
  else()
    target_link_libraries(fps_with_profiler OpenGL::GL glfw Freetype::Freetype)
  endif()

  add_executable(cartgrid cartgrid.cpp)
  target_link_libraries(cartgrid OpenGL::GL glfw Freetype::Freetype)

  # Repeated removal and re-addition of VisualModels to check memory stability
  add_executable(cartgrid_colourbar_repeat cartgrid_colourbar_repeat.cpp)
  target_link_libraries(cartgrid_colourbar_repeat OpenGL::GL glfw Freetype::Freetype)

  # Like HexGrid::resampleImage, this one uses an omp call that only works on Mac with
  # libomp, so avoid compiling this example on a Mac.
  if(NOT APPLE)
    add_executable(grid_resampled_image grid_resampled_image.cpp)
    if(OpenMP_CXX_FOUND)
      target_link_libraries(grid_resampled_image OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype)
    else()
      target_link_libraries(grid_resampled_image OpenGL::GL glfw Freetype::Freetype)
    endif()
  endif()

  add_executable(hexgrid hexgrid.cpp)
  target_link_libraries(hexgrid OpenGL::GL glfw Freetype::Freetype)

  add_executable(unicode_coordaxes unicode_coordaxes.cpp)
  target_link_libraries(unicode_coordaxes OpenGL::GL glfw Freetype::Freetype)

  add_executable(colourbar colourbar.cpp)
  target_link_libraries(colourbar OpenGL::GL glfw Freetype::Freetype)

  add_executable(colourbar_interactive colourbar_interactive.cpp)
  target_link_libraries(colourbar_interactive OpenGL::GL glfw Freetype::Freetype)

  add_executable(scatter_hex_mercator scatter_hex_mercator.cpp)
  target_link_libraries(scatter_hex_mercator OpenGL::GL glfw Freetype::Freetype)

  add_executable(scatter3 scatter3.cpp)
  target_link_libraries(scatter3 OpenGL::GL glfw Freetype::Freetype)

  add_executable(rosenbrock rosenbrock.cpp)
  target_compile_definitions(rosenbrock PUBLIC FLT=float)
  target_link_libraries(rosenbrock OpenGL::GL glfw Freetype::Freetype)

  if(HDF5_FOUND)
    add_executable(rosenbrock_asa rosenbrock_asa.cpp)
    target_compile_definitions(rosenbrock_asa PUBLIC FLT=float VISUALISE)
    target_link_libraries(rosenbrock_asa OpenGL::GL glfw Freetype::Freetype)
  endif(HDF5_FOUND)

  # Utility prog to test your .svg boundary files
  add_executable(show_svg_boundary show_svg_boundary.cpp)
  target_link_libraries(show_svg_boundary ${ARMADILLO_LIBRARY} ${ARMADILLO_LIBRARIES} OpenGL::GL glfw Freetype::Freetype)

  # HexGrid::resampleImage uses an omp call that only works on Mac with
  # libomp, so avoid compiling this example on a Mac.
  if(NOT APPLE)
    add_executable(hexgrid_image hexgrid_image.cpp)
    if(OpenMP_CXX_FOUND)
      target_link_libraries(hexgrid_image OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype)
    else()
      target_link_libraries(hexgrid_image OpenGL::GL glfw Freetype::Freetype)
    endif()

    add_executable(hexgrid_image_rect hexgrid_image_rect.cpp)
    if(OpenMP_CXX_FOUND)
      target_link_libraries(hexgrid_image_rect OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype)
    else()
      target_link_libraries(hexgrid_image_rect OpenGL::GL glfw Freetype::Freetype)
    endif()

    add_executable(hexgrid_image_onsphere hexgrid_image_onsphere.cpp)
    if(OpenMP_CXX_FOUND)
      target_link_libraries(hexgrid_image_onsphere OpenMP::OpenMP_CXX OpenGL::GL glfw Freetype::Freetype)
    else()
      target_link_libraries(hexgrid_image_onsphere OpenGL::GL glfw Freetype::Freetype)
    endif()
  endif(NOT APPLE)

  add_executable(showcase showcase.cpp)
  target_link_libraries(showcase OpenGL::GL glfw Freetype::Freetype)

endif(ARMADILLO_FOUND)

add_executable(helloworld helloworld.cpp)
target_link_libraries(helloworld OpenGL::GL glfw Freetype::Freetype)

add_executable(helloworld_nomx helloworld_nomx.cpp)
target_link_libraries(helloworld_nomx OpenGL::GL glfw Freetype::Freetype)

add_executable(helloversion helloversion.cpp)
target_link_libraries(helloversion OpenGL::GL glfw Freetype::Freetype)

add_executable(myvisual myvisual.cpp)
target_link_libraries(myvisual OpenGL::GL glfw Freetype::Freetype)

# mplot::Grid is runtime-configured
add_executable(grid_simple grid_simple.cpp)
target_link_libraries(grid_simple OpenGL::GL glfw Freetype::Freetype)

add_executable(grid_flat_dynamic grid_flat_dynamic.cpp)
target_link_libraries(grid_flat_dynamic OpenGL::GL glfw Freetype::Freetype)

add_executable(colourmap_test colourmap_test.cpp)
target_link_libraries(colourmap_test OpenGL::GL glfw Freetype::Freetype)

add_executable(colourmap_browser colourmap_browser.cpp)
target_link_libraries(colourmap_browser OpenGL::GL glfw Freetype::Freetype)

# grid_simple, but orthographic view is the default
add_executable(grid_ortho grid_simple.cpp)
target_compile_definitions(grid_ortho PUBLIC ORTHOGRAPHIC=1)
target_link_libraries(grid_ortho OpenGL::GL glfw Freetype::Freetype)

# similar to grid_simple but with a random value for the pixel which highlight better the different perspective
# mplot::Grid is runtime-configured
add_executable(grid_simple_rand grid_simple_rand.cpp)
target_link_libraries(grid_simple_rand OpenGL::GL glfw Freetype::Freetype)

# grid_simple_rand, but orthographic view is the default
add_executable(grid_ortho_rand grid_simple_rand.cpp)
target_compile_definitions(grid_ortho_rand PUBLIC ORTHOGRAPHIC=1)
target_link_libraries(grid_ortho_rand OpenGL::GL glfw Freetype::Freetype)

add_executable(grid_image grid_image.cpp)
target_link_libraries(grid_image OpenGL::GL glfw Freetype::Freetype)

add_executable(grid_border grid_border.cpp)
target_link_libraries(grid_border OpenGL::GL glfw Freetype::Freetype)

add_executable(grid_border2 grid_border2.cpp)
target_link_libraries(grid_border2 OpenGL::GL glfw Freetype::Freetype)

add_executable(grid_layout grid_layout.cpp)

add_executable(hsvwheel hsvwheel.cpp)
target_link_libraries(hsvwheel OpenGL::GL glfw Freetype::Freetype)

add_executable(cyclic_colour cyclic_colour.cpp)
target_link_libraries(cyclic_colour OpenGL::GL glfw Freetype::Freetype)

add_executable(colourmaps_desaturating colourmaps_desaturating.cpp)
target_link_libraries(colourmaps_desaturating OpenGL::GL glfw Freetype::Freetype)

add_executable(colourmaps_other colourmaps_other.cpp)
target_link_libraries(colourmaps_other OpenGL::GL glfw Freetype::Freetype)

add_executable(colourmaps_crameri colourmaps_crameri.cpp)
target_link_libraries(colourmaps_crameri OpenGL::GL glfw Freetype::Freetype)

add_executable(colourmaps_cet colourmaps_cet.cpp)
target_link_libraries(colourmaps_cet OpenGL::GL glfw Freetype::Freetype)

add_executable(colourmaps_lenthe colourmaps_lenthe.cpp)
target_link_libraries(colourmaps_lenthe OpenGL::GL glfw Freetype::Freetype)

add_executable(colourmaps_matplotlib colourmaps_matplotlib.cpp)
target_link_libraries(colourmaps_matplotlib OpenGL::GL glfw Freetype::Freetype)

if(HAVE_STD_FORMAT)
  add_executable(colourmaps_mono colourmaps_mono.cpp)
  target_link_libraries(colourmaps_mono OpenGL::GL glfw Freetype::Freetype)

  add_executable(colourmaps_hsv1d colourmaps_hsv1d.cpp)
  target_link_libraries(colourmaps_hsv1d OpenGL::GL glfw Freetype::Freetype)
endif()

add_executable(logisticmap logisticmap.cpp)
target_link_libraries(logisticmap OpenGL::GL glfw Freetype::Freetype)

add_executable(quiver quiver.cpp)
target_link_libraries(quiver OpenGL::GL glfw Freetype::Freetype)

add_executable(vectorvis vectorvis.cpp)
target_link_libraries(vectorvis OpenGL::GL glfw Freetype::Freetype)

add_executable(rotating_models rotating_models.cpp)
target_link_libraries(rotating_models OpenGL::GL glfw Freetype::Freetype)

add_executable(scatter scatter.cpp)
target_link_libraries(scatter OpenGL::GL glfw Freetype::Freetype)

# This sphere example uses constexpr code
if(NOT APPLE)
  add_executable(scatter_ico scatter_ico.cpp)
  target_link_libraries(scatter_ico OpenGL::GL glfw Freetype::Freetype)

  add_executable(scatter_geodesic scatter_geodesic.cpp)
  target_link_libraries(scatter_geodesic OpenGL::GL glfw Freetype::Freetype)

  # Instancing also not available on Apple (limited to OpenGL 4.1)
  add_executable(scatter_instanced scatter_instanced.cpp)
  target_link_libraries(scatter_instanced OpenGL::GL glfw Freetype::Freetype)
  add_executable(breadcrumbs breadcrumbs.cpp)
  target_link_libraries(breadcrumbs OpenGL::GL glfw Freetype::Freetype)
endif()

add_executable(scatter_dynamic scatter_dynamic.cpp)
target_link_libraries(scatter_dynamic OpenGL::GL glfw Freetype::Freetype)

add_executable(duochrome duochrome.cpp)
target_link_libraries(duochrome OpenGL::GL glfw Freetype::Freetype)

add_executable(graph1 graph1.cpp)
target_link_libraries(graph1 OpenGL::GL glfw Freetype::Freetype)

# Shows how to write a program that uses non-multicontext aware, globally aliased GL functions:
add_executable(graph1_nomx graph1_nomx.cpp)
target_link_libraries(graph1_nomx OpenGL::GL glfw Freetype::Freetype)

if(HAVE_STD_FORMAT)
  add_executable(graph_line graph_line.cpp)
  target_link_libraries(graph_line OpenGL::GL glfw Freetype::Freetype)

  add_executable(graph_line_xcross graph_line_xcross.cpp)
  target_link_libraries(graph_line_xcross OpenGL::GL glfw Freetype::Freetype)

  add_executable(graph_histo graph_histo.cpp)
  target_link_libraries(graph_histo OpenGL::GL glfw Freetype::Freetype)

  add_executable(graph_distributions graph_distributions.cpp)
  target_link_libraries(graph_distributions OpenGL::GL glfw Freetype::Freetype)
endif()

add_executable(graph_dynamic_x2 graph_dynamic_x2.cpp)
target_link_libraries(graph_dynamic_x2 OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_fouraxes graph_fouraxes.cpp)
target_link_libraries(graph_fouraxes OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_incoming_data graph_incoming_data.cpp)
target_link_libraries(graph_incoming_data OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_incoming_data_rescale graph_incoming_data_rescale.cpp)
target_link_libraries(graph_incoming_data_rescale OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_twinax graph_twinax.cpp)
target_link_libraries(graph_twinax OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_rightaxis graph_rightaxis.cpp)
target_link_libraries(graph_rightaxis OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_dynamic_sine graph_dynamic_sine.cpp)
target_link_libraries(graph_dynamic_sine OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_dynamic_sine_rescale graph_dynamic_sine_rescale.cpp)
target_link_libraries(graph_dynamic_sine_rescale OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_change_xaxis graph_change_xaxis.cpp)
target_link_libraries(graph_change_xaxis OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_bar graph_bar.cpp)
target_link_libraries(graph_bar OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_logist graph_logist.cpp)
target_link_libraries(graph_logist OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_logist2 graph_logist2.cpp)
target_link_libraries(graph_logist2 OpenGL::GL glfw Freetype::Freetype)

add_executable(graph_coords graph_coords.cpp)
target_link_libraries(graph_coords OpenGL::GL glfw Freetype::Freetype)

add_executable(linregr linregr.cpp)
target_link_libraries(linregr OpenGL::GL glfw Freetype::Freetype)

# By default, the multi-context aware GLAD headers are used, so multiple windows is guaranteed to work.
add_executable(twowindows twowindows.cpp)
target_link_libraries(twowindows OpenGL::GL glfw Freetype::Freetype)

# In practice, the non-multicontext headers will work for two windows on the same graphics device.
add_executable(twowindows_nomx twowindows_nomx.cpp)
target_compile_definitions(twowindows_nomx PUBLIC GLAD_OPTION_MX=1)
target_link_libraries(twowindows_nomx OpenGL::GL glfw Freetype::Freetype)

add_executable(twowindows_mutex twowindows_mutex.cpp)
target_link_libraries(twowindows_mutex OpenGL::GL glfw Freetype::Freetype)

add_executable(threewindows threewindows.cpp)
target_link_libraries(threewindows OpenGL::GL glfw Freetype::Freetype)

add_executable(rod rod.cpp)
target_link_libraries(rod OpenGL::GL glfw Freetype::Freetype)

add_executable(rod_with_normals rod_with_normals.cpp)
target_link_libraries(rod_with_normals OpenGL::GL glfw Freetype::Freetype)

add_executable(rod_pan rod_pan.cpp)
target_link_libraries(rod_pan OpenGL::GL glfw Freetype::Freetype)

# This example uses constexpr code
if(NOT APPLE)
  add_executable(grating grating.cpp)
  target_link_libraries(grating OpenGL::GL glfw Freetype::Freetype)
endif()

add_executable(cone cone.cpp)
target_link_libraries(cone OpenGL::GL glfw Freetype::Freetype)

add_executable(ring ring.cpp)
target_link_libraries(ring OpenGL::GL glfw Freetype::Freetype)

# This sphere example uses constexpr code
if(NOT APPLE)
  add_executable(sphere sphere.cpp)
  target_link_libraries(sphere OpenGL::GL glfw Freetype::Freetype)
endif()

add_executable(ellipsoid ellipsoid.cpp)
target_link_libraries(ellipsoid OpenGL::GL glfw Freetype::Freetype)

add_executable(component_models component_models.cpp)
target_link_libraries(component_models OpenGL::GL glfw Freetype::Freetype)

add_executable(component_models2 component_models2.cpp)
target_link_libraries(component_models2 OpenGL::GL glfw Freetype::Freetype)

# constexpr code won't work on Apple Clang
if(NOT APPLE)
  add_executable(icosahedron icosahedron.cpp)
  target_link_libraries(icosahedron OpenGL::GL glfw Freetype::Freetype)

  add_executable(testce testce.cpp)

  add_executable(geodesic geodesic.cpp)
  target_link_libraries(geodesic OpenGL::GL glfw Freetype::Freetype)

  add_executable(geodesic_with_normals geodesic_with_normals.cpp)
  target_link_libraries(geodesic_with_normals OpenGL::GL glfw Freetype::Freetype)

  add_executable(geodesic_ce geodesic_ce.cpp)
  target_link_libraries(geodesic_ce OpenGL::GL glfw Freetype::Freetype)
endif()

add_executable(model_crawler model_crawler.cpp)
target_link_libraries(model_crawler OpenGL::GL glfw Freetype::Freetype)

add_executable(tri tri.cpp)
target_link_libraries(tri OpenGL::GL glfw Freetype::Freetype)

add_executable(draw_triangles draw_triangles.cpp)
target_link_libraries(draw_triangles OpenGL::GL glfw Freetype::Freetype)

add_executable(draw_triangles_intersections draw_triangles_intersections.cpp)
target_link_libraries(draw_triangles_intersections OpenGL::GL glfw Freetype::Freetype)

add_executable(triangle_intersect triangle_intersect.cpp)
target_link_libraries(triangle_intersect OpenGL::GL glfw Freetype::Freetype)

add_executable(voronoi_random voronoi_random.cpp)
target_link_libraries(voronoi_random OpenGL::GL glfw Freetype::Freetype)

add_executable(voronoi_boundary voronoi_boundary.cpp)
target_link_libraries(voronoi_boundary OpenGL::GL glfw Freetype::Freetype)

add_executable(voronoi_rectangular voronoi_rectangular.cpp)
target_link_libraries(voronoi_rectangular OpenGL::GL glfw Freetype::Freetype)

add_executable(voronoi_vectordata voronoi_vectordata.cpp)
target_link_libraries(voronoi_vectordata OpenGL::GL glfw Freetype::Freetype)

add_executable(voronoi_function voronoi_function.cpp)
target_link_libraries(voronoi_function OpenGL::GL glfw Freetype::Freetype)

add_executable(voronoi_function_flat voronoi_function_flat.cpp)
target_link_libraries(voronoi_function_flat OpenGL::GL glfw Freetype::Freetype)

add_executable(voronoi_fixed voronoi_fixed.cpp)
target_link_libraries(voronoi_fixed OpenGL::GL glfw Freetype::Freetype)

add_executable(voronoi_fixed_xz voronoi_fixed_xz.cpp)
target_link_libraries(voronoi_fixed_xz OpenGL::GL glfw Freetype::Freetype)

add_executable(voronoi_fixed_nearlyz voronoi_fixed_nearlyz.cpp)
target_link_libraries(voronoi_fixed_nearlyz OpenGL::GL glfw Freetype::Freetype)

add_executable(rectangle rectangle.cpp)
target_link_libraries(rectangle OpenGL::GL glfw Freetype::Freetype)

add_executable(rhombo rhombo.cpp)
target_link_libraries(rhombo OpenGL::GL glfw Freetype::Freetype)

add_executable(rhombo_scene rhombo_scene.cpp)
target_link_libraries(rhombo_scene OpenGL::GL glfw Freetype::Freetype)

add_executable(rhombo_scene2 rhombo_scene2.cpp)
target_link_libraries(rhombo_scene2 OpenGL::GL glfw Freetype::Freetype)

add_executable(cube cube.cpp)
target_link_libraries(cube OpenGL::GL glfw Freetype::Freetype)

# Eigen finding is not reliable on Github Mac OS runners
if(NOT APPLE)
  if(Eigen3_FOUND)
    add_executable(cubetrans cubetrans.cpp)
    target_include_directories (cubetrans BEFORE PRIVATE ${EIGEN3_INCLUDE_DIR})
    target_link_libraries(cubetrans OpenGL::GL glfw Freetype::Freetype)
  endif(Eigen3_FOUND)
endif()

add_executable(cubetrans2 cubetrans2.cpp)
target_link_libraries(cubetrans2 OpenGL::GL glfw Freetype::Freetype)

add_executable(quads quads.cpp)
target_link_libraries(quads OpenGL::GL glfw Freetype::Freetype)

add_executable(quadsmesh quads.cpp)
target_compile_definitions(quadsmesh PUBLIC MESH=1)
target_link_libraries(quadsmesh OpenGL::GL glfw Freetype::Freetype)

add_executable(pointrows pointrows.cpp)
target_link_libraries(pointrows OpenGL::GL glfw Freetype::Freetype)

add_executable(pointrows_mesh pointrows.cpp)
target_compile_definitions(pointrows_mesh PUBLIC MESH=1)
target_link_libraries(pointrows_mesh OpenGL::GL glfw Freetype::Freetype)

add_executable(jsonconfig jsonconfig.cpp)
target_link_libraries(jsonconfig)

add_executable(randvec randvec.cpp)
target_link_libraries(randvec OpenGL::GL glfw Freetype::Freetype)

# Demo of sm::math's bootstrap implementation
add_executable(bootstrap bootstrap.cpp)
target_link_libraries(bootstrap OpenGL::GL glfw Freetype::Freetype)

add_executable(vvec_convolve vvec_convolve.cpp)
target_link_libraries(vvec_convolve OpenGL::GL glfw Freetype::Freetype)

add_executable(vvec_gauss vvec_gauss.cpp)
target_link_libraries(vvec_gauss OpenGL::GL glfw Freetype::Freetype)


add_executable(vvec_rgb_gaussians vvec_rgb_gaussians.cpp)
target_link_libraries(vvec_rgb_gaussians OpenGL::GL glfw Freetype::Freetype)

add_executable(vvec_smoothgauss vvec_smoothgauss.cpp)
target_link_libraries(vvec_smoothgauss OpenGL::GL glfw Freetype::Freetype)

add_executable(quit_callback quit_callback.cpp)
target_link_libraries(quit_callback OpenGL::GL glfw Freetype::Freetype)

add_executable(izhikevich izhikevich.cpp)
target_link_libraries(izhikevich OpenGL::GL glfw Freetype::Freetype)

add_executable(izhikevich_alt izhikevich_alt.cpp)
target_link_libraries(izhikevich_alt OpenGL::GL glfw Freetype::Freetype)

add_executable(curvytelly curvytelly.cpp)
target_link_libraries(curvytelly OpenGL::GL glfw Freetype::Freetype)

add_executable(curvytelly_pipe curvytelly_pipe.cpp)
target_link_libraries(curvytelly_pipe OpenGL::GL glfw Freetype::Freetype)

add_executable(curvytelly_chequered_pipe curvytelly_chequered_pipe.cpp)
target_link_libraries(curvytelly_chequered_pipe OpenGL::GL glfw Freetype::Freetype)

add_executable(lighting_test lighting_test.cpp)
target_link_libraries(lighting_test OpenGL::GL glfw Freetype::Freetype)

add_executable(healpix healpix.cpp)
target_link_libraries(healpix OpenGL::GL glfw Freetype::Freetype)

add_executable(lines lines.cpp)
target_link_libraries(lines OpenGL::GL glfw Freetype::Freetype)

add_executable(line line.cpp)
target_link_libraries(line OpenGL::GL glfw Freetype::Freetype)

add_executable(txt txt.cpp)
target_link_libraries(txt OpenGL::GL glfw Freetype::Freetype)
target_compile_definitions(txt PUBLIC GLAD_OPTION_MX=1)

add_executable(maketicks maketicks.cpp)

add_executable(coordarrows coordarrows.cpp)
target_link_libraries(coordarrows OpenGL::GL glfw Freetype::Freetype)

add_executable(sph_to_cart sph_to_cart.cpp)
target_link_libraries(sph_to_cart OpenGL::GL glfw Freetype::Freetype)

add_executable(polar polar.cpp)
target_link_libraries(polar OpenGL::GL glfw Freetype::Freetype)

if(HAVE_STD_FORMAT)
  add_executable(zernike_radial zernike_radial.cpp)
  target_link_libraries(zernike_radial OpenGL::GL glfw Freetype::Freetype)

  add_executable(zernike zernike.cpp)
  target_link_libraries(zernike OpenGL::GL glfw Freetype::Freetype)

  add_executable(vonmises vonmises.cpp)
  target_link_libraries(vonmises OpenGL::GL glfw Freetype::Freetype)
endif()

add_executable(show_boundingboxes show_boundingboxes.cpp)
target_link_libraries(show_boundingboxes OpenGL::GL glfw Freetype::Freetype)

# if have compound-ray header
add_executable(cray_eye cray_eye.cpp)
target_link_libraries(cray_eye OpenGL::GL glfw Freetype::Freetype)

if(ARMADILLO_FOUND)
  # Make use of principle component analysis from arma in this example
  add_executable(ellipse_pca ellipse_pca.cpp)
  target_link_libraries(ellipse_pca ${ARMADILLO_LIBRARY} ${ARMADILLO_LIBRARIES} OpenGL::GL glfw Freetype::Freetype)
endif()

add_executable(trace_boundary trace_boundary.cpp)
target_link_libraries(trace_boundary OpenGL::GL glfw Freetype::Freetype)
