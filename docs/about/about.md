---
title: What is mathplot?
layout: home
permalink: /about
parent: Welcome
nav_order: 1
---
# What is it?
mathplot is a library of **modules-native C++** code for **data visualization**.

mathplot provides **data visualization** for simulations of dynamical systems, agent-based models or, in fact, any program written in C++.

It helps with:

* **OpenGL visualizations of your program while it runs**. A modern OpenGL visualization
  scheme called **[mplot::Visual](/mathplot/ref/visual/)**
  provides the ability to visualise 2D and 3D graphs
  of surfaces, lines, bars, scatter plots and quiver plots with minimal
  processing overhead.

You can also use its sibling, [sebsjames/maths](https://github.com/sebsjames/maths) which gives you the `sm` namespace and:

* **Convenient maths**. Several of the sebsjames/maths modules provide easy ways to do some of the mathematical basics you're going to use regularly. Random number generators, vector maths, simple matrices, ranges, scaling and templated mathematical constants. You don't *have* to use them, but they're available.

There is also:

* **OpenGL shader compute management** mathplot provides a framework for managing shader compute programs in **[mplot::gl::compute_manager](https://github.com/sebsjames/mathplot/blob/main/mplot/gl/compute_manager.h)**. ([Shader compute example](https://github.com/sebsjames/mathplot/blob/main/examples/gl_compute/shadercompute.cpp))

Although this may or may not be split out in the future.