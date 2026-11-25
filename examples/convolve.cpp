/*
 * Test convolution of some data defined on a HexGrid (using HexGrid::convolve)
 */
#include <memory>
#include <cstdint>
#include <iostream>
#include <set>

import sm.mathconst;
import sm.constexpr_math;
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
               const sm::vvec<float>& input,
               const sm::vvec<float>& kern_data, const sm::vvec<float>& kern_data_mex,
               const sm::vvec<float>& convolved, const sm::vvec<float>& convolved_mex)
{
    // Visualize the 3 maps
    auto hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, offset);
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&input);
    hgv->zScale.compute_scaling (0.0f, 10.0f);
    hgv->cm.setType (mplot::ColourMapType::Viridis);
    hgv->addLabel ("Input", { -0.3f, -0.45f, 0.01f }, mplot::TextFeatures(0.06f, mplot::colour::white));
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

    // Mex
    offset[1] += 0.6f;
    offset[0] += 1.0f;
    kgv = std::make_unique<mplot::HexGridVisual<float>>(&kern_hg, offset);
    kgv->set_parent (v.get_id());
    kgv->setScalarData (&kern_data_mex);
    kgv->zScale = kgvp->zScale;
    kgv->cm.setType (mplot::ColourMapType::Viridis);
    kgv->addLabel ("Mex hat", { 0.1f, 0.14f, 0.01f }, mplot::TextFeatures(0.04f, mplot::colour::white));
    kgv->finalize();
    v.addVisualModel (kgv); // Gets a non-owning pointer to kgv from the addVisualModel call

    offset[1] -= 0.6f;
    rgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, offset);
    rgv->set_parent (v.get_id());
    rgv->setScalarData (&convolved_mex);
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

    // Create a circular HexGrid to contain the convolution kernel data
    constexpr float sigma = 0.025f;
    sm::hexgrid<float> kernel_hg (0.01f, 20.0f * sigma, 0.0f);
    kernel_hg.set_circular_boundary (6.0f * sigma);

    // Calculate convolution kernels
    sm::vvec<float> kern_data_gauss (kernel_hg.num(), 0.0f);
    sm::vvec<float> kern_data_mex (kernel_hg.num(), 0.0f);

    // The central bump
    constexpr float one_over_sigma_root_2_pi = 1.0f / sigma * sm::cem::sqrt (sm::mathconst<float>::two_pi);
    constexpr float two_sigma_sq = 2.0f * sigma * sigma;

    // Gaussian dist. result, and a running sum of the results:
    for (auto& k : kernel_hg.hexen) {
        // Gaussian profile based on the hex's distance from centre, which is
        // already computed in each sm::hex as hex<>::r
        float gauss1 = (one_over_sigma_root_2_pi * sm::cem::exp ( -(k.r * k.r) / two_sigma_sq ));
        kern_data_gauss[k.vi] = gauss1;
    }
    std::cout << "Pre-renormalized Gaussian range: " << kern_data_gauss.range() << " and sum " << kern_data_gauss.sum() << std::endl;

    // renormalize
    kern_data_gauss /= kern_data_gauss.sum();

    kern_data_mex = kern_data_gauss - (kern_data_gauss.range().max / 2.0f);

    // Call the convolution method from hexgrid, populating this vvec:
    sm::vvec<float> convolved_gauss (hg.num(), 0.0f);
    sm::algo::hexgrid::convolve (hg, kernel_hg, kern_data_gauss, input_rand, convolved_gauss);
    sm::vvec<float> convolved_mex (hg.num(), 0.0f);
    sm::algo::hexgrid::convolve (hg, kernel_hg, kern_data_mex, input_rand, convolved_mex);

    // Call the drawing function
    sm::vec<float, 3> offset = { -0.5f, 0.0f, 0.0f };
    draw_set (v, offset, hg, kernel_hg, input_rand, kern_data_gauss, kern_data_mex, convolved_gauss, convolved_mex);

    // New data - an ellipse
    sm::vvec<float> input_blocks (hg.num(), 0.0f);
    std::set<std::uint32_t> ellipse_indices = sm::algo::hexgrid::ellipse_indices (hg, 0.04f, 0.02f,
                                                                                  sm::mathconst<float>::pi_over_3,
                                                                                  sm::vec<float, 2>{0.0f, 0.0f});
    for (auto i : ellipse_indices) { input_blocks[i] = 1.0f; }
    std::cout << "Hexgrid: " << hg.output() << std::endl;
    for (auto& hx : hg.hexen) { if (hx.x < -0.15f) { input_blocks[hx.vi] = 1.0f; } }
    sm::algo::hexgrid::convolve (hg, kernel_hg, kern_data_gauss, input_blocks, convolved_gauss);
    sm::algo::hexgrid::convolve (hg, kernel_hg, kern_data_mex, input_blocks, convolved_mex);
    offset = { -0.5f, -1.5f, 0.0f };
    draw_set (v, offset, hg, kernel_hg, input_blocks, kern_data_gauss, kern_data_mex, convolved_gauss, convolved_mex);


    v.keepOpen();
}
