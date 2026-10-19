---
title: mplot::VisualResources
parent: Internal classes
grand_parent: Reference
permalink: /ref/visualint/visualresources
layout: page
nav_order: 1
---
```c++
import mplot.visualresources;
```

`mplot::VisualResources` manages per-program resources. It manages the FreeType fonts in your program (by managing a map of `mplot::gl::VisualFace` objects).

This class is a mathplot-internal class and there is typically no
access of its methods in mathplot client code.

`VisualResources` is a singleton class, accessed via the static
instance function `VisualResources::i()`. It comes into existence when
the first `mplot::Visual` accesses it for the first time in a program.

Freetype library management occurs in
`VisualResources::freetype_init(Visual<>*)`. Originally I believed
that I required only a single Freetype library instance (allocated
with the function `FT_Init_FreeType`). This was the reason for carrying
out Freetype library init within the VisualResources class. However, I
found it necessary to initialize one Freetype library instance for
*each* `mplot::Visual`. This suggests that a better design would be
for `mplot::Visual` to own the freetype library pointer/memory. For
now, Freetype init remains in `VisualResources`. The Freetype library
instances are stored in a member attribute which is a map of pointers:
`VisualResources::freetypes`. This map uses the `Visual` instance
address pointers as a key.

Another member attribute is `faces` which maps unique_ptrs to [`mplot::gl::VisualFace`](/mathplot/ref/visual/visualface) instances with a key which is a tuple of a font identifier ([`mplot::VisualFont`](/mathplot/ref/visual/visualface#visualfont)), a font texture resolution and a pointer to the relevant `Visual`.

VisualResources also manages some resources associated with instanced rendering (an OpenGL 4.3+ feature).
