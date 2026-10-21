/*
 * An example mplot::Visual scene, containing hexgrids.
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

import sm.vec;
import sm.hexgrid;
import sm.algo.hexgrid;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;

int main()
{
    // Contructor args are width, height, title
    mplot::Visual<mplot::gl::version_4_1> v(1600, 1000, "mplot::HexGridVisual");
    // You can set a field of view (in degrees)
    v.fov = 15;
    // The coordinate arrows can be shown/hidden
    v.showCoordArrows (false);
    // You can set the background (white, black, or any other colour)
    v.backgroundWhite();
    // You can switch on the "lighting shader" which puts diffuse light into the scene
    v.lightingEffects();

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will
    // all be discarded.
    sm::hexgrid<float, sm::hexalign::point_up> hg1(0.01f, 0.205f, 0.0f);
    hg1.set_circular_boundary (0.1f);
    //hg1.set_boundary_on_outer_edge();
    std::cout << "Number of pixels in point_up grid:" << hg1.num() << std::endl;

    sm::hexgrid<float, sm::hexalign::flat_up> hg2(0.01f, 0.205f, 0.0f);
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

    // Reduced hexgrid...
    sm::hexgrid<float, sm::hexalign::point_up> hg1r = sm::algo::hexgrid::reduced_hexgrid (hg1);
    std::cout << "\n\n\nLAST GRID\n\n";
    sm::hexgrid<float, sm::hexalign::point_up> hg1rr = sm::algo::hexgrid::reduced_hexgrid (hg1r);

    //std::cout << "hg1rr: " << hg1rr.output() << std::endl;

    // sm::HexVisMode::HexInterp to see the hexagons or sm::HexVisMode::Triangles for a smoother surface plot
    const mplot::HexVisMode visMode = mplot::HexVisMode::HexInterp;

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    sm::vec<float, 3> offset = { 0.0f, 0.0f, 0.0f };
    auto hgv1 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Ice);
    hgv1->setScalarData (&data1);
    hgv1->hexVisMode = visMode;
    hgv1->zScale.null_scaling();
    hgv1->addLabel ("hexalign::point_up (3D)", sm::vec<>{ -hg1.width()/2.1f, -hg1.width()/1.6f }, mplot::TextFeatures(0.02f));
    hgv1->finalize();
    v.addVisualModel (hgv1);

    offset[0] += 1.5f * hg1.width();
    auto hgv2 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hg2, offset);
    hgv2->set_parent (v.get_id());
    hgv2->cm.setType (mplot::ColourMapType::Ice);
    hgv2->showboundary = true;
    hgv2->setScalarData (&data2);
    hgv2->hexVisMode = visMode;
    hgv2->zScale.null_scaling();
    // You can transform the view of the hexgrid in HexGridVisual (transforms may also be applied to the underlying hexgrid)
    // This applies a scaling first, and then a reflection.
    //hgv2->tfm = sm::mat<float, 3>::reflection (sm::vec<float>::ux()) * sm::mat<float, 3>::identity() * 0.5f;
    hgv2->addLabel ("hexalign::flat_up\n(2D, with boundary)", sm::vec<>{ -hg1.width()/2.6f, -hg1.width()/1.6f }, mplot::TextFeatures(0.02f));
    hgv2->finalize();
    v.addVisualModel (hgv2);

    offset = {0.0f, 0.0f, 0.01f };
    sm::vvec<float> data3 (hg1r.num(), 0.0f);
    // First recreate data according to same algo (rather than sampling from hg1)
    for (unsigned int ri = 0; ri < hg1r.num(); ++ri) {
        data3[ri] = 0.05f + 0.05f * std::sin (20.0f * hg1r.d_x[ri]) * std::sin (10.0f * hg1r.d_y[ri]) ; // Range 0->1
    }
    auto hgv3 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1r, offset);
    hgv3->set_parent (v.get_id());
    hgv3->cm.setType (mplot::ColourMapType::Ice);
    hgv3->showboundary = true;
    hgv3->setAlpha (1.0f);
    hgv3->setScalarData (&data3);
    hgv3->hexVisMode = visMode;
    hgv3->zScale.null_scaling();
    hgv3->addLabel ("reduced point_up", sm::vec<>{ -hg1.width()/2.6f, -hg1.width()/1.6f }, mplot::TextFeatures(0.02f));
    hgv3->finalize();
    v.addVisualModel (hgv3);

#if 1
    offset = {0.0f, 0.0f, 0.02f };
    sm::vvec<float> data4 (hg1rr.num(), 0.0f);
    // First recreate data according to same algo (rather than sampling from hg1)
    for (unsigned int ri = 0; ri < hg1rr.num(); ++ri) {
        data4[ri] = 0.05f + 0.05f * std::sin (20.0f * hg1rr.d_x[ri]) * std::sin (10.0f * hg1rr.d_y[ri]) ; // Range 0->1
    }
    auto hgv4 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1rr, offset);
    hgv4->set_parent (v.get_id());
    hgv4->cm.setType (mplot::ColourMapType::Ice);
    hgv4->showboundary = true;
    hgv4->setAlpha (0.5f);
    hgv4->setScalarData (&data4);
    hgv4->hexVisMode = visMode;
    hgv4->zScale.null_scaling();
    hgv4->addLabel ("double reduced", sm::vec<>{ -hg1.width()/2.6f, -hg1.width()/1.6f }, mplot::TextFeatures(0.02f));
    hgv4->finalize();
    v.addVisualModel (hgv4);
#endif

    v.keepOpen();
}
