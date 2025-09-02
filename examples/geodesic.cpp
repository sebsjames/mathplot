/*
 * Visualize an Icosahedron
 */
#include <iostream>
#include <fstream>
#include <cmath>
#include <array>
#include <stdexcept>
#include <string>

#include <sm/vec>

#include <mplot/Visual.h>
#include <mplot/ColourMap.h>
#include <mplot/GeodesicVisual.h>

int main()
{
    int rtn = -1;

    mplot::Visual v(1024, 768, "Geodesic Polyhedra (ordered vertices/faces)");
    v.showCoordArrows (true);
    v.showUserFrame (true); // hmm, causes crash
    v.options.set (mplot::visual_options::rotateAboutSceneOrigin, false);

    try {
        sm::vec<float, 3> offset = { 0.0, 0.0, 0.0 };
        sm::vec<float, 3> step = { 2.2, 0.0, 0.0 };

        mplot::ColourMap<float> cm (mplot::ColourMapType::Jet);
        int imax = 4;
        for (int i = 0; i < imax; ++i) {
            auto cl = cm.convert (i / static_cast<float>(imax - 1));
            auto gv1 = std::make_unique<mplot::GeodesicVisual<float>> (offset + step * i, 0.9f);
            v.bindmodel (gv1);
            gv1->iterations = i;
            std::string lbl = std::string("iterations = ") + std::to_string(i);
            gv1->addLabel (lbl, {0, -1, 0}, mplot::TextFeatures(0.06f));
            gv1->cm.setType (mplot::ColourMapType::Jet);
            gv1->colour_bb = cl;
            gv1->finalize();

            // re-colour after construction
            auto gv1p = v.addVisualModel (gv1);
            float imax_mult = 1.0f / static_cast<float>(imax);
            // sequential colouring:
            size_t sz1 = gv1p->data.size();
            gv1p->data.linspace (0.0f, 1+i * imax_mult, sz1);
            gv1p->reinitColours();
        }

        v.keepOpen();

    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        rtn = -1;
    }

    return rtn;
}
