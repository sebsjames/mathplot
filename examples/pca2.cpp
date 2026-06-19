#include <memory>
#include <iostream>

import sklearn.iris;
import sm.vvec;
import sm.vec;
import sm.mat;
import sm.pca;
import sm.random;

import mplot.visual;
import mplot.graphvisual;

int main()
{
    sm::rand_normal<double> rng1 (0, 3);
    sm::rand_normal<double> rng2 (0, 1);

    // Fill x with elliptical scattered data
    constexpr std::uint32_t N = 2;
    constexpr std::uint32_t ndata = 5000;
    sm::vvec<sm::vec<double, 2>> x (ndata);
    for (std::uint32_t i = 0; i < ndata; ++i) {
        x[i][0] = rng1.get();
        x[i][1] = rng2.get();
    }
    // Rotate
    sm::mat<double, 2> mr;
    mr.rotate (sm::mathconst<double>::pi_over_8);
    sm::vvec<double> _x (ndata);
    sm::vvec<double> _y (ndata); // for graphing
    for (std::uint32_t i = 0; i < ndata; ++i) {
        x[i] = mr * x[i];
        _x[i] = x[i][0];
        _y[i] = x[i][1];
    }


    sm::pca::result<double, N> pca_res = sm::pca::compute<double, N> (x);
    for (std::uint32_t i = 0; i < N; ++i) {
        std::cout << "PC " << (i + 1) << " = " << pca_res.pc_ev_real[i] << " which accounts for " << pca_res.pc_mags[i] << " of the variability\n";
        std::cout << "\n" << pca_res.x_proj[i] << "\n";
    }

    mplot::Visual v(1024, 768, "Made with mplot::GraphVisual");
    auto gv = std::make_unique<mplot::GraphVisual<double>> (sm::vec<float>{});
    gv->set_parent (v.get_id());
    gv->setlimits (-10, 10, -10, 10);
    mplot::DatasetStyle ds (mplot::stylepolicy::markers);
    gv->setdata (_x, _y, ds);
    gv->finalize();
    v.addVisualModel (gv);

    gv = std::make_unique<mplot::GraphVisual<double>> (sm::vec<float>{1.5});
    gv->set_parent (v.get_id());
    gv->setlimits (-10, 10, -10, 10);
    gv->setdata (pca_res.x_proj[0], pca_res.x_proj[1], ds);
    gv->finalize();
    v.addVisualModel (gv);

    v.keepOpen();
}
