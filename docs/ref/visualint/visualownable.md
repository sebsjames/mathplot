---
title: mplot::VisualOwnable
parent: Internal classes
grand_parent: Reference
permalink: /ref/visualint/visualownable
layout: page
nav_order: 9
---

`VisualOwnable` is the bottom level base class for `mplot::Visual`.
It has functionality that requires OpenGL function calls, but does not add
anything that is specific to a particular windowing system.

This class is a mathplot-internal class and there is typically no
access of its methods in mathplot client code.

However, if you want to incorporate mathplot graphics into another
windowing system, this class provides the drawing functionality. For examples, see [mplot/qt/viswidget.h](https://github.com/sebsjames/mathplot/blob/main/mplot/qt/viswidget.h) for Qt and [mplot/wx/viswx.h](https://github.com/sebsjames/mathplot/blob/main/mplot/wx/viswx.h) for wxWidgets. Note that these have not yet been converted to C++ modules (an exercise for the reader).
