# mathplot: plotting and data visualization for c++

![A banner image morphologica VisualModels](https://github.com/ABRG-Models/morphologica/blob/main/examples/screenshots/banner2.png?raw=true)

**Header-only library code to visualize C++ numerical simulations using fast, modern OpenGL.**

Mathplot is a library for drawing 3D data visualization objects called `VisualModels`.

It used to be called morphologica...

It can also be integrated with other GUI frameworks including Qt (see [**examples/qt/**](https://github.com/ABRG-Models/morphologica/tree/main/examples/qt)) and [wxWidgets](https://www.wxwidgets.org/) (see [**examples/wx/**](https://github.com/ABRG-Models/morphologica/tree/main/examples/wx)).

Mathplot is compatible with **Linux** (including **Raspberry Pi**), **Mac OS** and **Windows**.

You'll find all of the **library code** in the [**morph**](https://github.com/ABRG-Models/morphologica/tree/main/morph) directory and you can find **example code and screenshots** [here](https://github.com/ABRG-Models/morphologica/tree/main/examples). There is also a **template project** [that uses morphologica](https://github.com/ABRG-Models/morphologica_template) to help you incorporate the library into your own work.

morphologica has a **documentation and reference** website at https://abrg-models.github.io/morphologica/.

## Quick Start

This quick start shows dependency installation for Linux, because on this platform, it's a single call to apt (or your favourite package manager). If you're using a Mac, see [README.build.mac](https://github.com/ABRG-Models/morphologica/tree/main/README.build.mac.md) for help getting dependencies in place. It's [README.build.windows](https://github.com/ABRG-Models/morphologica/tree/main/README.build.windows.md) for Windows users. For notes on supported compilers, see [README.build.compiler](https://github.com/ABRG-Models/morphologica/tree/main/README.build.compiler.md)

```bash
# Install dependencies for building graph1.cpp and (almost) all the other examples (assuming Debian-like OS)
sudo apt install build-essential cmake git wget \
                 nlohmann-json3-dev librapidxml-dev \
                 freeglut3-dev libglu1-mesa-dev libxmu-dev libxi-dev \
                 libglfw3-dev libfreetype-dev libarmadillo-dev libhdf5-dev

git clone --recurse-submodules git@github.com:sebjames/mathplot   # Get your copy of the morphologica code
cd mathplot
mkdir build         # Create a build directory
cd build
cmake ..            # Call cmake to generate the makefiles
make graph1         # Compile a single one of the examples. Add VERBOSE=1 to see the compiler commands.
./examples/graph1   # Run the program. You should see a graph of a cubic function.
# After closing the graph1 program, open its source code and modify something (see examples/graph2.cpp for ideas)
gedit ../examples/graph1.cpp
```
The program graph1.cpp is:
```c++
// Visualize a graph. Minimal example showing how a default graph appears
#include <morph/Visual.h>
#include <morph/GraphVisual.h>
#include <morph/vvec.h>

int main()
{
    // Set up a morph::Visual 'scene environment'.
    morph::Visual v(1024, 768, "Made with morph::GraphVisual");
    // Create a new GraphVisual object with offset within the scene of 0,0,0
    auto gv = std::make_unique<morph::GraphVisual<double>> (morph::vec<float>({0,0,0}));
    // Boilerplate bindmodel function call - do this for every model you add to a Visual
    v.bindmodel (gv);
    // Data for the x axis. A vvec is like std::vector, but with built-in maths methods
    morph::vvec<double> x;
    // This works like numpy's linspace() (the 3 args are "start", "end" and "num"):
    x.linspace (-0.5, 0.8, 14);
    // Set a graph up of y = x^3
    gv->setdata (x, x.pow(3));
    // finalize() makes the GraphVisual compute the vertices of the OpenGL model
    gv->finalize();
    // Add the GraphVisual OpenGL model to the Visual scene (which takes ownership of the unique_ptr)
    v.addVisualModel (gv);
    // Render the scene on the screen until user quits with 'Ctrl-q'
    v.keepOpen();
    return 0;
}
```
The program generates a clean looking graph...

![Screenshot of graph1.cpp output showing a cubic function](https://github.com/ABRG-Models/morphologica/blob/main/examples/screenshots/graph1.png?raw=true)

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
See the [coding README](https://github.com/ABRG-Models/morphologica/blob/main/README.coding.md) for a description of some of the main classes that morphologica provides and the [reference website](https://abrg-models.github.io/morphologica/) for more comprehensive information.

## What is morphologica?

This header-only C++ code provides **dynamic runtime visualization** for simulations of dynamical systems and agent-based models.

It helps with:

* **Visualizing your model while it runs**. A modern OpenGL visualization
  scheme called **[morph::Visual](https://github.com/ABRG-Models/morphologica/blob/main/morph/Visual.h)**
  provides the ability to visualise 2D and 3D graphs
  of surfaces, lines, bars, scatter plots and quiver plots with minimal
  processing overhead. Here's a [morph::Visual helloworld](https://github.com/ABRG-Models/morphologica/blob/main/examples/helloworld.cpp) and [a more complete example](https://github.com/ABRG-Models/morphologica/blob/main/examples/visual.cpp). It's almost as easy to [draw a graph in C++ with morphologica](https://github.com/ABRG-Models/morphologica/blob/main/examples/graph1.cpp) as it is to do so [in Python](https://github.com/ABRG-Models/morphologica/blob/main/examples/graph1.py).

## Code documentation

See [the reference documentation website](https://abrg-models.github.io/morphologica/) for a guide to the main classes.

morphologica code is enclosed in the **morph** namespace. If the reference site doesn't cover it, then the header files (They're all in [morph/](https://github.com/ABRG-Models/morphologica/tree/main/morph)) contain code documentation.

You can find example programs which are compiled when you do the standard
cmake-driven build of morphologica in both the [tests/](https://github.com/ABRG-Models/morphologica/tree/main/tests) subdirectory
and the [examples/](https://github.com/ABRG-Models/morphologica/tree/main/examples) subdirectory. The readme in examples/ has some nice
screenshots.

For full, compilable, standalone examples of the code, see the
[standalone_examples/](https://github.com/ABRG-Models/morphologica/tree/main/standalone_examples) subdirectory. You can use these as templates for creating
your own projects that use morphologica library code.

There is also a template repository that demonstrates how you can create a project that *uses* morphologica: [ABRG-Models/morphologica_template](https://github.com/ABRG-Models/morphologica_template).

For more info on how to set up CMake files to build a program using morphologica (and some hints as to what you'll need to do with an alternative directed build system), see [README.cmake.md](https://github.com/ABRG-Models/morphologica/blob/main/README.cmake.md).

## Credits

Authorship of morphologica code is given in each file. Copyright in
the software is owned by the authors.

morphologica is made possible by a number of third party projects whose source code is included in this repository. These include [lodepng](https://github.com/lvandeve/lodepng), [rapidxml](http://rapidxml.sourceforge.net/), [incbin](https://github.com/graphitemaster/incbin), [UniformBicone](https://github.com/wlenthe/UniformBicone), [jcvoronoi](https://github.com/JCash/voronoi) and the [HEALPix implementation from Astrometry.net](https://astrometry.net/). Thanks to the authors of these projects!

morphologica is distributed under the terms of the Apache License, version 2 (see
[LICENSE.txt](https://github.com/ABRG-Models/morphologica/blob/main/LICENSE.txt)).
