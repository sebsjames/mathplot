# Mathplot: plotting and data visualization for C++

![A banner image mathplot VisualModels](https://github.com/sebsjames/mathplot/blob/main/examples/screenshots/banner2.png?raw=true)

<!-- Currently these two are expected failures - compilers are rejecting valid C++:
![Windows 25 build test](https://github.com/sebsjames/mathplot/actions/workflows/cmake-windows-2025.yml/badge.svg)
![Ubuntu gcc 16 build test](https://github.com/sebsjames/mathplot/actions/workflows/ubuntu-cmakeninja-gcc16.yml/badge.svg)
-->
![Mac OS homebrew clang build test](https://github.com/sebsjames/mathplot/actions/workflows/mac-cmakeninja-clang.yml/badge.svg)
![Ubuntu clang 20 build test](https://github.com/sebsjames/mathplot/actions/workflows/ubuntu-cmakeninja-clang20.yml/badge.svg)
![Ubuntu clang 22 build test](https://github.com/sebsjames/mathplot/actions/workflows/ubuntu-cmakeninja-clang22.yml/badge.svg)

[![Mentioned in Awesome C++](https://awesome.re/mentioned-badge.svg)](https://github.com/myd7349/awesome-cpp?tab=readme-ov-file#data-visualization)

**Modular C++20 library code to visualize numerical simulations using modern OpenGL.**

Mathplot is a library for drawing **3D data visualization** objects such as graphs, surfaces and grids.
Take a look at the [**screenshot gallery**](https://sebsjames.github.io/mathplot/#gallery) to see some examples.

Mathplot is compatible with **Linux** (including **Raspberry Pi**), **Mac OS** and **Windows**, *but* compiler support is still patchy.  On Linux or Mac, you need to use LLVM/clang version 20 or higher (Apple Clang from Xcode does not support C++ modules). GCC is currently [known not to compile the code](https://github.com/sebsjames/mathplot/issues/148). On Windows, [VisualStudio also seems to have a modules-related bug](https://github.com/sebsjames/mathplot/issues/149) as it does not compile the code either.

You'll find all of the **library code** in the [**mplot**](https://github.com/sebsjames/mathplot/tree/main/mplot) directory and you can find **example code** [here](https://github.com/sebsjames/mathplot/tree/main/examples). There is also a **template project** [that uses mathplot](https://github.com/sebsjames/mathplot_template) to help you incorporate the library into your own work.

Mathplot has a **documentation and reference** website at https://sebsjames.github.io/mathplot/.

Mathplot can be **integrated with GUI frameworks** including [Qt](https://doc.qt.io/) (see [**examples/qt/**](https://github.com/sebsjames/mathplot/tree/main/examples/qt)), [wxWidgets](https://www.wxwidgets.org/) (see [**examples/wx/**](https://github.com/sebsjames/mathplot/tree/main/examples/wx)) and [Dear ImGui](https://github.com/ocornut/imgui) (see this [ImGui example](https://github.com/sebsjames/mathplot_imgui)). Dear ImGui is the easiest way to add GUI control to your visualizations.

## Quick Start

### Compilers that compile all the examples

* clang-20
* clang-21
* clang-22
* clang-23 (prerelease)

### Compilers that are almost there

* GCC (just [Bug 124888](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124888) to resolve)
* VisualStudio

This quick start shows dependency installation for Linux, because on this platform, it's a single call to apt (or your favourite package manager). If you're using a Mac, see [README.build.mac](https://github.com/sebsjames/mathplot/tree/main/README.build.mac.md) for help getting dependencies in place. It's [README.build.windows](https://github.com/sebsjames/mathplot/tree/main/README.build.windows.md) for Windows users. For notes on supported compilers, see [README.build.compiler](https://github.com/sebsjames/mathplot/tree/main/README.build.compiler.md)

```bash
# Install dependencies for building graph1.cpp and all the other examples (assuming Debian-like OS)
sudo apt install build-essential cmake git ninja-build \
                 librapidxml-dev freeglut3-dev libglu1-mesa-dev libxmu-dev \
                 libxi-dev libglfw3-dev libfreetype-dev

# Install a very up to date compiler
sudo apt install clang-20 clang-tools-20

git clone --recurse-submodules git@github.com:sebsjames/mathplot   # Get your copy of the mathplot code
cd mathplot
mkdir build         # Create a build directory
cd build
CC=clang-20 CXX=clang++-20 cmake .. -GNinja    # Call cmake to generate the makefiles, specifying clang-20 compiler
ninja graph1        # Compile a single one of the examples. Add VERBOSE=1 to see the compiler commands.
./examples/graph1   # Run the program. You should see a graph of a cubic function.
# After closing the graph1 program, open its source code and modify something (see examples/graph2.cpp for ideas)
gedit ../examples/graph1.cpp
```
The program graph1.cpp is:
```c++
// Visualize a graph. Minimal example showing how a default graph appears
#include <memory>
import mplot.visual;
import mplot.graphvisual; // exports sm.vvec and sm.vec

int main()
{
    // Set up a mplot::Visual 'scene environment'.
    mplot::Visual v(1024, 768, "Made with mplot::GraphVisual");
    // Create a GraphVisual object (obtaining a unique_ptr to the object) with a spatial offset within the scene of 0,0,0
    auto gv = std::make_unique<mplot::GraphVisual<double>> (sm::vec<float>({0,0,0}));
    // This mandatory line of boilerplate code sets the parent ID in GraphVisual
    gv->set_parent (v.get_id());
    // Data for the x axis. A vvec is like std::vector, but with built-in maths methods
    sm::vvec<double> x;
    // This works like numpy's linspace() (the 3 args are "start", "end" and "num"):
    x.linspace (-0.5, 0.8, 14);
    // Set a graph up of y = x^3
    gv->setdata (x, x.pow(3));
    // finalize() makes the GraphVisual compute the vertices of the OpenGL model
    gv->finalize();
    // Add the GraphVisual OpenGL model to the Visual scene, transferring ownership of the unique_ptr
    v.addVisualModel (gv);
    // Render the scene on the screen until user quits with 'Ctrl-q'
    v.keepOpen();
    // Because v owns the unique_ptr to the GraphVisual, its memory will be deallocated when v goes out of scope.
    return 0;
}
```
The program generates a clean looking graph...

![Screenshot of graph1.cpp output showing a cubic function](https://github.com/sebsjames/mathplot/blob/main/examples/screenshots/graph1.png?raw=true)

...and the code is only a few lines longer than an equivalent Python program, graph1.py:
```Python
# Visualize the graph from graph1.cpp in Python
import matplotlib.pyplot as plt
import numpy as np

# Create some data for the x axis
x = np.linspace(-0.5, 0.8, 14)
# Set a graph up of y = x^3
plt.plot(x, np.power(x,3), '-o')
# Add labels
plt.title('Made with Python/numpy/matplotlib')
plt.xlabel('x')
plt.ylabel('y')
# Render the graph on the screen until user quits with 'q'
plt.show()
```
## What is Mathplot?

This modular C++ code provides **dynamic runtime visualization**
for your programs. It was developed to visualize simulations of dynamical
systems and agent-based models in real-time.

A modern OpenGL visualization scheme called
  **[mplot::Visual](https://github.com/sebsjames/mathplot/blob/main/mplot/Visual.h)**
  provides the ability to visualise 2D and 3D graphs of surfaces,
  lines, bars, scatter plots and quiver plots with minimal processing
  overhead. Here's a [mplot::Visual
  helloworld](https://github.com/sebsjames/mathplot/blob/main/examples/helloworld.cpp)
  and [a more complete
  example](https://github.com/sebsjames/mathplot/blob/main/examples/visual.cpp). It's
  almost as easy to [draw a graph in C++ with
  mathplot](https://github.com/sebsjames/mathplot/blob/main/examples/graph1.cpp)
  as it is to do so [in
  Python](https://github.com/sebsjames/mathplot/blob/main/examples/graph1.py).

## Code documentation

See [the reference documentation website](https://sebsjames.github.io/mathplot/) for a guide to the main classes.

Mathplot code is enclosed in the **mplot** namespace. If the reference site doesn't cover it, then the source code (found in [mplot/](https://github.com/sebsjames/mathplot/tree/main/mplot)) contains code documentation.

You can find example programs which are compiled when you do the standard
cmake-driven build of mathplot in both the [tests/](https://github.com/sebsjames/mathplot/tree/main/tests) subdirectory
and the [examples/](https://github.com/sebsjames/mathplot/tree/main/examples) subdirectory.

There is also a template repository that demonstrates how you can create a project that *uses* mathplot: [sebsjames/mathplot_template](https://github.com/sebsjames/mathplot_template).

For more info on how to set up CMake files to build a program using mathplot (and some hints as to what you'll need to do with an alternative directed build system), see [README.cmake.md](https://github.com/sebsjames/mathplot/blob/main/README.cmake.md).

## Credits

Authorship of Mathplot code is given in each file. Copyright in
the software is owned by the authors. Refer to [morphologica](https://github.com/ABRG-Models/morphologica) for historical code authorship details (some of the code distributed here was written by contributors to morphologica). The original idea to use OpenGL to graph hexagonal grids came from [Stuart Wilson](https://github.com/stuartwilson). [Seb James](https://github.com/sebjameswml) reimplemented Stuart's hexgrid plotting code in modern OpenGL and developed morphologica into a general purpose visualization library. In 2025, [Seb](https://github.com/sebsjames) changed the name to *Mathplot* to indicate to new developers what functionality the library provides.

Mathplot is made possible by a number of third party projects whose source code is included in this repository. These include [lodepng](https://github.com/lvandeve/lodepng), [rapidxml](http://rapidxml.sourceforge.net/) and [incbin](https://github.com/graphitemaster/incbin). Colourmaps come from [UniformBicone](https://github.com/wlenthe/UniformBicone), [Fabio Crameri](https://www.fabiocrameri.ch/colourmaps/), [Colorcet](https://colorcet.com/gallery.html) and matplotlib. Thanks to the authors of these projects!

Mathplot is distributed under the terms of the Apache License, version 2 (see
[LICENSE.txt](https://github.com/sebsjames/mathplot/blob/main/LICENSE.txt)).
