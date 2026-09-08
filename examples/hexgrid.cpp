/*
 * An example mplot::Visual scene, containing a HexGrid.
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

import sm.vec;
import sm.hexgrid;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;

int main()
{
    // Contructor args are width, height, title
    mplot::Visual<mplot::gl::version_4_1> v(1600, 1000, "mplot::HexGridVisual");
    // You can set a field of view (in degrees)
    v.fov = 15;
    // set the x/y offset. Try pressing 'z' in the app window to see what the current sceneTrans is
    v.setSceneTransXY (0.0f, 0.0f);
    // Make this larger to "scroll in and out of the image" faster. 0.02f is the default in VisualBase.
    v.scenetrans_stepsize = 0.02f;
    // The coordinate arrows can be hidden
    v.showCoordArrows (true);
    // You can set the background (white, black, or any other colour)
    v.backgroundWhite();
    // You can switch on the "lighting shader" which puts diffuse light into the scene
    //v.lightingEffects();
    // Add some text labels to the scene
    v.addLabel ("This is a\nmplot::HexGridVisual\nobject", {0.26f, -0.16f, 0.0f});

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will
    // all be discarded.
    sm::hexgrid<float, sm::hexalign::flat_up> hg(0.01f, 3.0f, 0.0f);
    hg.set_circular_boundary (0.6f);
    std::cout << "Number of pixels in grid:" << hg.num() << std::endl;


    // Make some dummy data (a sine wave) to make an interesting surface
    std::vector<float> data(hg.num(), 0.0f);
    for (unsigned int ri = 0; ri < hg.num(); ++ri) {
        data[ri] = 0.05f + 0.05f * std::sin (20.0f * hg.d_x[ri]) * std::sin (10.0f * hg.d_y[ri]) ; // Range 0->1
    }

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    sm::vec<float, 3> offset = { 0.0f, -0.05f, 0.0f };
    auto hgv = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hg, offset);
    hgv->set_parent (v.get_id());
    //hgv->wireframe (true);
    hgv->cm.setType (mplot::ColourMapType::Ice);
    hgv->setScalarData (&data);
    hgv->hexVisMode = mplot::HexVisMode::HexInterp; // Or sm::HexVisMode::Triangles for a smoother surface plot
    hgv->finalize();

    if (v.checkContext() == true) {
        std::cout << "I have the context after hgv->finalize()\n";
    } else {
        std::cout << "I don't have the context after hgv->finalize()\n";
    }

    v.addVisualModel (hgv);

    if (v.checkContext() == true) {
        std::cout << "I have the context after addVisualModel\n";
    } else {
        std::cout << "I don't have the context after addVisualModel()\n";
    }

    v.keepOpen();

    if (v.checkContext() == true) {
        std::cout << "I have the context after user requested exit\n";
    } else {
        std::cout << "I don't have the context after user requested exit\n";
    }

    return 0;
}
