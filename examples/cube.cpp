// Draw a cube with RhomboVisual

#include <memory>

import mplot.visual;
import mplot.rhombovisual;

int main()
{
    // Create a scene
    mplot::Visual v(1024, 768, "A cube");
    v.showCoordArrows (true);
    v.lightingEffects();

    // Parameters of the model
    sm::vec<float, 3> offset = { 1, -0.5, -0.5 };   // a within-scene offset

    sm::vec<float, 3> e1 = { 1, 0, 0 };
    sm::vec<float, 3> e2 = { 0, 1, 0 };
    sm::vec<float, 3> e3 = { 0, 0, 1 };

    sm::vec<float, 3> colour1 = { 0.35,  0.76,  0.98 };  // RGB colour triplet

    auto rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, colour1);
    rv->set_parent (v.get_id());
    rv->name = "Cube.002";
    rv->facecm = mplot::ColourMapType::Rainbow; // Try Rainbow, Batlow, Tofino
    rv->annotate = true;
    rv->finalize();
    v.addVisualModel (rv);

    v.keepOpen();

    return 0;
}
