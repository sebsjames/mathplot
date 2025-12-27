/*
 * "Component model" example.
 *
 * This is an example of building a VisualModel that contains another VisualModel as a component.
 *
 * IHaveAComonentVisual visualizes an ellipsoid with normals all in one VisualModel. The
 * NormalsVisual is a component of IHaveAComonentVisual.
 *
 * \author Seb James
 * \date 27 December 2025
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

// This VisualModel draws an ellipsoid, and has a component model (NormalsVisual) that draws arrows for the normals.
template <int glver = mplot::gl::version_4_1>
struct IHaveAComponentVisual : public mplot::VisualModel<glver>
{
    IHaveAComponentVisual (const sm::vec<float> _offset)
    {
        this->viewmatrix.translate (_offset);

        // The NormalsVisual is the component
        auto nrm = std::make_unique<mplot::NormalsVisual<glver>> (this);
        // NB: You DON'T bindmodel() a component model at this point. components will use the binding of the owning VM
        // NB: ALSO, you don't finalize before adding. The owning VM's finalize will call the component finalize()
        this->nrms = this->addVisualModel (nrm); // bindmodel and finalize have to happen when IHaveAComponentVisual::finalize runs
    }

    // Holding a pointer to the component allows access to its features by client code
    mplot::NormalsVisual<glver>* nrms = nullptr;

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
    mplot::Visual v(1024, 768, "Component model");
    v.lightingEffects (true);

    // When you *use* a component model in client code, you can't tell any difference:
    auto pvm = std::make_unique<IHaveAComponentVisual<>> (sm::vec<>{}); // just like usual
    v.bindmodel (pvm);      // just as you always bindmodel here
    pvm->finalize();        // as usual, finalize before addVisualModel
    v.addVisualModel (pvm); // as usual

    v.keepOpen();
}
