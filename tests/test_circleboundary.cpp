#include <memory>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <stdexcept>

import sm.hexgrid;
import sm.vec;

import mplot.visual;
import mplot.hexgridvisual;
import mplot.tools;

int main (int argc, char** argv)
{
    int rtn = 0;

    mplot::Visual v(1024, 768, "Ellipse");
    v.zNear = 0.001;

    bool holdVis = false;
    if (argc > 1) {
        std::string a1(argv[1]);
        if (a1.size() > 0) { holdVis = true; }
    }
    std::cout << "NB: Provide a cmd line arg (anything) to see the graphical window for this program" << std::endl;

    sm::hexgrid<float> hg(0.01, 3, 0);
    hg.set_circular_boundary (1);

    std::cout << hg.extent() << std::endl;
    std::cout << "Number of hexes in grid:" << hg.num() << std::endl;
    std::cout << "Last vector index:" << hg.last_vector_index() << std::endl;

    if (hg.num() != 36624) { rtn = -1; }

    std::vector<float> data;
    unsigned int nhex = hg.num();
    data.resize(nhex, 0.0);

    // Make some dummy data (a sine wave)
    for (unsigned int hi=0; hi<nhex; ++hi) {
        data[hi] = 0.5 + 0.5*sin(hg.d_x[hi]); // Range 0->1
    }
    std::cout << "Created " << data.size() << " floats in data" << std::endl;

    sm::vec<float, 3> offset = { 0.0, 0.0, 0.0 };
    auto hgv = std::make_unique<mplot::HexGridVisual<float>> (&hg, offset);
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&data);
    hgv->cm.setType (mplot::ColourMapType::Rainbow);
    hgv->zScale.set_params(0,0);
    hgv->finalize();
    v.addVisualModel (hgv);
    v.render();

    if (holdVis == true) { v.keepOpen(); }

    return rtn;
}
