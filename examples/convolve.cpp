/*
 * Test convolution of some data defined on a HexGrid (using HexGrid::convolve)
 */
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <memory>

import sm.random;
import sm.scale;
import sm.vec;
import sm.hexgrid;
import sm.algo.hexgrid;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;

int main()
{
    int rtn = 0;

    mplot::Visual v(800,600,"Convolution window");
    v.zNear = 0.001;
    v.backgroundBlack();
    v.setSceneTransZ (-3.0f);

    // Create an elliptical hexgrid for the input/output domains
    sm::hexgrid<float> hg(0.01, 3, 0);
    hg.set_elliptical_boundary (0.45, 0.3);

    // Populate a vector of floats with data
    std::vector<float> data (hg.num(), 0.0f);
    //mplot::rand_normal<float> rng (0.1f, 0.05f);
    sm::rand_uniform<float> rng;
    float nonconvolvedSum = 0.0f;
    for (float& d : data) {
        d = rng.get();
        nonconvolvedSum += d;
    }

    // Create a circular HexGrid to contain the Gaussian convolution kernel
    float sigma = 0.025f;
    sm::hexgrid<float> kernel(0.01, 20.0f*sigma, 0);
    kernel.set_circular_boundary (6.0f*sigma);
    std::vector<float> kerneldata (kernel.num(), 0.0f);
    // Once-only parts of the calculation of the Gaussian.
    float one_over_sigma_root_2_pi = 1 / sigma * 2.506628275;
    float two_sigma_sq = 2.0f * sigma * sigma;
    // Gaussian dist. result, and a running sum of the results:
    float gauss = 0;
    float sum = 0;
    for (auto& k : kernel.hexen) {
        // Gaussian profile based on the hex's distance from centre, which is
        // already computed in each Hex as Hex::r
        gauss = (one_over_sigma_root_2_pi * std::exp ( -(k.r*k.r) / two_sigma_sq ));
        kerneldata[k.vi] = gauss;
        sum += gauss;
    }
    // Renormalise
    for (auto& k : kernel.hexen) { kerneldata[k.vi] /= sum; }

    // A vector for the result
    std::vector<float> convolved (hg.num(), 0.0f);

    // Call the convolution method from hexgrid:
    sm::algo::hexgrid::convolve (hg, kernel, kerneldata, data, convolved);

    float convolvedSum = 0.0f;
    for (float& d : convolved) { convolvedSum += d; }

    std::cout << "Unconvolved sum: " << nonconvolvedSum << ", convolved sum: " << convolvedSum << "\n";

    // Visualize the 3 maps
    sm::vec<float, 3> offset = { -0.5, 0.0, 0.0 };
    auto hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, offset);
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&data);
    hgv->cm.setType(mplot::ColourMapType::Viridis);
    hgv->addLabel ("Input", { -0.3f, -0.45f, 0.01f }, mplot::TextFeatures(0.1f, mplot::colour::white));
    hgv->finalize();
    // Get the non-owning pointer to hgv from the addVisualModel call
    auto hgvp = v.addVisualModel (hgv);

    offset[1] += 0.6f;
    auto kgv = std::make_unique<mplot::HexGridVisual<float>>(&kernel, offset);
    kgv->set_parent (v.get_id());
    kgv->setScalarData (&kerneldata);
    kgv->cm.setType(mplot::ColourMapType::Viridis);
    kgv->finalize();
    auto kgvp = v.addVisualModel (kgv);

    // Labels can be added after finalize() and after addVisualModel
    kgvp->addLabel ("Kernel", { 0.1f, 0.14f, 0.01f }, mplot::TextFeatures(0.1f, mplot::colour::white));

    offset[1] -= 0.6f;
    offset[0] += 1.0f;
    auto rgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, offset);
    rgv->set_parent (v.get_id());
    rgv->setScalarData (&convolved);
    rgv->cm.setType(mplot::ColourMapType::Viridis);
    rgv->finalize();
    rgv->addLabel ("Output", { -0.3f, -0.45f, 0.01f }, mplot::TextFeatures(0.1f, mplot::colour::white));
    rgv->finalize();
    auto rgvp = v.addVisualModel (rgv);

    // Demonstrate how to divide existing scale by 10:
    float newGrad = hgvp->zScale.get_params(0)/10.0;
    // Set this in a new zscale object:
    sm::scale<float> zscale;
    zscale.set_params (newGrad, 0);
    // Use the un-owned pointer rgvp:
    rgvp->updateZScale (zscale);

    while (v.readyToFinish() == false) {
        v.waitevents (0.018);
        v.render();
    }

    return rtn;
}
