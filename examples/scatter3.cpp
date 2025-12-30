/*
 * Visualize a test surface
 */
#include <iostream>
#include <fstream>
#include <cmath>
#include <array>

#include <sm/scale>
#include <sm/vec>
#include <sm/vvec>

#include <mplot/Visual.h>
#include <mplot/ColourMap.h>
#include <mplot/Scatter3Visual.h>

int main()
{
    int rtn = -1;

    mplot::Visual v(1024, 768, "mplot::Scatter3Visual");
    v.lightingEffects();

    sm::vec<float, 3> offset = { 0.0, 0.0, 0.0 };
    sm::scale<float> scale1;
    scale1.setParams (1.0, 0.0);

    // Note use of sm::vvecs here, which can be passed into
    // VisualDataModel::setDataCoords(std::vector<vec<float>>* _coords)
    // and setScalarData(const std::vector<T>* _data)
    // This is possible because sm::vvec derives from std::vector.
    sm::vvec<sm::vec<float, 3>> points(20*20);
    sm::vvec<float> data(20*20);
    size_t k = 0;
    for (int i = -10; i < 10; ++i) {
        for (int j = -10; j < 10; ++j) {
            float x = 0.1*i;
            float y = 0.1*j;
            // z is some function of x, y
            float z = x * std::exp(-(x*x) - (y*y));
            points[k] = {x, y, z};
            data[k] = z;
            k++;
        }
    }

    auto sv = std::make_unique<mplot::Scatter3Visual<float>> (offset);
    v.bindmodel (sv);
    sv->setDataCoords (&points);
    sv->setScalarData (&data);
    sv->radiusFixed = 0.03f;
    sv->colourScale = scale1;
    sv->addLabel ("test", sm::vec<>{1,0,0}, mplot::TextFeatures(0.05f)); // THIS is critical to set up textures for the component model
    sv->cm.setType (mplot::ColourMapType::Plasma);
    sv->finalize();
    v.addVisualModel (sv);

    std::cout << "\n\nAbout to v.render()\n\n";
    v.render();
    while (!v.readyToFinish()) {
        v.waitevents (0.05);
    }

    return rtn;
}
