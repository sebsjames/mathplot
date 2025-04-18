/*
 * Demonstrate the CurvyTellyVisual as a way to draw a stripey pipe
 */
#include <morph/mathconst.h>
#include <morph/vec.h>
#include <morph/vvec.h>
#include <morph/grid.h>
#include <morph/VisualCompoundRay.h>
#include <morph/CurvyTellyVisual.h>

int main()
{
    morph::VisualCompoundRay<> v(1600, 1000, "CurvyTellyVisual as a stripey pipe");

    // Make a grid to display the stripes.
    // In x, make it as many wide as there will be facets on the tube.
    // In y, make it as many long as you want there to be stripes
    // Choose x/y spacing to suit required circumference and length
    constexpr unsigned int n_x = 100;
    constexpr unsigned int n_y = 50;
    constexpr float length = 10.0f;
    constexpr float radius = 1.0f;
    constexpr float circum = morph::mathconst<float>::two_pi * radius;
    constexpr morph::vec<float, 2> grid_spacing = { circum/n_x, length/n_y };
    morph::grid grid(n_x, n_y, grid_spacing);
    std::cout << "Number of pixels in grid:" << grid.n() << std::endl;

    // The Grid is wrapped around its y axis, so different values of y give different stripes. Here
    // we create some data with each row in the data set to an alternating 0 / increasing colour
    morph::vvec<float> stripe_data (grid.n(), 0.0f);
    for (unsigned int y = 0; y < n_y; ++y) {
        float colr = static_cast<float>(y) / static_cast<float>(n_y);
        for (unsigned int x = 0; x < n_x; ++x) {
            // First strip is increasing value (colr), second is 0, third is colr, etc
            stripe_data[x + n_x * y] = (y % 2 == 0 ? colr : 0.0f);
        }
    }

    morph::vec<float> offset = { 0, 0, -length/2 };
    auto ctv = std::make_unique<morph::CurvyTellyVisual<float>>(&grid, offset);
    v.bindmodel (ctv);
    ctv->setScalarData (&stripe_data);
    ctv->radius = radius;
    ctv->angle_to_subtend = morph::mathconst<float>::two_pi; // 2pi is default
    ctv->tb_frames = false;
    ctv->lr_frames = false;
    ctv->cm.setType (morph::ColourMapType::Plasma);
    ctv->finalize();
    v.addVisualModel (ctv);

    v.keepOpen();

    return 0;
}
