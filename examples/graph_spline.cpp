// Graph of some random points and a spline fit.
#include <memory>
#include <cstdint>
import sm.vec;
import sm.vvec;
//import sm.random;
import sm.spline;
import mplot.visual;
import mplot.graphvisual;

int main()
{
    mplot::Visual v(1024, 768, "Cubic spline example");
    auto gv = std::make_unique<mplot::GraphVisual<double>> (sm::vec<float>({0,0,0}));
    gv->set_parent (v.get_id());

    // 10 points from which to make our spline
    sm::vvec<double> x (10);
    x.linspace (0, 9);
    sm::vvec<double> y (10);
    y.randomize();

    // We package the spline fitting points in a fixed size container, as sm::spline must know at
    // compile time how many points there are.
    sm::vvec<sm::vec<double, 2>> pts(10);
    for (std::uint32_t i = 0; i < 10; ++i) { pts[i] = { x[i], y[i] }; }
    sm::spline<double> s (pts);

    sm::vvec<double> x1 (100);
    x1.linspace (0, 9);
    sm::vvec<double> y1 = s.compute (x1);

    mplot::DatasetStyle ds0 (mplot::stylepolicy::lines);
    ds0.datalabel = "Spline fit";
    gv->setdata (x1, y1, ds0);

    mplot::DatasetStyle ds1 (mplot::stylepolicy::markers);
    ds1.datalabel = "Points";
    gv->setdata (x, y, ds1);

    gv->finalize();
    v.addVisualModel (gv);
    v.keepOpen();
}
