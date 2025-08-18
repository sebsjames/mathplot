#include <mplot/Visual.h>
#include <mplot/GraphVisual.h>
#include <sm/vvec>
#include "spline"

int main()
{
    using F = float;

    sm::vvec<F> v0 = { 1, 2, 0.5, 3, 2, 0.2 };
    sm::vvec<F> v = v0;
    sm::algo::cubic_spline<F> (v, 50);
    sm::vvec<F> x0;
    x0.linspace<sm::vvec<F>::endpoint::no> (0, 6, 6);
    // or x0.linspace (0, 5, 6);
    sm::vvec<F> x;
    x.linspace (0.0, static_cast<F>(v.size() - 1) / 50.0, v.size());

    mplot::Visual vis(1024, 768, "Spline fit");
    auto gv = std::make_unique<mplot::GraphVisual<F>> (sm::vec<float>({0,0,0}));
    vis.bindmodel (gv);
    mplot::DatasetStyle dsm (mplot::stylepolicy::markers);
    dsm.markercolour = mplot::colour::blue;
    dsm.datalabel = "data";
    mplot::DatasetStyle dsl (mplot::stylepolicy::lines);
    dsl.linecolour = mplot::colour::crimson;
    dsl.datalabel = "cubic spline";
    gv->setdata (x, v, dsl);
    gv->setdata (x0, v0, dsm);
    gv->finalize();
    vis.addVisualModel (gv);

    vis.keepOpen();

    return 0;
}
