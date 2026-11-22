# Build mathplot on Apple Mac

You don't need to *build* mathplot to use the modules, but you *will* need to install the dependencies.

The cmake-driven mathplot build process compiles a set of test and example programs which require all of the dependencies to be met.

You'll need a C++ compiler that can compile C++20 modules. That's *not* Apple Clang provided by XCode at present. Instead, you can `brew install llvm`.

## Installation dependencies for Mac

Install LLVM/Clang version 20 or above by your preferred method.

Installation of most of the other dependencies can be achieved using Mac
ports (Option 1, below). This does lead to the installation of a great deal of
additional software, some of which can conflict with Mac system
software (that's libiconv, in particular). However, a clean install of
Mac ports will successfully install the dependencies for
mathplot. Another managed option to to use the brew system (Option 3, below).

I'd advise you to use Option 1: Mac Ports only if you *already* use Mac Ports, and Option 3 if you already use brew.
Otherwise, prefer Option 2: Manual dependency builds.

### Option 1: Mac Ports

Install Mac ports, following the instructions on
http://www.macports.org/. This will guide you to install the XCode
command line tools, then install the Mac ports installation package.

Finally, use Mac ports to install the rest of the dependencies:

```sh
sudo port install cmake
```

*Be aware that if you have conflicting versions of any of the
 libraries in another location (such as /usr/local), you may run into
 problems during the build.*

Now skip Option 2 and go to **Common manual dependency builds** to
compile glfw3 by hand.

### Option 2: Manual dependency builds

It's much cleaner to build each of the dependencies by hand. That
means first installing cmake, which I do with a binary package from
https://cmake.org/download/ and then compiling any of the deps you need.

After downloading and installing cmake using the MacOS installer, I
add these lines to ~/.zprofile so that I can type cmake at the terminal:

```sh
# Add cmake bin directory to path so you can type 'cmake'
export PATH="/Applications/CMake.app/Contents/bin:${PATH}"
```

### Option 3: brew

The mathplot github actions for mac runners use brew to install
dependencies. The command for the basic dependencies is

```sh
brew install libomp glfw llvm
```

#### Qt5

If present, the Qt5 components Core, Gui and Widgets components are used to compile some example Qt mathplot programs. I've not tested this on Mac.


#### glfw3

The modern OpenGL code in mathplot requires the GL-window managing library GLFW3. If you can't brew install, you can compile it like this:

```
cd ~/src
git clone https://github.com/glfw/glfw.git
cd glfw
mkdir build
cd build
cmake ..
make
sudo make install
```

## Build mathplot on Mac

To build mathplot itself, it's a CMake/ninja process:

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

## Tests

To run the tests you have to build them by changing your cmake command in the recipe above to:
```sh
cmake .. -GNinja -DBUILD_TESTS=ON
```

To run the test suite, use the `ctest` command in the build directory.
