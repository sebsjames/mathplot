// Draw a cube with RhomboVisual and then make up vectors to transform with sm::mat<>s

#include <memory>
#include <iostream>

import sm.mat;

import mplot.compoundray.visual;
import mplot.rhombovisual;
import mplot.vectorvisual;
import mplot.spherevisual;
import mplot.coordarrows;

int main()
{
    // Create a scene
    mplot::compoundray::Visual v(1024, 768, "A cube");
    v.showCoordArrows (true);
    v.coordArrowsInScene (false);
    v.lightingEffects();

    // Parameters of the model
    sm::vec<float, 3> offset = { 0, 0, 0 };   // a within-scene offset

    sm::vec<float, 3> e1 = { 1, 0, 0 };
    sm::vec<float, 3> e2 = { 0, 1, 0 };
    sm::vec<float, 3> e3 = { 0, 0, 1 };

    sm::vec<float, 3> colour1 = { 0.35,  0.76,  0.98 };  // RGB colour triplet

    auto rv = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, colour1);
    rv->set_parent (v.get_id());
    rv->raise_corner = true;
    rv->name = "Cube.002";
    rv->facecm = mplot::ColourMapType::Rainbow; // Try Rainbow, Batlow, Tofino
    rv->annotate = true;
    rv->tf.fontres = 64;
    //rv->setAlpha (0.7f);
    rv->finalize();

    // locations/vectors
    sm::vec<> l1_s = { 0.8, 1, 0.5 }; // start location
    sm::vec<> mv1 = { 0.2, 0, 0 };  // movement to edge
    sm::vec<> mv2 = { 0.1, 0, 0 };  // movement past edge
    sm::vec<> ra = { 0, 0, -1 };    // rotn axis
    sm::vec<> d_l1_s_s = { 0, 0, 0 };
    sm::vec<> d_l1_s_e = { 0.3, 0, 0 }; // Direction at l1_s - our step length
    sm::vec<> d_l1_s = d_l1_s_e - d_l1_s_s;
    auto rotang1 = sm::mathconst<float>::pi / 2;

    // matrix transformation
    sm::mat<float, 4> m1t;
    sm::mat<float, 4> m1tor;
    sm::mat<float, 4> m1r;
    sm::mat<float, 4> m1torb;
    sm::mat<float, 4> m1t2;
    // sequence:
    m1t.translate (mv1);
    m1tor.translate (-(l1_s + mv1));
    m1r.rotate (ra, rotang1);
    m1torb.translate (l1_s + mv1);
    m1t2.translate (m1r * mv2);
    // Combine by multiplication:
    sm::mat<float, 4> m1 = m1t2 * m1torb * m1r * m1tor * m1t;

    // Apply matrix
    sm::vec<> l1_e = (m1 * l1_s).less_one_dim();
    sm::vec<> d_l1_e_s = (m1 * d_l1_s_s).less_one_dim();
    sm::vec<> d_l1_e_e = (m1 * d_l1_s_e).less_one_dim();
    sm::vec<> d_l1_e = d_l1_e_e - d_l1_e_s;

    auto sv = std::make_unique<mplot::SphereVisual<>>(l1_s, 0.005, mplot::colour::magenta);
    sv->set_parent (v.get_id());
    sv->finalize();
    v.addVisualModel (sv);

    auto vvm = std::make_unique<mplot::VectorVisual<float, 3>>(l1_s);
    vvm->set_parent (v.get_id());
    vvm->thevec = d_l1_s;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness *= 0.02;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::crimson;
    vvm->finalize();
    v.addVisualModel (vvm);

    sv = std::make_unique<mplot::SphereVisual<>>(l1_e, 0.02, mplot::colour::goldenrod3);
    sv->set_parent (v.get_id());
    sv->finalize();
    v.addVisualModel (sv);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(l1_e);
    vvm->set_parent (v.get_id());
    vvm->thevec = d_l1_e;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness *= 0.02;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::blue;
    vvm->finalize();
    v.addVisualModel (vvm);

    // second movement, this one with a CoordArrows object
    // An CoordArrows model, with non-defaults
    sm::vec<> ca1 = {.5, 1, .5};
    sm::vec<> cshift1 = sm::vec<>{1,0,-1} * (sm::mathconst<float>::root_2 / 10.0f);
    ca1 -= cshift1;

    // 4 frames to transform
    sm::vec<mplot::CoordArrows<>*, 4> ptrs;
    for (int i = 0; i < 4; ++i) {
        auto cavm = std::make_unique<mplot::CoordArrows<>> (ca1);
        cavm->set_parent (v.get_id());
        cavm->init (sm::vec<>::ux(), sm::vec<>::uy(), sm::vec<>::uz());
        cavm->lengths = sm::vec<>{.04,.04,.04} * (1+i);
        cavm->thickness = 0.5f;
        cavm->em = 0.0f;
        cavm->showneg = true;
        cavm->endsphere_size = 0.1f;
        cavm->finalize();
        ptrs[i] = v.addVisualModel (cavm);
    }

    std::cout << "cam frame 0 viewmatrix:\n" << ptrs[0]->getViewMatrix() << std::endl;

    m1t.set_identity();
    m1tor.set_identity();
    m1r.set_identity();
    m1torb.set_identity();
    m1t2.set_identity();

    m1t.translate (cshift1);

    m1tor.translate (-(ca1 + cshift1));

    ra = { -1, 0, -1 }; // diagonal
    sm::vec<> n1 = { 0, 1, 0 };
    sm::vec<> coroffs = { 0, 0.2, 0 };
    sm::vec<> n2 = (-e1 - e3).cross (coroffs - e3);
    n2 = -n2;
    n2.renormalize();
    std::cout << "n2: " << n2 << std::endl;
    float rotang = n1.angle (n2, ra);
    ra.renormalize(); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    std::cout << "axis: " << ra << " angle " << rotang << std::endl;
    m1r.rotate (ra, rotang);
    std::cout << "m1r = \n" << m1r << std::endl;

    m1torb.translate (ca1 + cshift1);

    m1t2.translate (m1r * cshift1);

    ptrs[1]->premultViewMatrix (m1t);
    ptrs[2]->premultViewMatrix (m1r * m1tor * m1t);
    ptrs[3]->premultViewMatrix (m1torb * m1r * m1tor * m1t);

    std::cout << "cam frame 1 viewmatrix:\n" << ptrs[1]->getViewMatrix() << std::endl;

    sv = std::make_unique<mplot::SphereVisual<>>(l1_e, 0.01, mplot::colour::mediumpurple1);
    sv->set_parent (v.get_id());
    sv->finalize();
    v.addVisualModel (sv);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(sm::vec<>{1, 1.2, 0});
    vvm->set_parent (v.get_id());
    vvm->thevec = n2/5.0f;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness *= 0.02;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::cadetblue1;
    vvm->finalize();
    v.addVisualModel (vvm);
    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(sm::vec<>{1, 1, 1});
    vvm->set_parent (v.get_id());
    vvm->thevec = n2/5.0f;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness *= 0.02;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::cadetblue1;
    vvm->finalize();
    v.addVisualModel (vvm);

    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(sm::vec<>{0, 1, 1});
    vvm->set_parent (v.get_id());
    vvm->thevec = n1/5.0f;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness *= 0.02;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::cadetblue1;
    vvm->finalize();
    v.addVisualModel (vvm);
    vvm = std::make_unique<mplot::VectorVisual<float, 3>>(sm::vec<>{0, 1, 0});
    vvm->set_parent (v.get_id());
    vvm->thevec = n1/5.0f;
    vvm->vgoes = mplot::VectorGoes::FromOrigin;
    vvm->thickness *= 0.02;
    vvm->fixed_colour = true;
    vvm->single_colour = mplot::colour::cadetblue1;
    vvm->finalize();
    v.addVisualModel (vvm);


    v.addVisualModel (rv); // Cube last to ensure we see other vectors through it.
    v.keepOpen();

    return 0;
}
