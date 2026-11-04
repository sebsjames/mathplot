/*
 * Showing an ellipse drawn with sm::algo::hexgrid::ellipse_indices()
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
import sm.algo.hexgrid;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;

int main()
{
    // Contructor args are width, height, title
    mplot::Visual<mplot::gl::version_4_1> v(1800, 1800, "mplot::HexGridVisual examples");
    v.setSceneTrans (sm::vec<float,3>{ float{-0.019244}, float{0.0875307}, float{-1.78171} });
    v.fov = 15;
    v.lightingEffects();
    v.rotateAboutNearest (true);

    sm::vec<float> label_pos = { -0.1f, -0.15f, 0.0f };
    // Choose font size 0.02 and bitmap density 64 (higher than default as the text is quite big on
    // the screen)
    const mplot::TextFeatures tf(0.02f, 64);

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will all be
    // discarded.
    sm::hexgrid<float, sm::hexalign::point_up> hg1(0.005f, 0.605f, 0.0f);
    hg1.set_elliptical_boundary (0.2f, 0.1f);
    sm::vvec<float> data1 (hg1.num(), 0.0f);

    // Mark indices
    sm::vvec<std::uint32_t> ellipse_indices = sm::algo::hexgrid::ellipse_indices (hg1, 0.04f, 0.02f, sm::mathconst<float>::pi_over_3);
    for (auto i : ellipse_indices) { data1[i] = 0.05f; }

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    sm::vec<float, 3> offset = { 0.0f, 0.0f, 0.0f };
    auto hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::CET_L04);
    hgv_pu->setScalarData (&data1);
    hgv_pu->showboundary = true;
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->hex_size_factor = 0.95f;
    hgv_pu->hex_draw_threshold = 0.1f;
    hgv_pu->hex_invisible_alpha = 0.5f;
    hgv_pu->addLabel ("ellipse", label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    v.keepOpen();
}
