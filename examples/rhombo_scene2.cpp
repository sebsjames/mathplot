// A scene of rhombohedrons useful for developing scene rotation by user control
#include <memory>

import mplot.colourmap;
import mplot.visual;
import mplot.rhombovisual;

int main()
{
    // Create a scene
    mplot::Visual v(1024, 768, "Rhombohedrons");
    v.lightingEffects (true);

    // Parameters of the model
    sm::vec<float, 3> offset = { -1,  0,  0 };   // a within-scene offset
    sm::vec<float, 3> e1 = { 0.25,  0,  0 };
    sm::vec<float, 3> e2 = { 0.1,  0.25,  0 };
    sm::vec<float, 3> e3 = { 0,  0.0,  0.25 };
    mplot::ColourMap<float> cmap(mplot::ColourMapType::Rainbow);

    // 0
    offset = { 0, 0, 0 };
    auto rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1/3, e2/3, e3/3, cmap.convert(0.1f));
    rv->set_parent (v.get_id());
    rv->finalize();
    v.addVisualModel (rv);

    offset = { -2, 0, 0 };
    rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, cmap.convert(1.0f));
    rv->set_parent (v.get_id());
    rv->finalize();
    v.addVisualModel (rv);

    offset = { 2, 0, 0 };
    rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, cmap.convert(0.5f));
    rv->set_parent (v.get_id());
    rv->finalize();
    v.addVisualModel (rv);

    offset = { 0, 2, 0 };
    rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, cmap.convert(0.3333f));
    rv->set_parent (v.get_id());
    rv->finalize();
    v.addVisualModel (rv);

    offset = { 0, -2, 0 };
    rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, cmap.convert(0.25f));
    rv->set_parent (v.get_id());
    rv->finalize();
    v.addVisualModel (rv);

    offset = { 0, 0, 2 };
    rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, cmap.convert(0.2f));
    rv->set_parent (v.get_id());
    rv->finalize();
    v.addVisualModel (rv);

    offset = { 0, 0, -2 };
    rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, cmap.convert(0.1f));
    rv->set_parent (v.get_id());
    rv->finalize();
    v.addVisualModel (rv);
    v.render();

    v.keepOpen();

    return 0;
}
