/*
 * An example mplot::Visual scene, containing one hexgrid
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
            data[ri] = 0.05f * std::sin (20.0f * hg.d_x[ri]) * std::sin (10.0f * hg.d_y[ri]) ; // Range 0->1
        }
    }
}

int main()
{
    // Contructor args are width, height, title
    mplot::Visual<mplot::gl::version_4_1> v(1800, 1800, "mplot::HexGridVisual examples");
    // Set inital scene translation
    v.setSceneTrans (sm::vec<float,3>{ float{-0.019244}, float{0.0875307}, float{-1.78171} });
    // You can set a field of view (in degrees)
    v.fov = 15;
    // The coordinate arrows can be shown/hidden
    v.showCoordArrows (false);
    // You can set the background (white, black, or any other colour)
    v.backgroundWhite();
    // You can switch on the "lighting shader" which puts diffuse light into the scene
    v.lightingEffects();
    v.rotateAboutNearest (true);

    sm::vec<float> label_pos = { -0.1f, -0.15f, 0.0f };
    const mplot::TextFeatures tf(0.02f);

    // Create a HexGrid to show in the scene. Hexes outside the circular boundary will all be
    // discarded.
    sm::hexgrid<float, sm::hexalign::point_up> hg1(0.01f, 0.205f, 0.0f);
    hg1.set_circular_boundary (0.1f);
    // Make data for hg1
    sm::vvec<float> data1 = {};
    hexexample::orthogonal_sines (hg1, data1);

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    sm::vec<float, 3> offset = { 0.0f, 0.0f, 0.0f };
    auto hgv_pu = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, mplot::gl::version_4_1>>(&hg1, offset);
    hgv_pu->set_parent (v.get_id());
    hgv_pu->cm.setType (mplot::ColourMapType::CET_L04);
    hgv_pu->setScalarData (&data1);
    hgv_pu->showboundary = true;
    hgv_pu->hexVisMode = mplot::HexVisMode::HexInterp;
    hgv_pu->hex_draw_threshold = 0.5f;
    hgv_pu->hex_invisible_alpha = 0.5f;
    hgv_pu->addLabel ("hexalign::point_up\n3D (value is z and colour)\ncircular boundary\nboundary shown\nLower values transparent\nHexVisMode::HexInterp\nColourMapType::CET_L04",
                      label_pos, tf);
    hgv_pu->finalize();
    v.addVisualModel (hgv_pu);

    v.keepOpen();
}
