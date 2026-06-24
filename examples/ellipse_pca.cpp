/*
 * Applying principle component analysis (using arma) to some random data
 */
#include <memory>
#include <iostream>
#include <armadillo>

import sm.mathconst;
import sm.random;
import sm.mat;

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
    mplot::Visual v(1024, 768, "Principle component analysis with armadillo");
    // Create a GraphVisual object (obtaining a unique_ptr to the object) with a spatial offset within the scene of 0,0,0
    auto gv = std::make_unique<mplot::GraphVisual<float>> (sm::vec<float>{-0.5f,-0.5f,0.0f});
    mplot::DatasetStyle ds (mplot::stylepolicy::markers);
    ds.datalabel = std::string("data");
    gv->set_parent (v.get_id());
    gv->setlimits (-8, 8, -8, 8);
    gv->setdata (_x, ds);

    std::cout << "\narma gives:\n";
    // Place data in arma::Mat
    arma::Mat<float> x(_x.size(), 2);
    for (unsigned int i = 0; i < _x.size(); ++i) {
        x(i, 0) = _x[i][0];
        x(i, 1) = _x[i][1];
    }

    // Apply arma::princomp
    arma::Mat<float> co, sc;
    arma::Col<float> lat, tsq;
    arma::princomp (co, sc, lat, tsq, x);

    std::cout << "coeff: " << co << std::endl;
    //std::cout << "scores: " << sc << std::endl;
    std::cout << "latent: " << lat << std::endl;

    // Mat access is (r, c)
    sm::vec<float, 2> pc1vec = { co(0, 0), co(1, 0) };
    float angle1 = pc1vec.angle();
    std::cout << "Angle of first component " << pc1vec << " is " << angle1 * sm::mathconst<float>::rad2deg
              << " and length is " << lat(0) << std::endl;
    sm::vec<float, 2> pc2vec = { co(0, 1), co(1, 1) };
    float angle2 = pc2vec.angle();
    std::cout << "Angle of 2nd component " << pc2vec << " is " << angle2 * sm::mathconst<float>::rad2deg
              << " and length is " << lat(1) << std::endl;

    mplot::DatasetStyle ds2 (mplot::stylepolicy::lines);
    // Show 3 sigma axes
    ds2.setcolour (mplot::colour::crimson);
    ds2.datalabel = std::string("PC1 3") + mplot::unicode::toUtf8 (mplot::unicode::sigma);
    sm::vvec<sm::vec<float, 2>> pc1vv = { {0,0}, pc1vec * 3.0f * std::sqrt(lat(0))};
    gv->setdata (pc1vv, ds2);

    ds2.setcolour (mplot::colour::orange);
    ds2.datalabel = std::string("PC2 3") + mplot::unicode::toUtf8 (mplot::unicode::sigma);
    sm::vvec<sm::vec<float, 2>> pc2vv = { {0,0}, pc2vec * 3.0f * std::sqrt(lat(1)) };
    gv->setdata (pc2vv, ds2);

    gv->finalize();
    v.addVisualModel (gv);
    v.keepOpen();
}
