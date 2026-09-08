/*
 * An example mplot::Visual scene, containing a HexGrid, onto which is sampled an image. In this
 * case the image is rectangular, and we apply the mask-outside-radius method.
 */

#include <memory>
#include <iostream>
#include <string>

import sm.vec;
import sm.vvec;
import sm.hexgrid;
import sm.algo.hexgrid;

import mplot.loadpng;
import mplot.visual;
import mplot.hexgridvisual;

int main()
{
    mplot::Visual v(1600, 1000, "Demo of hexgrid::resample_image");

    sm::hexgrid hg(0.01f, 3.0f, 0.0f);
    hg.set_rectangular_boundary (2.0f, 0.5f);

    // Load a rectangular image with the help of mplot::loadpng().
    std::string fn = "../examples/bike256_65.png";
    sm::vvec<float> image_data;
    sm::vec<unsigned int, 2> dims = mplot::loadpng (fn, image_data);
    std::cout << "Loaded image with dims: " << dims << std::endl;

    // This controls how large the photo will be on the hexgrid
    sm::vec<float,2> image_scale = {2.0f, 2.0f};
    // You can shift the photo with an offset if necessary
    sm::vec<float,2> image_offset = {0.0f, 0.0f};

    // Here's the hexgrid method that will resample the square pixel grid onto the hex grid
    sm::vvec<float> hex_image_data = sm::algo::hexgrid::resample_image (hg, image_data, dims[0], image_scale, image_offset);

    sm::algo::hexgrid::mask_outside_radius (hg, hex_image_data, 0.75f, 0.0f);

    // Now visualise with a HexGridVisual
    auto hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>({0,0,0}));
    hgv->set_parent (v.get_id());

    // Set the image data as the scalar data for the HexGridVisual
    hgv->setScalarData (&hex_image_data);
    // The inverse greyscale map is appropriate for a monochrome image
    hgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    // As it's an image, we don't want relief, so set the zScale to have a zero gradient
    hgv->zScale.set_params (0, 1);

    hgv->finalize();
    v.addVisualModel (hgv);

    v.keepOpen();

    return 0;
}
