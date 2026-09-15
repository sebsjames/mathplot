/*
 * An example mplot::Visual scene, containing hexgrids.
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <format>

import sm.vec;
import sm.mat;
import sm.hexgrid;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;
import mplot.vectorvisual;

int main()
{
    // Contructor args are width, height, title
    mplot::Visual<mplot::gl::version_4_1> v(1600, 1000, "mplot::HexGridVisual");
    v.lightingEffects();

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will
    // all be discarded.
    constexpr std::int32_t Nwidth = 50;
    sm::hexgrid<float, sm::hexalign::point_up> hg1(0.01f, Nwidth * 0.01f, 0.0f);
    //hg1.set_circular_boundary (0.1f);
    hg1.set_boundary_on_outer_edge();
    std::cout << "Number of pixels in point_up grid:" << hg1.num() << std::endl;

    // Unit vectors in hex image space
    auto uv0 = sm::vec<float, 2>{ 1, 0 } * hg1.d;                                      // 'r'
    auto uv1 = sm::vec<float, 2>{ 0.5f, sm::mathconst<float>::root_3_over_2 } * hg1.d; // 'g'
    // Combined into a matrix V
    sm::mat<float, 2, 2> V;
    V.set_col(0, uv0);
    V.set_col(1, uv1);
    std::cout << "V =\n" << V << std::endl;

    // Compute U from V
    sm::mat<float, 2, 2> U = V.transpose().inverse();
    std::cout << "U =\n" << U << std::endl;
    std::cout << "With U.col(0) = " << U.col(0) << " and U.col(1) = " << U.col(1) << std::endl;

    // Multiply U or V by a vector to get the Cartesian distance for a given ri, gi

    std::cout << "Lengths: V0: " << V.col(0).length() << " V1: " << V.col(1).length() << std::endl;
    std::cout << "Lengths: U0: " << U.col(0).length() << " U1: " << U.col(1).length() << std::endl;

    // U will have lengths of order 1 / V.col(0).length(), so use V.col(0).length() as multiplying scaler
    float Uscale = V.col(0).length() * V.col(0).length();
    std::cout << "Uscale = " << Uscale << std::endl;

    // A test displacement in ri, gi on the image hex grid
    auto ri_gi = sm::vec<float, 2>{1, 2};
    std::cout << "V * " << ri_gi << ": " << (V * ri_gi) << " (testV)" << std::endl;

    std::cout << "U * " << ri_gi << ": " << (U * ri_gi) << std::endl;

    // V * ri_gi is Cartesian
    auto testV = (V * ri_gi); // ri_gi to cartesian
    std::cout << "Uinv * " << testV << " = " << U.inverse() * testV << std::endl;

    // cartesian to ri_gi (image space):
    auto rigi_image = V.inverse() * testV;
    std::cout << "Cartesian location " << testV << " to rigi:"  <<  rigi_image << std::endl;



    // Swap cols?
    auto tmp0 = U.col(0);
    U.set_col(0, U.col(1));
    U.set_col(1, tmp0);

    auto testU = U * ri_gi;

    // Make some dummy data (a sine wave) to make an interesting surface
    std::vector<float> data1(hg1.num(), 0.0f);
    for (unsigned int hi = 0; hi < hg1.num(); ++hi) {
        auto rigi = sm::vec<float, 2>{ static_cast<float>(hg1.d_ri[hi]), static_cast<float>(hg1.d_gi[hi]) };
        auto xy = V * rigi;
        data1[hi] = 0.05f + 0.05f * std::sin (20.0f * xy[0]) * std::sin (10.0f * xy[1]) ; // Range 0->1
    }

    auto hi = hg1.find_hex_at ({4,4,4});
    data1[hi->vi] = 0.15f;

    // sm::HexVisMode::HexInterp to see the hexagons or sm::HexVisMode::Triangles for a smoother surface plot
    const mplot::HexVisMode visMode = mplot::HexVisMode::HexInterp;

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    sm::vec<float, 3> offset = { 0.0f, -0.05f, 0.0f };
    auto hgv1 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Ice);
    hgv1->zScale.null_scaling();
    hgv1->setScalarData (&data1);
    hgv1->hexVisMode = visMode;
    hgv1->addLabel ("hexalign::point_up", sm::vec<>{ 0.0f, -hg1.width()/1.8f }, mplot::TextFeatures(0.02f));
    hgv1->finalize();
    v.addVisualModel (hgv1);

    auto vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset);
    vvm->set_parent (v.get_id());
    vvm->thevec = V.col(0).plus_one_dim();
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg1.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::deeppink2;
    vvm->finalize();
    v.addVisualModel (vvm);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset);
    vvm->set_parent (v.get_id());
    vvm->thevec = V.col(1).plus_one_dim();
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg1.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::royalblue2;
    vvm->finalize();
    v.addVisualModel (vvm);

    // Show our test location as a magenta vector in image space
    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset);
    vvm->set_parent (v.get_id());
    vvm->thevec = testV.plus_one_dim();
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg1.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::magenta;
    vvm->finalize();
    v.addVisualModel (vvm);

    // rg axes
    sm::vec<float> gridoffs = {hg1.d * 6, 0, 0};

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset + gridoffs);
    vvm->set_parent (v.get_id());
    vvm->thevec = sm::vec<float, 3>{1, 0, 0} * hg1.d; // r on flatgrid
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg1.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::crimson;
    vvm->finalize();
    v.addVisualModel (vvm);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset + gridoffs);
    vvm->set_parent (v.get_id());
    vvm->thevec = sm::vec<float, 3>{0.5f, sm::mathconst<float>::root_3_over_2, 0} * hg1.d;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg1.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::springgreen2;
    vvm->finalize();
    v.addVisualModel (vvm);

    //
    // flat_up
    //

    // second hexgrid from U
    sm::hexgrid<float, sm::hexalign::flat_up> hg2(U.col(0).length(), Nwidth * U.col(0).length(), 0.0f);
    hg2.set_boundary_on_outer_edge();
    std::cout << "Number of pixels in flat_up grid:" << hg2.num() << std::endl;
    std::vector<float> data2(hg2.num(), 0.0f);
    for (unsigned int hi = 0; hi < hg2.num(); ++hi) {
        auto rigi = sm::vec<float, 2>{ static_cast<float>(hg2.d_ri[hi]), static_cast<float>(hg2.d_gi[hi]) };
        auto xy = U * rigi * Uscale;
        data2[hi] = 0.05f + 0.05f * std::sin (20.0f * xy[0]) * std::sin (10.0f * xy[1]) ; // Range 0->1
    }

    offset[0] += 1.1f * hg1.width();

    auto hgv2 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hg2, offset);
    hgv2->set_parent (v.get_id());
    hgv2->cm.setType (mplot::ColourMapType::Ice);
    hgv2->zoom = (Uscale);
    hgv2->zScale.null_scaling();
    hgv2->setScalarData (&data2);
    hgv2->hexVisMode = visMode;
    hgv2->addLabel (std::format("hexalign::flat_up 1/{} size", 1/Uscale), sm::vec<>{ 0.0f, -hg2.width() * Uscale/1.8f }, mplot::TextFeatures(0.02f));
    hgv2->finalize();
    v.addVisualModel (hgv2);

    // U vectors give a basis
    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset);
    vvm->set_parent (v.get_id());
    vvm->thevec = U.col(0).plus_one_dim();
    vvm->thevec *= Uscale;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg2.d * Uscale / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::deeppink2;
    vvm->finalize();
    v.addVisualModel (vvm);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset);
    vvm->set_parent (v.get_id());
    vvm->thevec = U.col(1).plus_one_dim();
    vvm->thevec *= Uscale;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg2.d * Uscale / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::royalblue2;
    vvm->finalize();
    v.addVisualModel (vvm);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset);
    vvm->set_parent (v.get_id());
    vvm->thevec = testU.plus_one_dim();
    vvm->thevec *= Uscale;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg2.d * Uscale / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::magenta;
    vvm->finalize();
    v.addVisualModel (vvm);

#if 0 // a magenta test vector

    // rgb vectors on flat_up hexgrid
    gridoffs = {0, hg2.d * 6, 0};
    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset + gridoffs);
    vvm->set_parent (v.get_id());
    vvm->thevec = sm::vec<float, 3>{sm::mathconst<float>::root_3_over_2, 0.5f, 0} * hg2.d; // r on flatgrid
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg2.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::crimson;
    vvm->finalize();
    v.addVisualModel (vvm);
    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset + gridoffs);
    vvm->set_parent (v.get_id());
    vvm->thevec = sm::vec<float, 3>{0, 1, 0} * hg2.d;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg2.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::springgreen2;
    vvm->finalize();
    v.addVisualModel (vvm);
#endif
    v.keepOpen();

    return 0;
}
