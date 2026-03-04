/*!
 * \file
 *
 * Awesome graphics code for high performance graphing and visualisation. This is the abstract base
 * class for the Visual scene classes (it contains common functionality, but no GL)
 *
 * \author Seb James
 * \date March 2025
 */
#pragma once

#include <string>
#include <array>
#include <vector>
#include <map>
#include <tuple>
#include <memory>
#include <functional>
#include <cstddef>

import sm.flags;
import sm.quaternion;
import sm.mat;
import sm.vec;

#include <mplot/gl/version.h>
#include <mplot/VisualModel.h>
#include <mplot/TextFeatures.h>
#include <mplot/TextGeometry.h>
#include <mplot/VisualCommon.h>
#include <mplot/gl/shaders.h>
#include <mplot/keys.h>
#include <mplot/version.h>

#include <nlohmann/json.hpp>
#include <mplot/CoordArrows.h>
#include <mplot/RodVisual.h>
#include <mplot/tools.h>

#include <mplot/VisualDefaultShaders.h>

// Use Lode Vandevenne's PNG encoder
#define LODEPNG_NO_COMPILE_DECODER 1
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS 1
#include <mplot/lodepng.h>

namespace mplot
{
    //! Here are our boolean state flags
    enum class visual_state : uint32_t
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
        //! When true, the instanced data SSBO needs to be copied to the GPU
        instancedNeedsUpdate,
        //! Left mouse button is down
        mouseButtonLeftPressed,
        //! Right mouse button is down
        mouseButtonRightPressed
    };

    //! Boolean options - similar to state, but more likely to be modified by client code
    enum class visual_options : uint32_t
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
        viewFollowsVMBehind,
    };

    //! Whether to render with perspective or orthographic
    enum class perspective_type : uint32_t
    {
        perspective,
        orthographic
    };

#ifdef __APPLE__
    // https://stackoverflow.com/questions/35715579/opengl-created-window-size-twice-as-large
    static constexpr double retinaScale = 2; // deals with quadrant issue on osx
#else
    static constexpr double retinaScale = 1; // Qt has devicePixelRatio() to get retinaScale.
#endif

    /*!
     * VisualBase, the mplot::Visual 'scene' base class
     *
     * A base class for visualising computational models on an OpenGL screen.
     *
     * This contains code that is not OpenGL dependent. OpenGL dependent code is in
     * VisualOwnable or VisualOwnableMX.
     *
     * For mathplot program using GLFW windows, Inheritance chain will either be:
     *
     *   VisualBase -> VisualOwnable -> VisualNoMX            for single context GL, global fn aliases
     *
     *   VisualBase -> VisualOwnableMX -> VisualMX -> Visual  for multi context, GL fn pointers (GLAD only)
     *
     * mathplot based widgets, such as the Qt compatible mplot::qt::viswidget have this:
     *
     *   VisualBase -> VisualOwnable -> viswidget             for single context GL, global fn aliases
     *
     *   VisualBase -> VisualOwnableMX -> viswidget_mx        for single context GL, global fn aliases
     *
     * \tparam glver The OpenGL version, encoded as a single int (see mplot::gl::version)
     */
    template <int glver = mplot::gl::version_4_1>
    class VisualBase
    {
    public:
        /*!
         * Default constructor is used when incorporating Visual inside another object
         * such as a QWidget.  We have to wait on calling init functions until an OpenGL
         * environment is guaranteed to exist.
         */
        VisualBase() {
            this->sceneview.translate (this->scenetrans_default);
            this->sceneview_tr.translate (this->scenetrans_default);
        }

        /*!
         * Construct a new visualiser. The rule is 1 window to one Visual object. So, this creates a
         * new window and a new OpenGL context.
         */
        VisualBase (const int _width, const int _height, const std::string& _title, const bool _version_stdout = true)
            : window_w(_width)
            , window_h(_height)
            , title(_title)
        {
            this->sceneview.translate (this->scenetrans_default);
            this->sceneview_tr.translate (this->scenetrans_default);
            this->options.set (visual_options::versionStdout, _version_stdout);
            this->init_gl(); // abstract
        }

        //! Deconstruct gl memory/context
        virtual void deconstructCommon() = 0;

        // We do not manage OpenGL context, but it is simpler to have a no-op set/releaseContext for some of the GL setup functions
        virtual void setContext() {}       // no op here
        virtual void releaseContext() {}   // no op here
        virtual void setSwapInterval() {}  // no op here
        virtual void swapBuffers() {}      // no op here

        // A callback friendly wrapper for setContext
        static void set_context (mplot::VisualBase<glver>* _v) { _v->setContext(); };
        // A callback friendly wrapper for releaseContext
        static void release_context (mplot::VisualBase<glver>* _v) { _v->releaseContext(); };

        // Public init that is given a context (window or widget) and then sets up the
        // VisualResource, shaders and so on.
        void init (mplot::win_t* ctx)
        {
            this->window = ctx;
            this->init_resources();
            this->init_gl();
        }

    protected:
        virtual void freetype_init() = 0; // does this need to be here?

    public:
        // Do one-time init of resources (such as freetypes, windowing system etc)
        virtual void init_resources() = 0;

        //! Take a screenshot of the window. Return vec containing width * height or {-1, -1} on
        //! failure. Set transparent_bg to get a transparent background.
        virtual sm::vec<int, 2> saveImage (const std::string& img_filename, const bool transparent_bg = false) = 0;

        /*!
         * Set up the passed-in VisualModel (or indeed, VisualTextModel) with functions that need access to Visual attributes.
         */
        template <typename T>
        void bindmodel (std::unique_ptr<T>& model)
        {
            model->set_parent (this);
            model->get_shaderprogs = &mplot::VisualBase<glver>::get_shaderprogs;
            model->get_gprog = &mplot::VisualBase<glver>::get_gprog;
            model->get_tprog = &mplot::VisualBase<glver>::get_tprog;
            model->instanced_needs_update = &mplot::VisualBase<glver>::instanced_needs_update;
        }

        /*!
         * Add a VisualModel to the scene as a unique_ptr. The Visual object takes ownership of the
         * unique_ptr. The index into Visual::vm is returned.
         */
        template <typename T>
        unsigned int addVisualModelId (std::unique_ptr<T>& model)
        {
            std::unique_ptr<mplot::VisualModel<glver>> vmp = std::move(model);
            if (vmp->instanced()) { this->state.set (visual_state::haveInstanced, true); }
            this->vm.push_back (std::move(vmp));
            unsigned int rtn = (this->vm.size()-1);
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
            for (unsigned int modelId = 0; modelId < this->vm.size(); ++modelId) {
                if (this->vm[modelId].get() == vmp) {
                    rtn = vmp;
                    break;
                }
            }
            return rtn;
        }

        void setFollowedVM (const mplot::VisualModel<glver>* vm_to_follow)
        {
            for (unsigned int modelId = 0; modelId < this->vm.size(); ++modelId) {
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
        mplot::VisualModel<glver>* getVisualModel (unsigned int modelId) { return (this->vm[modelId].get()); }

        //! Remove the VisualModel with ID \a modelId from the scene.
        void removeVisualModel (unsigned int modelId) { this->vm.erase (this->vm.begin() + modelId); }

        //! Remove the VisualModel whose pointer matches the VisualModel* vmp
        void removeVisualModel (mplot::VisualModel<glver>* vmp)
        {
            unsigned int modelId = 0;
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
        static void callback_render (mplot::VisualBase<glver>* _v) { _v->render(); };

        //! Render the scene
        virtual void render() noexcept = 0;

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

        //! The OpenGL shader programs have an integer ID and are stored in a simple struct. There's
        //! one for graphical objects and a text shader program, which uses textures to draw text on
        //! quads.
        mplot::visgl::visual_shaderprogs shaders;
        //! Which shader is active for graphics shading. In practice, this is always 'projection2d'
        mplot::visgl::graphics_shader_type active_gprog = mplot::visgl::graphics_shader_type::none;
        //! Stores the info required to load the 2D projection shader
        std::vector<mplot::gl::ShaderInfo> proj2d_shader_progs;
        //! Stores the info required to load the text shader
        std::vector<mplot::gl::ShaderInfo> text_shader_progs;

        // These static functions will be set as callbacks in each VisualModel object.
        static mplot::visgl::visual_shaderprogs get_shaderprogs (mplot::VisualBase<glver>* _v) { return _v->shaders; };
        static GLuint get_gprog (mplot::VisualBase<glver>* _v) { return _v->shaders.gprog; };
        static GLuint get_tprog (mplot::VisualBase<glver>* _v) { return _v->shaders.tprog; };

        static void instanced_needs_update (mplot::VisualBase<glver>* _v) { _v->instancedNeedsUpdate (true); }

        //! The colour of ambient and diffuse light sources
        sm::vec<float, 3> light_colour = { 1.0f, 1.0f, 1.0f };
        //! Strength of the ambient light
        float ambient_intensity = 1.0f;
        //! Position of a diffuse light source
        sm::vec<float, 3> diffuse_position = { 5.0f, 5.0f, 15.0f };
        //! Strength of the diffuse light source
        float diffuse_intensity = 0.0f;

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

        //! Does our instanced data need to be pushed over to the GPU during render()?
        bool instancedNeedsUpdate() const { return this->state.test (visual_state::instancedNeedsUpdate); }
        void instancedNeedsUpdate (const bool val) { this->state.set (visual_state::instancedNeedsUpdate, val); }

        /*
         * User-settable projection values for the near clipping distance, the far clipping distance
         * and the field of view of the camera.
         */

        float zNear = 0.001f;
        float zFar = 300.0f;
        float fov = 30.0f;

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

        void lightingEffects (const bool effects_on = true)
        {
            this->ambient_intensity = effects_on ? 0.4f : 1.0f;
            this->diffuse_intensity = effects_on ? 0.6f : 0.0f;
        }

        //! Save all the VisualModels in this Visual out to a GLTF format file
        virtual void savegltf (const std::string& gltf_file)
        {
            std::ofstream fout;
            fout.open (gltf_file, std::ios::out|std::ios::trunc);
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

        void set_winsize (int _w, int _h) { this->window_w = _w; this->window_h = _h; }

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
            sm::quaternion<T> newpos = r_cur0.slerp (r_targ0, prop);
            return newpos; // rather than prop, as in get_cam_movement
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

        sm::mat<float, 4> update_folcam_viewmatrix()
        {
            sm::mat<float, 4> fol_cur;

            if (this->followedVM == nullptr) { return fol_cur; }

            // Target view from the followedVM
            //sm::mat<float, 4> fol_targ = this->followedVM->getViewMatrix() * this->folcam_offset;
            sm::mat<float, 4> rmat;
            rmat.rotate (folcam_offset_rot);
            sm::mat<float, 4> fol_targ = this->followedVM->getViewMatrix() * rmat;
            fol_targ.translate (folcam_offset_tr);

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
            sm::vec<float> pos_shift = this->get_cam_movement<float> (fol_cur, fol_targ,
                                                                      this->followedVM_vel, this->trans_tc);
            // get_cam_rotation computes the rotation for the next camera position
            sm::quaternion<float> cam_rotn = this->get_cam_rotation<float> (r_cur0, fol_targ,
                                                                            this->followedVM_rvel, this->rotn_tc);

            // set the translation/rotation into fol_cur
            fol_cur.pretranslate (pos_shift);
            fol_cur.rotate (cam_rotn);

            // Distance to rotation centre should be the distance to the followedVM
            this->d_to_rotation_centre = folcam_offset_tr.length();

            // fol_cur now contains the new position and orientation for the following camera
            return fol_cur;
        }

        // A follow-me camera view
        void computeSceneview_for_follower()
        {
            sm::mat<float, 4> folcam_viewmatrix = this->update_folcam_viewmatrix();
            constexpr sm::mat<float, 4> rotn_y = rotate_about_y();
            this->sceneview = rotn_y * folcam_viewmatrix.inverse();
            this->savedSceneview = this->sceneview;
        }

        // This is called every time render() is called
        void computeSceneview()
        {
            if (this->options.test (visual_options::viewFollowsVMBehind) && this->followedVM != nullptr) {
                // Use scenetrans_delta to shift the view with the scrollwheel
                this->folcam_offset_tr += this->scenetrans_delta;
                this->scenetrans_delta.zero();
                if (this->state.test (visual_state::scrolling)) { this->state.reset (visual_state::scrolling); }
                this->computeSceneview_for_follower();
                return;
            }

            if (std::abs(this->scenetrans_delta.sum()) > 0.0f || this->rotation_delta.is_zero_rotation() == false) {
                // Calculate model view transformation - transforming from "model space" to "worldspace".
                //std::cout << "standard view, call computeSceneview_about_rotation_centre\n";
                this->computeSceneview_about_rotation_centre();
            } // else don't change sceneview
            //else { std::cout << "No changing sceneview...\n"; }

            //std::cout << "sceneview\n" << sceneview << std::endl;

            if (this->state.test (visual_state::scrolling)) {
                this->scenetrans_delta.zero();
                this->state.reset (visual_state::scrolling);
            }

            if (this->options.test (visual_options::viewFollowsVMTranslations)
                && this->followedVM != nullptr
                && this->followedLastViewMatrix != this->followedVM->getViewMatrix()) {

                // Move camera the difference between followedLastViewMatrix and
                // followedVM->getViewMatrix() in the screen frame of reference.
                sm::vec<float> fol_screenframe = (this->sceneview * followedLastViewMatrix.translation()
                                                  - this->sceneview * followedVM->getViewMatrix().translation()).less_one_dim();

                this->sceneview.pretranslate (fol_screenframe);
                this->sceneview_tr.pretranslate (fol_screenframe);
                this->savedSceneview.pretranslate (fol_screenframe);
                this->savedSceneview_tr.pretranslate (fol_screenframe);

                this->followedLastViewMatrix = this->followedVM->getViewMatrix();
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

        // Initialize OpenGL shaders, set some flags (Alpha, Anti-aliasing), read in any external
        // state from json, and set up the coordinate arrows and any VisualTextModels that will be
        // required to render the Visual.
        virtual void init_gl() = 0;

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

        //! Current window width
        int window_w = 640;
        //! Current window height
        int window_h = 480;

        //! The title for the Visual. Used in window title and if saving out 3D model or png image.
        std::string title = "mathplot";

        //! The user's 'selected visual model'. For model specific changes to alpha and possibly colour
        unsigned int selectedVisualModel = 0u;

        //! A little model of the coordinate axes.
        std::unique_ptr<mplot::CoordArrows<glver>> coordArrows;

        //! Position coordinate arrows on screen. Configurable at mplot::Visual construction.
        sm::vec<float, 2> coordArrowsOffset = { -0.8f, -0.8f };

        //! Show the user's frame of reference as a model in the scene coords (for debug)
        std::unique_ptr<mplot::RodVisual<glver>> userFrame;

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
        //! movements. Initialized in VisualOwnable(No)MX constructor.
        sm::mat<float, 4> sceneview;

        //! The non-rotating sceneview matrix, updated only from mouse translations (avoiding rotations)
        sm::mat<float, 4> sceneview_tr;

        //! Saved sceneview at mouse button down
        sm::mat<float, 4> savedSceneview;

        //! Saved sceneview_tr
        sm::mat<float, 4> savedSceneview_tr;

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
        bool key_callback (int _key, int scancode, int action, int mods) // can't be virtual.
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
                this->fov -= 2;
                if (this->fov < 1.0) { this->fov = 2.0; }
                std::cout << "FOV reduced to " << this->fov << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::p && (mods & keymod::control) && action == keyaction::press) {
                this->fov += 2;
                if (this->fov > 179.0) { this->fov = 178.0; }
                std::cout << "FOV increased to " << this->fov << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::u && (mods & keymod::control) && action == keyaction::press) {
                this->zNear /= 2;
                std::cout << "zNear reduced to " << this->zNear << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::i && (mods & keymod::control) && action == keyaction::press) {
                this->zNear *= 2;
                std::cout << "zNear increased to " << this->zNear << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::left_bracket && (mods & keymod::control) && action == keyaction::press) {
                this->zFar /= 2;
                std::cout << "zFar reduced to " << this->zFar << std::endl;
            }
            if (this->state.test (visual_state::sceneLocked) == false
                && _key == key::right_bracket && (mods & keymod::control) && action == keyaction::press) {
                this->zFar *= 2;
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
            uint32_t ci = 0;
            if (options.test (visual_options::boundingBoxesToJson)) {
                fout.open ("/tmp/mathplot_bounding_boxes.json", std::ios::out | std::ios::trunc);
                if (fout.is_open()) { fout << "{\n"; }
            }

            std::multimap<float, std::tuple<sm::vec<float>, mplot::VisualModel<glver>*> > possible_centres;
            auto vmi = this->vm.begin();
            while (vmi != this->vm.end()) {

                if ((*vmi)->flags.test (mplot::vm_bools::compute_bb) && !(*vmi)->flags.test (mplot::vm_bools::twodimensional)) {

                    sm::vec<float> tr_bb_centre = (this->savedSceneview * (*vmi)->get_viewmatrix_bb_centre()).less_one_dim();

                    if (options.test (visual_options::boundingBoxesToJson) && fout.is_open()) {
                        sm::range<sm::vec<float>> modelbb = (*vmi)->bb; // Get the VisualModel bounding box
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
                        possible_centres.insert ({ pdist, { tr_bb_centre, (*vmi).get() } });
                    }
                }
                ++vmi;
            }

            if (options.test (visual_options::boundingBoxesToJson) && fout.is_open()) {
                fout << "  \"n\": " << ci << "\n}\n";
                fout.close();
            }

            if (!possible_centres.empty()) {
                const auto [rcentre, vmptr] = possible_centres.begin()->second;
                this->rotation_centre = rcentre;
                this->d_to_rotation_centre = this->rotation_centre.length();
                if (options.test (visual_options::highlightRotationVM)) { vmptr->show_bb (true); }
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

        virtual void mouse_button_callback (int button, int action, int mods = 0)
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

        virtual bool window_size_callback (int width, int height)
        {
            this->window_w = width;
            this->window_h = height;
            return true; // needs_render
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

        //! Extra key callback handling, making it easy for client programs to implement their own actions
        virtual void key_callback_extra ([[maybe_unused]] int key, [[maybe_unused]] int scancode,
                                         [[maybe_unused]] int action, [[maybe_unused]] int mods) {}

        //! Extra mousebutton callback handling, making it easy for client programs to implement their own actions
        virtual void mouse_button_callback_extra ([[maybe_unused]] int button, [[maybe_unused]] int action,
                                                  [[maybe_unused]] int mods) {}

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
