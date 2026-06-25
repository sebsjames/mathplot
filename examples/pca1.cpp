// Principal component analysis on the scikit iris data
#include <memory>
#include <iostream>
#include <cstdint>

import sklearn.iris;
import sm.vvec;
import sm.vec;
import sm.mat;
import sm.pca;

import mplot.visual;
import mplot.graphvisual;
import mplot.scattervisual;
import mplot.triaxesvisual;

int main()
{
    sm::vvec<sm::vec<double, 4>> x;
    x.resize (sklearn::iris.size());
    for (std::uint32_t i = 0; i < sklearn::iris.size(); ++i) {
        x[i] = sklearn::iris[i].template as<double>();
    }
    sm::pca::result<double, 4> pca_res = sm::pca::compute<double, 4> (x);
    for (std::uint32_t i = 0; i < 4; ++i) {
        std::cout << "PC " << (i + 1) << " = " << pca_res.pc_vectors[i] << " which accounts for " << pca_res.pc_proportions[i] << " of the variability\n";
    }
    // Projecting the input data is a separate function call
    sm::pca::transform (pca_res);

    // 2D graph of the first two components
    mplot::Visual v(1024, 768, "Made with mplot::GraphVisual");
    auto gv = std::make_unique<mplot::GraphVisual<double>> (sm::vec<float>{});
    gv->set_parent (v.get_id());
    mplot::DatasetStyle ds (mplot::stylepolicy::markers);
    gv->setdata (pca_res.x_proj[0], pca_res.x_proj[1], ds);
    gv->finalize();
    v.addVisualModel (gv);

    // 3D graph of the first three components
    sm::vvec<sm::vec<float, 3>> points (pca_res.x_proj[0].size());
    for (std::uint32_t i = 0; i < pca_res.x_proj[0].size(); ++i) {
        auto d = sm::vec<double>{ pca_res.x_proj[0][i], pca_res.x_proj[1][i], pca_res.x_proj[2][i] };
        points[i] = d.as<float>();
    }

    sm::vvec<float> id_data (sklearn::iris_flowertype.size(), 0.0f);
    for (std::uint32_t i = 0; i < sklearn::iris_flowertype.size(); ++i) {
        id_data[i] = static_cast<float>(sklearn::iris_flowertype[i]) / 3;
    }

    auto sv = std::make_unique<mplot::ScatterVisual<float>> (sm::vec<float>{1.5});
    sv->set_parent (v.get_id());
    sv->setDataCoords (&points);
    sv->setScalarData (&id_data);
    sv->radiusFixed = 0.03f;
    sv->cm.setType (mplot::ColourMapType::Plasma);
    sv->labelIndices = false;
    sv->finalize();
    v.addVisualModel (sv);

    auto tv = std::make_unique<mplot::TriaxesVisual<float>> (sm::vec<float>{1.5});
    tv->set_parent (v.get_id());
    tv->input_max = {3,3,3};
    tv->axis_ends = {3,3,3};
    tv->finalize();
    v.addVisualModel (tv);

    v.keepOpen();
}
