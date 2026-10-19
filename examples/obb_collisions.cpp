/*
 * Two cuboids. One movable and rotatable. For collision detection development.
 */
#include <iostream>
#include <array>
#include <stdexcept>
#include <string>
#include <memory>
#include <cstdint>

import sm.vec;
import sm.quaternion;
import sm.mat;

import mplot.visual;
import mplot.colour;
import mplot.rhombovisual;

// A derived Visual to handle key commands
struct myvisual final : public mplot::Visual<>
{
    myvisual (int _w, int _h, const std::string& title) : mplot::Visual<> (_w, _h, title) {}
    // Movement state (class and bitset)
    enum class move_sense : std::uint16_t
    {
        forward, backward, left, right, up, down,
        rot_up, rot_down, rot_left, rot_right, rot_roll_left, rot_roll_right
    };
    sm::flags<move_sense> move_state;
    bool moving() { return this->move_state.any(); }
protected:
    void key_callback_extra (int key, [[maybe_unused]] int scancode, int action, int mods) override
    {
        // Process press/repeat key actions (none will work with Ctrl or Shift)
        if (action == mplot::keyaction::press && !(mods & mplot::keymod::shift)) {
            if (key == mplot::key::w) {
                this->move_state.set (move_sense::forward);
            } else if (key == mplot::key::a && !mods) {
                this->move_state.set (move_sense::left);
            } else if (key == mplot::key::d) {
                this->move_state.set (move_sense::right);
            } else if (key == mplot::key::s) {
                this->move_state.set (move_sense::backward);
            } else if (key == mplot::key::p) {
                this->move_state.set (move_sense::up);
            } else if (key == mplot::key::l) {
                this->move_state.set (move_sense::down);
            } else if (key == mplot::key::up) {
                this->move_state.set (move_sense::rot_up);
            } else if (key == mplot::key::down) {
                this->move_state.set (move_sense::rot_down);
            } else if (key == mplot::key::left) {
                this->move_state.set (move_sense::rot_left);
            } else if (key == mplot::key::right) {
                this->move_state.set (move_sense::rot_right);
            } else if (key == mplot::key::comma) {
                this->move_state.set (move_sense::rot_roll_left);
            } else if (key == mplot::key::period) {
                this->move_state.set (move_sense::rot_roll_right);
            }

        } else if (action == mplot::keyaction::release && !(mods & mplot::keymod::shift)) {

            if (key == mplot::key::w) {
                this->move_state.reset (move_sense::forward);
            } else if (key == mplot::key::a && !mods) {
                this->move_state.reset (move_sense::left);
            } else if (key == mplot::key::d) {
                this->move_state.reset (move_sense::right);
            } else if (key == mplot::key::s) {
                this->move_state.reset (move_sense::backward);
            } else if (key == mplot::key::p) {
                this->move_state.reset (move_sense::up);
            } else if (key == mplot::key::l) {
                this->move_state.reset (move_sense::down);
            } else if (key == mplot::key::up) {
                this->move_state.reset (move_sense::rot_up);
            } else if (key == mplot::key::down) {
                this->move_state.reset (move_sense::rot_down);
            } else if (key == mplot::key::left) {
                this->move_state.reset (move_sense::rot_left);
            } else if (key == mplot::key::right) {
                this->move_state.reset (move_sense::rot_right);
            } else if (key == mplot::key::comma) {
                this->move_state.reset (move_sense::rot_roll_left);
            } else if (key == mplot::key::period) {
                this->move_state.reset (move_sense::rot_roll_right);
            }
        }

        if (key == mplot::key::h && (mods & mplot::keymod::control) && action == mplot::keyaction::press) {
            // App specific help
            std::cout << "\nobb_collisions specific help:\n"
                      << "wasd: Fwd/Left/Back/Right\n"
                      << "p: Up\n"
                      << "l: Down\n"
                      << "Arrow keys: Pitch and Yaw\n"
                      << "<>: Roll\n"
                      << std::flush;
        }
    }
};

int main()
{
    myvisual v(1024, 768, "Collisions between cuboids");
    v.lightingEffects();

    sm::vec<float> offset = {1,0,0};

    // Cuboid rhombos
    sm::vec<float> e1 = sm::vec<>::ux();
    sm::vec<float> e2 = sm::vec<>::uy();
    sm::vec<float> e3 = sm::vec<>::uz();

    auto rv1 = std::make_unique<mplot::RhomboVisual<>> (-offset, e1, e2, e3, mplot::colour::blueviolet);
    rv1->set_parent (v.get_id());
    rv1->finalize();
    auto rv1p = v.addVisualModel (rv1);

    e1 *= 2.0f;
    offset = {};
    auto rv2 = std::make_unique<mplot::RhomboVisual<>> (offset, e1, e2, e3, mplot::colour::seagreen2);
    rv2->set_parent (v.get_id());
    rv2->finalize();
    auto rv2p = v.addVisualModel (rv2);

    sm::mat<float, 4> moving_vm = rv2p->getViewMatrix();

    // Movement increment
    const float mvinc = 0.005f;
    const float anginc = 0.002f;

    // Get the unchanging oriented bounding box for rv1p.
    auto obb1 = rv1p->get_viewmatrix_obb();

    while (!v.readyToFinish()) {
        v.render();
        v.wait (0.01);

        if (v.move_state.test (myvisual::move_sense::forward)) {
            // Move viewmatrix -z
            moving_vm.translate (-sm::vec<>::uz() * mvinc);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::backward)) {
            moving_vm.translate ( sm::vec<>::uz() * mvinc);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::left)) {
            moving_vm.translate (-sm::vec<>::ux() * mvinc);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::right)) {
            moving_vm.translate ( sm::vec<>::ux() * mvinc);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::up)) {
            moving_vm.translate ( sm::vec<>::uy() * mvinc);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::down)) {
            moving_vm.translate (-sm::vec<>::uy() * mvinc);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::rot_up)) {
            sm::quaternion<float> qr (sm::vec<>::ux(), -anginc);
            moving_vm.rotate (qr);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::rot_down)) {
            sm::quaternion<float> qr (sm::vec<>::ux(), anginc);
            moving_vm.rotate (qr);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::rot_left)) {
            sm::quaternion<float> qr (sm::vec<>::uy(), anginc);
            moving_vm.rotate (qr);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::rot_right)) {
            sm::quaternion<float> qr (sm::vec<>::uy(), -anginc);
            moving_vm.rotate (qr);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::rot_roll_left)) {
            sm::quaternion<float> qr (sm::vec<>::uz(), anginc);
            moving_vm.rotate (qr);
            rv2p->setViewMatrix (moving_vm);
        } else if (v.move_state.test (myvisual::move_sense::rot_roll_right)) {
            sm::quaternion<float> qr (sm::vec<>::uz(), -anginc);
            moving_vm.rotate (qr);
            rv2p->setViewMatrix (moving_vm);
        }


        if (v.moving()) {
            // Now detect collision
            if (rv2p->collision_detect (obb1)) { // or use rv2p->collision_detect (rv1p)
                if (rv1p->col != mplot::colour::crimson) {
                    rv1p->col = mplot::colour::crimson;
                    rv1p->reinit();
                }
                if (rv2p->col != mplot::colour::indian_red) {
                    rv2p->col = mplot::colour::indian_red;
                    rv2p->reinit();
                }
            } else {
                if (rv1p->col != mplot::colour::blueviolet) {
                    rv1p->col = mplot::colour::blueviolet;
                    rv1p->reinit();
                }
                if (rv2p->col != mplot::colour::seagreen2) {
                    rv2p->col = mplot::colour::seagreen2;
                    rv2p->reinit();
                }
            }
        }
    }

    return 0;
}
