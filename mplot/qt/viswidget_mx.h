#pragma once

#include <iostream>
#include <functional>

struct QOpenGLWidget; // fwd decl

// VisualOwnableMX is going to be owned by the QOpenGLWidget
// Define mplot::win_t before #including mplot/VisualOwnableMX.h
namespace mplot { using win_t = QOpenGLWidget; }
#include <mplot/VisualOwnableMX.h> // Glad multi OpenGL context class

#include <QtWidgets/QOpenGLWidget>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QMouseEvent>
#include <QWheelEvent>

// We need to be able to convert from Qt keycodes to mplot keycodes
#include <mplot/qt/keycodes.h>


namespace mplot {
    namespace qt {

        constexpr int gl_version = mplot::gl::version_4_1;

        // How many separate OpenGL contexts (i.e. how many viswidget_mxs) to support in one Qt program?
        constexpr int max_contexts = 32; // with 32 we use 32 * 8 bytes of memory = 256 bytes.

        // A container class to manage a getProcAddress function from each viswidget_mx/QOpenGLWidget context
        struct gl_contexts
        {
            static auto& i() // The instance public function.
            {
                static gl_contexts instance;
                return instance;
            }

            // Set the context. Store the QOpenGLContext pointer into ctx_ptrs[widget_index].
            template<int widget_index>
            void set_context (QOpenGLContext* _ctx)
            {
                static_assert (widget_index < mplot::qt::max_contexts);
                ctx_ptrs[widget_index] = _ctx;
            }
#if 0
            template<int widget_index> requires (widget_index < mplot::qt::max_contexts)
            void make_current (QSurface* surf) { ctx_ptrs[widget_index]->makeCurrent (surf); }

            template<int widget_index> requires (widget_index < mplot::qt::max_contexts)
            void done_current() { ctx_ptrs[widget_index]->doneCurrent(); }

            template<int widget_index> requires (widget_index < mplot::qt::max_contexts)
            bool supports_mt() { return ctx_ptrs[widget_index]->supportsThreadedOpenGL(); }

            template<int widget_index> requires (widget_index < mplot::qt::max_contexts)
            QOpenGLContext* share_context() { return ctx_ptrs[widget_index]->shareContext(); }

            template<int widget_index> requires (widget_index < mplot::qt::max_contexts)
            QOpenGLContextGroup* share_group() { return ctx_ptrs[widget_index]->shareGroup(); }
#endif
            // The static getProcAddress function for the index widget_index.
            template<int widget_index>
            static QFunctionPointer getProcAddress (const char* name)
            {
                static_assert (widget_index < mplot::qt::max_contexts);
                if (mplot::qt::gl_contexts::i().ctx_ptrs[widget_index] == nullptr) { return nullptr; }
                return mplot::qt::gl_contexts::i().ctx_ptrs[widget_index]->getProcAddress (name);
            }

        private:
            gl_contexts() { ctx_ptrs = { nullptr }; }
            ~gl_contexts() {}
            std::array<QOpenGLContext*, mplot::qt::max_contexts> ctx_ptrs;
        };

        // A mplot::Visual widget. You have to choose and provide a widget_index in the range [0,
        // mplot::gl::max_contexts)
        template<int widget_index>
        struct viswidget_mx : public QOpenGLWidget //, protected QOpenGLFunctions_4_1_Core
        {
            // Unlike the GLFW or mplot-in-a-QWindow schemes, we hold the mplot::VisualOwnableMX
            // inside the widget.
            mplot::VisualOwnableMX<gl_version> v;

            // In your Qt code, build VisualModels that should be added to the scene and add them to this.
            std::vector<std::unique_ptr<mplot::VisualModel<gl_version>>> newvisualmodels;
            std::vector<mplot::VisualModel<gl_version>*> model_ptrs;

            // if >-1, then that model needs a reinit.
            int needs_reinit = -1;
            void set_model_needs_reinit (int model_idx, bool reinit_required = true)
            {
                this->needs_reinit = reinit_required ? model_idx : -1;
            }

            viswidget_mx (QWidget* parent = 0) : QOpenGLWidget(parent)
            {
                static_assert (widget_index < mplot::qt::max_contexts);
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
                // Initialise mplot::Visual, which must set up GLAD's access to the OpenGL context
                mplot::qt::gl_contexts::i().set_context<widget_index> (this->context());
                v.init_glad (mplot::qt::gl_contexts::getProcAddress<widget_index>);
                v.init (this);

                // Switch on multisampling anti-aliasing (with the num samples set in constructor)
                v.glfn->Enable (GL_MULTISAMPLE);
            }

            void resizeGL (int w, int h) override
            {
                double dpr = this->devicePixelRatio();
                v.set_winsize (static_cast<int>(std::round(w * dpr)), static_cast<int>(std::round(h * dpr)));
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
