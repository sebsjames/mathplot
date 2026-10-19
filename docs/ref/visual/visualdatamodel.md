---
title: mplot::VisualDataModel
parent: Visualization API
grand_parent: Reference
permalink: /ref/visual/visualdatamodel
layout: page
nav_order: 4
---
```c++
import mplot.visualdatamodel;
```

# Introduction

`mplot::VisualDataModel` is an intermediate `VisualModel` class which manages data structures for several of the VisualModels provided by mathplot.

The idea behind `VisualDataModel` was to make a uniform interface to containers of 3D coordinates and containers for scalar or vector data values to display at those coordinates. The data is referred to by member pointers and then there are [`sm::scale`](https://sebsjames.github.io/maths/ref/scale) objects and a [`mplot::ColourMap`](/ref/visual/colourmap)  that are common across all visualizations.

The VisualModels [`ScatterVisual`](https://github.com/sebsjames/mathplot/blob/main/mplot/ScatterVisual.h), [`QuiverVisual`](https://github.com/sebsjames/mathplot/blob/main/mplot/QuiverVisual.h) [`HexGridVisual`](https://github.com/sebsjames/mathplot/blob/main/mplot/HexGridVisual.h) and [`GridVisual`](https://github.com/sebsjames/mathplot/blob/main/mplot/GridVisual.h) are all derived from `mplot::VisualDataModel`. If you are building a class to visualize 3D scalar or vector fields, you may wish to derive from `VisualDataModel` rather than directly from `mplot::VisualModel`.

# Declaration

```c++
namespace mplot {
    //! Class for VisualModels that visualize data of type T. T is probably float or
    //! double, but may be integer types, too.
    template <typename T, int glver = mplot::gl::version_4_1>
    class VisualDataModel : public VisualModel<glver>
    { ...
```

# Data members

`VisualDataModel` provides these member attributes:

```c++
        ColourMap<float> cm;
```

`VisualDataModel::cm` is a [`mplot::ColourMap`](/ref/visual/colourmap) to be used by any visualization built on this base class.

```c++
        Scale<T, float> colourScale;
        Scale<T, float> colourScale2;
        Scale<T, float> colourScale3;
```
These [`sm::scale`](https://sebsjames.github.io/maths/ref/scale) objects provide a scaling between the data values referred to by `VisualDataModel::scalarData` and `VisualDataModel::vectorData` and the colour map `VisualDataModel::cm`, which takes input in the range [0, 1]. `colourScale2` and `colourScale3` are only required when the ColourMap is a 2D map (of type `ColourMapType::Duochrome` or `ColourMapType::HSV` or a 3D map (`ColourMapType::Trichrome`).

```c++
        sm::scale<T, float> zScale;
```
`zScale` provides a scaling between the values pointed to by `VisualDataModel::scalarData` and the units in the 3D scene. This is important when plotting 3D surfaces and 3D scatter plots.

```c++
        sm::scale<vec<T>> vectorScale;
```
This is a vector scaling which may be used to visualize `vectorData`. It is used when making quiver plots of vector fields.

```c++
        const std::vector<T>* scalarData = nullptr;
```

`scalarData` points to an array of scalar values that form part of a visualization. For example, these could be the values of a 2D scalar field. Scaling to the model coordinate frame is achieved with `VisualDataModel::zScale` and scaling to colours with `VisualDataModel::cm`.

```c++
        const std::vector<vec<T>>* vectorData = nullptr;
```

`scalarData` points to an array of `sm::vec<T, 3>` 3D vectors that form part of a visualization. For example, these could be the values of a vector field. The magnitudes of these vectors may need to be scaled to display them in the model coordinate frame (Hence `VisualDataModel::vectorScale`).

```c++
        std::vector<vec<float>>* dataCoords = nullptr;
```
`dataCoords` is an array of 3D vectors in the model coordinate frame at which the contents of `scalarData` or `vectorData` should be visualized.

# Member methods

Most of the member methods are setters/updaters for the data attributes and their scalings. The pure setters are somewhat redundant, as all the members of `VisualDataModel` are public. However, the update* functions all call `VisualModel::reinit` after changing the data to visualize. These update functions are used when changing a model to display new data from your simulation or data input.