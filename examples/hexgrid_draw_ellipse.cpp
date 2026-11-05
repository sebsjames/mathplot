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
#include <set>
#include <list>
#include <format>
#include <tuple>

import sm.vec;
import sm.hexgrid;
import sm.algo.hexgrid;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;

int main (int argc, char** argv)
{
    float ex = 0.01f;
    if (argc > 1) {
        ex = std::stof (argv[1]);
    }
    std::cout << "Your unused value: " << ex << std::endl;

    // Contructor args are width, height, title
    mplot::Visual<mplot::gl::version_4_1> v(1800, 1800, "mplot::HexGridVisual examples");
    v.setSceneTrans (sm::vec<float,3>{ float{-0.019244}, float{0.0875307}, float{-1.78171} });
    v.fov = 15;
    v.lightingEffects();
    v.rotateAboutNearest (true);

    sm::vec<float> label_pos = { -0.13f, -0.15f, 0.0f };
    // Choose font size 0.02 and bitmap density 64 (higher than default as the text is quite big on
    // the screen)
    const mplot::TextFeatures tf(0.008f, 48);

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will all be
    // discarded.
    sm::hexgrid<float, sm::hexalign::point_up> hg1(0.005f, 0.28f, 0.0f);
    hg1.set_boundary_on_outer_edge();
    sm::vvec<float> data1 (hg1.num(), 0.002f);

    // Mark indices
    std::set<std::uint32_t> ellipse_indices = sm::algo::hexgrid::ellipse_indices (hg1, 0.04f, 0.02f,
                                                                                  sm::mathconst<float>::pi_over_3,
                                                                                  sm::vec<float, 2>{0.0f, 0.0f});
#if 0
    // Mark indices for a second ellipse
    std::set<std::uint32_t> ellipse2_indices = sm::algo::hexgrid::ellipse_indices (hg1, 0.04f, 0.01f,
                                                                                   2.0f * sm::mathconst<float>::pi_over_3,
                                                                                   sm::vec<float, 2>{ex, -0.04f});
    // Combine the sets
    ellipse_indices.insert (ellipse2_indices.begin(), ellipse2_indices.end());
#endif
    // Set data for the indices
    for (auto i : ellipse_indices) { data1[i] = 0.01f; }

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    sm::vec<float, 3> offset = { 0.0f, 0.0f, 0.0f };
    auto hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::CET_L18);
    hgv_pu->colourScale.compute_scaling (0.0f, 0.015f);
    hgv_pu->zScale.null_scaling();
    hgv_pu->setScalarData (&data1);
    hgv_pu->showboundary = true;
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->hex_size_factor = 0.95f;
    hgv_pu->addLabel ("ellipse drawing with algo::hexgrid::ellipse_indices", label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    std::cout << "====================\n\n";

    sm::hexgrid<float, sm::hexalign::point_up> hg2 (1.64572e-05f, 0.000197486f, 0.0f);
    hg2.set_boundary_on_outer_edge();
    sm::vvec<float> data2 (hg2.num(), 0.002f);
    auto indices = sm::algo::hexgrid::ellipse_indices (hg2, 1.0f * 1.64572e-05f, 3.5f * 1.64572e-05f, 0.0f);
    //auto indices = sm::algo::hexgrid::ellipse_indices (hg2, 3.5f * 1.64572e-05f, 1.0f * 1.64572e-05f, 0.0f);
    for (auto vi : indices) {
        data2[vi] = 0.01f;
    }

    offset = { 0.3f, 0.0f, 0.0f };
    hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg2, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::CET_L18);
    hgv_pu->colourScale.compute_scaling (0.0f, 0.015f);
    hgv_pu->zScale.null_scaling();
    hgv_pu->setScalarData (&data2);
    hgv_pu->showboundary = true;
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->hex_size_factor = 0.95f;
    hgv_pu->addLabel ("tiny ellipse (1000x hexgrid size)", label_pos, tf);
    hgv_pu->finalize();
    auto ptr = v.addVisualModel (hgv_pu);
    ptr->scaleViewMatrix (1000);



    v.keepOpen();
}
