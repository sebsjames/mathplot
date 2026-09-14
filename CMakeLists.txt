# 3.28.5: C++20 modules support 3.30 gives C++23 import std;
cmake_minimum_required(VERSION 3.28.5)

# For future import std; support
# set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

project(mathplot LANGUAGES CXX C)
# Note that the project version is encoded in mplot/version.h and not in this CMakeLists.txt

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  (This can be changed with `cmake -DCMAKE_INSTALL_PREFIX=/some/place`")

# The use of C++20 modules mandates us to use compilers that are
# recent enough to support C++23. C++23 will allow future use of import std;
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# For future import std; support
# set(CMAKE_CXX_EXTENSIONS ON)
# Tell CMake that we explicitly want `import std`. This will initialize the
# property on all targets declared after this to 1
# set(CMAKE_CXX_MODULE_STD 1)

include(CheckIncludeFileCXX) # CHECK_INCLUDE_FILE_CXX will be used in later scripts

# Add the host definition to CXXFLAGS along with other switches
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES MSVC)
  # Set flags for Windows.
  # -DNOMINMAX to prefer std::min and std::max
  # /EHsc required for use of exceptions
  # /Zc:__cplusplus ensures __cplusplus define does not lie
  # /constexpr:steps is equivalent to -fconstexpr-steps or -fconstexpr-ops-limit
  if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX /EHsc /Zc:__cplusplus /constexpr:steps500000000")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX /EHsc /Zc:__cplusplus /clang:-fconstexpr-steps=500000000")
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES Intel)
  # To use Intel compiler, you can call cmake as: `cmake -DCMAKE_CXX_COMPILER=icpc ..` or `CXX=icpc cmake ..`
  message(WARNING "Intel compiler has not been tested for some time")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++20 -xHOST -O3")
else() # GCC or Clang
  if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.1)
      message("-- GCC version OK. (require 16.1+)")
    else()
      message(WARNING "GCC version 16.1+ is required to compile this project. Alternatively, try clang-20+")
    endif()
  endif()
  # Warnings we suppress:
  # -Wpsabi -- Reason forgotten
  # -Wunknown-pragmas -- because sm::grid has some '#pragma omp' accelerated methods that we don't need in these examples
  set(COMPREHENSIVE_WARNING_FLAGS "-Wall -Wextra -Wpedantic -pedantic-errors -Werror -Wfatal-errors -Wno-psabi -Wno-unknown-pragmas")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ${COMPREHENSIVE_WARNING_FLAGS} -O3")
  if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    # Make it possible to compile complex constexpr functions (gcc only)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconstexpr-ops-limit=5000000000")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconstexpr-steps=500000000")
  endif()
endif()

# In Visual Studio, if CMAKE_BUILD_TYPE is "Debug", some programs will run very slowly
if(WIN32)
  set(CMAKE_BUILD_TYPE "Release")
endif(WIN32)

# Some programs require the ability to define literal template args of
# type std::array (a C++-20 feature). Clang versions less than 18 can't.
set(ARRAY_LITERAL_TEMPL_ARGS_ARE_OK 0)
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 18)
    set(ARRAY_LITERAL_TEMPL_ARGS_ARE_OK 1)
  endif()
else()
  set(ARRAY_LITERAL_TEMPL_ARGS_ARE_OK 1)
endif()

# OpenMP only used for the fps targets
option(USE_OPENMP "Search for, and use OpenMP in sm::grid and the fps example" OFF)
if(USE_OPENMP)
  find_package(OpenMP 3)
endif()

# The code in VisualFace which builds the Vera family truetype fonts
# into the program binary needs to have a define of MPLOT_FONTS_DIR,
# so set it up here:
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMPLOT_FONTS_DIR=\"\\\"${PROJECT_SOURCE_DIR}/fonts\\\"\"")
# Note that in client code you may have to do something similar. For
# example, if you're compiling with mathplot in a subdirectory, you may need:
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMPLOT_FONTS_DIR=\"\\\"${PROJECT_SOURCE_DIR}/mathplot/fonts\\\"\"")

# Lib finding

set(OpenGL_GL_PREFERENCE "GLVND") # Following `cmake --help-policy CMP0072`
if(APPLE OR WIN32)
  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")
  endif()
  find_package(OpenGL REQUIRED)
else()
  find_package(OpenGL REQUIRED EGL) # GL ES 3.1 used in one gl compute example
endif()

find_package(glfw3 3.2 REQUIRED)
# Find the HDF5 library. To prefer the use of static linking of HDF5, set HDF5_USE_STATIC_LIBRARIES first
#set(HDF5_USE_STATIC_LIBRARIES ON)
find_package(HDF5)
if(HDF5_FOUND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HDF5_DEFINITIONS}")
endif()

include_directories(${OPENGL_INCLUDE_DIR})
if(HDF5_FOUND)
  include_directories(${HDF5_INCLUDE_DIR})
endif()
include_directories(${GLFW3_INCLUDE_DIR})

# New, text rendering dependencies
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

# Include the maths subdirectory (sebsjames/maths is git-submoduled)
# If the subdirectory is not available, try to include maths from system
file(GLOB bundled_maths ${PROJECT_SOURCE_DIR}/maths/*)
if(bundled_maths)
  message(STATUS "Using bundled maths")
  include_directories(${PROJECT_SOURCE_DIR}/maths)
else()
  message(STATUS "Using maths from vcpkg")
  find_path(MATHS_INCLUDE_DIRS "sm/algo" REQUIRED)
  include_directories(${MATHS_INCLUDE_DIRS})
endif()

# Use packaged nlohmann json
# find_package(nlohmann_json REQUIRED)
# Now nlohmann is submoduled to get an up-to-date version (for C++ modules)
include_directories("${PROJECT_SOURCE_DIR}/json/include")

# If Qt5 is present, then some optional Qt examples will be compiled
find_package(Qt5 QUIET COMPONENTS Gui Core Widgets)
if(Qt5_FOUND)
  message(STATUS "Compiling Qt examples")
else()
  message(STATUS "NOT compiling Qt examples (Qt5 libraries not found)")
endif()


# wxwidgets. Optional. Use `apt install libwxgtk3.2-dev` on Debian/Ubuntu Linux (Min compatible Ubuntu version is 23.04)
set(WX_OPTIONAL_COMPONENTS "")
find_package(wxWidgets 3.1 COMPONENTS gl core base ${WX_OPTIONAL_COMPONENTS} net QUIET)
if(wxWidgets_FOUND)
  message(STATUS "Compiling wx examples")
else()
  message(STATUS "NOT compiling wx examples (no wxWidgets library found)")
endif()

# rapidxml is bundled in the source, but prefer packaged header
check_include_file_cxx ("rapidxml/rapidxml.hpp" HAVE_RAPIDXML)
if (NOT HAVE_RAPIDXML)
  include_directories("${PROJECT_SOURCE_DIR}/include/rapidxml-1.13")
endif() # else we have it in a directory included by default in the path

#
# sebsjames/maths module groups
#

# Include maths module defs
include(${PROJECT_SOURCE_DIR}/maths/cmake/module_definitions.cmake)
setup_module_variables_for_maths (${PROJECT_SOURCE_DIR}/maths ${PROJECT_SOURCE_DIR}/json)

# Include our own module defs
include(${PROJECT_SOURCE_DIR}/cmake/module_definitions.cmake)
setup_module_variables_for_mathplot (${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/maths ${PROJECT_SOURCE_DIR}/json)


# All the mathplot headers are here
add_subdirectory(mplot)

# Unit testing using the ctest framework
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
  message ("-- Building test programs (-DBUILD_TESTS=ON)")
  enable_testing()
  add_subdirectory(tests)
endif(BUILD_TESTS)

option(BUILD_BUGREP "Build bugrep" OFF)
if(BUILD_BUGREP)
  # bug reproduction
  message ("-- Building bug report programs (-DBUILD_BUGREP=ON)")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -freport-bug")
  endif()
  add_subdirectory(bugrep)
endif(BUILD_BUGREP)

# incbin for Visual Studio; code-generation for colour tables
option(BUILD_UTILS "Build developer utils" OFF)
if(BUILD_UTILS)
  add_subdirectory(buildtools)
endif(BUILD_UTILS)

# Install the font files for program that need to to work with an
# *installed* mathplot, as opposed to an in-tree mathplot.
add_subdirectory(fonts)

# Example code (you can also see tests/ for examples)
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(BUILD_EXAMPLES)

# Additional examples (requires BUILD_EXAMPLES ON)
option(BUILD_DOC_SCREENSHOTS "Build documentation screenshot examples" OFF)

# Some examples may require additional dependencies such as libgbm that can't be detected via
# a typical find_package() call. Make these optional with a cmake flag.
option(BUILD_OPTIONAL_EXAMPLES "Build examples that require additional non-find_package-able dependencies" OFF)

# first we can indicate the documentation build as an option and set it to ON by default
option(BUILD_DOC "Build documentation" OFF)
if(BUILD_DOC)
  # check if Doxygen is installed
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the application
    add_custom_target(doc_doxygen ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM )
  else(DOXYGEN_FOUND)
    message("Need doxygen for documentation")
  endif(DOXYGEN_FOUND)
endif(BUILD_DOC)

# For debugging of variables:
option(DEBUG_VARIABLES OFF)
if(DEBUG_VARIABLES)
  get_cmake_property(_variableNames VARIABLES)
  foreach(_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
  endforeach()
endif(DEBUG_VARIABLES)
