/*
 * An example mplot::Visual scene, containing a HexGrid, onto which is sampled an image. Then
 * projected onto a sphere.
 *
 * \author Seb James
 * \date June 2024
 */

#include <memory>
#include <iostream>
#include <string>
#include <cmath>

import sm.vec;
import sm.vvec;
import sm.hexgrid;
import sm.algo.hexgrid;

import mplot.loadpng;
import mplot.visual;
import mplot.hexgridvisual;

enum class spherical_projection
{
    mercator,
    equirectangular,
    cassini,
    splodge
};

int main()
{
    using mc = sm::mathconst<float>;
    constexpr spherical_projection proj = spherical_projection::mercator;

    mplot::Visual v(1600, 1000, "Spherically transformed hexgrid");

    // radius of sphere
    constexpr float r_sph = 1.0f;

    constexpr float hex_d = 0.02;
    constexpr float hex_span = mc::two_pi * r_sph;
    sm::hexgrid hg(hex_d, 2.0f * hex_span, 0.0f);
    if constexpr (proj == spherical_projection::splodge) {
        hg.set_circular_boundary (0.95f * r_sph);
    } else {
        hg.set_circular_boundary (0.5f * mc::pi * r_sph);
        // You can set a rectangular boundary too:
        //hg.set_rectangular_boundary (0.5f * mc::pi * r_sph, 0.5f * mc::pi * r_sph);
    }

    // Load an image with mplot::loadpng()
    std::string fn = "../examples/bike256.png";
    sm::vvec<float> image_data;
    sm::vec<unsigned int, 2> dims = mplot::loadpng (fn, image_data);

    // This controls how large the photo will be on the HexGrid
    sm::vec<float,2> image_scale = {3.2f, 3.2f};
    if constexpr (proj == spherical_projection::splodge) {
        image_scale = {2.8f, 2.8f};
    }

    // You can shift the photo with an offset if necessary
    sm::vec<float,2> image_offset = {0.0f, 0.0f};

    // Here's the HexGrid method that will resample the square pixel grid onto the hex grid
    sm::vvec<float> hex_image_data = sm::algo::hexgrid::resample_image (hg, image_data, dims[0], image_scale, image_offset);

    // hg has d_x and d_y. Can make up a new container of 3D locations for each hex.
    sm::vvec<sm::vec<float, 3>> sphere_coords(hg.num());
    for (unsigned int i = 0u; i < hg.num(); ++i) {
        // This is the inverse Mercator projection.
        // See https://stackoverflow.com/questions/12732590/how-map-2d-grid-points-x-y-onto-sphere-as-3d-points-x-y-z
        sm::vec<float, 2> xy = { hg.d_x[i], hg.d_y[i] };
        float longitude = 0.0f; // or lambda
        float latitude = 0.0f;  // or phi
        if constexpr (proj == spherical_projection::equirectangular) {
            float phi0 = 0.0f;
            float phi1 = 0.0f;
            float lambda0 = 0.0f;
            longitude = xy[0] / (r_sph * std::cos(phi1)) + lambda0;
            latitude = xy[1] / r_sph + phi0;
        } else if constexpr (proj == spherical_projection::cassini) {
            // Spherical Cassini
            float phi0 = 0.0f;
            float lambda0 = 0.0f;
            float D = xy[1] / r_sph + phi0;
            longitude = lambda0 + std::atan2 (std::tan(xy[0]/r_sph), std::cos(D));
            latitude = std::asin (std::sin(D) * std::cos(xy[0]/r_sph));

        } else if constexpr (proj == spherical_projection::splodge) {
            // It's all below
        } else { // Default Mercator projection
            longitude = xy[0] / r_sph;
            latitude = 2.0f * std::atan (std::exp(xy[1]/r_sph)) - mc::pi_over_2;
        }

        if constexpr (proj == spherical_projection::splodge) {
            // In the splodge projection we just 'throw' the 2D plane onto a sphere
            float z_sq = r_sph * r_sph - xy.sq().sum();
            float z_sph = 0.0f;
            if (z_sq >= 0.0f)  {
                z_sph = std::sqrt (z_sq);
            } else {
                z_sph = -std::sqrt (-z_sq); // Anything beyond the edge of r_sph will make a nice bell shape
            }
            sphere_coords[i] = xy.plus_one_dim (z_sph);
        } else { // it's a serious projection
            float coslat = std::cos (latitude);
            float sinlat = std::sin (latitude);
            float coslong = std::cos (longitude);
            float sinlong = std::sin (longitude);
            sphere_coords[i] =  { r_sph * coslat * coslong, r_sph * coslat * sinlong , r_sph * sinlat };
        }
    }

    std::string label = "Mercator";
    if constexpr (proj == spherical_projection::equirectangular) {
        label = "Equirectangular";
    } else if constexpr (proj == spherical_projection::cassini) {
        label = "Spherical Cassini";
    } else if constexpr (proj == spherical_projection::splodge) {
        label = "'Raised Sphere'";
    }

    // Now visualise with a HexGridVisual
    auto hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>{1.5,0,0});
    hgv->set_parent (v.get_id());
    // Set the image data as the scalar data for the HexGridVisual
    hgv->setScalarData (&hex_image_data);
    // This will make it a spherical projection (these coords will override the 2D coords in the hexgrid)
    hgv->setDataCoords (&sphere_coords);
    // The inverse greyscale map is appropriate for a monochrome image
    hgv->cm.setType (mplot::ColourMapType::Inferno);
    hgv->addLabel (label, sm::vec<float>{ 0, -1.1f * r_sph, 0 }, mplot::TextFeatures(0.05f));
    hgv->finalize();
    v.addVisualModel (hgv);

    // Let's have a flat one alongside for comparison
    hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>{-1.5,0,-1});
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&hex_image_data);
    // The only real difference is that this has no hgv->setDataCoords(&sphere_coords) call.
    hgv->cm.setType (mplot::ColourMapType::Inferno);
    hgv->zScale.set_params (0, 1); // sets a z offset of 1 across the hexgrid
    hgv->addLabel (std::string("2D hexgrid"),
                   sm::vec<float>{1.2*r_sph, -1.2*r_sph, 1},
                   mplot::TextFeatures(0.05f));
    hgv->finalize();
    v.addVisualModel (hgv);

    v.keepOpen();

    return 0;
}
