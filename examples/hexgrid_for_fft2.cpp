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
import sm.hexfft;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;
import mplot.vectorvisual;
import mplot.scattervisual;

int main()
{
    // Contructor args are width, height, title
    mplot::Visual<mplot::gl::version_4_1> v(1600, 1000, "mplot::HexGridVisual");
    v.lightingEffects();

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will
    // all be discarded.
    constexpr std::int32_t Nwidth = 15;
    const float d = 0.1f;
    sm::hexgrid<float, sm::hexalign::point_up> hg1(d, Nwidth * d, 0.0f);
    hg1.set_even_rectangular_boundary (1.0f, 1.0f);
    std::cout << "Number of pixels in point_up grid:" << hg1.num() << std::endl;

    // Recreate the points
    sm::vec<float, 2> cnt = { d / 4.0f, d * std::sin(sm::mathconst<float>::deg2rad * 60) * 0.5f };
    std::vector<sm::bezcoord<float>> bpoints = hg1.rectangle_compute (1.0f, 1.0f, cnt);
    sm::vvec<sm::vec<float, 3>> points;
    sm::vvec<float> pdata;
    for (auto p : bpoints) {
        points.push_back ({p.x(), p.y(), 0.0f});
        pdata.push_back (1.0f);
        std::cout << p << std::endl;
    }

    sm::vec<float, 3> offset = { 0.0f, -0.05f, 0.0f };

    // ScatterVisual...
    auto sv = std::make_unique<mplot::ScatterVisual<float>> (offset);
    sv->set_parent (v.get_id());
    sv->setDataCoords (&points);
    sv->setScalarData (&pdata);
    sv->radiusFixed = 0.01f;
    sv->cm.setType (mplot::ColourMapType::Plasma);
    sv->finalize();
    v.addVisualModel (sv);

    auto V = sm::hexfft::make_V<float>();
    V *= d;

    // Make some dummy data (a sine wave) to make an interesting surface
    sm::vvec<float> data1(hg1.num(), 0.0f);
    for (unsigned int hi = 0; hi < hg1.num(); ++hi) {
        auto rigi = sm::vec<float, 2>{ static_cast<float>(hg1.d_ri[hi]), static_cast<float>(hg1.d_gi[hi]) };
        auto xy = V * rigi;
        data1[hi] = 0.05f + 0.05f * std::sin (2.0f * xy[0]) * std::sin (1.0f * xy[1]) ; // Range 0->1
    }
    auto hi = hg1.find_hex_at ({0,0,0});
    data1[hi->vi] = 0.15f;

    // sm::HexVisMode::HexInterp to see the hexagons or sm::HexVisMode::Triangles for a smoother surface plot
    const mplot::HexVisMode visMode = mplot::HexVisMode::HexInterp;

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    auto hgv1 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Ice);
    hgv1->zScale.null_scaling();
    hgv1->setScalarData (&data1);
    hgv1->hexVisMode = visMode;
    hgv1->addLabel ("hexalign::point_up", sm::vec<>{ 0.0f, -hg1.width()/1.8f }, mplot::TextFeatures(0.02f));
    hgv1->finalize();
    v.addVisualModel (hgv1);

    //
    // flat_up
    //

    sm::hexfft::spectrum<float> fft_data = sm::hexfft::fft (hg1, data1);

    auto fhgv = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up>>(fft_data.hgf.get(), sm::vec<float>{2.0f, 0.0f});
    fhgv->set_parent (v.get_id());
    fhgv->zoom = (fft_data.Uscale);
    fhgv->zScale.null_scaling();
    fhgv->setScalarData (&data1);
    fhgv->colourScale.compute_scaling (-900, 1200);
    fhgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    fhgv->finalize();
    v.addVisualModel (fhgv);
    v.keepOpen();

    return 0;
}
