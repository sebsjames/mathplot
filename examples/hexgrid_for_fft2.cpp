/*
 * An example mplot::Visual scene, containing hexgrids.
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <format>
#include <complex>

import sm.vec;
import sm.mat;
import sm.hexgrid;
import sm.hexfft;
import sm.grid;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;
import mplot.vectorvisual;
import mplot.scattervisual;
import mplot.gridvisual;

int main()
{
    // Contructor args are width, height, title
    mplot::Visual<mplot::gl::version_4_1> v(1600, 1000, "mplot::HexGridVisual");
    v.lightingEffects();

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will
    // all be discarded.
    constexpr std::int32_t Nwidth = 36;
    const float d = 0.1f;
    sm::hexgrid<float, sm::hexalign::point_up> hg1(d, Nwidth * d, 0.0f);
    //hg1.set_rectangular_boundary (12u, 12u);
    hg1.set_circular_boundary (0.3f);
    std::cout << "Number of pixels in point_up grid:" << hg1.num() << std::endl;

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
    auto mx = data1.max();
    data1[hi->vi] = 1.1f * mx;

    // sm::HexVisMode::HexInterp to see the hexagons or sm::HexVisMode::Triangles for a smoother surface plot
    const mplot::HexVisMode visMode = mplot::HexVisMode::HexInterp;

    sm::vec<> offset = {};

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    auto hgv1 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Ice);
    hgv1->showboundary = true;
    hgv1->zScale.null_scaling();
    hgv1->setScalarData (&data1);
    hgv1->hexVisMode = visMode;
    hgv1->addLabel ("hexalign::point_up", sm::vec<>{ 0.0f, -hg1.width()/1.8f }, mplot::TextFeatures(0.02f));
    hgv1->finalize();
    v.addVisualModel (hgv1);

    //
    // flat_up
    //

    sm::hexfft::fft<float, true> hfft;
    hfft.init (&hg1);
    hfft.forward (data1);

    sm::vvec<float> fft_r (hfft.X_hexgrid.size());
    sm::vvec<float> fft_i (hfft.X_hexgrid.size());
    for (std::uint32_t i = 0; i < fft_r.size(); ++i) {
        fft_r[i] = std::real(hfft.X_hexgrid[i]);
        fft_i[i] = std::imag(hfft.X_hexgrid[i]);
    }

    // Data on grids
    // rows/cols:
    sm::vec<float, 2> grid_spacing = {hfft.hg->d, hfft.hg->d};
    constexpr sm::vec<float, 2> null_offset = {0.0f, 0.0f};
    sm::grid<std::uint32_t, float> grid(hfft.asa_cols, hfft.asa_rows, grid_spacing, null_offset,
                                        sm::griddomainwrap::none,
                                        sm::gridorder::bottomleft_to_topright_colmaj);
    sm::vvec<float> d0 (hfft.d0.size());
    sm::vvec<float> d1 (hfft.d1.size());
    sm::vvec<float> X0 (hfft.X0.size());
    sm::vvec<float> X1 (hfft.X1.size());

    for (std::uint32_t i = 0; i < d0.size(); ++i) {
        d0[i] = std::real (hfft.d0[i]);
        d1[i] = std::real (hfft.d1[i]);
        X0[i] = std::real (hfft.X0[i]);
        X1[i] = std::real (hfft.X1[i]);
    }

    offset[1] -= (hfft.hg->width() / 2) + 1.25f * hfft.asa_rows * grid_spacing[1];

    // Grid 1 ds.first
    auto gv = std::make_unique<mplot::GridVisual<float>>(&grid, offset);
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&d0);
    gv->zScale.null_scaling();
    gv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    gv->addLabel ("d0 (odd input rows)", sm::vec<float>({0,-0.1,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    offset[1] -= 1.25f * hfft.asa_rows * grid_spacing[1];

    gv = std::make_unique<mplot::GridVisual<float>>(&grid, offset);
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&d1);
    gv->zScale.null_scaling();
    gv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    gv->addLabel ("d1 (even)", sm::vec<float>({0,-0.1,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    offset[0] += 1.25f * hfft.asa_cols * grid_spacing[0];

    gv = std::make_unique<mplot::GridVisual<float>>(&grid, offset);
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&X0);
    gv->zScale.null_scaling();
    gv->cm.setType (mplot::ColourMapType::Ice);
    gv->addLabel ("X0 (odd input rows)", sm::vec<float>({0,-0.1,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    auto x0_scale = gv->colourScale;
    v.addVisualModel (gv);

    offset[1] -= 1.25f * hfft.asa_rows * grid_spacing[1];

    gv = std::make_unique<mplot::GridVisual<float>>(&grid, offset);
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&X1);
    gv->colourScale = x0_scale;
    gv->zScale.null_scaling();
    gv->cm.setType (mplot::ColourMapType::Ice);
    gv->addLabel ("X1 (even)", sm::vec<float>({0,-0.1,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    offset[1] -= 1.25f * 2 * hfft.asa_rows * grid_spacing[1];

    offset[0] -= 1.25f * hfft.asa_cols * grid_spacing[0];

    // Viz the ASA-compliant hexgrid
    hgv1 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(hfft.hg_asa.get(), offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Ice);
    hgv1->showboundary = true;
    hgv1->zScale.null_scaling();
    hgv1->setScalarData (&hfft.data_asa_real);
    hgv1->hexVisMode = visMode;
    hgv1->addLabel ("ASA hexgrid", sm::vec<>{ 0.0f, -hg1.width()/1.8f }, mplot::TextFeatures(0.02f));
    hgv1->finalize();
    v.addVisualModel (hgv1);

    auto fhgv = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up>>(hfft.hgf.get(), sm::vec<float>{3.0f, 0.0f});
    fhgv->set_parent (v.get_id());
    fhgv->zoom = (hfft.Uscale);
    fhgv->zScale.null_scaling();
    fhgv->setScalarData (&fft_r);
    fhgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    fhgv->finalize();
    v.addVisualModel (fhgv);

   sm::vvec<std::complex<float>> invimg = hfft.inverse();

    // Re-show the X0/X1 grids
    for (std::uint32_t i = 0; i < hfft.X0.size(); ++i) {
        d0[i] = std::real (hfft.d0[i]);
        d1[i] = std::real (hfft.d1[i]);
        X0[i] = std::real (hfft.X0[i]);
        X1[i] = std::real (hfft.X1[i]);
    }


    offset[0] += 1.25f * hfft.asa_cols * grid_spacing[0];
    gv = std::make_unique<mplot::GridVisual<float>>(&grid, offset);
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&X0);
    gv->zScale.null_scaling();
    gv->colourScale = x0_scale;
    gv->cm.setType (mplot::ColourMapType::Ice);
    gv->addLabel ("X0 (odd input rows)", sm::vec<float>({0,-0.1,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    offset[1] -= 1.25f * hfft.asa_rows * grid_spacing[1];
    gv = std::make_unique<mplot::GridVisual<float>>(&grid, offset);
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&X1);
    gv->zScale.null_scaling();
    gv->colourScale = x0_scale;
    gv->cm.setType (mplot::ColourMapType::Ice);
    gv->addLabel ("X1 (even)", sm::vec<float>({0,-0.1,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    offset[0] += 1.25f * hfft.asa_cols * grid_spacing[0];
    offset[1] += 1.25f * hfft.asa_rows * grid_spacing[1];
    gv = std::make_unique<mplot::GridVisual<float>>(&grid, offset);
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&d0);
    gv->zScale.null_scaling();
    //gv->colourScale = x0_scale;
    gv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    gv->addLabel ("d0 (odd input rows)", sm::vec<float>({0,-0.1,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    offset[1] -= 1.25f * hfft.asa_rows * grid_spacing[1];
    gv = std::make_unique<mplot::GridVisual<float>>(&grid, offset);
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&d1);
    gv->zScale.null_scaling();
    //gv->colourScale = x0_scale;
    gv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    gv->addLabel ("d1 (even)", sm::vec<float>({0,-0.1,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    offset[0] += 1.0f;
    // Viz the ASA-compliant hexgrid
    hgv1 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(hfft.hg_asa.get(), offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Ice);
    hgv1->showboundary = true;
    hgv1->zScale.null_scaling();
    hgv1->setScalarData (&hfft.data_asa_real);
    hgv1->hexVisMode = visMode;
    hgv1->addLabel ("ASA hexgrid", sm::vec<>{ 0.0f, -hg1.width()/1.8f }, mplot::TextFeatures(0.02f));
    hgv1->finalize();
    v.addVisualModel (hgv1);

    // Final image
    offset[1] += 1.5f;
    offset[0] += 0.5f;
    sm::vvec<float> img_r (invimg.size(), 0.0f);
    for (std::uint32_t i = 0; i < invimg.size(); ++i) { img_r[i] = std::real (invimg[i]); }

    hgv1 = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Ice);
    hgv1->showboundary = true;
    hgv1->zScale.null_scaling();
    hgv1->setScalarData (&img_r);
    hgv1->hexVisMode = visMode;
    hgv1->addLabel ("hexalign::point_up", sm::vec<>{ 0.0f, -hg1.width()/1.8f }, mplot::TextFeatures(0.02f));
    hgv1->finalize();
    v.addVisualModel (hgv1);

    v.keepOpen();
    return 0;
}
