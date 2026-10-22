/*
 * An example mplot::Visual scene, containing hexgrids.
 *
 * Hexagonal grids were the first VisualModel I coded in mathplot. The interest in hexgrids came
 * from Stuart Wilson, who wanted to compute reaction diffusion systems in C++. Those early hexgrids
 * took part in this paper about whisker barrels: https://elifesciences.org/articles/55588
 *
 * This program shows some examples of the hexgrids that you can work with using sm::hexgrid and
 * mplot::HexGridVisual.
 *
 * Author: Seb James
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <map>
#include <list>
#include <format>
#include <tuple>

import sm.vec;
import sm.hexgrid;
import sm.algo.hexgrid; // hexgrid algorithms, particularly the ability to reduce down to the 'next-coarser' grid

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;

// We make some dummy data (a sine wave) to show on our hexgrids and either colour or elevation
namespace hexexample
{
    template<typename F, sm::hexalign A>
    void orthogonal_sines (sm::hexgrid<F, A>& hg, sm::vvec<F>& data)
    {
        data.resize (hg.num(), F{0});
        for (unsigned int ri = 0; ri < hg.num(); ++ri) {
            data[ri] = 0.05f + 0.05f * std::sin (20.0f * hg.d_x[ri]) * std::sin (10.0f * hg.d_y[ri]) ; // Range 0->1
        }
    }
}

int main()
{
    // Contructor args are width, height, title
    mplot::Visual<mplot::gl::version_4_1> v(1800, 1500, "mplot::HexGridVisual examples");
    // Set inital scene translation
    v.setSceneTrans (sm::vec<float,3>{ float{-0.627816}, float{0.566485}, float{-5.9089} });
    // You can set a field of view (in degrees)
    v.fov = 15;
    // The coordinate arrows can be shown/hidden
    v.showCoordArrows (false);
    // You can set the background (white, black, or any other colour)
    v.backgroundWhite();
    // You can switch on the "lighting shader" which puts diffuse light into the scene
    v.lightingEffects();

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will all be
    // discarded.
    sm::hexgrid<float, sm::hexalign::point_up> hg1(0.01f, 0.205f, 0.0f);
    hg1.set_circular_boundary (0.1f);
    // Make data for hg1
    sm::vvec<float> data1 = {};
    hexexample::orthogonal_sines (hg1, data1);

    // A second hexgrid and data. This is a 'flat_up' grid
    sm::hexgrid<float, sm::hexalign::flat_up> hg2(0.01f, 0.205f, 0.0f);
    hg2.set_boundary_on_outer_edge();
    sm::vvec<float> data2 = {};
    hexexample::orthogonal_sines (hg2, data2);

    // Reduced hexgrid, copied from hg1r
    sm::hexgrid<float, sm::hexalign::point_up> hg1r = sm::algo::hexgrid::reduced_hexgrid (hg1);
    sm::vvec<float> data1r = {};
    hexexample::orthogonal_sines (hg1r, data1r);

    // Doubly reduced hexgrid
    sm::hexgrid<float, sm::hexalign::point_up> hg1rr = sm::algo::hexgrid::reduced_hexgrid (hg1r);
    sm::vvec<float> data1rr = {};
    hexexample::orthogonal_sines (hg1rr, data1rr);

    // And again
    sm::hexgrid<float, sm::hexalign::point_up> hg1rrr = sm::algo::hexgrid::reduced_hexgrid (hg1rr);
    sm::vvec<float> data1rrr = {};
    hexexample::orthogonal_sines (hg1rrr, data1rrr);

    // More flat up grids (for reduction)
    sm::hexgrid<float, sm::hexalign::flat_up> hgf1(0.01f, 0.205f, 0.0f);
    hgf1.set_circular_boundary (0.1f);
    sm::vvec<float> dataf1 = {};
    hexexample::orthogonal_sines (hgf1, dataf1);

    // Create a reduced hexgrid
    sm::hexgrid<float, sm::hexalign::flat_up> hgf1r = sm::algo::hexgrid::reduced_hexgrid (hgf1);
    // Regenerate data for hgf1r
    sm::vvec<float> dataf1r = {};
    hexexample::orthogonal_sines (hgf1r, dataf1r);
    // Also *map* data from dataf1 onto dataf1r_mapped
    auto mapping1 = sm::algo::hexgrid::reduced_hexgrid_mapping (hgf1, hgf1r);
    sm::vvec<float> dataf1r_mapped = {};
    sm::algo::hexgrid::map_to_reduced_hexgrid (mapping1, dataf1, dataf1r_mapped);

    sm::hexgrid<float, sm::hexalign::flat_up> hgf1rr = sm::algo::hexgrid::reduced_hexgrid (hgf1r);
    sm::vvec<float> dataf1rr = {};
    hexexample::orthogonal_sines (hgf1rr, dataf1rr);

    // Layout parameters
    constexpr float rh = 0.46f; // row height
    constexpr float cw = 0.3f; // col width
    sm::vec<float> label_pos = { -0.1f, -0.15f, 0.0f };
    const mplot::TextFeatures tf(0.02f);

    /*
     * Row 1
     */

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    sm::vec<float, 3> offset = { 0.0f, 0.0f, 0.0f };
    auto hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::CET_L04);
    hgv_pu->setScalarData (&data1);
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->addLabel ("hexalign::point_up\n3D (value is z and colour)\ncircular boundary\nboundary not shown\nHexVisMode::HexInterp\nColourMapType::CET_L04",
                      label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    offset[0] += cw;
    auto hgv_fl = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hg2, offset);
    hgv_fl->set_parent (v.get_id());
    hgv_fl->cm.setType (mplot::ColourMapType::Cork);
    hgv_fl->showboundary = true;
    hgv_fl->setScalarData (&data2);
    hgv_fl->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_fl->zScale.null_scaling();
    hgv_fl->addLabel ("hexalign::flat_up\n2D (value is only colour)\noriginal hex boundary\nboundary indicated in black\nHexVisMode::HexInterp\nColourMapType::Cork",
                      label_pos, tf);
    hgv_fl->finalize();
    v.addVisualModel (hgv_fl);

    offset[0] += cw;
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::Batlow);
    hgv_pu->showboundary = true;
    hgv_pu->setScalarData (&data1);
    hgv_pu->hexVisMode = mplot::HexVisMode::Triangles;
    hgv_pu->zScale.null_scaling();
    hgv_pu->addLabel ("hexalign::point_up\n2D\ncircular boundary\nboundary shown\nHexVisMode::Triangles\nColourMapType::Batlow",
                      label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    offset[0] += cw;
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::Plasma);
    hgv_pu->showboundary = true;
    hgv_pu->setScalarData (&data1);
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->zScale.null_scaling();
    hgv_pu->addLabel ("hexalign::point_up\n2D\ncircular boundary\nboundary shown\nHexVisMode::HexInterp\nColourMapType::Plasma",
                      label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    offset[0] += cw;
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::Acton);
    hgv_pu->showboundary = true;
    hgv_pu->setScalarData (&data1);
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->addLabel ("hexalign::point_up\n3D (boundary always at z=0)\ncircular boundary\nboundary shown\nHexVisMode::HexInterp\nColourMapType::Acton",
                      label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    /*
     * Row 2
     */
    label_pos = { -0.1f, -0.175f, 0.0f };

    offset = {0.0f, -rh, 0.0f };
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::Ice);
    hgv_pu->setScalarData (&data1);
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->zScale.null_scaling();
    hgv_pu->addLabel ("hexalign::point_up\n2D (flat)\ncircular boundary\nboundary shown\nColourMapType::Ice", label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    offset = {cw, -rh, 0.01f };
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1r, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::Ice);
    hgv_pu->showboundary = true;
    hgv_pu->setScalarData (&data1r);
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->zScale.null_scaling();
    hgv_pu->addLabel ("Grid reduced once", label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    offset = {2 * cw, -rh, 0.02f };
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1rr, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::Ice);
    hgv_pu->showboundary = true;
    hgv_pu->setScalarData (&data1rr);
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->zScale.null_scaling();
    hgv_pu->addLabel ("Grid reduced twice", label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    offset = {3 * cw, -rh, 0.0f };
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::Ice);
    hgv_pu->setScalarData (&data1);
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->zScale.null_scaling();
    hgv_pu->addLabel ("Previous 3 shown layered\nLayers separated in z\nUpper two have setAlpha(0.5)", label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    offset = {3 * cw, -rh, 0.01f };
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1r, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::Ice);
    hgv_pu->showboundary = true;
    hgv_pu->setAlpha (0.5f);
    hgv_pu->setScalarData (&data1r);
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->zScale.null_scaling();
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    offset = {3 * cw, -rh, 0.02f };
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1rr, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::Ice);
    hgv_pu->showboundary = true;
    hgv_pu->setAlpha (0.5f);
    hgv_pu->setScalarData (&data1rr);
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->zScale.null_scaling();
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    /*
     * Row 3
     */

    offset = {0.0f, -2 * rh, 0.0f };
    hgv_fl = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hgf1, offset);
    hgv_fl->set_parent (v.get_id());
    hgv_fl->cm.setType (mplot::ColourMapType::CET_L07);
    hgv_fl->setScalarData (&dataf1);
    hgv_fl->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_fl->zScale.null_scaling();
    hgv_fl->addLabel ("hexalign::flat_up\n2D (flat)\ncircular boundary\nboundary shown\nColourMapType::CET_L07", label_pos, tf);
    hgv_fl->finalize();
    v.addVisualModel (hgv_fl);

    offset = {cw, -2 * rh, 0.01f };
    hgv_fl = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hgf1r, offset);
    hgv_fl->set_parent (v.get_id());
    hgv_fl->cm.setType (mplot::ColourMapType::CET_L07);
    hgv_fl->showboundary = true;
    hgv_fl->setScalarData (&dataf1r);
    hgv_fl->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_fl->zScale.null_scaling();
    hgv_fl->addLabel ("Grid reduced once\nData regenerated", label_pos, tf);
    hgv_fl->finalize();
    v.addVisualModel (hgv_fl);

    offset = {2 * cw, -2 * rh, 0.01f };
    hgv_fl = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hgf1r, offset);
    hgv_fl->set_parent (v.get_id());
    hgv_fl->cm.setType (mplot::ColourMapType::CET_L07);
    hgv_fl->showboundary = true;
    hgv_fl->setScalarData (&dataf1r_mapped);
    hgv_fl->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_fl->zScale.null_scaling();
    hgv_fl->addLabel ("Grid reduced once\nData mapped", label_pos, tf);
    hgv_fl->finalize();
    v.addVisualModel (hgv_fl);

    offset = {3 * cw, -2 * rh, 0.02f };
    hgv_fl = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hgf1rr, offset);
    hgv_fl->set_parent (v.get_id());
    hgv_fl->cm.setType (mplot::ColourMapType::CET_L07);
    hgv_fl->showboundary = true;
    hgv_fl->setScalarData (&dataf1rr);
    hgv_fl->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_fl->zScale.null_scaling();
    hgv_fl->addLabel ("Grid reduced twice", label_pos, tf);
    hgv_fl->finalize();
    v.addVisualModel (hgv_fl);

    offset = {4 * cw, -2 * rh, 0.0f };
    hgv_fl = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hgf1, offset);
    hgv_fl->set_parent (v.get_id());
    hgv_fl->cm.setType (mplot::ColourMapType::CET_L07);
    hgv_fl->setScalarData (&dataf1);
    hgv_fl->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_fl->zScale.null_scaling();
    hgv_fl->addLabel ("Previous 3 shown layered\nLayers separated in z\nUpper two have setAlpha(0.5)", label_pos, tf);
    hgv_fl->finalize();
    v.addVisualModel (hgv_fl);

    offset = {4 * cw, -2 * rh, 0.01f };
    hgv_fl = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hgf1r, offset);
    hgv_fl->set_parent (v.get_id());
    hgv_fl->cm.setType (mplot::ColourMapType::CET_L07);
    hgv_fl->showboundary = true;
    hgv_fl->setAlpha (0.5f);
    hgv_fl->setScalarData (&dataf1r);
    hgv_fl->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_fl->zScale.null_scaling();
    hgv_fl->finalize();
    v.addVisualModel (hgv_fl);

    offset = {4 * cw, -2 * rh, 0.02f };
    hgv_fl = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::flat_up, mplot::gl::version_4_1>>(&hgf1rr, offset);
    hgv_fl->set_parent (v.get_id());
    hgv_fl->cm.setType (mplot::ColourMapType::CET_L07);
    hgv_fl->showboundary = true;
    hgv_fl->setAlpha (0.5f);
    hgv_fl->setScalarData (&dataf1rr);
    hgv_fl->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_fl->zScale.null_scaling();
    hgv_fl->finalize();
    v.addVisualModel (hgv_fl);

    v.keepOpen();
}
