/*
 * Test/demo what hexgrids do when transforms are introduced.
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

import sm.vec;
import sm.hexgrid;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;
import mplot.vectorvisual;

template<sm::hexalign halign>
void draw (mplot::Visual<>& v, sm::vec<> offset);

int main()
{
    mplot::Visual<> v(1600, 1000, "mplot::HexGridVisual transforms");
    draw<sm::hexalign::point_up> (v, sm::vec<>{});
    draw<sm::hexalign::flat_up> (v, sm::vec<>{ 0.0f, -0.3f, 0.0f });
    v.keepOpen();
}

// Draw a single grid and its neighbour vectors
template<sm::hexalign halign>
void draw_one (mplot::Visual<>& v, sm::hexgrid<float, halign>& hg,
               const std::vector<sm::vec<>>* data1, const sm::vec<>& offset, const std::string& label)
{
    // sm::HexVisMode::HexInterp to see the hexagons or sm::HexVisMode::Triangles for a smoother surface plot
    const mplot::HexVisMode visMode = mplot::HexVisMode::HexInterp;

    // Add a HexGridVisual to display the HexGrid within the sm::Visual scene
    auto hgv1 = std::make_unique<mplot::HexGridVisual<float, halign>>(&hg, offset);
    hgv1->set_parent (v.get_id());
    hgv1->cm.setType (mplot::ColourMapType::Duochrome);
    hgv1->setVectorData (data1);
    hgv1->hexVisMode = visMode;
    hgv1->zScale.null_scaling();
    hgv1->addLabel (label, sm::vec<>{ -0.1f, -0.15f }, mplot::TextFeatures(0.02f));
    hgv1->finalize();
    v.addVisualModel (hgv1);

    auto vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset);
    vvm->set_parent (v.get_id());
    vvm->thevec = hg.dirn_r();
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::crimson;
    vvm->finalize();
    v.addVisualModel (vvm);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset);
    vvm->set_parent (v.get_id());
    vvm->thevec = hg.dirn_g();
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::springgreen2;
    vvm->finalize();
    v.addVisualModel (vvm);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(offset);
    vvm->set_parent (v.get_id());
    vvm->thevec = hg.dirn_b();
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness = hg.d / 10;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::royalblue2;
    vvm->finalize();
    v.addVisualModel (vvm);
}

// Draw the original and all transformed grids in one row.
template<sm::hexalign halign>
void draw (mplot::Visual<mplot::gl::version_4_1>& v, sm::vec<> offset)
{
    sm::hexgrid<float, halign> hg1(0.01f, 0.205f, 0.0f);
    hg1.set_boundary_on_outer_edge();
    auto ch = hg1.hexen.begin();

    std::vector<sm::vec<>> data1(hg1.num());
    for (unsigned int ri = 0; ri < hg1.num(); ++ri) {
        // We visualize the original x/y position on the grid with a Duochrome colourmap
        data1[ri][0] = hg1.d_x[ri];
        data1[ri][1] = hg1.d_y[ri];
    }

    // Centre hex is reddish
    data1[ch->vi] = { 0.1f, 0, 0 };
    // neighbour0 is greenish
    auto chn0 = ch->get_neighbour(0);
    data1[chn0->vi] = { 0, 0.1f, 0 };

    // Draw original grid
    draw_one (v, hg1, &data1, offset, "Original grid");

    // A rotational transform
    sm::mat<float, 4> rotn;
    rotn.rotate (sm::vec<>::uz(), sm::mathconst<float>::pi_over_2);

    // A reflection
    sm::mat<float, 4> mirror = sm::mat<float, 4>::reflection (sm::vec<>{}, sm::vec<>::ux());

    sm::hexgrid<float, halign> hg1_mirror = hg1;
    hg1_mirror.transform (mirror);
    offset[0] += 0.3f;
    draw_one (v, hg1_mirror, &data1, offset, "Mirror (y-z)");

    sm::hexgrid<float, halign> hg1_rotate = hg1;
    hg1_rotate.transform (rotn);
    offset[0] += 0.3f;
    draw_one (v, hg1_rotate, &data1, offset, "Rotate (pi/2)");

    sm::hexgrid<float, halign> hg1_mirr_rot = hg1;
    hg1_mirr_rot.transform (mirror * rotn);
    offset[0] += 0.3f;
    draw_one (v, hg1_mirr_rot, &data1, offset, "Rotn then mirr");

    sm::hexgrid<float, halign> hg1_rot_mirr = hg1;
    hg1_rot_mirr.transform (rotn * mirror);
    offset[0] += 0.3f;
    draw_one (v, hg1_rot_mirr, &data1, offset, "Mirr then rotn");
}
