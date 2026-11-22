# Building with mathplot on GNU/Linux

You don't need to *build* mathplot to use the modules, but
you *will* need to install the dependencies.

The cmake-driven mathplot build process compiles a set of test and
example programs which require all of the dependencies to be met.

Programs that ```import``` mathplot headers will also need to link to
some or all of those dependencies. Finally, you'll need the cmake
program and a C++ compiler which can compile c++-20 code.

## *Required*: Install dependencies

mathplot code depends on OpenGL, Freetype and glfw3.

### Package-managed dependencies for Ubuntu/Debian

To install the visualization dependencies on Ubuntu or Debian Linux:

```sh
sudo apt install build-essential cmake ninja-build git \
                 freeglut3-dev libglu1-mesa-dev libxmu-dev libxi-dev \
                 libglfw3-dev libfreetype-dev
```
NB: cmake must be version 3.28.5 or higher. On Ubuntu 24.04 the package managed version is too old; I compile the latest CMake from source and install in /usr/local

For the optional dependencies it's:
```sh
sudo apt install qtcreator qtbase5-dev libwxgtk3.2-dev libegl-dev libgbm-dev
```
* Qt library. Installing qtcreator will bring in the Qt5 libraries that are used to compile some Qt-mathplot example programs. It almost certainly possible to install *only* the Qt5 Core, Gui and Widgets libraries, but that hasn't been verified. On recent Ubuntu systems, you may well need qtbase5-dev to get the cmake scripts to `find_package(Qt5...)`.
* WxWindows. libwxgtk3.2-dev (you'll need Ubuntu 23.04+) will enable the compilation of mathplot-wxWidgets example programs.
* EGL. Required to build GLES applications that are compatible with Raspberry Pi 4 and 5.
* GBM. Required only for window-less OpenGL compute compilations. Currently that's one example program only.

### Package-managed dependencies for Arch Linux

On Arch Linux, all required dependencies are available in the official repository. They can be installed as follows:

```shell
sudo pacman -S vtk lapack blas freeglut glfw-wayland
```

**Note:** Specify `glfw-x11` instead of `glfw-wayland` if you use X.org.

## *Optional*: Build mathplot examples or tests

To build the mathplot example programs, it's the usual CMake process:

```sh
cd ~/src
git clone https://github.com/sebsjames/mathplot.git
cd mathplot
git submodule init
git submodule update
mkdir build
cd build
cmake .. -GNinja
ninja
```
### Building test programs (or NOT building the examples)

By default, the example programs are built with the call to `ninja`, but unit test programs are not. To build test programs, and control whether example programs are compiled, use the cmake flags `BUILD_TESTS` and `BUILD_EXAMPLES`, changing your cmake line to:
```sh
cmake .. -GNinja -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF # Build tests but not examples
# ...etc
```

There is also the flag `BUILD_OPTIONAL_EXAMPLES` to enable any programs that use additional dependencies that can't be tested for in the base `CMakeLists.txt`. Currently, that's just one example that uses libgbm.

If you need to build the test programs with a specific compiler, such
as g++-17 or clang, then you just change the cmake call in the recipe
above. It becomes:

```sh
CXX=g++-17 cmake .. -DBUILD_TESTS=ON
```
To run the test suite, use the `ctest` command in the build directory or `make test`.

### Build the client code

See the top level README for a quick description of how to include mathplot in your client code and [README.cmake.md] for more information.
