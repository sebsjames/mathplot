/*
 * Test convolution of some data defined on a HexGrid (using HexGrid::convolve)
 */
#include <memory>
#include <cmath>
#include <iostream>

import sm.random;
import sm.scale;
import sm.vec;
import sm.vvec;
import sm.hexgrid;
import sm.algo.hexgrid;

import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;

// Draw HexGridVisuals for the input, the kernel and the convolved output
void draw_set (mplot::Visual<>& v, sm::vec<float, 3> offset,
               const sm::hexgrid<float>& hg, const sm::hexgrid<float>& kern_hg,
               const sm::vvec<float>& data, const sm::vvec<float>& kern_data, const sm::vvec<float>& convolved)
{
    // Visualize the 3 maps
    auto hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, offset);
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&data);
    hgv->zScale.compute_scaling (0.0f, 10.0f);
    hgv->cm.setType (mplot::ColourMapType::Viridis);
    hgv->addLabel ("Random", { -0.3f, -0.45f, 0.01f }, mplot::TextFeatures(0.06f, mplot::colour::white));
    hgv->finalize();
    v.addVisualModel (hgv);

    offset[1] += 0.6f;
    offset[0] += 1.0f;
    auto kgv = std::make_unique<mplot::HexGridVisual<float>>(&kern_hg, offset);
    kgv->set_parent (v.get_id());
    kgv->setScalarData (&kern_data);
    kgv->cm.setType (mplot::ColourMapType::Viridis);
    kgv->finalize();
    auto kgvp = v.addVisualModel (kgv); // Gets a non-owning pointer to kgv from the addVisualModel call
    // Labels can be added after finalize() and after addVisualModel
    kgvp->addLabel ("Gaussian bump", { 0.1f, 0.14f, 0.01f }, mplot::TextFeatures(0.04f, mplot::colour::white));

    offset[1] -= 0.6f;
    auto rgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, offset);
    rgv->set_parent (v.get_id());
    rgv->setScalarData (&convolved);
    rgv->zScale.compute_scaling (0.0f, 10.0f); // set a manual z scaling
    rgv->cm.setType (mplot::ColourMapType::Viridis);
    rgv->finalize();
    rgv->addLabel ("Output", { -0.3f, -0.45f, 0.01f }, mplot::TextFeatures(0.06f, mplot::colour::white));
    rgv->finalize();
    v.addVisualModel (rgv);
}

int main()
{
    mplot::Visual v(800, 600, "Convolution window");
    v.backgroundBlack();

    // Create an elliptical hexgrid for the input/output domains
    sm::hexgrid<float> hg (0.01f, 3.0f, 0.0f);
    hg.set_elliptical_boundary (0.45f, 0.3f);

    // Populate a vector of floats with random data
    sm::vvec<float> input_rand (hg.num(), 0.0f);
    input_rand.randomize();

    // Create a circular HexGrid to contain the Gaussian convolution kernel
    constexpr float sigma = 0.025f;
    sm::hexgrid<float> kernel_hg(0.01f, 20.0f * sigma, 0.0f);
    kernel_hg.set_circular_boundary (6.0f * sigma);
    sm::vvec<float> kern_data_gauss (kernel_hg.num(), 0.0f);

    // Once-only parts of the calculation of the Gaussian.
    constexpr float one_over_sigma_root_2_pi = 1.0f / sigma * 2.506628275f;
    constexpr float two_sigma_sq = 2.0f * sigma * sigma;
    // Gaussian dist. result, and a running sum of the results:
    float sum = 0.0f;
    for (auto& k : kernel_hg.hexen) {
        // Gaussian profile based on the hex's distance from centre, which is
        // already computed in each Hex as Hex::r
        float gauss = (one_over_sigma_root_2_pi * std::exp ( -(k.r * k.r) / two_sigma_sq ));
        kern_data_gauss[k.vi] = gauss;
        sum += gauss;
    }
    kern_data_gauss /= sum; // renormalize

    // Call the convolution method from hexgrid, populating this vvec:
    sm::vvec<float> convolved (hg.num(), 0.0f);
    sm::algo::hexgrid::convolve (hg, kernel_hg, kern_data_gauss, input_rand, convolved);

    // Call the drawing function
    sm::vec<float, 3> offset = { -0.5f, 0.0f, 0.0f };
    draw_set (v, offset, hg, kernel_hg, input_rand, kern_data_gauss, convolved);

    v.keepOpen();
}
