/*!
 * \file
 *
 * Awesome graphics code for high performance graphing and visualisation.
 *
 * This is intermediate class that sets up (multicontext aware) GL, leaving choice of window system
 * (GLFW3/Qt/wx/etc) to a derived class such as mplot::Visual or mplot::qt::viswidget.
 *
 * This class is 'ownable', and can be used in other window drawing system such as Qt and wx, as
 * well as within mplot::Visual, which marries it with the GLFW3 windowing system.
 *
 * Created by Seb James on 2025/03/01, from mplot::Visual.h
 *
 * \author Seb James
 * \date March 2025
 */
module;

#include <cstdint>
#include <iostream>
#ifndef _MSC_VER
# include <fstream>
#endif
#include <string>
#include <array>
#include <vector>
#include <map>
#include <utility>
#include <memory>
#include <functional>
#include <cstddef>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>

#include <mplot/gl/shaders.h>
#include <mplot/gl/loadshaders_mx.h>

#include <mplot/VisualDefaultShaders.hpp>

export module mplot.visualownable;

#indef _MSC_VER
import <fstream>
#endif

export import mplot.version;
export import mplot.visualmodel;
export import mplot.win_t;
export import mplot.visualcommon;
import mplot.visualresources;
export import mplot.visualtextmodel;
export import mplot.textgeometry;
export import mplot.textfeatures;
import mplot.coordarrows;
export import mplot.gl.version;
import mplot.gl.util;
import mplot.tools;
import mplot.loadpng; // Use Lode Vandevenne's PNG encoder
export import mplot.keys;
import nlohmann.json;

import sm.mathconst;
export import sm.vec;
export import sm.flags;
import sm.quaternion;
import sm.mat;

export namespace mplot
{
    //! Here are our boolean state flags
    enum class visual_state : std::uint32_t
    {
        readyToFinish,
        //! paused can be set true so that pauseOpen() can be used to display the window mid-simulation
        paused,
        //! If you set this to true, then the mouse movements won't change scenetrans or rotation.
        sceneLocked,
        //! When true, cursor movements induce rotation of scene
        rotateMode,
        //! When true, rotations about the third axis are possible.
        rotateModMode,
        //! When true, cursor movements induce translation of scene
        translateMode,
        //! We are scrolling (and so we will need to zero scenetrans_delta after enacting the change)
        scrolling,
        //! True means that at least one of our VisualModels is an instanced rendering model
        haveInstanced,
        //! Left mouse button is down
        mouseButtonLeftPressed,
        //! Right mouse button is down
        mouseButtonRightPressed,
        //! If client code changes viewfollowsVMTranslations or viewFollowsVMBehind, this should be
        //! set to signal to computeSceneview that the lastSceneview should be saved or restored to.
        viewFollowsModeChanged,
        //! Set true while the sceneview is 'zooming back' to the overview mode (or the whereever-you-left-it mode)
        viewTransition,
        //! Set true if we're in the middle of a scripted sceneview change, such as zooming in over a period of time
        viewAutomation
    };

    //! Boolean options - similar to state, but more likely to be modified by client code
    enum class visual_options : std::uint32_t
    {
        //! Set true to disable the 'X' button on the Window from exiting the program
        preventWindowCloseWithButton,
        //! Set to true to show the coordinate arrows
        showCoordArrows,
        //! If true, then place the coordinate arrows at the origin of the scene, rather than offset.
        coordArrowsInScene,
        //! Show user frame of reference (for debug)
        showUserFrame,
        //! Set to true to show the title text within the scene
        showTitle,
        //! Set true to output some user information to stdout (e.g. user requested quit)
        userInfoStdout,
        //! If true, output mplot version to stdout
        versionStdout,
        //! If true (the default), then call swapBuffers() at the end of render()
        renderSwapsBuffers,
        /*!
         * If true, rotation is about the scene origin, rather than the most central VisualModel.
         *
         * If false, the system finds the most central VisualModel, and rotates about the centroid
         * of the bounding box that surrounds that VisualModel.
         */
        rotateAboutSceneOrigin,
        /*!
         * If true, horizontal mouse movements rotate the scene about a chosen vertical axis, and
         * vertical mouse movements rotate the vertical axis about the bottom of the user's
         * viewport. This will be familiar to Blender users.  Additionally, if the ctrl-modified
         * mouse move mode is enabled, the scene is tilted about the axis coming out of the
         * viewport.
         *
         * If false, horizontal mouse movements rotate the scene about the vertical axis of the
         * user's viewport, vertcial mouse movements rotate the scene about the horizontal axis of
         * the viewport, and ctrl-modified mouse movements rotate the scene about the axis coming
         * out of the viewport. This was the original scene navigation scheme in mathplot and before
         * that in morphologica.
         */
        rotateAboutVertical,
        /*!
         * If true, write bounding boxes out to a json file /tmp/mathplot_bounding_boxes.json that
         * can be read with the show_boundingboxes program
         */
        boundingBoxesToJson,
        //! If true, draw all the bounding boxes around the VisualModels
        showBoundingBoxes,
        /*!
         * If true, then turn on the bounding box for the VM about which we are rotating and turn
         * the others off (ignoring the value of 'showBoundingBoxes')
         */
        highlightRotationVM,
        /*!
         * If true, the view of the scene follows a model translation (one of the VisualModels in
         * the scene has to be nominated as the 'model to follow'. Useful for top-down views. The
         * selected model to follow is in a member attribute followedModel
         */
        viewFollowsVMTranslations,
        /*!
         * The view 'camera' rotates with the selected VM (followedModel)
         */
        viewFollowsVMBehind
    };

    //! Whether to render with perspective or orthographic
    enum class perspective_type : std::uint32_t
    {
        perspective,
        orthographic
    };

    //! Automated sceneview transforms, i.e. 'film direction' events. enumerated type of event.
    enum class direction_event : std::uint32_t
    {
        sceneview,
        timed_translation,
        timed_rotation,
        timed_transform,  // Move towards a given sceneview
        timed_orbit,      // Starting from any sceneview, orbit a point
        unknown
    };

    //! Automated sceneview transforms, i.e. 'film direction' events. This holds the data for an event.
    struct direction_data
    {
        direction_event event = direction_event::unknown;
        sm::mat<float, 4> sceneview;         // a sceneview to move to, either instantaneously, or over a period of time
        sm::vec<float, 3> translation = {};  // A translation to apply to sceneview over time transform_time
        sm::vec<float, 3> orbit_centre = {}; // Centre of the orbit for timed_orbit
        sm::vec<float, 3> orbit_axis = sm::vec<float>::uz();
        float orbit_angle = sm::mathconst<float>::pi;
        sm::quaternion<float> rotation_start;// Holds the rotation of the sceneview at the start of the transform
        sm::quaternion<float> rotation;      // A rotation of sceneview to achieve over time transform_time.
        float about_vert_angle = 0.0f;       // A rotation about the scene's up axis (in degrees)
        float tilt_angle = 0.0f;
        bool min_jerk = true;                // Should the movement be minimum jerk, or linear?
        std::uint32_t transform_time_frames = 0u; // Specify transform time in frames
        std::uint32_t start_frame = std::numeric_limits<std::uint32_t>::max();
        std::uint32_t id = std::numeric_limits<std::uint32_t>::max(); // An ID for this direction (could be tied to movement frame)
    };

    /*!
     * VisualOwnable - An ownable 'scene' base class.
     *
     * This class assumes that GL functions have been loaded by the GLAD header system as a
     * GladGLContext pointer, which is called glfn here. GL function calls are glfn->Clear for
     * Clear() and glfn->Enable() for Enable() and so on.
     *
     * \tparam glver The OpenGL version, encoded as a single int (see mplot::gl::version)
     */
    template <std::int32_t glver = mplot::gl::version_4_1>
    class VisualOwnable
    {
    public:
        /*!
         * Default constructor is used when incorporating Visual inside another object
         * such as a QWidget.  We have to wait on calling init functions until an OpenGL
         * environment is guaranteed to exist.
         */
        VisualOwnable()
        {
            this->sceneview.translate (this->scenetrans_default);
            this->sceneview_tr.translate (this->scenetrans_default);
        }

        /*!
         * Construct a new visualiser. The rule is 1 window to one Visual object. So, this creates a
         * new window and a new OpenGL context.
         */
        VisualOwnable (const std::int32_t _width, const std::int32_t _height, const std::string& _title, const bool _version_stdout = true)
        {
            this->sceneview.translate (this->scenetrans_default);
            this->sceneview_tr.translate (this->scenetrans_default);
            this->window_w = _width;
            this->window_h = _height;
            this->title = _title;
            this->options.set (visual_options::versionStdout, _version_stdout);
            this->init_gl();
        }

        ~VisualOwnable() {}

        //! Deconstruct gl memory/context
        void deconstructCommon()
        {
            // Explicitly deconstruct any owned VisualModels
            this->vm.clear();
            // Explicitly deconstruct coordArrows, textModel and texts here
            this->coordArrows.reset (nullptr);
            this->textModel.reset (nullptr);
            for (std::uint32_t i = 0; i < this->texts.size(); ++i) { this->texts[i].reset (nullptr); }
            //for (auto& t : this->texts) { t.reset (nullptr); }

            if (this->shaders.gprog) {
                this->glfn->DeleteProgram (this->shaders.gprog);
                this->shaders.gprog = 0;
            }
            if (this->shaders.tprog) {
                this->glfn->DeleteProgram (this->shaders.tprog);
                this->shaders.tprog = 0;
            }
            this->free_gladgl_context (this->glfn);

            // Free up the Fonts associated with this mplot::Visual
            mplot::VisualResources<glver>::i().freetype_deinit (this->visual_id);
        }

        // Public init that is given a context (window or widget) and then sets up the
        // VisualResource, shaders and so on.
        void init (mplot::win_t* ctx)
        {
            this->window = ctx;
            this->init_resources();
            this->init_gl();
        }

        // Do one-time init of the Visual's resources. This gets/creates the VisualResources,
        // registers this visual with resources, calls init_window for any glfw stuff that needs to
        // happen, and lastly initializes the freetype code.
        void init_resources()
        {
            // VisualResources provides font management and GLFW management. Ensure it exists in memory.
            mplot::VisualResources<glver>::i().create();
            this->visual_id = mplot::VisualResources<glver>::i().register_visual (this->glfn, this->window);
            mplot::VisualResources<glver>::i().freetype_init (this->visual_id, this->glfn);
        }

        /*!
         * Add a VisualModel to the scene as a unique_ptr. The Visual object takes ownership of the
         * unique_ptr. The index into Visual::vm is returned.
         */
        template <typename T>
        std::uint32_t addVisualModelId (std::unique_ptr<T>& model)
        {
            std::unique_ptr<mplot::VisualModel<glver>> vmp = std::move(model);
            vmp->set_parent (this->visual_id);
            if (vmp->instanced()) { this->state.set (visual_state::haveInstanced, true); }
            this->vm.push_back (std::move(vmp));
            std::uint32_t rtn = (this->vm.size()-1);
            return rtn;
        }

        /*!
         * Add a VisualModel to the scene as a unique_ptr. The Visual object takes ownership of the
         * unique_ptr. A non-owning pointer to the model is returned.
         */
        template <typename T>
        T* addVisualModel (std::unique_ptr<T>& model)
        {
            std::unique_ptr<mplot::VisualModel<glver>> vmp = std::move(model);
            vmp->set_parent (this->visual_id);
            if (vmp->instanced()) { this->state.set (visual_state::haveInstanced, true); }
            this->vm.push_back (std::move(vmp));
            return static_cast<T*>(this->vm.back().get());
        }

        /*!
         * Test the pointer vmp. Return vmp if it is owned by a unique_ptr in
         * Visual::vm. If it is not present, return nullptr.
         */
        const mplot::VisualModel<glver>* validVisualModel (const mplot::VisualModel<glver>* vmp) const
        {
            const mplot::VisualModel<glver>* rtn = nullptr;
            for (std::uint32_t modelId = 0; modelId < this->vm.size(); ++modelId) {
                if (this->vm[modelId].get() == vmp) {
                    rtn = vmp;
                    break;
                }
            }
            return rtn;
        }

        /*!
         * For the pointer vmp, if it is owned by a unique_ptr in Visual::vm, then return the index
         * into Visual::vm at which it lives. This is its model ID. If it does not exist in
         * Visual::vm, then return std::numeric_limits<uint32_t>::max().
         */
        std::uint32_t getVisualModelId (const mplot::VisualModel<glver>* vmp) const
        {
            std::uint32_t rtn = std::numeric_limits<uint32_t>::max();
            for (std::uint32_t modelId = 0; modelId < this->vm.size(); ++modelId) {
                if (this->vm[modelId].get() == vmp) {
                    rtn = modelId;
                    break;
                }
            }
            return rtn;
        }

        void setFollowedVM (const mplot::VisualModel<glver>* vm_to_follow)
        {
            for (std::uint32_t modelId = 0; modelId < this->vm.size(); ++modelId) {
                if (this->vm[modelId].get() == vm_to_follow) {
                    this->followedVM = this->vm[modelId].get();
                    this->followedLastViewMatrix = this->followedVM->getViewMatrix();
                    break;
                }
            }
        }

        /*!
         * VisualModel Getter
         *
         * For the given \a modelId, return a (non-owning) pointer to the visual model.
         *
         * \return VisualModel pointer
         */
        mplot::VisualModel<glver>* getVisualModel (std::uint32_t modelId) { return (this->vm[modelId].get()); }

        //! Remove the VisualModel with ID \a modelId from the scene.
        void removeVisualModel (std::uint32_t modelId) { this->vm.erase (this->vm.begin() + modelId); }

        //! Remove the VisualModel whose pointer matches the VisualModel* vmp
        void removeVisualModel (mplot::VisualModel<glver>* vmp)
        {
            std::uint32_t modelId = 0;
            bool found_model = false;
            for (modelId = 0; modelId < this->vm.size(); ++modelId) {
                if (this->vm[modelId].get() == vmp) {
                    found_model = true;
                    break;
                }
            }
            if (found_model == true) { this->vm.erase (this->vm.begin() + modelId); }
        }

        void clear () { this->vm.clear(); }

        void set_cursorpos (double _x, double _y) { this->cursorpos = {static_cast<float>(_x), static_cast<float>(_y)}; }

        //! A callback function
        static void callback_render (mplot::VisualOwnable<glver>* _v) { _v->render(); };

        //! GLAD OpenGL function context pointer. A copy is stored in VisualResources.
        GladGLContext* glfn = nullptr;

        //! Stores the OpenGL function context version that was loaded
        std::int32_t glfn_version = 0;

        //! Graphics context functions that refer to the window system (GLFW usually) are defined in derived class
        virtual void setContext() = 0;
        virtual void releaseContext()  = 0;
        virtual void swapBuffers() = 0;
        virtual void setSwapInterval() = 0;

        //! Take a screenshot of the window. Return vec containing width * height or {-1, -1} on
        //! failure. Set transparent_bg to get a transparent background.
        sm::vec<std::int32_t, 2> saveImage (const std::string& img_filename, const bool transparent_bg = false)
        {
            using namespace std::chrono;
            using sc = std::chrono::steady_clock;
            constexpr bool profile_saveimage = false;
            sc::time_point t0, t1, t2;

            this->setContext();

            if constexpr (profile_saveimage) { t0 = sc::now(); }

            std::int32_t viewport[4]; // current viewport
            this->glfn->GetIntegerv (GL_VIEWPORT, viewport);

            sm::vec<std::int32_t, 2> dims;
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

            for (std::int32_t i = 0; i < dims[1]; ++i) {
                std::int32_t rev_line = (dims[1] - i - 1) * 4 * dims[0];
                std::int32_t for_line = i * 4 * dims[0];
                if (transparent_bg) {
                    for (std::int32_t j = 0; j < 4 * dims[0]; ++j) {
                        rbits[rev_line + j] = bits[for_line + j];
                    }
                } else {
                    for (std::int32_t j = 0; j < 4 * dims[0]; ++j) {
                        rbits[rev_line + j] = (j % 4 == 3) ? 255 : bits[for_line + j];
                    }
                }
            }

            if constexpr (profile_saveimage) { t1 = sc::now(); }

            std::uint32_t error = 0;

            // If filename ends with pnm, then save in pnm format; otherwise save as PNG
            bool pnm_save = false;
            if (img_filename.find ("pnm") == img_filename.size() - 3) { pnm_save = true; }
            if (pnm_save == true) {
                error = mplot::pnm_encode (img_filename, rbits.get(), dims[0], dims[1]);
            } else {
                error = mplot::png_encode (img_filename, rbits.get(), dims[0], dims[1]); // Need alternative
            }

            if constexpr (profile_saveimage) {
                t2 = sc::now();
                sc::duration t_d = t1 - t0;
                sc::duration t_d2 = t2 - t1;
                std::cout << "Bit-collection: " << duration_cast<milliseconds>(t_d).count() << " ms"
                          << " and png/pnm_encode: " << duration_cast<milliseconds>(t_d2).count() << " ms\n";
            }

            if (error) {
                if (pnm_save == true) {
                    std::cerr << "pnm encoder error " << error << std::endl;
                } else {
                    std::cerr << "encoder error " << error << ": " << mplot::png_error_text (error) << std::endl;
                }
                dims.set_from (-1);
                return dims;
            }
            return dims;
        }

        //! Render the scene
        void render() noexcept
        {
            ++this->render_counter;

            this->setContext();

            this->glfn->UseProgram (this->shaders.gprog);
            this->glfn->Viewport (0, 0, this->window_w * this->window_scale_w, this->window_h * this->window_scale_h);

            // Set the perspective
            if (this->ptype == perspective_type::orthographic) {
                this->setOrthographic();
            } else if (this->ptype == perspective_type::perspective) {
                this->setPerspective();
            } else {
                // unknown projection
                return;
            }

            // Calculate model view transformation - transforming from "model space" to "worldspace".
            this->computeSceneview();

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
            if (loc_p != -1) { this->glfn->UniformMatrix4fv (loc_p, 1, GL_FALSE, this->projection.arr.data()); }

            // Switch back to the regular shader prog and render the VisualModels.
            this->glfn->UseProgram (this->shaders.gprog);

            // Set the projection matrix just once
            loc_p = this->glfn->GetUniformLocation (this->shaders.gprog, static_cast<const GLchar*>("p_matrix"));
            if (loc_p != -1) { this->glfn->UniformMatrix4fv (loc_p, 1, GL_FALSE, this->projection.arr.data()); }

            if ((this->ptype == perspective_type::orthographic || this->ptype == perspective_type::perspective)
                && this->options.test (visual_options::showCoordArrows)) {
                // Ensure coordarrows centre sphere will be visible on BG:
                this->coordArrows->setColourForBackground (this->bgcolour); // releases context...
                this->setContext(); // ...so re-acquire if we're managing it

                if (this->options.test (visual_options::coordArrowsInScene) == true) {
                    this->coordArrows->setSceneMatrix (this->sceneview);
                } else {
                    this->positionCoordArrows();
                }
                this->coordArrows->render();
            }

            if (this->haveInstanced() && mplot::VisualResources<glver>::i().get_instanced_needs_update (this->visual_id)) {
                 mplot::VisualResources<glver>::i().copy_instance_ssbo_to_gpu();
                 mplot::VisualResources<glver>::i().instanced_needs_update (this->visual_id, false);
            }

            auto vmi = this->vm.begin();
            while (vmi != this->vm.end()) {
                if ((*vmi)->twodimensional() == true) {
                    // It's a two-d thing. Use the companion 'scene trans only' matrix, which avoids any rotations
                    (*vmi)->setSceneMatrix (this->sceneview_tr);
                } else {
                    (*vmi)->setSceneMatrix (this->sceneview);
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

            if (this->options.test (visual_options::renderSwapsBuffers) == true) {
                this->swapBuffers();
            }
        }

        //! Compute a translation vector for text position, using Visual::text_z.
        sm::vec<float, 3> textPosition (const sm::vec<float, 2> p0_coord)
        {
            // For the depth at which a text object lies, use this->text_z.  Use forward
            // projection to determine the correct z coordinate for the inverse
            // projection.
            sm::vec<float, 4> point =  { 0.0f, 0.0f, this->text_z, 1.0f };
            sm::vec<float, 4> pp = this->projection * point;
            float coord_z = pp[2]/pp[3]; // divide by pp[3] is divide by/normalise by 'w'.
            // Construct the point for the location of the text
            sm::vec<float, 4> p0 = { p0_coord.x(), p0_coord.y(), coord_z, 1.0f };
            // Inverse project the point
            sm::vec<float, 3> v0;
            v0.set_from (this->invproj * p0);
            return v0;
        }

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

        //! Add a label _text to the scene at position _toffset. Font features are
        //! defined by the tfeatures. Return geometry of the text.
        mplot::TextGeometry addLabel (const std::string& _text,
                                      const sm::vec<float, 3>& _toffset,
                                      const mplot::TextFeatures& tfeatures = mplot::TextFeatures(0.01f))
        {
            this->setContext();
            if (this->shaders.tprog == 0) { throw std::runtime_error ("No text shader prog."); }
            auto tmup = std::make_unique<mplot::VisualTextModel<glver>> (tfeatures);
            tmup->set_parent (this->visual_id);
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
            tmup->set_parent (this->visual_id);
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
                std::uint8_t* glv = (std::uint8_t*)this->glfn->GetString(GL_VERSION);
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
            mplot::VisualResources<glver>::i().set_gprog (this->visual_id, this->shaders.gprog);

            // A specific text shader is loaded for text rendering
            this->text_shader_progs = {
                {GL_VERTEX_SHADER, "VisText.vert.glsl", mplot::getDefaultTextVtxShader(glver), 0 },
                {GL_FRAGMENT_SHADER, "VisText.frag.glsl" , mplot::getDefaultTextFragShader(glver), 0 }
            };
            this->shaders.tprog = mplot::gl::LoadShadersMX (this->text_shader_progs, this->glfn);
            mplot::VisualResources<glver>::i().set_tprog (this->visual_id, this->shaders.tprog);

            // OpenGL options
            this->glfn->Enable (GL_DEPTH_TEST);
            this->glfn->Enable (GL_BLEND);
            this->glfn->BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            this->glfn->Disable (GL_CULL_FACE);
            mplot::gl::Util::checkError (__FILE__, __LINE__, this->glfn);

            this->read_scenetrans_from_json();

            // Use coordArrowsOffset to set the location of the CoordArrows *scene*
            this->coordArrows = std::make_unique<mplot::CoordArrows<glver>>();
            this->coordArrows->set_parent (this->visual_id);
            // And NOW we can proceed to init (lengths, thickness, em size for labels):
            this->coordArrows->init (sm::vec<>{0.1f, 0.1f, 0.1f}, 1.0f, 0.01f);
            this->coordArrows->finalize(); // VisualModel::finalize releases context (normally this is the right thing)...
            this->setContext();            // ...but we've got more work to do, so re-acquire context (if we're managing it)

            mplot::gl::Util::checkError (__FILE__, __LINE__, this->glfn);

            // Set up the title, which may or may not be rendered
            mplot::TextFeatures title_tf(0.035f, 64);
            this->textModel = std::make_unique<mplot::VisualTextModel<glver>> (title_tf);
            this->textModel->set_parent (this->visual_id);
            this->textModel->setSceneTranslation ({0.0f, 0.0f, 0.0f});
            this->textModel->setupText (this->title);

            this->releaseContext();
        }

        //! A VisualTextModel for a title text.
        std::unique_ptr<mplot::VisualTextModel<glver>> textModel = nullptr;
        //! Text models for labels
        std::vector<std::unique_ptr<mplot::VisualTextModel<glver>>> texts;

    public:
        //! The OpenGL shader programs have an integer ID and are stored in a simple struct. There's
        //! one for graphical objects and a text shader program, which uses textures to draw text on
        //! quads.
        mplot::visgl::visual_shaderprogs shaders; // stored in VisualResources too.

        //! Stores the info required to load the 2D projection shader
        std::vector<mplot::gl::ShaderInfo> proj2d_shader_progs;
        //! Stores the info required to load the text shader
        std::vector<mplot::gl::ShaderInfo> text_shader_progs;

        //! The colour of ambient and diffuse light sources
        sm::vec<float, 3> light_colour = { 1.0f, 1.0f, 1.0f };
        //! Strength of the ambient light
        float ambient_intensity = 1.0f;
        //! Position of a diffuse light source
        sm::vec<float, 3> diffuse_position = { 5.0f, 5.0f, 15.0f };
        //! Strength of the diffuse light source
        float diffuse_intensity = 0.0f;

        // Track how many calls to render have been made. At 1000 FPS this overflows at 10^17 seconds which is about 10^9 years.
        std::uint64_t render_counter = 0u;
        // A move counter. May be incremented in step with render_counter, or it may not (for example, it may restart)
        std::uint64_t move_counter = 0u;

        //! Compute position and rotation of coordinate arrows in the bottom left of the screen
        void positionCoordArrows()
        {
            // Find out the location of the bottom left of the screen and make the coord
            // arrows stay put there.

            // Add the depth at which the object lies.  Use forward projection to determine the
            // correct z coordinate for the inverse projection. This assumes only one object.
            sm::vec<float, 4> point =  { 0.0f, 0.0f, this->sceneview[14], 1.0f }; // sceneview[14] is 'scenetrans.z'
            sm::vec<float, 4> pp = this->projection * point;
            float coord_z = pp[2]/pp[3]; // divide by pp[3] is divide by/normalise by 'w'.

            // Construct the point for the location of the coord arrows
            sm::vec<float, 4> p0 = { this->coordArrowsOffset.x(), this->coordArrowsOffset.y(), coord_z, 1.0f };
            // Inverse project
            sm::vec<float, 3> v0;
            v0.set_from ((this->invproj * p0));
            // Translate the scene for the CoordArrows such that they sit in a single position on
            // the screen
            this->coordArrows->setSceneTranslation (v0);
            // Apply rotation to the coordArrows model
            sm::quaternion<float> svrq = this->sceneview.rotation();
            svrq.renormalize();
            this->coordArrows->setViewRotation (svrq);
        }

        // Update the coordinate axes labels
        void updateCoordLabels (const std::string& x_lbl, const std::string& y_lbl, const std::string& z_lbl)
        {
            this->coordArrows->clear();
            this->coordArrows->x_label = x_lbl;
            this->coordArrows->y_label = y_lbl;
            this->coordArrows->z_label = z_lbl;
            this->coordArrows->initAxisLabels();
            this->coordArrows->reinit();
        }

        // Update the lengths of the CoordArrows that (usually) appear in the corner of the screen
        void updateCoordLengths (const sm::vec<float, 3>& _lengths, const float _thickness = 1.0f)
        {
            this->coordArrows->lengths = _lengths;
            this->coordArrows->thickness = _thickness;
            this->coordArrows->clear();
            this->coordArrows->initAxisLabels();
            this->coordArrows->reinit();
        }

        // state defaults. All state is false by default
        constexpr sm::flags<visual_state> state_defaults()
        {
            sm::flags<visual_state> _state;
            return _state;
        }

        // State flags
        sm::flags<visual_state> state = state_defaults();

        // Options defaults.
        constexpr sm::flags<visual_options> options_defaults()
        {
            sm::flags<visual_options> _options;
            // Only with ImGui do we manually swap buffers, so this is true by default:
            _options.set (visual_options::renderSwapsBuffers);
            // For now, default to rotating about scene origin, as we ever did (Ctrl-k to change)
            _options.set (visual_options::rotateAboutSceneOrigin);
            // Also, for now, keep the Blender-like 'rotateAboutVertical' as a non-default option (Ctrl-d to change)
            _options.set (visual_options::rotateAboutVertical, false);

            return _options;
        }

        // Option flags
        sm::flags<visual_options> options = options_defaults();

        //! Returns true when the program has been flagged to end
        bool readyToFinish() const { return this->state.test (visual_state::readyToFinish); }

        //! Returns true if we are in the paused state
        bool paused() const { return this->state.test (visual_state::paused); }

        //! True if one of our added VisualModels is an instanced model
        bool haveInstanced() const { return this->state.test (visual_state::haveInstanced); }

        /*
         * User-settable projection values for the near clipping distance, the far clipping distance
         */
        float zNear = 0.001f;
        float zFar = 300.0f;

        /*
         * User settable field of view of the camera in degrees. Note that the field of view is
         * measured from the top of the field to the bottom of the field (rather than from the left
         * to the right).
         */
        float fov = 30.0f;

        //! Setter for fov
        void set_vertical_fov (const float vfov) { this->fov = vfov; }
        //! Setter for fov, if you want to specify horizontal field of view
        void set_horizontal_fov (const float hfov)
        {
            float aspect = static_cast<float>(this->window_w) / static_cast<float>(this->window_h ? this->window_h : 1);
            this->fov = hfov / aspect;
        }

        //! Time constants for the way the camera moves between a follow-me view and a
        //! drone-view. One for translation, the other for rotation.
        float trans_tc = 0.09f;
        //! Rotational time constant
        float rotn_tc = trans_tc;

        //! Which was is up in the scene? In OpenGL it's usually y, but may be changed to z in some cases
        sm::vec<float> scene_up = sm::vec<float>::uy();
        //! Which way goes to the 'right' across the screen? Usually x
        sm::vec<float> scene_right = sm::vec<float>::ux();
        //! Out of the screen?
        sm::vec<float> scene_out = sm::vec<float>::uz();

        //! Setter for visual_options::showCoordArrows
        void showCoordArrows (const bool val) { this->options.set (visual_options::showCoordArrows, val); }

        //! If true, then place the coordinate arrows at the origin of the scene, rather than offset.
        void coordArrowsInScene (const bool val) { this->options.set (visual_options::coordArrowsInScene, val); }

        //! Rotate about the nearest VisualModel?
        void rotateAboutNearest (const bool val)
        { this->options.set (mplot::visual_options::rotateAboutSceneOrigin, (val ? false : true)); }

        //! Rotate about a vertical axis in the scene?
        void rotateAboutVertical (const bool val) { this->options.set (visual_options::rotateAboutVertical, val); }

        //! Set to true to show the title text within the scene
        void showTitle (const bool val) { this->options.set (visual_options::showTitle, val); }

        //! Set true to output some user information to stdout (e.g. user requested quit)
        void userInfoStdout (const bool val) { this->options.set (visual_options::userInfoStdout, val); }

        //! You can call this with val==false to manage exactly when you call the swapBuffer() method (for ImGui programs)
        void renderSwapsBuffers (const bool val) {  this->options.set (visual_options::renderSwapsBuffers, val); }

        //! How big should the steps in scene translation be when scrolling?
        float scenetrans_stepsize = 0.02f;

        //! If you set this to true, then the mouse movements won't change scenetrans or rotation.
        void sceneLocked (const bool val) { this->state.set (visual_state::sceneLocked, val); }

        //! Show bounding boxes?
        void showBoundingBoxes (const bool val) { this->options.set (visual_options::showBoundingBoxes, val); }

        //! Highlight (with a bounding box) the VisualModel being used for rotation?
        void highlightRotationVM (const bool val) { this->options.set (visual_options::highlightRotationVM, val); }

        //! Can change this to orthographic
        perspective_type ptype = perspective_type::perspective;

        //! Orthographic screen left-bottom coordinate (you can change these to encapsulate your models)
        sm::vec<float, 2> ortho_lb = { -1.3f, -1.0f };
        //! Orthographic screen right-top coordinate
        sm::vec<float, 2> ortho_rt = { 1.3f, 1.0f };

        //! The background colour; white by default.
        std::array<float, 4> bgcolour = { 1.0f, 1.0f, 1.0f, 0.5f };

        /*
         * User can directly set bgcolour for any background colour they like, but
         * here are convenience functions:
         */

        //! Set a white background colour for the Visual scene
        void backgroundWhite() { this->bgcolour = { 1.0f, 1.0f, 1.0f, 0.5f }; }
        //! Set a black background colour for the Visual scene
        void backgroundBlack() { this->bgcolour = { 0.0f, 0.0f, 0.0f, 0.0f }; }

        //! Set sceneview and sceneview_tr back to scenetrans_default
        void reset_sceneviews_to_scenetrans_default()
        {
            this->sceneview.set_identity();
            this->sceneview.translate (this->scenetrans_default);
            this->sceneview_tr.set_identity();
            this->sceneview_tr.translate (this->scenetrans_default);
            this->d_to_rotation_centre = -this->scenetrans_default[2];
        }

        void setSceneview (const sm::mat<float, 4>& sv)
        {
            this->lastSceneview = this->sceneview;
            this->sceneview = sv;
            this->sceneview_tr.set_identity();
            this->sceneview_tr.translate (sv.translation());
        }

        //! Set the scene's x and y values at the same time.
        void setSceneTransXY (const float _x, const float _y)
        {
            this->scenetrans_default[0] = _x;
            this->scenetrans_default[1] = _y;
            this->reset_sceneviews_to_scenetrans_default();
        }
        //! Set the scene's y value. Use this to shift your scene objects left or right
        void setSceneTransX (const float _x)
        {
            this->scenetrans_default[0] = _x;
            this->reset_sceneviews_to_scenetrans_default();
        }
        //! Set the scene's y value. Use this to shift your scene objects up and down
        void setSceneTransY (const float _y)
        {
            this->scenetrans_default[1] = _y;
            this->reset_sceneviews_to_scenetrans_default();
        }
        //! Set the scene's z value. Use this to bring the 'camera' closer to your scene
        //! objects (that is, your mplot::VisualModel objects).
        void setSceneTransZ (const float _z)
        {
            if (_z > 0.0f) {
                std::cerr << "WARNING setSceneTransZ(): Normally, the default z value is negative.\n";
            }
            this->scenetrans_default[2] = _z;
            this->reset_sceneviews_to_scenetrans_default();
        }
        void setSceneTrans (float _x, float _y, float _z)
        {
            if (_z > 0.0f) {
                std::cerr << "WARNING setSceneTrans(): Normally, the default z value is negative.\n";
            }

            this->scenetrans_default[0] = _x;
            this->scenetrans_default[1] = _y;
            this->scenetrans_default[2] = _z;
            this->reset_sceneviews_to_scenetrans_default();
        }
        void setSceneTrans (const sm::vec<float, 3>& _xyz)
        {
            if (_xyz[2] > 0.0f) {
                std::cerr << "WARNING setSceneTrans(vec<>&): Normally, the default z value is negative.\n";
            }
            this->scenetrans_default = _xyz;
            this->reset_sceneviews_to_scenetrans_default();
        }

        void setSceneRotation (const sm::quaternion<float>& _rotn)
        {
            this->rotation_default = _rotn;
            this->sceneview.rotate (_rotn);
        }

        // What is the scene view's current rotation quaternion?
        sm::quaternion<float> getSceneRotation() const { return this->sceneview.rotation(); }
        // What is the scene view's current translation?
        sm::vec<float> getSceneTranslation() const { return this->sceneview.translation(); }

        // API for client code to set a 'film direction' event. This may change the sceneview
        // immediately, or start a timed sequence of changes to animate the sceneview.
        void setCurrentDirectionEvent (const mplot::direction_data& dirn)
        {
            constexpr bool debug_dirn_event = false;

            // timed translation/rotation/transform are all 'viewAutomations'
            this->state.set (visual_state::viewAutomation);
            this->currentAutoSceneviewChange = dirn;
            this->currentAutoSceneviewChange.start_frame = this->render_counter;
            if constexpr (debug_dirn_event) {
                std::cout << __func__ << " event id " << this->currentAutoSceneviewChange.id
                          << " starting at frame " << this->currentAutoSceneviewChange.start_frame << std::endl;
            }
            // Auto translation behaves like a mouse-press then mouse-drag; computing a delta from a savedSceneview
            this->savedSceneview = this->sceneview;
            this->savedSceneview_tr = this->sceneview_tr;
            this->scenetrans_delta.zero();
            this->rotation_delta.reset();
            // If it's a rotation event, also need a rotation centre
            if (dirn.event == direction_event::timed_rotation) {
                this->find_rotation_centre();
            } else if (dirn.event == direction_event::timed_transform) {
                // Find the start and end translation
                this->currentAutoSceneviewChange.translation = dirn.sceneview.translation() - this->sceneview.translation();
                // Find the start and end rotation
                this->currentAutoSceneviewChange.rotation_start = this->sceneview.rotation(); // rotation at start
                this->currentAutoSceneviewChange.rotation_start.renormalize();
                this->currentAutoSceneviewChange.rotation = dirn.sceneview.rotation(); // rotation at end
                this->currentAutoSceneviewChange.rotation.renormalize();
                this->find_rotation_centre();
            } else if (dirn.event == direction_event::sceneview) {
                std::cout << "Set a sceneview event for current direction event" << std::endl;
                // An immediate sceneview event is just a timed_transform with the time for the transform set to 1
                this->currentAutoSceneviewChange.transform_time_frames = 2u;
                this->currentAutoSceneviewChange.min_jerk = false;
                // Find the start and end translation
                this->currentAutoSceneviewChange.translation = dirn.sceneview.translation() - this->sceneview.translation();
                // Find the start and end rotation
                this->currentAutoSceneviewChange.rotation_start = this->sceneview.rotation(); // rotation at start
                this->currentAutoSceneviewChange.rotation_start.renormalize();
                this->currentAutoSceneviewChange.rotation = dirn.sceneview.rotation(); // rotation at end
                this->currentAutoSceneviewChange.rotation.renormalize();
                this->find_rotation_centre();
            } else if (dirn.event == direction_event::timed_orbit) {
                // Might need this:
                this->currentAutoSceneviewChange.translation = dirn.sceneview.translation() - this->sceneview.translation();
            }
        }

        void lightingEffects (const bool effects_on = true)
        {
            this->ambient_intensity = effects_on ? 0.4f : 1.0f;
            this->diffuse_intensity = effects_on ? 0.6f : 0.0f;
        }

        //! Save all the VisualModels in this Visual out to a GLTF format file
        virtual void savegltf (const std::string& gltf_file)
        {
            std::ofstream fout (gltf_file, std::ios::out|std::ios::trunc);
            if (!fout.is_open()) { throw std::runtime_error ("Visual::savegltf(): Failed to open file for writing"); }
            fout << "{\n  \"scenes\" : [ { \"nodes\" : [ ";
            for (std::size_t vmi = 0u; vmi < this->vm.size(); ++vmi) {
                fout << vmi << (vmi < this->vm.size()-1 ? ", " : "");
            }
            fout << " ] } ],\n";

            fout << "  \"nodes\" : [\n";
            // for loop over VisualModels "mesh" : 0, etc
            for (std::size_t vmi = 0u; vmi < this->vm.size(); ++vmi) {
                fout << "    { \"mesh\" : " << vmi
                     << ", \"translation\" : " << this->vm[vmi]->translation_str()
                     << (vmi < this->vm.size()-1 ? " },\n" : " }\n");
            }
            fout << "  ],\n";

            fout << "  \"meshes\" : [\n";
            // for each VisualModel:
            for (std::size_t vmi = 0u; vmi < this->vm.size(); ++vmi) {
                fout << "    { ";
                if (!this->vm[vmi]->name.empty()) {
                    fout << "\"name\" : \"" << this->vm[vmi]->name << "\", ";
                }
                fout << "\"primitives\" : [ { \"attributes\" : { \"POSITION\" : " << 1+vmi*4
                     << ", \"COLOR_0\" : " << 2+vmi*4
                     << ", \"NORMAL\" : " << 3+vmi*4 << " }, \"indices\" : " << vmi*4 << ", \"material\": 0 } ] }"
                     << (vmi < this->vm.size()-1 ? ",\n" : "\n");
            }
            fout << "  ],\n";

            fout << "  \"buffers\" : [\n";
            for (std::size_t vmi = 0u; vmi < this->vm.size(); ++vmi) {
                // indices
                fout << "    {\"uri\" : \"data:application/octet-stream;base64," << this->vm[vmi]->indices_base64() << "\", "
                     << "\"byteLength\" : " << this->vm[vmi]->indices_bytes() << "},\n";
                // pos
                fout << "    {\"uri\" : \"data:application/octet-stream;base64," << this->vm[vmi]->vpos_base64() << "\", "
                     << "\"byteLength\" : " << this->vm[vmi]->vpos_bytes() << "},\n";
                // col
                fout << "    {\"uri\" : \"data:application/octet-stream;base64," << this->vm[vmi]->vcol_base64() << "\", "
                     << "\"byteLength\" : " << this->vm[vmi]->vcol_bytes() << "},\n";
                // norm
                fout << "    {\"uri\" : \"data:application/octet-stream;base64," << this->vm[vmi]->vnorm_base64() << "\", "
                     << "\"byteLength\" : " << this->vm[vmi]->vnorm_bytes() << "}";
                fout << (vmi < this->vm.size()-1 ? ",\n" : "\n");
            }
            fout << "  ],\n";

            fout << "  \"bufferViews\" : [\n";
            for (std::size_t vmi = 0u; vmi < this->vm.size(); ++vmi) {
                // indices
                fout << "    { ";
                fout << "\"buffer\" : " << vmi*4 << ", ";
                fout << "\"byteOffset\" : 0, ";
                fout << "\"byteLength\" : " << this->vm[vmi]->indices_bytes() << ", ";
                fout << "\"target\" : 34963 ";
                fout << " },\n";
                // vpos
                fout << "    { ";
                fout << "\"buffer\" : " << 1+vmi*4 << ", ";
                fout << "\"byteOffset\" : 0, ";
                fout << "\"byteLength\" : " << this->vm[vmi]->vpos_bytes() << ", ";
                fout << "\"target\" : 34962 ";
                fout << " },\n";
                // vcol
                fout << "    { ";
                fout << "\"buffer\" : " << 2+vmi*4 << ", ";
                fout << "\"byteOffset\" : 0, ";
                fout << "\"byteLength\" : " << this->vm[vmi]->vcol_bytes() << ", ";
                fout << "\"target\" : 34962 ";
                fout << " },\n";
                // vnorm
                fout << "    { ";
                fout << "\"buffer\" : " << 3+vmi*4 << ", ";
                fout << "\"byteOffset\" : 0, ";
                fout << "\"byteLength\" : " << this->vm[vmi]->vnorm_bytes() << ", ";
                fout << "\"target\" : 34962 ";
                fout << " }";
                fout << (vmi < this->vm.size()-1 ? ",\n" : "\n");
            }
            fout << "  ],\n";

            fout << "  \"accessors\" : [\n";
            for (std::size_t vmi = 0u; vmi < this->vm.size(); ++vmi) {
                this->vm[vmi]->computeVertexMaxMins();
                // indices
                fout << "    { ";
                fout << "\"bufferView\" : " << vmi*4 << ", ";
                fout << "\"byteOffset\" : 0, ";
                // 5123 unsigned short, 5121 unsigned byte, 5125 unsigned int, 5126 float:
                fout << "\"componentType\" : 5125, ";
                fout << "\"type\" : \"SCALAR\", ";
                fout << "\"count\" : " << this->vm[vmi]->indices_size();
                fout << "},\n";
                // vpos
                fout << "    { ";
                fout << "\"bufferView\" : " << 1+vmi*4 << ", ";
                fout << "\"byteOffset\" : 0, ";
                fout << "\"componentType\" : 5126, ";
                fout << "\"type\" : \"VEC3\", ";
                fout << "\"count\" : " << this->vm[vmi]->vpos_size()/3;
                // vertex position requires max/min to be specified in the gltf format
                fout << ", \"max\" : " << this->vm[vmi]->vpos_max() << ", ";
                fout << "\"min\" : " << this->vm[vmi]->vpos_min();
                fout << " },\n";
                // vcol
                fout << "    { ";
                fout << "\"bufferView\" : " << 2+vmi*4 << ", ";
                fout << "\"byteOffset\" : 0, ";
                fout << "\"componentType\" : 5126, ";
                fout << "\"type\" : \"VEC3\", ";
                fout << "\"count\" : " << this->vm[vmi]->vcol_size()/3;
                fout << "},\n";
                // vnorm
                fout << "    { ";
                fout << "\"bufferView\" : " << 3+vmi*4 << ", ";
                fout << "\"byteOffset\" : 0, ";
                fout << "\"componentType\" : 5126, ";
                fout << "\"type\" : \"VEC3\", ";
                fout << "\"count\" : " << this->vm[vmi]->vnorm_size()/3;
                fout << "}";
                fout << (vmi < this->vm.size()-1 ? ",\n" : "\n");
            }
            fout << "  ],\n";

            // Default material is single sided, so make it double sided
            fout << "  \"materials\" : [ { \"doubleSided\" : true } ],\n";

            fout << "  \"asset\" : {\n"
                 << "    \"generator\" : \"https://github.com/sebsjames/mathplot: mplot::Visual::savegltf() (ver "
                 << mplot::version_string() << ")\",\n"
                 << "    \"version\" : \"2.0\"\n" // This version is the *glTF* version.
                 << "  }\n";
            fout << "}\n";
            fout.close();
        }

        void set_winsize (const std::int32_t _w, const std::int32_t _h) { this->window_w = _w; this->window_h = _h; }

        // Accessing std::vector<std::unique_ptr<mplot::VisualModel<glver>>> vm; from external code
        std::vector<std::unique_ptr<mplot::VisualModel<glver>>>::const_iterator next_vm_accessor;
        void init_vm_accessor() { this->next_vm_accessor = this->vm.begin(); }
        mplot::VisualModel<glver>* get_next_vm_accessor()
        {
            mplot::VisualModel<glver>* cvm = nullptr;
            if (this->next_vm_accessor != this->vm.end()) {
                cvm = (*this->next_vm_accessor).get();
                this->next_vm_accessor++;
            }
            return cvm;
        }

        std::uint32_t get_id() const { return this->visual_id; }

    protected:

        //! Set up a perspective projection based on window width and height. Not public.
        void setPerspective()
        {
            // Calculate aspect ratio
            float aspect = static_cast<float>(this->window_w) / static_cast<float>(this->window_h ? this->window_h : 1);
            // Set perspective projection
            this->projection = sm::mat<float, 4>::perspective (this->fov, aspect, this->zNear, this->zFar);
            // Compute the inverse projection matrix
            this->invproj = this->projection.inverse();
        }

        /*!
         * Set an orthographic projection. This is not a public function. To choose orthographic
         * projection for your Visual, write something like:
         *
         * \code
         *   mplot::Visual<> v(width, height, title);
         *   v.ptype = mplot::perspective_type::orthographic;
         * \endcode
         */
        void setOrthographic()
        {
            this->projection = sm::mat<float, 4>::orthographic (this->ortho_lb, this->ortho_rt, this->zNear, this->zFar);
            this->invproj = this->projection.inverse();
        }

        // Rotate about the point this->rotation_centre. Subroutine for computeSceneview.
        void computeSceneview_about_rotation_centre()
        {
            sm::mat<float, 4> sv_tr;
            sm::mat<float, 4> sv_rot;
            sv_tr.translate (this->scenetrans_delta);
            // A rotation delta in world frame about the 'screen centre'
            sv_rot.translate (this->rotation_centre);
            sv_rot.rotate (this->rotation_delta);
            sv_rot.translate (-this->rotation_centre);

            this->sceneview = sv_tr * sv_rot * this->savedSceneview;
            this->sceneview_tr = sv_tr * this->savedSceneview_tr;
        }

        // Get a camera movement that moves us nearer to target.
        template<typename T>
        sm::vec<T, 3> get_cam_movement (sm::mat<T, 4>& current, const sm::mat<T, 4>& target,
                                        sm::vec<T, 3>& vel, const T tc) const
        {
            const sm::vec<T, 3> delta = target.translation() - current.translation();
            const sm::vec<T, 3> force = delta - (vel * T{2});
            sm::vec<T, 3> pos_shift = vel * tc;
            vel += force * tc;
            return pos_shift;
        }

        template<typename T>
        sm::quaternion<T> get_cam_rotation (const sm::quaternion<float>& r_cur0, const sm::mat<T, 4>& target,
                                            T& rvel, const T tc) const
        {
            sm::mat<T, 4> target0 = target;
            target0.translate (-target.translation());
            sm::quaternion<T> r_targ0 = target0.rotation();
            r_targ0.renormalize();

            sm::quaternion<T> r_sz = r_targ0 * r_cur0.inverse();
            sm::vec<T, 4> aa = r_sz.axis_angle();
            T delta = aa[3]; // The angle subtended by the rotation
            T force = delta - (rvel * T{2});
            // rvel is radpersec delta/tc
            T prop = rvel * tc;
            rvel += force * tc;
            if (r_cur0.checkunit() == false) { throw std::runtime_error ("r_cur0 is not a unit quaternion (could renormalize here)"); }
            sm::quaternion<T> newpos = r_cur0.slerp (r_targ0, prop);
            return newpos; // rather than prop, as in get_cam_movement
        }

        void switch_view_follows_mode()
        {
            // Relevant only if there is a followedVM
            if (this->followedVM == nullptr) { return; }

            if (this->options.test (visual_options::viewFollowsVMBehind) == true
                && this->options.test (visual_options::viewFollowsVMTranslations) == false) {
                this->options.reset (visual_options::viewFollowsVMBehind);
                this->options.set (visual_options::viewFollowsVMTranslations);
                this->state.set (visual_state::viewFollowsModeChanged);
                std::cout << "sceneview follows agent movements (overview)\n";
            } else { // this->options.test (mplot::visual_options::viewFollowsVMTranslations) == true
                this->options.set (visual_options::viewFollowsVMBehind);
                this->state.set (visual_state::viewFollowsModeChanged);
                this->options.reset (visual_options::viewFollowsVMTranslations);
                std::cout << "sceneview follows behind agent (follower view)\n";
            }
        }

        // Compile-time function to create a rotate-about-y transform
        static constexpr sm::mat<float, 4> rotate_about_y()
        {
            sm::mat<float, 4> r;
            r.rotate (sm::vec<>::uy(), sm::mathconst<float>::pi);
            return r;
        }

        // Hold an offset translation and rotation for the follow-me camera
        static constexpr sm::vec<float> folcam_offset_tr_default = {0, 0.01f, -0.06f};
        sm::vec<float> folcam_offset_tr = folcam_offset_tr_default;
        sm::quaternion<float> folcam_offset_rot;

        sm::mat<float, 4> update_viewmatrix_towards_target (const sm::mat<float, 4>& target)
        {
            sm::mat<float, 4> fol_cur;

            // Compute folcam_viewmatrix from sceneview (it's the inverse, along with a rotation)
            constexpr sm::mat<float, 4> rotn_y = rotate_about_y();
            sm::mat<float, 4> folcam_viewmatrix = this->sceneview.inverse() * rotn_y;

            const sm::vec<float> folcam_vm_trans = folcam_viewmatrix.translation();

            fol_cur.translate (folcam_vm_trans); // encode just the location of the following camera

            // The current rotation of the scene view
            folcam_viewmatrix.translate (-folcam_vm_trans);
            sm::quaternion<float> r_cur0 = folcam_viewmatrix.rotation();
            r_cur0.renormalize();

            // get_cam_movement computes the positional shift
            sm::vec<float> pos_shift = this->get_cam_movement<float> (fol_cur, target, this->followedVM_vel, this->trans_tc);
            // get_cam_rotation computes the rotation for the next camera position
            sm::quaternion<float> cam_rotn = this->get_cam_rotation<float> (r_cur0, target, this->followedVM_rvel, this->rotn_tc);

            // set the translation/rotation into fol_cur
            fol_cur.pretranslate (pos_shift);
            fol_cur.rotate (cam_rotn);

            // Distance to rotation centre should be the distance to the followedVM
            this->d_to_rotation_centre = folcam_offset_tr.length();

            // fol_cur now contains the new position and orientation for the following camera
            return fol_cur;
        }

        // To zoom down to, and follow behind the agent
        sm::mat<float, 4> update_folcam_viewmatrix()
        {
            if (this->followedVM == nullptr) { return sm::mat<float, 4>::identity(); }
            // Target view from the followedVM
            sm::mat<float, 4> rmat;
            rmat.rotate (this->folcam_offset_rot);
            // To smooth this, I need to keep a time-average of followedVM->getViewMatrix()...
            sm::mat<float, 4> fol_targ = this->followedVM->getViewMatrix() * rmat;
            fol_targ.translate (this->folcam_offset_tr);
            // FIXME: I may want to update the lastSceneview matrix as fol_targ moves.
            return update_viewmatrix_towards_target (fol_targ);
        }

        // To zoom back to the drone view
        sm::mat<float, 4> update_overcam_viewmatrix()
        {
            if (this->followedVM == nullptr) { return sm::mat<float, 4>::identity(); }
            // fol_targ will be the inverse of the lastSceneview
            constexpr sm::mat<float, 4> rotn_y = rotate_about_y();
            sm::mat<float, 4> targ = this->lastSceneview.inverse() * rotn_y;
            return update_viewmatrix_towards_target (targ);
        }

        // A follow-me camera view
        void computeSceneview_for_follower()
        {
            sm::mat<float, 4> folcam_viewmatrix = this->update_folcam_viewmatrix();
            constexpr sm::mat<float, 4> rotn_y = rotate_about_y();
            this->sceneview = rotn_y * folcam_viewmatrix.inverse();
            this->savedSceneview = this->sceneview;
        }

        // When we're not close to the 'overview cam' location (which is the last place you were
        // before you went into viewFollowsVMBehind mode) then we need this function to get there
        void computeSceneview_for_overcam()
        {
            sm::mat<float, 4> overcam_viewmatrix = this->update_overcam_viewmatrix(); // target is lastSceneview
            constexpr sm::mat<float, 4> rotn_y = rotate_about_y();
            this->sceneview = rotn_y * overcam_viewmatrix.inverse();
            this->savedSceneview = this->sceneview;
        }

        /*
         * For the time t, compute the location x of a 1D minimum-jerk trajectory from 0 to xf
         * carried out in time tf.
         */
        template<typename F>
        F compute_min_jerk (const F tf, const F xf, const F t) const noexcept
        {
            sm::mat<F, 3> A = {
                std::pow (tf, F{3}), F{3} * std::pow (tf, F{2}), F{6}  * tf,
                std::pow (tf, F{4}), F{4} * std::pow (tf, F{3}), F{2}  * std::pow (tf, F{2}),
                std::pow (tf, F{5}), F{5} * std::pow (tf, F{4}), F{20} * std::pow (tf, F{3})
            };
            sm::vec<F, 3> B = { xf, F{0}, F{0} };
            sm::vec<F, 3> X = A.inverse() * B; // X are the min. jerk coefficients

            F x = X[0] * t * t * t + X[1] * std::pow (t, F{4}) + X[2] * std::pow (t, F{5});
            return x;
        }

        // Helper function. Is the automatic sceneview change complete after since_frames?
        bool autoSceneviewComplete (const std::uint32_t since_frames)
        {
            return (since_frames > this->currentAutoSceneviewChange.transform_time_frames);
        }

        // Choreographed sceneview changes (direction_event)
        void computeSceneview_for_automation()
        {
            // A constexpr rotation matrix, used below
            constexpr sm::mat<float, 4> rotn_y = rotate_about_y();

            // We're in an 'automation mode'. use this->currentAutoSceneviewChange
            std::uint32_t since_frames = this->render_counter - this->currentAutoSceneviewChange.start_frame;

            // Determine how far, in time, as a proportion, we are into the automated sceneview movement
            float propn = static_cast<float>(since_frames) / static_cast<float>(this->currentAutoSceneviewChange.transform_time_frames);

            if (this->currentAutoSceneviewChange.event == direction_event::timed_translation) {

                if (this->autoSceneviewComplete (since_frames)) {
                    // Completed movement time
                    this->scenetrans_delta = this->currentAutoSceneviewChange.translation;
                    this->computeSceneview_about_rotation_centre();
                    this->state.set (visual_state::viewAutomation, false);
                    this->scenetrans_delta.zero();
                    this->savedSceneview = this->sceneview;
                    this->savedSceneview_tr = this->sceneview_tr;
                    if (this->followedVM != nullptr) { this->followedLastViewMatrix = this->followedVM->getViewMatrix(); }
                } else {
                    // Translate by an increment
                    if (this->currentAutoSceneviewChange.min_jerk) {
                        propn = this->compute_min_jerk (static_cast<float>(this->currentAutoSceneviewChange.transform_time_frames), 1.0f,
                                                        static_cast<float>(since_frames));
                    }

                    this->scenetrans_delta = propn * this->currentAutoSceneviewChange.translation;
                    this->computeSceneview_about_rotation_centre();
                }
                if (this->followedVM != nullptr) {
                    sm::mat<float, 4> sv_viewmatrix = this->sceneview.inverse() * rotn_y;
                    sm::mat<float, 4> fvm = this->followedVM->getViewMatrix();
                    this->d_to_rotation_centre = (fvm.translation() - sv_viewmatrix.translation()).length();
                }

            } else if (this->currentAutoSceneviewChange.event == direction_event::timed_rotation) {

                sm::vec<float> mod_up = this->savedSceneview.rotation() * this->scene_up;
                if (this->autoSceneviewComplete (since_frames)) {
                    // Apply full rotation
                    sm::quaternion<float> r1 (mod_up, -this->currentAutoSceneviewChange.about_vert_angle);
                    sm::quaternion<float> r2 (this->scene_right, -this->currentAutoSceneviewChange.tilt_angle);
                    this->rotation_delta = r2 * r1;
                    this->computeSceneview_about_rotation_centre();
                    this->state.set (visual_state::viewAutomation, false);
                    this->rotation_delta.reset();
                    this->savedSceneview = this->sceneview;
                    this->savedSceneview_tr = this->sceneview_tr;
                    if (this->followedVM != nullptr) { this->followedLastViewMatrix = this->followedVM->getViewMatrix(); }
                } else { // Rotate by an increment
                    if (this->currentAutoSceneviewChange.min_jerk) {
                        propn = this->compute_min_jerk (static_cast<float>(this->currentAutoSceneviewChange.transform_time_frames), 1.0f,
                                                        static_cast<float>(since_frames));
                    }
                    // make rotation_delta from this->currentAutoSceneviewChange.rotation and apply
                    sm::quaternion<float> r1 (mod_up, -propn * this->currentAutoSceneviewChange.about_vert_angle);
                    sm::quaternion<float> r2 (this->scene_right, -propn * this->currentAutoSceneviewChange.tilt_angle);
                    this->rotation_delta = r2 * r1;
                    this->computeSceneview_about_rotation_centre();
                }
                if (this->followedVM != nullptr) {
                    sm::mat<float, 4> sv_viewmatrix = this->sceneview.inverse() * rotn_y;
                    sm::mat<float, 4> fvm = this->followedVM->getViewMatrix();
                    this->d_to_rotation_centre = (fvm.translation() - sv_viewmatrix.translation()).length();
                }

            } else if (this->currentAutoSceneviewChange.event == direction_event::timed_transform
                       || this->currentAutoSceneviewChange.event == direction_event::sceneview) { // translation and rotation

                if (this->autoSceneviewComplete (since_frames)) { // transform is done
                    this->state.set (visual_state::viewAutomation, false);
                    this->scenetrans_delta.zero();
                    this->rotation_delta.reset();
                    this->savedSceneview = this->sceneview;
                    this->savedSceneview_tr = this->sceneview_tr;
                    // Update followedLastViewMatrix now!
                    if (this->followedVM != nullptr) { this->followedLastViewMatrix = this->followedVM->getViewMatrix(); }
                } else { // transform, incrementally
                    if (this->currentAutoSceneviewChange.min_jerk) {
                        // propn is time from 0 to 1 of our movement. Compute min-jerk movement x(t)
                        // = a3 t^3 + a4 t^4 + a5 t^5.  We always pass xf = 1, as we are getting a
                        // proportion of the trajectory, regardless of the length (or amount of
                        // rotation) of our sceneview move.)
                        propn = this->compute_min_jerk (static_cast<float>(this->currentAutoSceneviewChange.transform_time_frames), 1.0f,
                                                        static_cast<float>(since_frames));
                    }
                    // Ensure propn is in range before calling slerp
                    if (propn > 1.0f) { propn = 1.0f; }
                    if (propn < 0.0f) { propn = 0.0f; }
                    // Translate by an increment
                    this->scenetrans_delta = propn * this->currentAutoSceneviewChange.translation;
                    // Rotate...
                    sm::quaternion<float> slerped = this->currentAutoSceneviewChange.rotation_start.slerp (currentAutoSceneviewChange.rotation, propn);
                    {
                        // Here's how to check the error information in the returned quaternion.
                        if (slerped.w == std::numeric_limits<float>::max()) { std::cout << __func__ << ": propn = " << propn << " was out of the range [0, 1]\n"; }
                        if (slerped.x == std::numeric_limits<float>::max()) { std::cout << __func__ << ": rotation_start was not normalized\n"; }
                        if (slerped.y == std::numeric_limits<float>::max()) { std::cout << __func__ << ": rotation was not normalized\n"; }

                        // rather than computeSceneview_about_rotation_centre, we compute
                        // translation and rotation right here (this could probably be consolidated to use fewer mats)
                        sm::mat<float, 4> sv_tr;
                        sm::mat<float, 4> sv_rot;
                        sv_tr.translate (this->savedSceneview.translation());
                        sv_tr.translate (this->scenetrans_delta);
                        sv_rot.rotate (slerped);
                        this->sceneview = sv_tr * sv_rot;
                        this->sceneview_tr = sv_tr;
                    }
                }
                if (this->followedVM != nullptr) {
                    sm::mat<float, 4> sv_viewmatrix = this->sceneview.inverse() * rotn_y;
                    sm::mat<float, 4> fvm = this->followedVM->getViewMatrix();
                    this->d_to_rotation_centre = (fvm.translation() - sv_viewmatrix.translation()).length();
                } else {
                    this->d_to_rotation_centre = std::abs (this->sceneview[14]);
                }

            } else if (this->currentAutoSceneviewChange.event == direction_event::timed_orbit) {
                if (this->autoSceneviewComplete (since_frames)) { // transform is done
                    this->state.set (visual_state::viewAutomation, false);
                    this->scenetrans_delta.zero();
                    this->rotation_delta.reset();
                    this->savedSceneview = this->sceneview;
                    this->savedSceneview_tr = this->sceneview_tr;
                    // Update followedLastViewMatrix now!
                    if (this->followedVM != nullptr) { this->followedLastViewMatrix = this->followedVM->getViewMatrix(); }
                } else { // orbit, incrementally
                    if (this->currentAutoSceneviewChange.min_jerk) {
                        propn = this->compute_min_jerk (static_cast<float>(this->currentAutoSceneviewChange.transform_time_frames), 1.0f,
                                                        static_cast<float>(since_frames));
                    }
                    sm::mat<float, 4> orb_trans;
                    orb_trans.translate (this->currentAutoSceneviewChange.orbit_centre);
                    sm::mat<float, 4> orb_trans_back;
                    orb_trans_back.translate (-this->currentAutoSceneviewChange.orbit_centre);
                    sm::mat<float, 4> orb_rot;
                    orb_rot.rotate (this->currentAutoSceneviewChange.orbit_axis, propn * this->currentAutoSceneviewChange.orbit_angle);
                    this->sceneview = this->savedSceneview * orb_trans * orb_rot * orb_trans_back;
                }
            } else {
                std::cout << "Unknown direction_event\n";
                this->state.set (visual_state::viewAutomation, false);
            }
        }

        // This is called every time render() is called
        void computeSceneview()
        {
            if (this->options.test (visual_options::viewFollowsVMBehind) && this->followedVM != nullptr) {

                if (this->state.test (visual_state::viewFollowsModeChanged)) {
                    // Record sceneview now
                    this->lastSceneview = this->sceneview;
                    this->lastSceneview_tr = this->sceneview_tr;
                    // THEN, we can update lastSceneview whilst also updating agent location
                    // then zoom back to lastSceneview as the target.
                    this->state.reset (visual_state::viewFollowsModeChanged);
                }

                // Use scenetrans_delta to shift the view with the scrollwheel
                this->folcam_offset_tr += this->scenetrans_delta;
                this->scenetrans_delta.zero();
                if (this->state.test (visual_state::scrolling)) {
                    this->state.reset (visual_state::scrolling);
                    // scrolling during a view automation should cancel the view automation
                    this->state.reset (visual_state::viewAutomation);
                }
                this->computeSceneview_for_follower();

            } else {

                // A constexpr rotation matrix, used below
                constexpr sm::mat<float, 4> rotn_y = rotate_about_y();

                if (this->state.test (visual_state::viewFollowsModeChanged)) {
                    // Changed back to normal, non-follower mode. Zoom back, so set viewTransition
                    this->state.set (visual_state::viewTransition);
                    this->state.reset (visual_state::viewFollowsModeChanged);
                }

                // if sceneview is not close to lastSceneview and we have no commanded rotations
                // (scenetrans_delta and rotation_delta are 0) then we make changes to return to
                // lastSceneview:
                if (this->state.test (visual_state::viewTransition)
                    && this->followedVM != nullptr
                    && std::abs(this->scenetrans_delta.sum()) == 0.0f
                    && this->rotation_delta.is_zero_rotation() == true) {

                    // zoom towards lastSceneview:
                    this->computeSceneview_for_overcam();
                    // Update d_to_rotation_centre with distance to followedVM
                    // scene view in world frame
                    sm::mat<float, 4> sv_viewmatrix = this->sceneview.inverse() * rotn_y;
                    // followed vm
                    sm::mat<float, 4> fvm = this->followedVM->getViewMatrix();

                    this->d_to_rotation_centre = (fvm.translation() - sv_viewmatrix.translation()).length();

                    // Did we get there? If so, set viewTransition false
                    if ((this->sceneview.translation() - this->lastSceneview.translation()).length() < 0.001f) {
                        this->state.reset (visual_state::viewTransition);
                    }
                } else if (this->state.test (visual_state::viewAutomation)) {

                    this->computeSceneview_for_automation();

                } else {

                    if (std::abs(this->scenetrans_delta.sum()) > 0.0f || this->rotation_delta.is_zero_rotation() == false) {
                        // Calculate model view transformation - transforming from "model space" to "worldspace".
                        this->computeSceneview_about_rotation_centre();
                        // As we had a commanded movement, cancel the viewTransition and any viewAutomation
                        this->state.reset (visual_state::viewTransition);
                        this->state.reset (visual_state::viewAutomation);
                    } // else don't change sceneview

                    if (this->state.test (visual_state::scrolling)) {
                        this->scenetrans_delta.zero();
                        this->state.reset (visual_state::scrolling);
                    }
                }

                if (this->options.test (visual_options::viewFollowsVMTranslations)
                    && this->followedVM != nullptr
                    && this->state.test (visual_state::viewAutomation) == false
                    && this->followedLastViewMatrix != this->followedVM->getViewMatrix()) {

                    // Move camera the difference between followedLastViewMatrix and
                    // followedVM->getViewMatrix() in the screen frame of reference.
                    sm::vec<float> fol_screenframe = (this->sceneview * followedLastViewMatrix.translation()
                                                      - this->sceneview * followedVM->getViewMatrix().translation()).less_one_dim();

                    this->sceneview.pretranslate (fol_screenframe);
                    this->sceneview_tr.pretranslate (fol_screenframe);
                    this->savedSceneview.pretranslate (fol_screenframe);
                    this->savedSceneview_tr.pretranslate (fol_screenframe);
                    this->lastSceneview.pretranslate (fol_screenframe);
                    this->lastSceneview_tr.pretranslate (fol_screenframe);

                    this->followedLastViewMatrix = this->followedVM->getViewMatrix();
                }
            }
        }

        //! A vector of pointers to all the mplot::VisualModels (HexGridVisual,
        //! ScatterVisual, etc) which are going to be rendered in the scene.
        std::vector<std::unique_ptr<mplot::VisualModel<glver>>> vm;

        //! If the view should follow a model (options viewFollowsVMTranslations and ...Rotations), this is the one.
        mplot::VisualModel<glver>* followedVM = nullptr;

        //! Holds the current velocy of the followedVM follower
        sm::vec<float> followedVM_vel = {};
        //! Current rotational speed (how fast we slerp)
        float followedVM_rvel = 0.0f;

        //! Holds the viewmatrix of the followedVM the last time we called render
        sm::mat<float, 4> followedLastViewMatrix;

        // Read-from-json code that is called from init_gl in all implementations:
        void read_scenetrans_from_json()
        {
            // If possible, read in scenetrans and rotation state from a special config file
            try {
                nlohmann::json vconf;
                std::ifstream fi;
                fi.open ("/tmp/Visual.json", std::ios::in);
                fi >> vconf;
                this->scenetrans_default[0] = vconf.contains("scenetrans_x") ? vconf["scenetrans_x"].get<float>() : this->scenetrans_default[0];
                this->scenetrans_default[1] = vconf.contains("scenetrans_y") ? vconf["scenetrans_y"].get<float>() : this->scenetrans_default[1];
                this->scenetrans_default[2] = vconf.contains("scenetrans_z") ? vconf["scenetrans_z"].get<float>() : this->scenetrans_default[2];

                this->rotation_default.w = vconf.contains("scenerotn_w") ? vconf["scenerotn_w"].get<float>() : this->rotation_default.w;
                this->rotation_default.x = vconf.contains("scenerotn_x") ? vconf["scenerotn_x"].get<float>() : this->rotation_default.x;
                this->rotation_default.y = vconf.contains("scenerotn_y") ? vconf["scenerotn_y"].get<float>() : this->rotation_default.y;
                this->rotation_default.z = vconf.contains("scenerotn_z") ? vconf["scenerotn_z"].get<float>() : this->rotation_default.z;

                this->sceneview.set_identity();
                this->sceneview.translate (this->scenetrans_default);
                this->sceneview.rotate (this->rotation_default);
                this->sceneview_tr.set_identity();
                this->sceneview_tr.translate (this->scenetrans_default);
                this->scenetrans_delta.zero();
                this->rotation_delta.reset();

            } catch (...) {
                // No problem if we couldn't read /tmp/Visual.json
            }
        }

        //! The window (and OpenGL context) for this Visual
        mplot::win_t* window = nullptr;

        //! Each window has an ID number, which is passed to the owned VisualModels
        std::uint32_t visual_id = std::numeric_limits<std::uint32_t>::max();

        //! Current window width
        std::int32_t window_w = 640;
        //! Current window height
        std::int32_t window_h = 480;
        //! Window viewport scaling
        float window_scale_w = 1.0f;
        float window_scale_h = 1.0f;

        //! The title for the Visual. Used in window title and if saving out 3D model or png image.
        std::string title = "mathplot";

        //! The user's 'selected visual model'. For model specific changes to alpha and possibly colour
        std::uint32_t selectedVisualModel = 0u;

        //! A little model of the coordinate axes.
        std::unique_ptr<mplot::CoordArrows<glver>> coordArrows;

        //! Position coordinate arrows on screen. Configurable at mplot::Visual construction.
        sm::vec<float, 2> coordArrowsOffset = { -0.8f, -0.8f };

        /*
         * Variables to manage projection and rotation of the scene
         */

        //! Current cursor position
        sm::vec<float,2> cursorpos = {};

        //! The default z position for VisualModels should be 'away from the screen' (negative) so we can see them!
        constexpr static float zDefault = -5.0f;

        //! A delta scene translations
        sm::vec<float, 3> scenetrans_delta = {};

        //! Default for scene translation. This is a scene position that can be reverted to, to
        //! 'reset the view'. This is copied into sceneview when user presses Ctrl-a.
        sm::vec<float, 3> scenetrans_default = { 0.0f, 0.0f, zDefault };

        //! The world depth at which text objects should be rendered
        float text_z = -1.0f;

        //! Screen coordinates of the position of the last mouse press
        sm::vec<float, 2> mousePressPosition = {};

        //! Add additional rotation to the scene
        sm::quaternion<float> rotation_delta;

        //! The default rotation of the scene, to reconstruct the default sceneview matrix/reset rotation.
        sm::quaternion<float> rotation_default;

        //! A coordinate in the scene about which to perform a mouse-driven rotation. May be set to
        //! the centre of the closest VisualModel object.
        sm::vec<float, 3> rotation_centre = {};

        // Distance to the 'rotation centre'. Used to scale the effect of the scroll wheel
        float d_to_rotation_centre = -zDefault;

        //! The projection matrix is a member of this class. Value is set during setPerspective() or setOrthographic()
        sm::mat<float, 4> projection;

        //! The inverse of the projection. Value is set during setPerspective() or setOrthographic()
        sm::mat<float, 4> invproj;

        //! The sceneview matrix, which changes as the user moves the view with mouse
        //! movements. Initialized in VisualOwnable constructor.
        sm::mat<float, 4> sceneview;
        //! The non-rotating sceneview matrix, updated only from mouse translations (avoiding rotations)
        sm::mat<float, 4> sceneview_tr;

        //! Saved sceneview at mouse button down
        sm::mat<float, 4> savedSceneview;
        //! Saved sceneview_tr
        sm::mat<float, 4> savedSceneview_tr;

        //! The sceneview when the user switched to a follow-me mode. This can become a target to return to when switching back.
        sm::mat<float, 4> lastSceneview;
        //! translation only version of lastSceneview
        sm::mat<float, 4> lastSceneview_tr;

        // auto sceneview change. Active if visual_state::viewAutomation is set true in this->state.
        mplot::direction_data currentAutoSceneviewChange;

    public:

        //! Getter for d_to_rotation_centre
        float get_d_to_rotation_centre() const { return this->d_to_rotation_centre; }

        /*
         * Generic callback handlers
         */

        using keyaction = mplot::keyaction;
        using keymod = mplot::keymod;
        using key = mplot::key;
        // The key_callback handler uses GLFW codes, but they're in a mplot header (keys.h)
        template<bool owned = true>
        bool key_callback (std::int32_t _key, std::int32_t scancode, std::int32_t action, std::int32_t mods) // can't be virtual.
        {
            bool needs_render = false;

            if constexpr (owned == true) { // If Visual is 'owned' then the owning system deals with program exit
                // Exit action
                if (_key == key::q && (mods & keymod::control) && action == keyaction::press) {
                    this->signal_to_quit();
                }
            }

            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::c  && (mods & keymod::control) && action == keyaction::press) {
                this->options.flip (visual_options::showCoordArrows);
                needs_render = true;
            }

            if (_key == key::h && (mods & keymod::control) && action == keyaction::press) {
                // Help to stdout:
                std::cout << "Ctrl-h: Output this help to stdout\n"
                          << "Mouse-primary: rotate mode (use Ctrl to change axis)\n"
                          << "Mouse-secondary: translate mode\n";
                if constexpr (owned == true) { // If Visual is 'owned' then the owning system deals with program exit
                    std::cout << "Ctrl-q: Request exit\n";
                }
                std::cout << "Ctrl-v: Un-pause\n"
                          << "Ctrl-l: Toggle the scene lock\n"
                          << "Ctrl-c: Toggle coordinate arrows\n"
                          << "Ctrl-s: Take a snapshot\n"
                          << "Ctrl-m: Save 3D models in .gltf format (open in e.g. blender)\n"
                          << "Ctrl-a: Reset default view\n"
                          << "Ctrl-o: Reduce field of view\n"
                          << "Ctrl-p: Increase field of view\n"
                          << "Ctrl-7: Decrease ambient light intensity\n"
                          << "Ctrl-8: Increase ambient light intensity\n"
                          << "Ctrl-9: Decrease diffuse light intensity\n"
                          << "Ctrl-0: Increase diffuse light intensity\n"
                          << "Ctrl-e: Sceneview matrix to stdout\n"
                          << "Ctrl-y: Cycle perspective\n"
                          << "Ctrl-k: Toggle rotate about central model or scene origin\n"
                          << "Ctrl-b: Toggle between 'rotate about vertical', or 'mathplot tilt'\n"
                          << "Ctrl-d: Switch the vertical axis used in 'rotate about vertical' mode\n"
                          << "Ctrl-z: Show the current scenetrans/rotation and save to /tmp/Visual.json\n"
                          << "Ctrl-u: Reduce zNear cutoff plane\n"
                          << "Ctrl-i: Increase zNear cutoff plane\n"
                          << "Ctrl-j: Toggle bounding boxes\n"
                          << "Ctrl-Shift-s: Output shaders to stdout\n"
                          << "F1-F10: Select model index (with shift: toggle hide)\n"
                          << "Shift-Left: Decrease opacity of selected model\n"
                          << "Shift-Right: Increase opacity of selected model\n"
                          << std::flush;
            }

            if (_key == key::l && (mods & keymod::control) && action == keyaction::press) {
                this->state.flip (visual_state::sceneLocked);
                std::cout << "Scene is now " << (this->state.test (visual_state::sceneLocked) ? "" : "un-") << "locked\n";
            }

            if (_key == key::e && (mods & keymod::control) && action == keyaction::press) {
                constexpr sm::mat<float, 4> rotn_y = rotate_about_y();
                sm::mat<float, 4> effective_viewmatrix = this->sceneview.inverse() * rotn_y;
                std::cout << this->title << " window:\n  Sceneview effective camera location is " << (effective_viewmatrix * sm::vec<>{}) << std::endl;
                std::cout << "  Sceneview matrix array:\n" << this->sceneview.str_arr() << std::endl;
            }

            if (_key == key::v && (mods & keymod::control) && action == keyaction::press) {
                if (this->state.test (visual_state::paused)) {
                    this->state.set (visual_state::paused, false);
                    std::cout << "Scene un-paused\n";
                } // else no-op
            }

            if (_key == key::s && (mods & (keymod::control | keymod::shift)) && action == keyaction::press) {

                if ((mods & (keymod::control | keymod::shift)) == (keymod::control | keymod::shift)) {
                    // Ctrl-Shift-s gives you the default shaders
                    std::cout << "The built-in shader programs are:\n";
                    std::cout << "\nVisual.vert.glsl\n"
                              << "----------------\n"
                              << mplot::getDefaultVtxShader(glver) << std::endl;
                    std::cout << "\nVisual.frag.glsl\n"
                              << "----------------\n"
                              << mplot::getDefaultFragShader(glver) << std::endl;
                    std::cout << "\nVisText.vert.glsl\n"
                              << "----------------\n"
                              << mplot::getDefaultTextVtxShader(glver) << std::endl;
                    std::cout << "\nVisText.frag.glsl\n"
                              << "----------------\n"
                              << mplot::getDefaultTextFragShader(glver) << std::endl;
                } else if (mods & keymod::control) {
                    // Ctrl-s saves a PNG
                    std::string fname (this->title);
                    mplot::tools::stripFileSuffix (fname);
                    fname += ".png";
                    // Make fname 'filename safe'
                    mplot::tools::conditionAsFilename (fname);
                    this->saveImage (fname);
                    std::cout << "Saved image to '" << fname << "'\n";
                }
            }

            // Save gltf 3D file
            if (_key == key::m && (mods & keymod::control) && action == keyaction::press) {
                std::string gltffile = this->title;
                mplot::tools::stripFileSuffix (gltffile);
                gltffile += ".gltf";
                mplot::tools::conditionAsFilename (gltffile);
                this->savegltf (gltffile);
                std::cout << "Saved 3D file '" << gltffile << "'\n";
            }

            if (_key == key::z && (mods & keymod::control) && action == keyaction::press) {
                sm::quaternion<float> rotn = this->sceneview.rotation();
                rotn.renormalize();
                sm::vec<float> scenetrans = this->sceneview.translation();
                std::cout << "Scenetrans setup code:\n    v.setSceneTrans (sm::vec<float,3>{ float{"
                          << scenetrans.x() << "}, float{"
                          << scenetrans.y() << "}, float{"
                          << scenetrans.z()
                          << "} });"
                          <<  "\n    v.setSceneRotation (sm::quaternion<float>{ float{"
                          << rotn.w << "}, float{" << rotn.x << "}, float{"
                          << rotn.y << "}, float{" << rotn.z << "} });\n";
                std::cout << "Writing scene trans/rotation into /tmp/Visual.json... ";
                std::ofstream fout;
                fout.open ("/tmp/Visual.json", std::ios::out|std::ios::trunc);
                if (fout.is_open()) {
                    fout << "{\"scenetrans_x\":" << scenetrans.x()
                         << ", \"scenetrans_y\":" << scenetrans.y()
                         << ", \"scenetrans_z\":" << scenetrans.z()
                         << ",\n \"scenerotn_w\":" << rotn.w
                         << ", \"scenerotn_x\":" <<  rotn.x
                         << ", \"scenerotn_y\":" <<  rotn.y
                         << ", \"scenerotn_z\":" <<  rotn.z << "}\n";
                    fout.close();
                    std::cout << "Success.\n";
                } else {
                    std::cout << "Failed.\n";
                }
            }

            // Set selected model
            if (_key == key::f1 && action == keyaction::press) {
                this->selectedVisualModel = 0;
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            } else if (_key == key::f2 && action == keyaction::press) {
                if (this->vm.size() > 1) { this->selectedVisualModel = 1; }
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            } else if (_key == key::f3 && action == keyaction::press) {
                if (this->vm.size() > 2) { this->selectedVisualModel = 2; }
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            } else if (_key == key::f4 && action == keyaction::press) {
                if (this->vm.size() > 3) { this->selectedVisualModel = 3; }
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            } else if (_key == key::f5 && action == keyaction::press) {
                if (this->vm.size() > 4) { this->selectedVisualModel = 4; }
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            } else if (_key == key::f6 && action == keyaction::press) {
                if (this->vm.size() > 5) { this->selectedVisualModel = 5; }
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            } else if (_key == key::f7 && action == keyaction::press) {
                if (this->vm.size() > 6) { this->selectedVisualModel = 6; }
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            } else if (_key == key::f8 && action == keyaction::press) {
                if (this->vm.size() > 7) { this->selectedVisualModel = 7; }
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            } else if (_key == key::f9 && action == keyaction::press) {
                if (this->vm.size() > 8) { this->selectedVisualModel = 8; }
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            } else if (_key == key::f10 && action == keyaction::press) {
                if (this->vm.size() > 9) { this->selectedVisualModel = 9; }
                std::cout << "Selected visual model index " << this->selectedVisualModel << std::endl;
            }

            // Toggle hide model if the shift key is down
            if ((_key == key::f10 || _key == key::f1 || _key == key::f2 || _key == key::f3
                 || _key == key::f4 || _key == key::f5 || _key == key::f6
                 || _key == key::f7 || _key == key::f8 || _key == key::f9)
                && action == keyaction::press && (mods & keymod::shift)) {
                this->vm[this->selectedVisualModel]->toggleHide();
            }

            // Increment/decrement alpha for selected model
            if (_key == key::left && (action == keyaction::press || action == keyaction::repeat) && (mods & keymod::shift)) {
                if (!this->vm.empty()) { this->vm[this->selectedVisualModel]->decAlpha(); }
            }
            if (_key == key::right && (action == keyaction::press || action == keyaction::repeat) && (mods & keymod::shift)) {
                if (!this->vm.empty()) { this->vm[this->selectedVisualModel]->incAlpha(); }
            }

            // Ambient and diffuse lighting intensity
            if (_key == key::n9 && action == keyaction::press && (mods & keymod::control)) {
                // decrease diffuse intensity
                this->diffuse_intensity -= 0.05f;
                if (this->diffuse_intensity <= 0.0f) { this->diffuse_intensity = 0.0f; }
                std::cout << "diffuse_intensity is now " << this->diffuse_intensity << std::endl;
            } else if (_key == key::n0 && action == keyaction::press && (mods & keymod::control)) {
                // increase diffuse intensity
                this->diffuse_intensity += 0.05f;
                if (this->diffuse_intensity > 10.0f) { this->diffuse_intensity = 10.0f; }
                std::cout << "diffuse_intensity is now " << this->diffuse_intensity << std::endl;
            } else if (_key == key::n7 && action == keyaction::press && (mods & keymod::control)) {
                // decrease ambient intensity
                this->ambient_intensity -= 0.05f;
                if (this->ambient_intensity <= 0.0f) { this->ambient_intensity = 0.0f; }
                std::cout << "ambient_intensity is now " << this->ambient_intensity << std::endl;
            } else if (_key == key::n8 && action == keyaction::press && (mods & keymod::control)) {
                // increase ambient intensity
                this->ambient_intensity += 0.05f;
                if (this->ambient_intensity > 10.0f) { this->ambient_intensity = 10.0f; }
                std::cout << "ambient_intensity is now " << this->ambient_intensity << std::endl;
            }

            // Reset view to default
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::a && (mods & keymod::control) && action == keyaction::press) {
                std::cout << "Reset to default view\n";
                this->sceneview.set_identity();
                this->sceneview_tr.set_identity();
                this->sceneview.translate (this->scenetrans_default);
                this->sceneview.rotate (this->rotation_default);
                this->sceneview_tr.translate (this->scenetrans_default);
                this->scenetrans_delta.zero();
                this->rotation_delta.reset();
                this->d_to_rotation_centre = -this->scenetrans_default[2];
                //this->folcam_offset = folcam_default();
                this->folcam_offset_tr = folcam_offset_tr_default;
                this->folcam_offset_rot.reset();
                needs_render = true;
            }

            if (_key == key::k && (action == keyaction::press || action == keyaction::repeat) && (mods & keymod::control)) {
                this->options.flip (visual_options::rotateAboutSceneOrigin);
                std::cout << "Rotating about "
                          << (this->options.test (visual_options::rotateAboutSceneOrigin) ? "scene origin" : "central model")
                          << std::endl;
            }

            if (_key == key::j && (action == keyaction::press || action == keyaction::repeat) && (mods & keymod::control)) {
                this->options.flip (visual_options::showBoundingBoxes);
                // Update all the VisualModels now:
                auto vmi = this->vm.begin();
                while (vmi != this->vm.end()) {
                    (*vmi)->show_bb (this->options.test (visual_options::showBoundingBoxes));
                    ++vmi;
                }
            }

            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::o && (mods & keymod::control) && action == keyaction::press) {
                this->fov -= 2.0f;
                if (this->fov < 1.0f) { this->fov = 2.0f; }
                std::cout << "FOV reduced to " << this->fov << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::p && (mods & keymod::control) && action == keyaction::press) {
                this->fov += 2.0f;
                if (this->fov > 179.0f) { this->fov = 178.0f; }
                std::cout << "FOV increased to " << this->fov << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::u && (mods & keymod::control) && action == keyaction::press) {
                this->zNear /= 2.0f;
                std::cout << "zNear reduced to " << this->zNear << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::i && (mods & keymod::control) && action == keyaction::press) {
                this->zNear *= 2.0f;
                std::cout << "zNear increased to " << this->zNear << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::left_bracket && (mods & keymod::control) && action == keyaction::press) {
                this->zFar /= 2.0f;
                std::cout << "zFar reduced to " << this->zFar << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::right_bracket && (mods & keymod::control) && action == keyaction::press) {
                this->zFar *= 2.0f;
                std::cout << "zFar increased to " << this->zFar << std::endl;
            }

            if (_key == key::y && (mods & keymod::control) && action == keyaction::press) {
                if (this->ptype == mplot::perspective_type::perspective) {
                    this->ptype = mplot::perspective_type::orthographic;
                } else if (this->ptype == mplot::perspective_type::orthographic) {
                    this->ptype = mplot::perspective_type::perspective;
                }
                needs_render = true;
            }

            if (_key == key::d && (mods & keymod::control) && action == keyaction::press) {
                this->switch_scene_vertical_axis();
            }

            if (_key == key::b && (mods & keymod::control) && action == keyaction::press) {
                this->options.flip (visual_options::rotateAboutVertical);
                if (this->options.test (visual_options::rotateAboutVertical)) {
                    std::cout << "Mouse rotates scene about vertical axis\n";
                } else {
                    std::cout << "Mouse tilts scene as in the original mathplot\n";
                }
            }

            this->key_callback_extra (_key, scancode, action, mods);

            return needs_render;
        }

        // Switch between 'z' up and 'y' up
        void switch_scene_vertical_axis()
        {
            if (this->scene_up == sm::vec<>::uy()) {
                std::cout << "Changing 'scene up' to uz\n";
                this->scene_up = sm::vec<>::uz();
                this->scene_right = sm::vec<>::ux();
                this->scene_out = -sm::vec<>::uy();
            } else if (this->scene_up == sm::vec<>::uz()) {
                std::cout << "Changing 'scene up' to uy\n";
                this->scene_up = sm::vec<>::uy();
                this->scene_right = sm::vec<>::ux();
                this->scene_out = sm::vec<>::uz();
            } else {
                std::cout << "Not changing user-specified 'scene up' from " << this->scene_up << "\n";
            }
        }

        //! Rotate the scene about axis by angle (angle in radians)
        void rotate_scene (const sm::vec<float>& axis, const float angle)
        {
            sm::quaternion<float> rotnQuat (axis, -angle);
            this->sceneview.rotate (rotnQuat);
        }

        //! Find the rotation centre; either the scene origin or the centre of a perceptually nearby VM
        void find_rotation_centre()
        {
            // When rotating about scene origin, find translation of scene centre from screen centre
            if (this->options.test (visual_options::rotateAboutSceneOrigin) == true) {
                this->rotation_centre = this->savedSceneview.translation();
                return;
            }

            // Otherwise, find the centre of a visual model to rotate about
            constexpr sm::vec<float> v1 = { 0.0f, 0.0f, -100.0f };
            constexpr sm::vec<float> v2 = { 0.0f, 0.0f, 100.0f };
            constexpr sm::vec<float> v2v1 = v1 - v2;

            // A rotation delta in world frame about the 'screen centre'. This is a default:
            if (this->rotation_centre == sm::vec<float>{}) {
                this->rotation_centre = { 0.0f, 0.0f, this->savedSceneview.translation().z() + this->scenetrans_delta.z() };
            }

            // There's an option to write out the bounding box corners to a file that can be
            // displayed with debug_boundingboxes.cpp
            std::ofstream fout;
            std::uint32_t ci = 0;
            if (options.test (visual_options::boundingBoxesToJson)) {
                fout.open ("/tmp/mathplot_bounding_boxes.json", std::ios::out | std::ios::trunc);
                if (fout.is_open()) { fout << "{\n"; }
            }

            //std::multimap<float, std::tuple<sm::vec<float>, mplot::VisualModel<glver>*> > possible_centres; // tuple: didn't work with g++
            std::multimap<float, std::pair<sm::vec<float>, mplot::VisualModel<glver>*> > possible_centres;    // pair
            auto vmi = this->vm.begin();
            while (vmi != this->vm.end()) {

                // vm_bools comes from VisualModel and would need to be shared
                if ((*vmi)->flags.test (mplot::vm_bools::compute_bb) && !(*vmi)->flags.test (mplot::vm_bools::twodimensional)) {

                    sm::vec<float> tr_bb_centre = (this->savedSceneview * (*vmi)->get_viewmatrix_bb_centre()).less_one_dim();

                    if (options.test (visual_options::boundingBoxesToJson) && fout.is_open()) {
                        sm::interval<sm::vec<float>> modelbb = (*vmi)->bb; // Get the VisualModel bounding box
                        modelbb -= (*vmi)->bb.mid();                    // centre the bounding box about (VM frame's) origin
                        modelbb += tr_bb_centre;
                        fout << "  \"b" << (ci + 1) << "\": [" << modelbb.min.str_comma_separated() << "],\n";
                        fout << "  \"b" << (ci + 2) << "\": [" << modelbb.max.str_comma_separated() << "],\n";
                        ci += 2;
                    }

                    // Highlight central VM in any case. Really, want to highlight the selected possible centre.
                    if (options.test (visual_options::highlightRotationVM)) { (*vmi)->show_bb (false); }

                    // Find perpendicular distance from line to point pc
                    sm::vec<float> cv = tr_bb_centre - v1;
                    float pdist = cv.length() * std::sin (v2v1.angle (cv));

                    if (tr_bb_centre[2] < 0.0f) { // Only if in front of viewer (z must be negative)
                        // Perp. distance as key, value is tuple of BB centre and visualmodel pointer
                        //possible_centres.insert ({ pdist, { tr_bb_centre, (*vmi).get() } });            // tuple
                        possible_centres.insert ({ pdist, std::make_pair (tr_bb_centre, (*vmi).get()) }); // pair
                    }
                }
                ++vmi;
            }

            if (options.test (visual_options::boundingBoxesToJson) && fout.is_open()) {
                fout << "  \"n\": " << ci << "\n}\n";
                fout.close();
            }

            if (!possible_centres.empty()) {
                //const auto [rcentre, vmptr] = possible_centres.begin()->second;                            // tuple
                std::pair<sm::vec<float>, mplot::VisualModel<glver>*> pr = possible_centres.begin()->second; // pair
                // this->rotation_centre = rcentre; // tuple
                this->rotation_centre = pr.first;   // pair
                this->d_to_rotation_centre = this->rotation_centre.length();
                //if (options.test (visual_options::highlightRotationVM)) { vmptr->show_bb (true); }   // tuple
                if (options.test (visual_options::highlightRotationVM)) { pr.second->show_bb (true); } // pair
            } // else don't change rotation_centre
        }

        virtual bool cursor_position_callback (double x, double y)
        {
            this->cursorpos[0] = static_cast<float>(x);
            this->cursorpos[1] = static_cast<float>(y);

            sm::vec<float, 3> mouseMoveWorld = { 0.0f, 0.0f, 0.0f };

            bool needs_render = false;

            // Mouse-movement gain
            constexpr float mm_gain = 160.0f;

            // This is "rotate the scene" (and not "rotate one VisualModel")
            if (this->state.test (visual_state::rotateMode)) {
                // Convert mousepress/cursor positions (in pixels) to the range -1 -> 1:
                sm::vec<float, 2> p0_coord = this->mousePressPosition;
                p0_coord -= this->window_w * 0.5f;
                p0_coord /= this->window_w * 0.5f;
                sm::vec<float, 2> p1_coord = this->cursorpos;
                p1_coord -= this->window_w * 0.5f;
                p1_coord /= this->window_w * 0.5f;
                // Note: don't update this->mousePressPosition until user releases button.

                // Add the depth at which the object lies.  Use forward projection to determine the
                // correct z coordinate for the inverse projection. This assumes only one object.
                sm::vec<float, 4> point = { 0.0f, 0.0f, this->savedSceneview.translation().z(), 1.0f };
                sm::vec<float, 4> pp = this->projection * point;
                float coord_z = pp[2] / pp[3]; // divide by pp[3] is divide by/normalise by 'w'.

                // p0_coord/p1_coord in range -1 to 1, with a z value of 1.
                sm::vec<float, 4> p0 = { p0_coord[0], p0_coord[1], coord_z, 1.0f };
                sm::vec<float, 4> p1 = { p1_coord[0], p1_coord[1], coord_z, 1.0f };

                // Apply the inverse projection to get two points in the world frame of reference
                // for the mouse movement
                sm::vec<float, 4> v0 = this->invproj * p0;
                sm::vec<float, 4> v1 = this->invproj * p1;

                /*
                 * This computes the difference between v0 and v1, the 2 mouse positions in the
                 * world space. Note the swap between x and y. mouseMoveWorld is used as the
                 * rotation axis in the viewer's frame of reference or its values are used to set
                 * rotations about scene axes (if rotateAboutVertical is true)
                 */
                if (this->state.test (visual_state::rotateModMode)) {
                    // Sort of "rotate the page" mode.
                    mouseMoveWorld[2] = (-(v1[1] - v0[1]) + (v1[0] - v0[0]));
                } else {
                    mouseMoveWorld[1] = -(v1[0] - v0[0]);
                    mouseMoveWorld[0] = -(v1[1] - v0[1]);
                }
                mouseMoveWorld *= mm_gain;

                if (this->options.test (visual_options::rotateAboutVertical) == true
                    && this->options.test (visual_options::viewFollowsVMBehind) == false) {

                    if (this->state.test (visual_state::rotateModMode)) {
                        // What to do about rotate mod mode in this rotation scheme? Rotate about the missing axis for now.
                        this->rotation_delta.set_rotation (this->scene_out, mouseMoveWorld[2] * -sm::mathconst<float>::deg2rad);
                    } else {
                        // For now, rotate about the scene up axis
                        sm::vec<> mod_up = this->savedSceneview.rotation() * this->scene_up;
                        sm::quaternion<float> r1 (mod_up, mouseMoveWorld[1] * -sm::mathconst<float>::deg2rad);
                        sm::quaternion<float> r2 (this->scene_right, mouseMoveWorld[0] * -sm::mathconst<float>::deg2rad);
                        this->rotation_delta = r2 * r1;
                    }
                } else if (this->options.test (visual_options::viewFollowsVMBehind) == true) {
                    //std::cout << "\nmouseMoveWorld[0]: " << mouseMoveWorld[0] << std::endl; // pitch
                    //std::cout << "mouseMoveWorld[1]: " << mouseMoveWorld[1] << std::endl;   // about +- 40ish. leftright yaw
                    float pitch = mouseMoveWorld[0];
                    float yaw = mouseMoveWorld[1];
                    pitch = pitch > 10.0f ? 10.0f : pitch;
                    pitch = pitch < -65.0f ? -65.0f : pitch; // negative pitch is 'looking down' on the agent
                    yaw = yaw > 45.0f ? 45.0f : yaw;
                    yaw = yaw < -45.0f ? -45.0f : yaw;

                    sm::quaternion<float> r1 (this->scene_up, yaw * sm::mathconst<float>::deg2rad);
                    sm::quaternion<float> r2 (this->scene_right, pitch * -sm::mathconst<float>::deg2rad);
                    this->folcam_offset_rot = r2 * r1;

                } else {
                    // rotation_delta is the mouse-commanded rotation in the scene frame of reference
                    this->rotation_delta.set_rotation (mouseMoveWorld, mouseMoveWorld.length() * -sm::mathconst<float>::deg2rad);
                }

                needs_render = true;

            } else if (this->state.test (visual_state::translateMode)) { // allow only rotate OR translate for a single mouse movement
                // Convert mousepress/cursor positions (in pixels) to the range -1 -> 1:
                sm::vec<float, 2> p0_coord = this->mousePressPosition;
                p0_coord -= this->window_w * 0.5f;
                p0_coord /= this->window_w * 0.5f;
                sm::vec<float, 2> p1_coord = this->cursorpos;
                p1_coord -= this->window_w * 0.5f;
                p1_coord /= this->window_w * 0.5f;

                this->mousePressPosition = this->cursorpos;

                // Add the depth at which the object lies.  Use forward projection to determine the
                // correct z coordinate for the inverse projection. This assumes only one object.
                sm::vec<float, 4> point =  { 0.0f, 0.0f, -this->d_to_rotation_centre, 1.0f };
                sm::vec<float, 4> pp = this->projection * point;
                float coord_z = pp[2] / pp[3]; // divide by pp[3] is divide by/normalise by 'w'.

                // Construct two points for the start and end of the mouse movement
                sm::vec<float, 4> p0 = { p0_coord[0], p0_coord[1], coord_z, 1.0f };
                sm::vec<float, 4> p1 = { p1_coord[0], p1_coord[1], coord_z, 1.0f };
                // Apply the inverse projection to get two points in the world frame of reference:
                sm::vec<float, 4> v0 = this->invproj * p0;
                sm::vec<float, 4> v1 = this->invproj * p1;
                // This computes the difference betwen v0 and v1, the 2 mouse positions in the world
                mouseMoveWorld[0] = (v1[0] / v1[3]) - (v0[0] / v0[3]);
                mouseMoveWorld[1] = (v1[1] / v1[3]) - (v0[1] / v0[3]);
                // Note: mouseMoveWorld[2] is unmodified

                // We "translate the whole scene" - used by 2D projection shaders
                this->scenetrans_delta[0] += mouseMoveWorld[0];

                if (this->options.test (visual_options::viewFollowsVMBehind) == true) {
                    this->scenetrans_delta[1] += mouseMoveWorld[1]; // opp. sense in follow-me
                } else {
                    this->scenetrans_delta[1] -= mouseMoveWorld[1];
                }

                needs_render = true; // updates viewproj; uses this->scenetrans
            }

            return needs_render;
        }

        virtual void mouse_button_callback (std::int32_t button, std::int32_t action, std::int32_t mods = 0)
        {
            // If the scene is locked, then ignore the mouse movements
            if (this->state.test (visual_state::sceneLocked)) { return; }

            // Record the position and rotation at which the button was pressed
            if (action == keyaction::press) { // Button down
                this->mousePressPosition = this->cursorpos;
                this->savedSceneview = this->sceneview;
                this->savedSceneview_tr = this->sceneview_tr;
                this->scenetrans_delta.zero();
                this->rotation_delta.reset();
            } else if (action == keyaction::release) {
                // On mouse button release, zero the deltas:
                this->scenetrans_delta.zero();
                this->rotation_delta.reset();
            }

            this->find_rotation_centre();

            if (button == mplot::mousebutton::left) { // Primary button means rotate
                if (action == keyaction::press) {
                    this->state.set (visual_state::mouseButtonLeftPressed);
                } else if (action == keyaction::release) {
                    this->state.set (visual_state::mouseButtonLeftPressed, false);
                }
                this->state.set (visual_state::rotateModMode, ((mods & keymod::control) ? true : false));
                this->state.set (visual_state::rotateMode, (action == keyaction::press));
                this->state.set (visual_state::translateMode, false);
            } else if (button == mplot::mousebutton::right) { // Secondary button means translate
                if (action == keyaction::press) {
                    this->state.set (visual_state::mouseButtonRightPressed);
                } else if (action == keyaction::release) {
                    this->state.set (visual_state::mouseButtonRightPressed, false);
                }
                this->state.set (visual_state::rotateMode, false);
                this->state.set (visual_state::translateMode, (action == keyaction::press));
            }

            this->mouse_button_callback_extra (button, action, mods);
        }

        virtual bool window_size_callback (std::int32_t width, std::int32_t height)
        {
            this->window_w = width;
            this->window_h = height;
            return true; // needs_render
        }

        virtual bool window_content_scale_callback (float xscl, float yscl)
        {
            bool nr = false;
            if (this->window_scale_w != xscl) {
                this->window_scale_w = xscl;
                nr = true;
            }
            if (this->window_scale_h != yscl) {
                this->window_scale_h = yscl;
                nr = true;
            }
            return nr;
        }

        virtual void window_close_callback()
        {
            if (this->options.test (visual_options::preventWindowCloseWithButton) == false) {
                this->signal_to_quit();
            } else {
                std::cerr << "Ignoring user request to exit (Visual::preventWindowCloseWithButton)\n";
            }
        }

        //! When user scrolls, we translate the scene
        virtual bool scroll_callback (double xoffset, double yoffset)
        {
            // yoffset non-zero indicates that the most common scroll wheel is changing. If there's
            // a second scroll wheel, xoffset will be passed non-zero. They'll be 0 or +/- 1.

            if (this->state.test (visual_state::sceneLocked)) { return false; }

            this->savedSceneview = this->sceneview;
            this->savedSceneview_tr = this->sceneview_tr;
            this->scenetrans_delta.zero();
            this->rotation_delta.reset();
            this->state.set (visual_state::scrolling);

            if (this->ptype == perspective_type::orthographic) {
                // In orthographic, the wheel should scale ortho_lb and ortho_rt
                sm::vec<float, 2> _lb = this->ortho_lb + (yoffset * this->scenetrans_stepsize);
                sm::vec<float, 2> _rt = this->ortho_rt - (yoffset * this->scenetrans_stepsize);
                if (_lb < 0.0f && _rt > 0.0f) {
                    this->ortho_lb = _lb;
                    this->ortho_rt = _rt;
                }

            } else { // perspective_type::perspective

                // xoffset does what mouse drag left/right in rotateModMode does (L/R scene trans)
                this->scenetrans_delta[0] -= xoffset * this->scenetrans_stepsize;

                // yoffset does the 'in-out zooming'

                // How to make scenetrans_stepsize adaptive to the scale of the environment and change when close to objects?
                float y_step = static_cast<float>(yoffset) * this->scenetrans_stepsize * this->d_to_rotation_centre;
                sm::vec<float, 4> scroll_move_y = { 0.0f, y_step, 0.0f, 1.0f };

                this->scenetrans_delta[2] += scroll_move_y[1];

                if (this->d_to_rotation_centre > (this->zFar / 2.0f) && scroll_move_y[1] < 0.0f) {
                    // Cancel movement
                    this->scenetrans_delta[2] = 0.0f;
                    scroll_move_y[1] = 0.0f;
                }

                this->d_to_rotation_centre -= this->scenetrans_delta[2];
            }
            return true; // needs_render
        }

        virtual void key_callback_extra ([[maybe_unused]] std::int32_t key, [[maybe_unused]] std::int32_t scancode,
                                         [[maybe_unused]] std::int32_t action, [[maybe_unused]] std::int32_t mods) {}

        virtual void mouse_button_callback_extra ([[maybe_unused]] std::int32_t button, [[maybe_unused]] std::int32_t action,
                                                  [[maybe_unused]] std::int32_t mods) {}

        //! A callback that client code can set so that it knows when user has signalled to
        //! mplot::Visual that it's quit time.
        std::function<void()> external_quit_callback;

    protected:
        //! This internal quit function sets a 'readyToFinish' flag that your code can respond to,
        //! and calls an external callback function that you may have set up.
        void signal_to_quit()
        {
            if (this->options.test (visual_options::userInfoStdout)) { std::cout << "User requested exit.\n"; }
            // 1. Set our 'readyToFinish' flag to true
            this->state.set (visual_state::readyToFinish);
            // 2. Call any external callback that's been set by client code
            if (this->external_quit_callback) { this->external_quit_callback(); }
        }

        //! Unpause, allowing pauseOpen() to return
        void unpause() { this->state.reset (visual_state::paused); }
    };

} // namespace mplot
