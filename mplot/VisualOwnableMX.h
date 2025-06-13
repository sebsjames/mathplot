/*!
 * \file
 *
 * Awesome graphics code for high performance graphing and visualisation.
 *
 * This is intermediate class that sets up (multicontext aware) GL, leaving choice of window system
 * (GLFW3/Qt/wx/etc) to a derived class such as mplot::VisualMX or mplot::qt::viswidget_mx.
 *
 * This class is 'ownable', and can be used in other window drawing system such as Qt and wx, as
 * well as within mplot::Visual[NoMx], which marries it with the GLFW3 windowing system.
 *
 * Created by Seb James on 2025/03/01, from mplot::Visual.h
 *
 * \author Seb James
 * \date March 2025
 */
#pragma once

#if defined __gl3_h_ || defined __gl_h_ // could get a fuller list from glfw.h
// GL headers appear to have been externally included.
#else
// Include GLAD header
# define GLAD_GL_IMPLEMENTATION
#  include <mplot/glad/gl_mx.h>
#endif // GL headers

// By including this header, you take out a contract that you ARE using multicontext (MX) GLAD
// headers. This must appear BEFORE the rest of the mplot headers.
namespace mplot { namespace gl { static constexpr int multicontext = 1; } }

#include <mplot/VisualResourcesMX.h>
#include <mplot/VisualTextModel.h>
#include <mplot/VisualBase.h>
#include <mplot/gl/loadshaders_mx.h>

namespace mplot {

    /*!
     * VisualOwnableMX - adds multi-context-safe GL calls to the 'scene' base class, VisualBase
     *
     * This class assumes that GL functions have been loaded by the GLAD header system as a
     * GladGLContext pointer, which is called glfn here. GL function calls are glfn->Clear for
     * Clear() and glfn->Enable() for Enable() and so on.
     *
     * \tparam glver The OpenGL version, encoded as a single int (see mplot::gl::version)
     */
    template <int glver = mplot::gl::version_4_1>
    class VisualOwnableMX : public mplot::VisualBase<glver>
    {
    public:
        /*!
         * Default constructor is used when incorporating Visual inside another object
         * such as a QWidget.  We have to wait on calling init functions until an OpenGL
         * environment is guaranteed to exist.
         */
        VisualOwnableMX() { }

        /*!
         * Construct a new visualiser. The rule is 1 window to one Visual object. So, this creates a
         * new window and a new OpenGL context.
         */
        VisualOwnableMX (const int _width, const int _height, const std::string& _title, const bool _version_stdout = true)
        {
            this->window_w = _width;
            this->window_h = _height;
            this->title = _title;
            this->options.set (visual_options::versionStdout, _version_stdout);

            this->init_gl();
        }

        //! Deconstruct gl memory/context
        void deconstructCommon()
        {
            // Explicitly deconstruct any owned VisualModels
            this->vm.clear();
            // Explicitly deconstruct coordArrows, textModel and texts here
            this->coordArrows.reset(nullptr);
            this->textModel.reset(nullptr);
            for (auto& t : this->texts) { t.reset(nullptr); }

            if (this->shaders.gprog) {
                this->glfn->DeleteProgram (this->shaders.gprog);
                this->shaders.gprog = 0;
                this->active_gprog = mplot::visgl::graphics_shader_type::none;
            }
            if (this->shaders.tprog) {
                this->glfn->DeleteProgram (this->shaders.tprog);
                this->shaders.tprog = 0;
            }
            this->free_gladgl_context (this->glfn);

            // Free up the Fonts associated with this mplot::Visual
            mplot::VisualResourcesMX<glver>::i().freetype_deinit (this);
        }

    protected:
        void freetype_init() final
        {
            // Now make sure that Freetype is set up (we assume that caller code has set the correct OpenGL context)
            mplot::VisualResourcesMX<glver>::i().freetype_init (this, this->glfn);
        }

    public:
        // Do one-time init of the Visual's resources. This gets/creates the VisualResourcesMX,
        // registers this visual with resources, calls init_window for any glfw stuff that needs to
        // happen, and lastly initializes the freetype code.
        void init_resources()
        {
            // VisualResources provides font management and GLFW management. Ensure it exists in memory.
            mplot::VisualResourcesMX<glver>::i().create();
            this->freetype_init();
        }

        //! GLAD OpenGL function context pointer
        GladGLContext* glfn = nullptr;

        //! Stores the OpenGL function context version that was loaded
        int glfn_version = 0;

        //! Take a screenshot of the window. Return vec containing width * height or {-1, -1} on
        //! failure. Set transparent_bg to get a transparent background.
        sm::vec<int, 2> saveImage (const std::string& img_filename, const bool transparent_bg = false)
        {
            this->setContext();

            GLint viewport[4]; // current viewport
            this->glfn->GetIntegerv (GL_VIEWPORT, viewport);

            sm::vec<int, 2> dims;
            dims[0] = viewport[2];
            dims[1] = viewport[3];
            auto bits = std::make_unique<GLubyte[]>(dims.product() * 4);
            auto rbits = std::make_unique<GLubyte[]>(dims.product() * 4);

            this->glfn->Finish(); // finish all commands of OpenGL
            this->glfn->PixelStorei (GL_PACK_ALIGNMENT, 1);
            this->glfn->PixelStorei (GL_PACK_ROW_LENGTH, 0);
            this->glfn->PixelStorei (GL_PACK_SKIP_ROWS, 0);
            this->glfn->PixelStorei (GL_PACK_SKIP_PIXELS, 0);
            this->glfn->ReadPixels (0, 0, dims[0], dims[1], GL_RGBA, GL_UNSIGNED_BYTE, bits.get());

            for (int i = 0; i < dims[1]; ++i) {
                int rev_line = (dims[1] - i - 1) * 4 * dims[0];
                int for_line = i * 4 * dims[0];
                if (transparent_bg) {
                    for (int j = 0; j < 4 * dims[0]; ++j) {
                        rbits[rev_line + j] = bits[for_line + j];
                    }
                } else {
                    for (int j = 0; j < 4 * dims[0]; ++j) {
                        rbits[rev_line + j] = (j % 4 == 3) ? 255 : bits[for_line + j];
                    }
                }
            }
            unsigned int error = lodepng::encode (img_filename, rbits.get(), dims[0], dims[1]);
            if (error) {
                std::cerr << "encoder error " << error << ": " << lodepng_error_text (error) << std::endl;
                dims.set_from (-1);
                return dims;
            }
            return dims;
        }

        //! Render the scene
        void render() noexcept final
        {
            this->setContext();

            if (this->ptype == perspective_type::orthographic || this->ptype == perspective_type::perspective) {
                if (this->active_gprog != mplot::visgl::graphics_shader_type::projection2d) {
                    if (this->shaders.gprog) { this->glfn->DeleteProgram (this->shaders.gprog); }
                    this->shaders.gprog = mplot::gl::LoadShadersMX (this->proj2d_shader_progs, this->glfn);
                    this->active_gprog = mplot::visgl::graphics_shader_type::projection2d;
                }
            } else if (this->ptype == perspective_type::cylindrical) {
                if (this->active_gprog != mplot::visgl::graphics_shader_type::cylindrical) {
                    if (this->shaders.gprog) { this->glfn->DeleteProgram (this->shaders.gprog); }
                    this->shaders.gprog = mplot::gl::LoadShadersMX (this->cyl_shader_progs, this->glfn);
                    this->active_gprog = mplot::visgl::graphics_shader_type::cylindrical;
                }
            }

            this->glfn->UseProgram (this->shaders.gprog);
            this->glfn->Viewport (0, 0, this->window_w * mplot::retinaScale, this->window_h * mplot::retinaScale);

            // Set the perspective
            if (this->ptype == perspective_type::orthographic) {
                this->setOrthographic();
            } else if (this->ptype == perspective_type::perspective) {
                this->setPerspective();
            } else if (this->ptype == perspective_type::cylindrical) {
                // Set cylindrical-specific uniforms
                GLint loc_campos = this->glfn->GetUniformLocation (this->shaders.gprog, static_cast<const GLchar*>("cyl_cam_pos"));
                if (loc_campos != -1) { this->glfn->Uniform4fv (loc_campos, 1, this->cyl_cam_pos.data()); }
                GLint loc_cyl_radius = this->glfn->GetUniformLocation (this->shaders.gprog, static_cast<const GLchar*>("cyl_radius"));
                if (loc_cyl_radius != -1) { this->glfn->Uniform1f (loc_cyl_radius, this->cyl_radius); }
                GLint loc_cyl_height = this->glfn->GetUniformLocation (this->shaders.gprog, static_cast<const GLchar*>("cyl_height"));
                if (loc_cyl_height != -1) { this->glfn->Uniform1f (loc_cyl_height, this->cyl_height); }
            } else {
                // unknown projection
                return;
            }

            // Calculate model view transformation - transforming from "model space" to "worldspace".
            sm::mat44<float> sceneview = this->scene;
            if (this->ptype == perspective_type::orthographic || this->ptype == perspective_type::perspective) {
                // This line translates from model space to world space. Avoid in cyl?
                //sceneview.translate (this->scenetrans); // send backwards into distance
            }
            // And this rotation completes the transition from model to world
            //sceneview.prerotate (this->rotation);

            // Clear color buffer and **also depth buffer**
            this->glfn->Clear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Set the background colour:
            this->glfn->ClearBufferfv (GL_COLOR, 0, this->bgcolour.data());

            // Lighting shader variables
            //
            // Ambient light colour
            GLint loc_lightcol = this->glfn->GetUniformLocation (this->shaders.gprog, static_cast<const GLchar*>("light_colour"));
            if (loc_lightcol != -1) { this->glfn->Uniform3fv (loc_lightcol, 1, this->light_colour.data()); }
            // Ambient light intensity
            GLint loc_ai = this->glfn->GetUniformLocation (this->shaders.gprog, static_cast<const GLchar*>("ambient_intensity"));
            if (loc_ai != -1) { this->glfn->Uniform1f (loc_ai, this->ambient_intensity); }
            // Diffuse light position
            GLint loc_dp = this->glfn->GetUniformLocation (this->shaders.gprog, static_cast<const GLchar*>("diffuse_position"));
            if (loc_dp != -1) { this->glfn->Uniform3fv (loc_dp, 1, this->diffuse_position.data()); }
            // Diffuse light intensity
            GLint loc_di = this->glfn->GetUniformLocation (this->shaders.gprog, static_cast<const GLchar*>("diffuse_intensity"));
            if (loc_di != -1) { this->glfn->Uniform1f (loc_di, this->diffuse_intensity); }

            // Switch to text shader program and set the projection matrix
            this->glfn->UseProgram (this->shaders.tprog);
            GLint loc_p = this->glfn->GetUniformLocation (this->shaders.tprog, static_cast<const GLchar*>("p_matrix"));
            if (loc_p != -1) { this->glfn->UniformMatrix4fv (loc_p, 1, GL_FALSE, this->projection.mat.data()); }

            // Switch back to the regular shader prog and render the VisualModels.
            this->glfn->UseProgram (this->shaders.gprog);

            // Set the projection matrix just once
            loc_p = this->glfn->GetUniformLocation (this->shaders.gprog, static_cast<const GLchar*>("p_matrix"));
            if (loc_p != -1) { this->glfn->UniformMatrix4fv (loc_p, 1, GL_FALSE, this->projection.mat.data()); }

            if ((this->ptype == perspective_type::orthographic || this->ptype == perspective_type::perspective)
                && this->options.test(visual_options::showCoordArrows)) {
                // Ensure coordarrows centre sphere will be visible on BG:
                this->coordArrows->setColourForBackground (this->bgcolour); // releases context...
                this->setContext(); // ...so re-acquire if we're managing it

                if (this->options.test (visual_options::coordArrowsInScene) == true) {
                    this->coordArrows->setSceneMatrix (sceneview);
                } else {
                    this->positionCoordArrows();
                }
                this->coordArrows->render();
            }

            sm::mat44<float> scenetransonly;
            scenetransonly.translate (this->scene.translation());

            auto vmi = this->vm.begin();
            while (vmi != this->vm.end()) {
                if ((*vmi)->twodimensional == true) {
                    // It's a two-d thing. Now what?
                    (*vmi)->setSceneMatrix (scenetransonly);
                } else {
                    (*vmi)->setSceneMatrix (sceneview);
                }
                (*vmi)->render();
                ++vmi;
            }

            sm::vec<float, 3> v0 = this->textPosition ({-0.8f, 0.8f});
            if (this->options.test (visual_options::showTitle) == true) {
                // Render the title text
                this->textModel->setSceneTranslation (v0);
                this->textModel->setVisibleOn (this->bgcolour);
                this->textModel->render();
            }

            auto ti = this->texts.begin();
            while (ti != this->texts.end()) {
                (*ti)->setSceneTranslation (v0);
                (*ti)->setVisibleOn (this->bgcolour);
                (*ti)->render();
                ++ti;
            }

            this->swapBuffers();
        }

        //! Glad MX specific callback
        static GladGLContext* get_glfn (mplot::VisualBase<glver>* _v)
        {
            return reinterpret_cast<mplot::VisualOwnableMX<glver>*>(_v)->glfn;
        };

    protected:
        // GLAD specific gl context creation/freeing. GladGLContext is a struct containing
        GladGLContext* create_gladgl_context (const GLADloadfunc procaddressfn)
        {
            GladGLContext* context = (GladGLContext*) calloc(1, sizeof(GladGLContext));
            if (!context) { return nullptr; }
            this->glfn_version = gladLoadGLContext (context, procaddressfn);
            // ...so glfn_version should (more or less) match the version specified in the glver
            // template arg
            return context;
        }
        void free_gladgl_context (GladGLContext *context)
        {
            if (context) { free(context); }
            context = nullptr;
        }

    public:
        void init_glad (GLADloadfunc procaddressfn)
        {
            // Create the OpenGL function context - a GladGLContext*
            this->glfn = this->create_gladgl_context (procaddressfn);

            if (!this->glfn) {
                std::cout << "Failed to initialize GLAD GL context" << std::endl;
                this->free_gladgl_context (this->glfn);
            }
        }

        template <typename T>
        void bindmodel (std::unique_ptr<T>& model)
        {
            model->set_parent (this);
            model->get_shaderprogs = &mplot::VisualBase<glver>::get_shaderprogs;
            model->get_gprog = &mplot::VisualBase<glver>::get_gprog;
            model->get_tprog = &mplot::VisualBase<glver>::get_tprog;
            model->get_glfn = &mplot::VisualOwnableMX<glver>::get_glfn;
        }

        //! Add a label _text to the scene at position _toffset. Font features are
        //! defined by the tfeatures. Return geometry of the text.
        mplot::TextGeometry addLabel (const std::string& _text,
                                      const sm::vec<float, 3>& _toffset,
                                      const mplot::TextFeatures& tfeatures = mplot::TextFeatures(0.01f))
        {
            this->setContext();
            if (this->shaders.tprog == 0) { throw std::runtime_error ("No text shader prog."); }
            auto tmup = std::make_unique<mplot::VisualTextModel<glver>> (tfeatures);
            this->bindmodel (tmup);
            if (tfeatures.centre_horz == true) {
                mplot::TextGeometry tg = tmup->getTextGeometry(_text);
                sm::vec<float, 3> centred_locn = _toffset;
                centred_locn[0] = -tg.half_width();
                tmup->setupText (_text, centred_locn, tfeatures.colour);
            } else {
                tmup->setupText (_text, _toffset, tfeatures.colour);
            }
            mplot::VisualTextModel<glver>* tm = tmup.get();
            this->texts.push_back (std::move(tmup));
            this->releaseContext();
            return tm->getTextGeometry();
        }

        //! Add a label _text to the scene at position _toffset. Font features are
        //! defined by the tfeatures. Return geometry of the text. The pointer tm is a
        //! return value that allows client code to change the text after the label has been added.
        mplot::TextGeometry addLabel (const std::string& _text,
                                      const sm::vec<float, 3>& _toffset,
                                      mplot::VisualTextModel<glver>*& tm,
                                      const mplot::TextFeatures& tfeatures = mplot::TextFeatures(0.01f))
        {
            this->setContext();
            if (this->shaders.tprog == 0) { throw std::runtime_error ("No text shader prog."); }
            auto tmup = std::make_unique<mplot::VisualTextModel<glver>> (tfeatures);
            this->bindmodel (tmup);
            if (tfeatures.centre_horz == true) {
                mplot::TextGeometry tg = tmup->getTextGeometry(_text);
                sm::vec<float, 3> centred_locn = _toffset;
                centred_locn[0] = -tg.half_width();
                tmup->setupText (_text, centred_locn, tfeatures.colour);
            } else {
                tmup->setupText (_text, _toffset, tfeatures.colour);
            }
            tm = tmup.get();
            this->texts.push_back (std::move(tmup));
            this->releaseContext();
            return tm->getTextGeometry();
        }

    protected:
        // Initialize OpenGL shaders, set some flags (Alpha, Anti-aliasing), read in any external
        // state from json, and set up the coordinate arrows and any VisualTextModels that will be
        // required to render the Visual.
        void init_gl()
        {
            this->setContext(); // if managing context

            if (this->options.test (visual_options::versionStdout) == true) {
                unsigned char* glv = (unsigned char*)this->glfn->GetString(GL_VERSION);
                std::cout << "This is version " << mplot::version_string()
                          << " of mplot::Visual<glver=" << mplot::gl::version::vstring (glver)
                          << "> running on OpenGL Version " << glv << std::endl;
            }

            this->setSwapInterval();

            // Load up the shaders
            this->proj2d_shader_progs = {
                {GL_VERTEX_SHADER, "Visual.vert.glsl", mplot::getDefaultVtxShader(glver), 0 },
                {GL_FRAGMENT_SHADER, "Visual.frag.glsl", mplot::getDefaultFragShader(glver), 0 }
            };
            this->shaders.gprog = mplot::gl::LoadShadersMX (this->proj2d_shader_progs, this->glfn);
            this->active_gprog = mplot::visgl::graphics_shader_type::projection2d;

            // Alternative cylindrical shader for possible later use. (NB: not loaded immediately)
            this->cyl_shader_progs = {
                {GL_VERTEX_SHADER, "VisCyl.vert.glsl", mplot::getDefaultCylVtxShader(glver), 0 },
                {GL_FRAGMENT_SHADER, "Visual.frag.glsl", mplot::getDefaultFragShader(glver), 0 }
            };

            // A specific text shader is loaded for text rendering
            this->text_shader_progs = {
                {GL_VERTEX_SHADER, "VisText.vert.glsl", mplot::getDefaultTextVtxShader(glver), 0 },
                {GL_FRAGMENT_SHADER, "VisText.frag.glsl" , mplot::getDefaultTextFragShader(glver), 0 }
            };
            this->shaders.tprog = mplot::gl::LoadShadersMX (this->text_shader_progs, this->glfn);

            // OpenGL options
            this->glfn->Enable (GL_DEPTH_TEST);
            this->glfn->Enable (GL_BLEND);
            this->glfn->BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            this->glfn->Disable (GL_CULL_FACE);
            mplot::gl::Util::checkError (__FILE__, __LINE__, this->glfn);

            // If possible, read in scenetrans and rotation state from a special config file
            try {
                nlohmann::json vconf;
                std::ifstream fi;
                fi.open ("/tmp/Visual.json", std::ios::in);
                fi >> vconf;
                sm::vec<float> scenetrans = this->scene.translation();
                scenetrans[0] = vconf.contains("scenetrans_x") ? vconf["scenetrans_x"].get<float>() : scenetrans[0];
                scenetrans[1] = vconf.contains("scenetrans_y") ? vconf["scenetrans_y"].get<float>() : scenetrans[1];
                scenetrans[2] = vconf.contains("scenetrans_z") ? vconf["scenetrans_z"].get<float>() : scenetrans[2];
                this->scene.translate (scenetrans);
                // Place the same numbers into scenetrans_default, too.
                this->scenetrans_default[0] = scenetrans[0];
                this->scenetrans_default[1] = scenetrans[1];
                this->scenetrans_default[2] = scenetrans[2];

                sm::quaternion<float> rotation;
                rotation.w = vconf.contains("scenerotn_w") ? vconf["scenerotn_w"].get<float>() : rotation.w;
                rotation.x = vconf.contains("scenerotn_x") ? vconf["scenerotn_x"].get<float>() : rotation.x;
                rotation.y = vconf.contains("scenerotn_y") ? vconf["scenerotn_y"].get<float>() : rotation.y;
                rotation.z = vconf.contains("scenerotn_z") ? vconf["scenerotn_z"].get<float>() : rotation.z;
                // Apply to scene
                this->scene.rotate (rotation);

            } catch (...) {
                // No problem if we couldn't read /tmp/Visual.json
            }

            // Use coordArrowsOffset to set the location of the CoordArrows *scene*
            this->coordArrows = std::make_unique<mplot::CoordArrows<glver>>();
            // For CoordArrows, because we don't add via Visual::addVisualModel(), we
            // have to set the get_shaderprogs function here:
            this->bindmodel (this->coordArrows);
            // And NOW we can proceed to init:
            this->coordArrows->init (this->coordArrowsLength, this->coordArrowsThickness, this->coordArrowsEm);
            this->coordArrows->finalize(); // VisualModel::finalize releases context (normally this is the right thing)...
            this->setContext();            // ...but we've got more work to do, so re-acquire context (if we're managing it)

            mplot::gl::Util::checkError (__FILE__, __LINE__, this->glfn);

            // Set up the title, which may or may not be rendered
            mplot::TextFeatures title_tf(0.035f, 64);
            this->textModel = std::make_unique<mplot::VisualTextModel<glver>> (title_tf);
            this->bindmodel (this->textModel);
            this->textModel->setSceneTranslation ({0.0f, 0.0f, 0.0f});
            this->textModel->setupText (this->title);

            this->releaseContext();
        }

        //! A VisualTextModel for a title text.
        std::unique_ptr<mplot::VisualTextModel<glver>> textModel = nullptr;
        //! Text models for labels
        std::vector<std::unique_ptr<mplot::VisualTextModel<glver>>> texts;
    };

} // namespace mplot
