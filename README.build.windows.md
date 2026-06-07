# Building with Visual Studio/VSCode

It is possible to build the examples and test programs using Visual Studio/VSCode.

## *Required*: Install dependencies

### Install vcpkg
vcpkg is a package manager for C++ libraries. It can be used to download and install the required dependencies for the examples and test programs.

To install vcpkg, follow the instructions at [vcpkg](https://github.com/microsoft/vcpkg).

After vcpkg is installed, you need to set the environment variable `VCPKG_ROOT` to the path where vcpkg is installed, then set the `CMAKE_TOOLCHAIN_FILE` variable to the path of the vcpkg toolchain file.

In Visual Studio, you can follow [Customize CMake build settings](https://learn.microsoft.com/en-us/cpp/build/customize-cmake-settings?view=msvc-170) to set `CMAKE_TOOLCHAIN_FILE`.

In VSCode, you can set these variables in the `settings.json`:
```json
{
     "cmake.configureSettings": {
         "CMAKE_TOOLCHAIN_FILE": "${env:VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
     },
}
```

### Install dependencies with vcpkg

#### Mainfest mode(recommended)

When `vcpkg.json` is in the root directory of the project, vcpkg will automatically install the required dependencies when you run the CMake configuration step.

#### Classic mode

To install the required dependencies, run the following command in the terminal:

```
vcpkg install armadillo freetype glfw3 hdf5 nlohmann-json opengl rapidxml
```

### Install Qt5

To use Qt, you need to install the Qt SDK using the [official installer](https://download.qt.io/official_releases/online_installers/).

Then add the Qt bin directory to the system PATH environment variable, and set the environment variable `QT5_DIR` to the Qt installation directory.

### Build the examples and test programs

You can build the examples and test programs using the Visual Studio/VSCode(with cmake plugins).

Or you can build in terminal:

``` cmake
cmake -B build
cmake --build build --config Release
```

# Building on Windows Subsystem for Linux

WSL is able to compile a program with OpenGL and display the graphics. In a recent (Feb 2025) test, it appeared to use software rendering, so the frame rate was slow, but, it *did* work.
