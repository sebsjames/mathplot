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
    v.lightingEffects();
    // Add some text labels to the scene
    //v.addLabel ("This is a\nmplot::HexGridVisual\nobject", {0.26f, -0.16f, 0.0f});

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will
    // all be discarded.
    sm::hexgrid<float, sm::hexalign::point_up> hg1(0.01f, 0.5f, 0.0f);
    //hg1.set_circular_boundary (0.1f);
    hg1.set_boundary_on_outer_edge();
    std::cout << "Number of pixels in point_up grid:" << hg1.num() << std::endl;

    sm::hexgrid<float, sm::hexalign::flat_up> hg2(0.01f, 0.5f, 0.0f);
    //hg2.set_circular_boundary (0.1f);
    hg2.set_boundary_on_outer_edge();
    std::cout << "Number of pixels in flat_up grid:" << hg2.num() << std::endl;

    // Make some dummy data (a sine wave) to make an interesting surface
    std::vector<float> data1(hg1.num(), 0.0f);
    for (unsigned int ri = 0; ri < hg1.num(); ++ri) {
        data1[ri] = 0.05f + 0.05f * std::sin (20.0f * hg1.d_x[ri]) * std::sin (10.0f * hg1.d_y[ri]) ; // Range 0->1
    }
    std::vector<float> data2(hg2.num(), 0.0f);
    for (unsigned int ri = 0; ri < hg2.num(); ++ri) {
        data2[ri] = 0.05f + 0.05f * std::sin (20.0f * hg2.d_x[ri]) * std::sin (10.0f * hg2.d_y[ri]) ; // Range 0->1
    }

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    sm::vec<float, 3> offset = { 0.0f, -0.05f, 0.0f };
    auto hgv1 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Ice);
    hgv1->setScalarData (&data1);
    hgv1->hexVisMode = mplot::HexVisMode::HexInterp; // Or sm::HexVisMode::Triangles for a smoother surface plot
    hgv1->finalize();
    v.addVisualModel (hgv1);

    offset[0] += 1.1f * hg1.width();
    auto hgv2 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hg2, offset);
    hgv2->set_parent (v.get_id());
    hgv2->cm.setType (mplot::ColourMapType::Ice);
    hgv2->setScalarData (&data2);
    hgv2->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv2->finalize();

    if (v.checkContext() == true) {
        std::cout << "I have the context after hgv->finalize()\n";
    } else {
        std::cout << "I don't have the context after hgv->finalize()\n";
    }

    v.addVisualModel (hgv2);

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
