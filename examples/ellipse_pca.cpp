/*
 * Applying principal component analysis (using arma) to some random data. Compare with sm::pca.
 */
#include <memory>
#include <iostream>
#include <armadillo>

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
    sm::rand_normal<float> rn1 (0.0f, 2.0f, 420);
    sm::rand_normal<float> rn2 (0.0f, 0.5f, 530);
    sm::vvec<sm::vec<float, 2>> _x (n_samp, {0});
    sm::mat<float, 2> rotn;
    rotn.rotate (sm::mathconst<float>::pi_over_8);
    for (unsigned int i = 0; i < n_samp; ++i) {
        _x[i] = rotn * sm::vec<float, 2>{ rn1.get(), rn2.get() };
        _x[i][0] += 2;
    }

    // Graph the data
    mplot::Visual v(1024, 768, "Principal component analysis with armadillo");

    // Create a GraphVisual object (obtaining a unique_ptr to the object) with a spatial offset within the scene of 0,0,0
    auto gv = std::make_unique<mplot::GraphVisual<float>> (sm::vec<float>{-1.6f,-0.5f,0.0f});
    mplot::DatasetStyle ds (mplot::stylepolicy::markers);
    ds.markersize = 0.01f;
    ds.datalabel = std::string("data");
    gv->set_parent (v.get_id());
    gv->setlimits (-8, 8, -8, 8);
    gv->setdata (_x, ds);

    // Graph for sm::pca
    auto gv2 = std::make_unique<mplot::GraphVisual<float>> (sm::vec<float>{0.2f,-0.5f,0.0f});
    gv2->set_parent (v.get_id());
    gv2->setlimits (-8, 8, -8, 8);
    gv2->setdata (_x, ds);

    //
    // arma::princomp
    //
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
    auto gvp = v.addVisualModel (gv);
    gvp->addLabel ("Armadillo", sm::vec<>{0.0f, 1.2f, 0.0f}, mplot::TextFeatures(0.075f));


    //
    // sm::pca
    //
    std::cout << "\nsm::pca gives:\n";

    sm::pca::result<float, 2> pca_res = sm::pca::compute<float, 2> (_x);

    for (std::uint32_t i = 0; i < 2; ++i) {
        std::cout << "PC " << (i + 1) << " = " << pca_res.pc_ev_real[i] << " which accounts for " << pca_res.pc_mags[i] << " of the variability\n";
    }

    std::cout << "covariance of x\n" << pca_res.covariance << std::endl;

    // What are these equivalents to?
    std::cout << "coeff? pca_res.pc_ev_real: " <<  pca_res.pc_ev_real << std::endl;
    std::cout << "latent? pca_res.pc_mags: " << pca_res.pc_mags << std::endl;

    sm::vec<sm::mat<float, 2>::eigenpair, 2> pairs = pca_res.covariance.eigenpairs();
    std::cout << "Eigenvalues: ";
    for (std::uint32_t i = 0; i < 2; ++i) { std::cout << pairs[i].eigenvalue << ", "; }
    std::cout << std::endl;

    std::cout << "Eigenvectors: ";
    for (std::uint32_t i = 0; i < 2; ++i) { std::cout << pairs[i].eigenvector << ", "; }
    std::cout << std::endl;

    // Mat access is (r, c)
    pc1vec = pca_res.pc_ev_real[0];
    angle1 = pc1vec.angle();
    std::cout << "Angle of first component " << pc1vec << " is " << angle1 * sm::mathconst<float>::rad2deg
              << " and length is " << pca_res.pc_mags[0] << std::endl;
    pc2vec = pca_res.pc_ev_real[1];
    angle2 = pc2vec.angle();
    std::cout << "Angle of 2nd component " << pc2vec << " is " << angle2 * sm::mathconst<float>::rad2deg
              << " and length is " << pca_res.pc_mags[1] << std::endl;

    // Show 3 sigma axes
    ds2.setcolour (mplot::colour::crimson);
    ds2.datalabel = std::string("PC1 3") + mplot::unicode::toUtf8 (mplot::unicode::sigma);
    pc1vv = { {0,0}, pc1vec * 3.0f * std::sqrt(pca_res.pc_mags[0])};
    gv2->setdata (pc1vv, ds2);

    ds2.setcolour (mplot::colour::orange);
    ds2.datalabel = std::string("PC2 3") + mplot::unicode::toUtf8 (mplot::unicode::sigma);
    pc2vv = { {0,0}, pc2vec * 3.0f * std::sqrt(pca_res.pc_mags[1]) };
    gv2->setdata (pc2vv, ds2);

    gv2->finalize();
    auto gv2p = v.addVisualModel (gv2);
    gv2p->addLabel ("sm::pca", sm::vec<>{0.0f, 1.2f, 0.0f}, mplot::TextFeatures(0.075f));


    // Graph projected data for armadillo
    sm::vvec<sm::vec<float, 2>> _scores (n_samp, {0});
    for (std::uint32_t i = 0; i < n_samp; ++i) {
        _scores[i][0] = sc(i,0);
        _scores[i][1] = sc(i,1);
    }
    auto gv3 = std::make_unique<mplot::GraphVisual<float>> (sm::vec<float>{-1.6f, -1.85f, 0.0f});
    gv3->set_parent (v.get_id());
    gv3->setlimits (-8, 8, -8, 8);
    ds.datalabel = "projected data";
    gv3->setdata (_scores, ds);
    gv3->finalize();
    v.addVisualModel (gv3);

    auto gv4 = std::make_unique<mplot::GraphVisual<float>> (sm::vec<float>{0.2f, -1.85f, 0.0f});
    gv4->set_parent (v.get_id());
    gv4->setlimits (-8, 8, -8, 8);
    gv4->setdata (pca_res.get_x_proj(), ds);
    gv4->finalize();
    v.addVisualModel (gv4);

    v.keepOpen();
}
