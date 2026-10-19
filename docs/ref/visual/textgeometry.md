---
title: mplot::TextGeometry
parent: Visualization API
grand_parent: Reference
permalink: /ref/visual/textgeometry
layout: page
nav_order: 12
---
```c++
import mplot.textgeometry;
```

# How long is that string?

When you add a label to a `VisualModel`, information about the text
that was added to the screen is returned in a `TextGeometry`
object. This makes it possible to determine where another section of
text should go.

The `TextGeometry` object allows you to find the `width()` and
`height()` of the string.

As another very small class, it's easiest to document it by showing it:

```c++
struct TextGeometry
{
    //! The sum of the advances of each glyph in the string
    float total_advance = 0.0f;
    //! The maximum extension above the baseline for any glyph in the string
    float max_bearingy = 0.0f;
    //! The maximum ymin - the extension of the lowest part of any glyph, like gpqy, etc.
    float max_drop = 0.0f;
    //! Return half the width of the string
    float half_width() { return this->total_advance * 0.5f; }
    //! The width is the total_advance.
    float width() { return this->total_advance; }
    //! The effective height is the maximum bearingy
    float height() { return this->max_bearingy; }
    //! Half the max_bearingy
    float half_height() { return this->max_bearingy * 0.5f; }
};
```

An example of its use can be found in [`ConfigVisual.cppm`](https://github.com/sebsjames/mathplot/blob/main/mplot/ConfigVisual.cppm), where text
information is shown line by line:

```c++
for (auto key : this->keys) {
    // For each key obtain the value from a sm::config object called conf:
    float value = conf->get<float>(key, 0.0f);
    // Create the label:
    std::string lbl = key + std::string(": ") + std::to_string(value);
    // Add the label to this VisualModel
    mplot::TextGeometry geom = this->addLabel (lbl, toffset, this->tfeatures);
    // Move toffset in the y direction, to 'print the next line'
    toffset[1] -= line_spacing * geom.height();
}
```

You can query any `VisualTextModel` for its `TextGeometry` with
`VisualTextModel::getTextGeometry()`. Your `VisualModel` objects each
contain a vector of pointers to `VisualTextModel` objects called
`VisualModel::texts`