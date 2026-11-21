/*
 * Another test program. This one is for use as a paper figure.
 */
#include <memory>
#include <utility>
#include <iostream>
#include <array>
#include <vector>
#include <list>

import sm.vec;
import sm.vvec;
import sm.interval;
import sm.bezcurve;
import sm.bezcurvepath;

import mplot.visual;
import mplot.graphvisual;

// Draw a bezcurve on the graph gv
void draw (mplot::GraphVisual<float>* gv,
           sm::bezcurvepath<FLT>& bcp,
           sm::vvec<sm::vec<FLT, 2>>& v,
           std::array<float, 3> linecolfit,
           float sz, bool drawuserctrl = true)
{
    unsigned int nFit = 200;
    sm::vvec<sm::vec<float, 2>> fitted (nFit);
    sm::vvec<sm::vec<float, 2>> tangents (nFit);
    sm::vvec<sm::vec<float, 2>> normals (nFit);
    // Compute the curve for plotting
    bcp.compute_points ((unsigned int)nFit);
    std::vector<sm::bezcoord<FLT>> coords = bcp.get_points();
    std::vector<sm::bezcoord<FLT>> tans = bcp.get_tangents();
    std::vector<sm::bezcoord<FLT>> norms = bcp.get_normals();
    for (unsigned int i = 0; i < nFit; ++i) {
        fitted[i] = sm::vec<double, 2>{coords[i].x(), coords[i].y()}.as_float();
        tangents[i] = sm::vec<double, 2>{tans[i].x(), tans[i].y()}.as_float();
        normals[i] = sm::vec<double, 2>{norms[i].x(), norms[i].y()}.as_float();
    }

    mplot::DatasetStyle dsl(mplot::stylepolicy::lines);
    dsl.linecolour = linecolfit;
    dsl.linewidth = sz/4.0f;
    gv->setdata (fitted, dsl);

    mplot::DatasetStyle dsm(mplot::stylepolicy::markers);
    dsm.markercolour = linecolfit;
    dsm.markersize = sz;
    dsm.markerstyle = mplot::markerstyle::circle;

    mplot::DatasetStyle dsb(mplot::stylepolicy::lines);
    dsb.markercolour = linecolfit;
    dsb.linecolour = linecolfit;
    dsb.linewidth = sz/6.0f;
    dsb.markersize = sz;

    // Add the control points in similar colours
    std::list<sm::bezcurve<FLT>> theCurves = bcp.curves;
    for (auto curv : theCurves) {
        sm::vvec<sm::vec<FLT, 2>> ctrlsd = curv.get_controls();
        sm::vvec<sm::vec<float, 2>> ctrls (ctrlsd.size());
        for (size_t i = 0; i < ctrlsd.size(); ++i) { ctrls[i] = ctrlsd[i].as_float(); }
        // Draw the control points
        gv->setdata (ctrls, dsm);

        // Draw in the lines to the control points
        sm::vvec<sm::vec<float, 2>> pspe = { ctrls[0], ctrls[1] };
        gv->setdata (pspe, dsb);

        sm::vvec<sm::vec<float, 2>> pspe2 = { ctrls[ctrls.size()-2], ctrls[ctrls.size()-1] };
        gv->setdata (pspe2, dsb);
    }

    if (drawuserctrl) {
        // The user control points
        sm::vvec<sm::vec<float, 2>> vf (v.size());
        for (size_t i = 0; i < v.size(); ++i) { vf[i] = v[i].as_float(); }
        gv->setdata (vf, dsm);
    }
}

int main (int argc, char** argv)
{
    namespace m = mplot;

    bool holdVis = false;
    if (argc > 1) {
        std::string a1(argv[1]);
        std::cout << "a1 is " << a1 << std::endl;
        if (a1.size() > 0) {
            holdVis = true;
        }
    }
    std::cout << "NB: Provide a cmd line arg (anything) to see the graphical window for this program" << std::endl;

    int rtn = 0; // Never decremented, so this is only really a compile/run without crashing test
    FLT fac = 3.4;
    FLT xoff = -400.0;

    sm::vvec<sm::vec<FLT, 2>> v = {
        {xoff+fac*200,fac*500 },
        {xoff+fac*300,fac*450 },
        {xoff+fac*400,fac*400 },
        {xoff+fac*450,fac*300 }
    };

    sm::vvec<sm::vec<FLT, 2>> w = {
        v.back(),
        {xoff+fac*440,fac*180 },
        {xoff+fac*580,fac*30 },
        {xoff+fac*630,fac*20 }
    };

    // First the analytical fit
    sm::bezcurve<FLT> cv1;
    cv1.fit (v);
    sm::bezcurve<FLT> cv2;
    cv2.fit (w);

    sm::bezcurvepath<FLT> bcp;
    bcp.add_curve (cv1);
    bcp.add_curve (cv2);

    sm::bezcurvepath<FLT> bcp1;
    bcp1.add_curve (cv1);
    sm::bezcurvepath<FLT> bcp2;
    bcp2.add_curve (cv2);

    if (holdVis == true) {
        // Create a frame as the background for our drawing.
        mplot::Visual<> scene(1600, 1000, "Beziers");
        sm::vec<float> offset = {-1, -1, 0};
        auto gv = std::make_unique<mplot::GraphVisual<float>>(offset);
        gv->set_parent (scene.get_id());
        gv->setsize (2,2);
        gv->setlimits (sm::interval<float>{200,1700}, sm::interval<float>{0,1700});

        std::cout << "Draw the two analytical best-fit curves..." << std::endl;
        draw (gv.get(), bcp1, v, mplot::colour::blue, 0.024);
        draw (gv.get(), bcp2, w, mplot::colour::crimson, 0.024);

        std::cout << "Do the control point-equalizing 0th order optimization..."<< std::endl;
        bool withopt = false;
        cv2.fit (w, cv1, withopt);

        bcp.remove_curve();
        bcp.remove_curve();
        bcp.add_curve (cv1);
        bcp.add_curve (cv2);

        sm::vvec<sm::vec<FLT, 2>> vw (v);
        vw.insert (vw.end(), w.begin(), w.end());

        draw (gv.get(), bcp, vw, mplot::colour::darkorchid2, 0.024, false);

        gv->finalize();
        scene.addVisualModel (gv);

        scene.keepOpen();
    }

    return rtn;
}
