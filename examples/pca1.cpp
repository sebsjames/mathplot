// Visualize a graph. Minimal example showing how a default graph appears
#include <memory>
#include <iostream>

import sklearn.iris;
import sm.vvec;
import sm.vec;
import sm.mat;
import sm.pca;

import mplot.visual;
import mplot.graphvisual;
import mplot.scattervisual;

int main()
{
    sm::vvec<sm::vec<double, 4>> x;
    x.resize (sklearn::iris.size());
    for (std::uint32_t i = 0; i < sklearn::iris.size(); ++i) {
        x[i] = sklearn::iris[i].template as<double>();
    }
    sm::pca::result<double, 4> pca_res = sm::pca::compute<double, 4> (x);
    for (std::uint32_t i = 0; i < 4; ++i) {
        std::cout << "PC " << (i + 1) << " = " << pca_res.pc_ev_real[i] << " which accounts for " << pca_res.pc_mags[i] << " of the variability\n";
        std::cout << "\n" << pca_res.x_proj[i] << "\n";
    }

    mplot::Visual v(1024, 768, "Made with mplot::GraphVisual");
    auto gv = std::make_unique<mplot::GraphVisual<double>> (sm::vec<float>{});
    gv->set_parent (v.get_id());
    mplot::DatasetStyle ds (mplot::stylepolicy::markers);
    gv->setdata (pca_res.x_proj[0], pca_res.x_proj[1], ds);
    gv->finalize();
    v.addVisualModel (gv);

    sm::vvec<sm::vec<float, 3>> points (pca_res.x_proj[0].size());
    for (std::uint32_t i = 0; i < pca_res.x_proj[0].size(); ++i) {
        auto d = sm::vec<double>{ pca_res.x_proj[0][i], pca_res.x_proj[1][i], pca_res.x_proj[2][i] };
        points[i] = d.as<float>();
    }

    sm::vvec<float> id_data (sklearn::iris_flowertype.size(), 0.0f);
    for (std::uint32_t i = 0; i < sklearn::iris_flowertype.size(); ++i) {
        id_data[i] = static_cast<float>(sklearn::iris_flowertype[i]) / 3;
    }

    // FIXME: Put a TriaxesVisual around this.
    auto sv = std::make_unique<mplot::ScatterVisual<float>> (sm::vec<float>{1.5});
    sv->set_parent (v.get_id());
    sv->setDataCoords (&points);
    sv->setScalarData (&id_data);
    sv->radiusFixed = 0.03f;
    sv->cm.setType (mplot::ColourMapType::Plasma);
    sv->labelIndices = false;
    sv->finalize();
    v.addVisualModel (sv);

    v.keepOpen();
}
