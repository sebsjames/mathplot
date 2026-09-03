/*
 * An example mplot::Visual scene, containing a HexGrid, onto which is sampled an image.
 */

#include <memory>
#include <string>
#include <iostream>
#include <complex>

import sm.vec;
import sm.vvec;
import sm.hexfft;
import sm.hexgrid;

import mplot.loadpng;
import mplot.visual;
import mplot.hexgridvisual;

int main()
{
    mplot::Visual v(1600, 1000, "Demo of sm::hexgrid::resample_image");

    sm::hexgrid hg(0.01f, 3.0f, 0.0f);
    hg.set_circular_boundary (1.2f);

    // Load an image with the help of mplot::loadpng().
    std::string fn = "../examples/bike256.png";
    sm::vvec<float> image_data;
    sm::vec<unsigned int, 2> dims = mplot::loadpng (fn, image_data);

    // This controls how large the photo will be on the HexGrid
    sm::vec<float, 2> image_scale = {2.5f, 2.5f};
    // You can shift the photo with an offset if necessary
    sm::vec<float, 2> image_offset = {0.0f, 0.0f};
    // Here's the HexGrid method that will resample the square pixel grid onto the hex grid
    sm::vvec<float> hex_image_data = hg.resample_image (image_data, dims[1], image_scale, image_offset);

    std::cout << "image_data size " << hex_image_data.size() << std::endl;

    auto hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>{});
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&hex_image_data);
    hgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    hgv->zScale.set_params (0, 1);
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

    // Real part
    hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>{2.5f});
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&fft_r);
    hgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    hgv->zScale.set_params (0, 1);
    hgv->finalize();
    v.addVisualModel (hgv);

    // Imaginary part
    hgv = std::make_unique<mplot::HexGridVisual<float>>(&hg, sm::vec<float>{2.5f, 2.5f});
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&fft_i);
    hgv->cm.setType (mplot::ColourMapType::GreyscaleInv);
    hgv->zScale.set_params (0, 1);
    hgv->finalize();
    v.addVisualModel (hgv);

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
    hgv->zScale.set_params (0, 1);
    hgv->finalize();
    v.addVisualModel (hgv);

    v.keepOpen();

    return 0;
}
