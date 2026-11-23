// Draw a cube with RhomboVisual and then make up vectors to transform with mat44s

#include <memory>

#include <Eigen/Dense>
#include <Eigen/Geometry>

import sm.mat;

import mplot.visual;
import mplot.rhombovisual;
import mplot.vectorvisual;
import mplot.spherevisual;

int main()
{
    // Create a scene
    mplot::Visual v(1024, 768, "A cube");
    v.showCoordArrows (true);
    v.coordArrowsInScene (true);
    v.lightingEffects();

    // Parameters of the model
    sm::vec<float, 3> offset = { 0, 0, 0 };   // a within-scene offset

    sm::vec<float, 3> e1 = { 1, 0, 0 };
    sm::vec<float, 3> e2 = { 0, 1, 0 };
    sm::vec<float, 3> e3 = { 0, 0, 1 };

    sm::vec<float, 3> colour1 = { 0.35,  0.76,  0.98 };  // RGB colour triplet

    auto rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, colour1);
    rv->set_parent (v.get_id());
    rv->name = "Cube.002";
    rv->facecm = mplot::ColourMapType::Rainbow; // Try Rainbow, Batlow, Tofino
    rv->annotate = true;
    rv->setAlpha (0.5f);
    rv->finalize();

    // Native locations/vectors
    sm::vec<> l1 = { 0.8, 1, 0.5 }; // start location
    sm::vec<> mv1 = { 0.2, 0, 0 };  // movement to edge
    sm::vec<> mv2 = { 0.1, 0, 0 };  // movement past edge
    sm::vec<> ra = { 0, 0, -1 };    // rotn axis
    sm::vec<> d1_s = { 0, 0, 0 };
    sm::vec<> d1_e = { 0.3, 0, 0 }; // Direction at l1 - our step length
    sm::vec<> d1 = d1_e - d1_s;

    auto rotang = sm::mathconst<float>::pi / 2;

    // Eigen copies
    Eigen::Vector3f el1 (l1[0], l1[1], l1[2]);
    Eigen::Vector3f emv1 (mv1[0], mv1[1], mv1[2]);
    Eigen::Vector3f emv2 (mv2[0], mv2[1], mv2[2]);
    Eigen::Vector3f era (ra[0], ra[1], ra[2]);
    Eigen::Vector3f ed1_s (d1_s[0], d1_s[1], d1_s[2]);
    Eigen::Vector3f ed1_e (d1_e[0], d1_e[1], d1_e[2]);

    // mat transformation
    sm::mat<float, 4> m1t;
    sm::mat<float, 4> m1tor;
    sm::mat<float, 4> m1r;
    sm::mat<float, 4> m1torb;
    sm::mat<float, 4> m1t2;
    // sequence:
    m1t.translate (mv1);
    m1tor.translate (-(l1 + mv1));
    m1r.rotate (ra, rotang);
    m1torb.translate (l1 + mv1);
    m1t2.translate (m1r * mv2);
    // Combine by multiplication:
    sm::mat<float, 4> m1 = m1t2 * m1torb * m1r * m1tor * m1t;

    // Eigen transformation
    Eigen::Transform<float, 3, Eigen::TransformTraits::Affine> em1t;
    Eigen::Transform<float, 3, Eigen::TransformTraits::Affine> em1tor;
    Eigen::Transform<float, 3, Eigen::TransformTraits::Affine> em1r;
    Eigen::Transform<float, 3, Eigen::TransformTraits::Affine> em1torb;
    Eigen::Transform<float, 3, Eigen::TransformTraits::Affine> em1t2;
    em1t.setIdentity();
    em1tor.setIdentity();
    em1r.setIdentity();
    em1torb.setIdentity();
    em1t2.setIdentity();
    // sequence:
    em1t.translate (emv1);
    em1tor.translate (-(el1 + emv1));
    em1r.rotate (Eigen::AngleAxisf(rotang, era));
    em1torb.translate (el1 + emv1);
    em1t2.translate (em1r * emv2);
    // Combine by multiplication:
    Eigen::Transform<float, 3, Eigen::TransformTraits::Affine> em1 = em1t2 * em1torb * em1r * em1tor * em1t;

    //em1 = em1t * em1r; IS NOT EQUAL TO:
    //em1.rotate (Eigen::AngleAxisf(rotang, era));
    //em1.translate (emv1);

    //em1 = em1t * em1r; IS NOT EQUAL TO:
    //em1.translate (emv1);
    //em1.prerotate (Eigen::AngleAxisf(rotang, era));

    //em1 = em1t * em1r; IS NOT EQUAL TO:
    //em1.prerotate (Eigen::AngleAxisf(rotang, era));
    //em1.translate (emv1);

    //em1 = em1t * em1r; IS NOT EQUAL TO:
    //em1.prerotate (Eigen::AngleAxisf(rotang, era)).translate (emv1);

    // This is quivalent to em1 = em1t * em1r;
    //em1.translate (emv1);
    //em1.rotate (Eigen::AngleAxisf(rotang, era));

    /**
     * IN EIGEN:
     *
     * The order in which you call translate() and rotate() is the left to right order in whcih the
     * matrices get multiplied
     *
     * THe prefix 'pre' refers to the left to right order of multiplications NOT the order in which
     * the transforms are applied.
     */

    // Apply sm::mat
    sm::vec<> l2 = (m1 * l1).less_one_dim();
    sm::vec<> d2_s = (m1 * d1_s).less_one_dim();
    sm::vec<> d2_e = (m1 * d1_e).less_one_dim();
    sm::vec<> d2 = d2_e - d2_s;
    // Apply Eigen
    Eigen::Vector3f el2 = (em1 * el1);
    Eigen::Vector3f ed2_s = em1 * ed1_s;
    Eigen::Vector3f ed2_e = em1 * ed1_e;
    Eigen::Vector3f ed2 = ed2_e - ed2_s;
    // Convert Eigen vector results to sm::vec
    sm::vec<> eig_l2 = { el2[0], el2[1], el2[2] };
    sm::vec<> eig_d2 = { ed2[0], ed2[1], ed2[2] };


    auto sv = std::make_unique<mplot::SphereVisual<>>(l1, 0.005, mplot::colour::magenta);
    sv->set_parent (v.get_id());
    sv->finalize();
    v.addVisualModel (sv);

    auto vvm = std::make_unique<mplot::VectorVisual<float, 3>>(l1);
    vvm->set_parent (v.get_id());
    vvm->thevec = d1;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness *= 0.02;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::crimson;
    vvm->finalize();
    v.addVisualModel (vvm);

    sv = std::make_unique<mplot::SphereVisual<>>(l2, 0.02, mplot::colour::goldenrod3);
    sv->set_parent (v.get_id());
    sv->finalize();
    v.addVisualModel (sv);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(l2);
    vvm->set_parent (v.get_id());
    vvm->thevec = d2;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness *= 0.02;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::blue;
    vvm->finalize();
    v.addVisualModel (vvm);

    sv = std::make_unique<mplot::SphereVisual<>>(eig_l2, 0.01, mplot::colour::mediumpurple1);
    sv->set_parent (v.get_id());
    sv->finalize();
    v.addVisualModel (sv);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(eig_l2);
    vvm->set_parent (v.get_id());
    vvm->thevec = eig_d2;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness *= 0.02;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::cadetblue1;
    vvm->finalize();
    v.addVisualModel (vvm);


    v.addVisualModel (rv); // Cube last (F7 to select) to ensure we see other vectors through it.

    v.keepOpen();

    return 0;
}
