/*
 * Demonstrate the CurvyTellyVisual as a way to draw a stripey pipe
 */
#include <memory>
#include <iostream>

import sm.mathconst;
import sm.vec;
import sm.vvec;
import sm.grid;
import sm.quaternion;
import mplot.loadpng;
import mplot.colourmap;
import mplot.visual;
import mplot.curvytellyvisual;

int main()
{
    mplot::Visual<> v(1600, 1000, "CurvyTellyVisual as a stripey pipe");

    // Make a grid to display the stripes.
    // In x, make it as many wide as there will be facets on the tube.
    // In y, make it as many long as you want there to be stripes
    // Choose x/y spacing to suit required circumference and length
    constexpr unsigned int n_x = 100;
    constexpr unsigned int n_y = 50;
    constexpr float length = 10.0f;
    constexpr float radius = 1.0f;
    constexpr float circum = sm::mathconst<float>::two_pi * radius;
    constexpr sm::vec<float, 2> grid_spacing = { circum/n_x, length/n_y };
    sm::grid grid(n_x, n_y, grid_spacing);
    std::cout << "Number of pixels in grid:" << grid.n() << std::endl;

    // The Grid is wrapped around its y axis, so different values of y give different stripes. Here
    // we create some data with each row in the data set to an alternating 0 / increasing colour
    sm::vvec<float> stripe_data (grid.n(), 0.0f);
    for (unsigned int y = 0; y < n_y; ++y) {
        float colr = static_cast<float>(y) / static_cast<float>(n_y);
        for (unsigned int x = 0; x < n_x; ++x) {
            // First strip is increasing value (colr), second is 0, third is colr, etc
            stripe_data[x + n_x * y] = (y % 2 == 0 ? colr : 0.0f);
        }
    }

    sm::vec<float> offset = { 0, 0, -length/2 };
    auto ctv = std::make_unique<mplot::CurvyTellyVisual<float>>(&grid, offset);
    ctv->set_parent (v.get_id());
    ctv->setScalarData (&stripe_data);
    ctv->radius = radius;
    ctv->angle_to_subtend = sm::mathconst<float>::two_pi; // 2pi is default
    ctv->tb_frames = false;
    ctv->lr_frames = false;
    ctv->cm.setType (mplot::ColourMapType::Plasma);
    ctv->finalize();
    v.addVisualModel (ctv);

    v.keepOpen();

    return 0;
}
