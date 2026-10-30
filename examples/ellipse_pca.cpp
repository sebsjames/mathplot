/*
 * Applying principle component analysis to some random data
 */
#include <iostream>
#include <memory>
#include <string>
#include <cmath>

import sm.mathconst;
import sm.random;
import sm.mat;
import sm.pca;

import mplot.visual;
import mplot.graphvisual;

int main()
{
    constexpr unsigned int n_samp = 100;

    // Create data: Draw some samples from 2D Gaussian and rotate
    sm::rand_normal<float> rn1 (0.0f, 2.0f);
    sm::rand_normal<float> rn2 (0.0f, 0.5f);
    sm::vvec<sm::vec<float, 2>> _x (n_samp, {0});
    sm::mat<float, 2> rotn;
    rotn.rotate (sm::mathconst<float>::pi_over_8);
    for (unsigned int i = 0; i < n_samp; ++i) {
        _x[i] = rotn * sm::vec<float, 2>{ rn1.get(), rn2.get() };
    }

    // Graph the data
    mplot::Visual v(1024, 768, "Principle component analysis with sm.pca");
    // Create a GraphVisual object (obtaining a unique_ptr to the object) with a spatial offset within the scene of 0,0,0
    auto gv = std::make_unique<mplot::GraphVisual<float>> (sm::vec<float>{-0.5f,-0.5f,0.0f});
    mplot::DatasetStyle ds (mplot::stylepolicy::markers);
    ds.datalabel = std::string("data");
    gv->set_parent (v.get_id());
    gv->setlimits (-8, 8, -8, 8);
    gv->setdata (_x, ds);

    // Compute principle components using sm.pca
    sm::pca::result<float, 2> pca_res = sm::pca::compute<float, 2> (_x);

    std::cout << "Angle of first component " << pca_res.pc_vectors[0] << " is "
              << pca_res.pc_vectors[0].angle() * sm::mathconst<float>::rad2deg
              << " and magnitude(variance) " << pca_res.pc_magnitudes[0] << std::endl;

    std::cout << "Angle of 2nd component " << pca_res.pc_vectors[1] << " is "
              << pca_res.pc_vectors[1].angle() * sm::mathconst<float>::rad2deg
              << " and magnitude(variance) " <<  pca_res.pc_magnitudes[1] << std::endl;

    // Show 3 sigma axes
    mplot::DatasetStyle ds2 (mplot::stylepolicy::lines);
    ds2.setcolour (mplot::colour::crimson);
    ds2.datalabel = std::string("PC1 3") + mplot::unicode::toUtf8 (mplot::unicode::sigma);
    sm::vvec<sm::vec<float, 2>> pc1vv = { {0,0}, pca_res.pc_vectors[0] * 3.0f * std::sqrt(pca_res.pc_magnitudes[0])};
    gv->setdata (pc1vv, ds2);

    ds2.setcolour (mplot::colour::orange);
    ds2.datalabel = std::string("PC2 3") + mplot::unicode::toUtf8 (mplot::unicode::sigma);
    sm::vvec<sm::vec<float, 2>> pc2vv = { {0,0}, pca_res.pc_vectors[1] * 3.0f * std::sqrt(pca_res.pc_magnitudes[1]) };
    gv->setdata (pc2vv, ds2);

    gv->finalize();
    v.addVisualModel (gv);
    v.keepOpen();
}
