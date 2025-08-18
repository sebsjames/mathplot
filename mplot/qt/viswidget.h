#pragma once

#include <iostream>
#include <functional>

class QOpenGLWidget;

// VisualOwnable is going to be owned by the QOpenGLWidget
// Define mplot::win_t before #including mplot/VisualOwnableNoMX.h
namespace mplot { using win_t = QOpenGLWidget; }
#include <mplot/VisualOwnableNoMX.h>

#include <QtWidgets/QOpenGLWidget>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QMouseEvent>
#include <QWheelEvent>

// We need to be able to convert from Qt keycodes to mplot keycodes
#include <mplot/qt/keycodes.h>

namespace mplot {
    namespace qt {

        // This must match the QOpenGLFunctions_4_1_Core class you derive from
        constexpr int gl_version = mplot::gl::version_4_1;

        struct OpenGLProcAddressHelper {
            inline static QOpenGLContext *ctx;

            static QFunctionPointer getProcAddress(const char *name) {
                return ctx->getProcAddress(name);
            }
        };

        // A mplot::VisualOwnable-based widget
        struct viswidget : public QOpenGLWidget
        {
            // Unlike the GLFW or mplot-in-a-QWindow schemes, we hold the mplot::VisualOwnable
            // inside the widget.
            mplot::VisualOwnableNoMX<gl_version> v;

            // In your Qt code, build VisualModels that should be added to the scene and add them to this.
            std::vector<std::unique_ptr<mplot::VisualModel<gl_version>>> newvisualmodels;
            std::vector<mplot::VisualModel<gl_version>*> model_ptrs;

            // if >-1, then that model needs a reinit.
            int needs_reinit = -1;
            void set_model_needs_reinit (int model_idx, bool reinit_required = true)
            {
                this->needs_reinit = reinit_required ? model_idx : -1;
            }

            viswidget (QWidget* parent = 0) : QOpenGLWidget(parent)
            {
                // You have to set the format in the constructor
                QSurfaceFormat format;
                format.setDepthBufferSize (4);
                format.setSamples (4);
                format.setStencilBufferSize (8);
                format.setVersion (mplot::gl::version::major (gl_version), mplot::gl::version::minor (gl_version));
                format.setProfile (QSurfaceFormat::CoreProfile);
                this->setFormat (format);
                this->setUpdateBehavior (QOpenGLWidget::NoPartialUpdate);
                this->setFocusPolicy (Qt::StrongFocus); // ensure keyPressEvents reach us
            }

        protected:

            void initializeGL() override
            {
                // Make sure we can call gl functions
                OpenGLProcAddressHelper::ctx = context();
                v.init_glad(OpenGLProcAddressHelper::getProcAddress);
                // Switch on multisampling anti-aliasing (with the num samples set in constructor)
                glEnable (GL_MULTISAMPLE);
                // Initialise mplot::VisualOwnable
                v.init (this);
            }

            void resizeGL (int w, int h) override
            {
                double dpr = this->devicePixelRatio();
                v.set_winsize (static_cast<int>(std::round(w * dpr)),
                               static_cast<int>(std::round(h * dpr)));
                this->update();
            }

            void paintGL() override
            {
                if (!this->newvisualmodels.empty()) {
                    // Now we iterate through newvisualmodels, finalize them and add them to mplot::Visual
                    for (unsigned int i = 0; i < newvisualmodels.size(); ++i) {
                        this->newvisualmodels[i]->finalize();
                        this->model_ptrs.push_back (this->v.addVisualModel (this->newvisualmodels[i]));
                    }
                    this->newvisualmodels.clear();
                }
                if (this->needs_reinit > -1) {
                    this->model_ptrs[this->needs_reinit]->reinit();
                    this->needs_reinit = -1;
                }
                v.render();
            }

            void mousePressEvent (QMouseEvent* event) override
            {
                v.set_cursorpos (event->x(), event->y());
                int bflg = event->button();
                int b = mplot::mousebutton::unhandled;
                b = bflg & Qt::LeftButton ? mplot::mousebutton::left : b;
                b = bflg & Qt::RightButton ? mplot::mousebutton::right : b;
                int mflg = event->modifiers();
                int mods = 0;
                if (mflg & Qt::ControlModifier) { mods |= mplot::keymod::control; }
                if (mflg & Qt::ShiftModifier) { mods |= mplot::keymod::shift; }
                v.mouse_button_callback (b, mplot::keyaction::press, mods);
                event->accept();
            }

            void mouseMoveEvent (QMouseEvent* event) override
            {
                if (v.cursor_position_callback (event->x(), event->y())) {
                    this->update();
                }
                event->accept();
            }

            void mouseReleaseEvent (QMouseEvent* event) override
            {
                v.set_cursorpos (event->x(), event->y());
                int bflg = event->button();
                int b =  mplot::mousebutton::unhandled;
                b = bflg & Qt::LeftButton ? mplot::mousebutton::left : b;
                b = bflg & Qt::RightButton ? mplot::mousebutton::right : b;
                v.mouse_button_callback (b, mplot::keyaction::release);
                event->accept();
            }

            void wheelEvent (QWheelEvent* event) override
            {
                QPoint numSteps = event->angleDelta() / 120;
                v.scroll_callback (numSteps.x(), numSteps.y());
                this->update();
                event->accept();
            }

            // Keyboard events...
            void keyPressEvent (QKeyEvent* event) override
            {
                int mflg = event->modifiers();
                int mods = 0;
                if (mflg & Qt::ControlModifier) { mods |= mplot::keymod::control; }
                if (mflg & Qt::ShiftModifier) { mods |= mplot::keymod::shift; }
                int mplot_keycode = mplot::qt::qtkey_to_mplotkey (event->key());
                // Could be keyaction::REPEAT in GLFW
                if (v.key_callback (mplot_keycode, 0, mplot::keyaction::press, mods)) {
                    this->update();
                }
                event->accept();
            }
        };
    } // qt
} // mplot
