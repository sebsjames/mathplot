/*
 * This program shows that you can make two separate VisualModels into a combined model in your
 * client code. It's not the primary intended way to use component VisualModels, but is left as an
 * example.
 */

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

#include <sm/vec>

#include <mplot/Visual.h>
#include <mplot/VisualModel.h>
#include <mplot/colour.h>
#include <mplot/NormalsVisual.h>

// Quick visual that simply draws ellipsoid
template <int glver = mplot::gl::version_4_1>
class PrimitiveVisual : public mplot::VisualModel<glver>
{
public:
    PrimitiveVisual (const sm::vec<float> _offset) { this->viewmatrix.translate (_offset); }

    void initializeVertices()
    {
        sm::mat44<float> tr;
        tr.rotate (sm::vec<>::uz(), sm::mathconst<float>::pi_over_4);
        this->computeEllipsoid (sm::vec<float>{0},
                                mplot::colour::royalblue,
                                mplot::colour::maroon3,
                                sm::vec<float>{1,2,3},
                                40, 40, tr);
    }
};

int main()
{
    mplot::Visual v(1024, 768, "Ellipsoid primitive");
    v.lightingEffects (true);

    auto pvm = std::make_unique<PrimitiveVisual<>> (sm::vec<>{});
    v.bindmodel (pvm);
    pvm->finalize();
    auto pvmp = v.addVisualModel (pvm);

    // Create an associate normals model
    auto nrm = std::make_unique<mplot::NormalsVisual<>> (pvmp);
    v.bindmodel (nrm);
    nrm->finalize();
    pvmp->addVisualModel (nrm); // Note we are adding to pvmp, and not to v

    v.keepOpen();
}
