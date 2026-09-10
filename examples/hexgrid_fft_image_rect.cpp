/*
 * An example mplot::Visual scene, containing a HexGrid, onto which is sampled an image. In this
 * case the image is rectangular.
 */

#include <memory>
#include <iostream>
#include <string>
#include <complex>

import sm.vec;
import sm.vvec;
import sm.hexfft;
import sm.hexgrid;
import sm.algo.hexgrid;
import sm.grid;

import mplot.loadpng;
import mplot.visual;
import mplot.hexgridvisual;
import mplot.gridvisual;

int main()
{
    mplot::Visual v(1600, 1000, "Hexagonal FFT");

    sm::hexgrid<float, sm::hexalign::point_up> hg(0.01f, 4.0f, 0.0f);
    hg.set_rectangular_boundary (2.0f, 2.0f);

    // Need flat_up for the frequency space
    //sm::hexgrid<float, sm::hexalign::flat_up> hgf(0.01f, 8.0f, 0.0f);
    //hgf.set_rectangular_boundary (4.0f, 4.0f);

    // Load a rectangular image with the help of mplot::loadpng().
    std::string fn = "../examples/bike256.png";
    sm::vvec<float> image_data;
    sm::vec<unsigned int, 2> dims = mplot::loadpng (fn, image_data);
    std::cout << "Loaded image with dims: " << dims << std::endl;

    // This controls how large the photo will be on the hexgrid
    sm::vec<float,2> image_scale = {2.0f, 2.0f};
    // You can shift the photo with an offset if necessary
    sm::vec<float,2> image_offset = {0.0f, 0.0f};

    // Here's the hexgrid method that will resample the square pixel grid onto the hex grid
    std::cout << "Start resample..." << std::endl;
    sm::vvec<float> hex_image_data = sm::algo::hexgrid::resample_image (hg, image_data, dims[0], image_scale, image_offset);
    std::cout << "resample complete" << std::endl;

    auto hgv = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up>>(&hg, sm::vec<float>({-6,1.25,0}));
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&hex_image_data);
    hgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    hgv->zScale.set_params (0, 0);
    hgv->addLabel ("Input hex image", sm::vec<float>({0,-1.2,0}), mplot::TextFeatures(0.05f));
    hgv->finalize();
    v.addVisualModel (hgv);

    // Transform with FFT
    sm::hexfft::spectrum<float> fft_data = sm::hexfft::fft (hg, hex_image_data);
    sm::vvec<float> fft_r (fft_data.hex_data.size());
    sm::vvec<float> fft_i (fft_data.hex_data.size());
    for (std::uint32_t i = 0; i < fft_r.size(); ++i) {
        fft_r[i] = std::real(fft_data.hex_data[i]);
        fft_i[i] = std::imag(fft_data.hex_data[i]);
    }

    // rows/cols:
    constexpr sm::vec<float, 2> grid_spacing = {0.01f, 0.01f};
    sm::grid<std::uint32_t, float> grid(fft_data.m, fft_data.n, grid_spacing);

    sm::vvec<float> d0 (fft_data.d_asa.first.data.size());
    sm::vvec<float> d1 (fft_data.d_asa.first.data.size());
    sm::vvec<float> X0 (fft_data.X_asa.first.data.size());
    sm::vvec<float> X1 (fft_data.X_asa.first.data.size());

    for (std::uint32_t i = 0; i < d0.size(); ++i) {
        d0[i] = std::real (fft_data.d_asa.first.data[i]);
        d1[i] = std::real (fft_data.d_asa.second.data[i]);
        X0[i] = std::real (fft_data.X_asa.first.data[i]);
        X1[i] = std::real (fft_data.X_asa.second.data[i]);
    }

    std::cout << "X0 mean/sd/range: " << X0.mean() << ", " << X0.std() << ", " << X0.range() << std::endl;
    std::cout << "X1 mean/sd/range: " << X1.mean() << ", " << X1.std() << ", " << X1.range() << std::endl;

    // Write out png of d1 for comparative image.
    sm::vvec<std::uint8_t> d1_rgb (fft_data.m * fft_data.n * 4);
    std::uint32_t j = 0;
    for (std::uint32_t i = fft_data.n - 1; i != std::numeric_limits<std::uint32_t>::max(); --i) {
        for (std::uint32_t k = 0; k < fft_data.m; ++k) { // col
            float val = std::round (d1[i * fft_data.m + k] * 255.0f);
            if (val < 0.0f || val > 255.0f) { throw std::runtime_error ("uhoh"); }
            d1_rgb[j++] = static_cast<std::uint8_t>(val);
            d1_rgb[j++] = static_cast<std::uint8_t>(val);
            d1_rgb[j++] = static_cast<std::uint8_t>(val);
            d1_rgb[j++] = 255u;
        }
    }
    mplot::png_encode ("../examples/bike256_d1.png", d1_rgb.data(), fft_data.m, fft_data.n);

    float hshift1 = 0.75f;
    // Grid 1 ds.first
    auto gv = std::make_unique<mplot::GridVisual<float>>(&grid, sm::vec<float>{-4.5f, 0.0f - hshift1});
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&d0);
    gv->zScale.set_params (0, 0);
    gv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    gv->addLabel ("d_asa.first (odd input rows)", sm::vec<float>({0,-0.2,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    gv = std::make_unique<mplot::GridVisual<float>>(&grid, sm::vec<float>{-4.5f, 2.0f - hshift1});
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&d1);
    gv->zScale.set_params (0, 0);
    gv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    gv->addLabel ("d_asa.second (even)", sm::vec<float>({0,-0.2,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    // FFT
    gv = std::make_unique<mplot::GridVisual<float>>(&grid, sm::vec<float>{-2.0f, 0.0f - hshift1});
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&X0);
    gv->zScale.set_params (0, 0);
    gv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    gv->addLabel ("X_asa.first (odd)", sm::vec<float>({0,-0.2,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    gv = std::make_unique<mplot::GridVisual<float>>(&grid, sm::vec<float>{-2.0f, 2.0f - hshift1});
    gv->set_parent (v.get_id());
    gv->gridVisMode = mplot::GridVisMode::RectInterp;
    gv->setScalarData (&X1);
    gv->zScale.set_params (0, 0);
    gv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    gv->addLabel ("X_asa.second (even)", sm::vec<float>({0,-0.2,0}), mplot::TextFeatures(0.05f));
    gv->finalize();
    v.addVisualModel (gv);

    // Real part
    auto fhgv = std::make_unique<mplot::HexGridVisual<float, sm::fft::hgf_align>>(fft_data.hgf.get(), sm::vec<float>{2.5f, 1.0f});
    fhgv->set_parent (v.get_id());
    fhgv->zoom = (fft_data.Uscale);
    fhgv->setScalarData (&fft_r);
    fhgv->colourScale.compute_scaling (-900, 1200);
    fhgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    fhgv->zScale.set_params (0, 0);
    fhgv->addLabel ("Combined FFT, real", sm::vec<float>({0,-1.2,0}), mplot::TextFeatures(0.05f));
    fhgv->finalize();
    v.addVisualModel (fhgv);

#if 0
    // Imaginary part
    fhgv = std::make_unique<mplot::HexGridVisual<float, sm::fft::hgf_align>>(fft_data.hgf.get(), sm::vec<float>{4.5f, 5.0f});
    fhgv->set_parent (v.get_id());
    fhgv->setScalarData (&fft_i);
    fhgv->colourScale.compute_scaling (-900, 1200);
    fhgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    fhgv->zScale.set_params (0, 0);
    fhgv->addLabel ("Combined FFT, imaginary", sm::vec<float>({0,-1.2,0}), mplot::TextFeatures(0.05f));
    fhgv->finalize();
    v.addVisualModel (fhgv);
#endif

#if 0
    // Reconstruct with inverse FFT
    sm::vvec<std::complex<float>> reconstructed = sm::hexfft::ifft (hg, fft_data.hex_data);
    sm::vvec<float> ifft_r (reconstructed.size());
    for (std::uint32_t i = 0; i < ifft_r.size(); ++i) {
        ifft_r[i] = std::real(reconstructed[i]);
    }

    // Reconstructed
    hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>{5.0f});
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&ifft_r);
    hgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    hgv->zScale.set_params (0, 0);
    hgv->addLabel ("Reconstructed from fft_data.hex_data", sm::vec<float>({-0.75,-1.2,0}), mplot::TextFeatures(0.05f));
    hgv->finalize();
    v.addVisualModel (hgv);

    // Reconstruct with inverse FFT 2
    sm::vvec<std::complex<float>> reconstructed2 = sm::hexfft::ifft (hg, fft_data);
    sm::vvec<float> ifft_r2 (reconstructed2.size());
    for (std::uint32_t i = 0; i < ifft_r2.size(); ++i) {
        ifft_r2[i] = std::real(reconstructed2[i]);
    }

    // Reconstructed
    hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>{5.0f, 2.5f});
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&ifft_r2);
    hgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    hgv->zScale.set_params (0, 0);
    hgv->addLabel ("Reconstructed from fft_data(.data)", sm::vec<float>({-0.75,-1.2,0}), mplot::TextFeatures(0.05f));
    hgv->finalize();
    v.addVisualModel (hgv);

    // Diff
    sm::vvec<float> di1 = (ifft_r - hex_image_data).abs();
    hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>{7.5f, 0.0f});
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&di1);
    hgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    hgv->zScale.set_params (0, 0);
    hgv->addLabel ("Diff of fft_data.hex_data reconstr", sm::vec<float>({-0.75,-1.2,0}), mplot::TextFeatures(0.05f));
    hgv->finalize();
    v.addVisualModel (hgv);
    std::cout << "Mean diff for fft_data.hex_data reconstr: " << di1.abs().mean() << std::endl;

    // Diff 2
    sm::vvec<float> di2 = (ifft_r2 - hex_image_data).abs();
    hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>{7.5f, 2.5f});
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&di2);
    hgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    hgv->zScale.set_params (0, 0);
    hgv->addLabel ("Diff of fft_data.(data) reconstr", sm::vec<float>({-0.75,-1.2,0}), mplot::TextFeatures(0.05f));
    hgv->finalize();
    v.addVisualModel (hgv);
    std::cout << "Mean diff for fft_data.(data) reconstr: " << di2.abs().mean() << std::endl;
#endif

    v.keepOpen();

    return 0;
}
