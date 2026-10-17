/*
 * An example mplot::Visual scene, containing hexgrid with boundary shown
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
    v.fov = 15;
    //v.rotateAboutNearest (true);

    sm::hexgrid<float, sm::hexalign::point_up> hg1(0.01f, 0.205f, 0.0f);
    hg1.set_circular_boundary (0.1f);
    //hg1.set_boundary_on_outer_edge();

    // Make some dummy data (a sine wave) to make an interesting surface
    std::vector<float> data1(hg1.num(), 0.0f);
    for (unsigned int ri = 0; ri < hg1.num(); ++ri) {
        data1[ri] = 0.05f + 0.05f * std::sin (20.0f * hg1.d_x[ri]) * std::sin (10.0f * hg1.d_y[ri]) ; // Range 0->1
    }

    // sm::HexVisMode::HexInterp to see the hexagons or sm::HexVisMode::Triangles for a smoother surface plot
    const mplot::HexVisMode visMode = mplot::HexVisMode::HexInterp;

    sm::vec<float, 3> offset = { 0.0f, -0.05f, 0.0f };
    auto hgv1 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Ice);
    hgv1->showboundary = true;
    hgv1->setScalarData (&data1);
    hgv1->hexVisMode = visMode;
    hgv1->zScale.null_scaling();
    hgv1->addLabel ("hexalign::point_up", sm::vec<>{ -hg1.width()/2.1f, -hg1.width()/1.6f }, mplot::TextFeatures(0.02f));
    hgv1->finalize();
    v.addVisualModel (hgv1);

    v.keepOpen();

    return 0;
}
