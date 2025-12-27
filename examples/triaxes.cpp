#include <mplot/Visual.h>
#include <mplot/TriaxesVisual.h>
int main()
{
    mplot::Visual v(1024, 768, "Triaxes standalone test");
    auto tax = std::make_unique<mplot::TriaxesVisual<float>> (sm::vec<>{});
    v.bindmodel (tax);
    tax->finalize();
    v.addVisualModel (tax);
    v.render();
    while (v.readyToFinish() == false) { v.waitevents(0.02); }
}
