/*!
 * \file
 *
 * Declares a VisualModel base class to hold the vertices that make up some individual model object
 * that can be part of an OpenGL scene.
 *
 * GL function calls are added in VisualModelImplNoMX.h and VisualModelImplMX.h
 *
 * \author Seb James
 * \date March 2025
 */

#pragma once

#if defined __gl3_h_ || defined __gl_h_
// GL headers have been externally included
#else
# error "GL headers should have been included already"
#endif

#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <iterator>
#include <string>
#include <memory>
#include <functional>
#include <cstddef>
#include <cmath>
#include <bitset>
#include <map>
#include <set>
#include <tuple>

#include <mplot/gl/version.h>

#include <sm/geometry_polyhedra>
#include <sm/quaternion>
#include <sm/mat>
#include <sm/vec>
#include <sm/vvec>
#include <sm/range>
#include <sm/mathconst>
#include <sm/base64>
#include <sm/algo>
#include <sm/flags>

#include <mplot/VisualCommon.h>
#include <mplot/colour.h>
#include <mplot/NavMesh.h>

namespace mplot
{
    union float_bytes
    {
        float f;
        uint8_t bytes[sizeof(float)];
    };

    // State/options flags
    enum class vm_bools : uint32_t
    {
        postVertexInitRequired,
        twodimensional,         // If true, then this VisualModel should always be viewed in a plane - it's a 2D model
        hide,                   // If true, then calls to VisualModel::render should return
        wireframe,              // If true, draw in GL's polygon GL_LINES mode (instead of GL_FILL)
        instanced,              // If true, draw this VisualModel with 'instancing' 1 or more times
        show_bb,                // If true, draw vertices/indices for the bounding box frame
        compute_bb              // For some models, it's not useful to compute the bounding box (e.g. coordinate arrows)
    };

    //! Forward declaration of a Visual class
    template <int> class VisualBase;

    /*!
     * OpenGL model base class
     *
     * This class is a base 'OpenGL model' class. It has the common code to create the vertices for
     * some individual OpengGL model which is to be rendered in a 3-D scene.
     *
     * This class contains some common 'object primitives' code, such as computeSphere and
     * computeCone, which compute the vertices that will make up sphere and cone, respectively.
     *
     * It contains no GL function calls, these are added in the derived classes VisualModelImplNoMX and
     * VisualModelImplMX.
     */
    template <int glver = mplot::gl::version_4_1>
    struct VisualModelBase
    {
        VisualModelBase() {}
        VisualModelBase (const sm::vec<float> _offset) { this->viewmatrix.translate (_offset); }
        virtual ~VisualModelBase() {} // virtual deconstructor to keep clang happy

        // A VisualModel may contain a number of component VisualModels. When render is called, each
        // component is rendered.
        std::vector<std::unique_ptr<mplot::VisualModelBase<glver>>> components;

        //! Add a VisualModel as a component to this VisualModel
        template <typename T>
        T* addVisualModel (std::unique_ptr<T>& model)
        {
            std::unique_ptr<mplot::VisualModelBase<glver>> vmp = std::move(model);
            vmp->name += " component";
            this->components.push_back (std::move(vmp));
            return static_cast<T*>(this->components.back().get());
        }

        // Copy function bindings to the components
        virtual void bindComponents() = 0;

        void finalizeComponents() { for (auto& model : this->components) { model->finalize(); } }

        /*!
         * Set up the passed-in VisualTextModel with functions that need access to the parent Visual attributes.
         */
        template <typename T>
        void bindmodel (std::unique_ptr<T>& model)
        {
            if (this->parentVis == nullptr) {
                throw std::runtime_error ("Can't bind a model, because I am not bound");
            }
            model->set_parent (this->parentVis);
            model->get_shaderprogs = &mplot::VisualBase<glver>::get_shaderprogs;
            model->get_gprog = &mplot::VisualBase<glver>::get_gprog;
            model->get_tprog = &mplot::VisualBase<glver>::get_tprog;
            model->setContext = &mplot::VisualBase<glver>::set_context;
            model->releaseContext = &mplot::VisualBase<glver>::release_context;
        }

        //! Common code to call after the vertices have been set up. GL has to have been initialised.
        virtual void postVertexInit() = 0;

        //! Initialize vertex buffer objects and vertex array object. Empty for 'text only' VisualModels.
        virtual void initializeVertices() = 0;

        //! Process vertices and find the bounding box
        void update_bb()
        {
            if (this->flags.test (vm_bools::compute_bb) == false) { return; }

            if (this->vertexPositions.size() % 3 != 0) {
                throw std::runtime_error ("VisualModelBase: vertexPositions size is not divisible by 3");
            }
            this->bb.search_init();
            for (std::size_t i = 0; i < this->vertexPositions.size(); i += 3) {
                this->bb.update (sm::vec<float>{ vertexPositions[i], vertexPositions[i+1], vertexPositions[i+2] });
            }
            // After finding the bounding box, make up the vertices to display it:
            this->computeBoundingBox();
        }

        /*!
         * Re-initialize the buffers. Client code might have appended to
         * vertexPositions/Colors/Normals and indices before calling this method.
         */
        virtual void reinit_buffers() = 0;

        //! reinit ONLY vertexColors buffer
        virtual void reinit_colour_buffer() = 0;

        virtual void clearTexts() = 0;

        //! Clear out the model, *including text models*
        void clear()
        {
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();
            this->clearTexts();
            this->idx = 0u;
            // Clear bounding box
            this->vpos_bb.clear();
            this->vnorm_bb.clear();
            this->vcol_bb.clear();
            this->indices_bb.clear();
            this->idx_bb = 0u;

            this->reinit_buffers();
        }

        //! Re-create the model - called after updating data
        void reinit()
        {
            if (this->setContext != nullptr) { this->setContext (this->parentVis); }
            // Fixme: Better not to clear, then repeatedly pushback here:
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();

            // Clear any bounding box too
            this->vpos_bb.clear();
            this->vnorm_bb.clear();
            this->vcol_bb.clear();
            this->indices_bb.clear();
            this->idx_bb = 0u;

            // NB: Do NOT call clearTexts() here! We're only updating the model itself.
            this->idx = 0u;
            this->initializeVertices();
            this->update_bb();
            this->reinit_buffers();
        }

        /*!
         * For some models it's important to clear the texts when reinitialising. This is NOT the
         * same as VisualModel::clear() followed by initializeVertices(). For the same effect, you
         * can call clearTexts() then reinit().
         */
        void reinit_with_clearTexts()
        {
            if (this->setContext != nullptr) { this->setContext (this->parentVis); }
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();

            this->clearTexts();
            this->idx = 0u;

            // Clear any bounding box too
            this->vpos_bb.clear();
            this->vnorm_bb.clear();
            this->vcol_bb.clear();
            this->indices_bb.clear();
            this->idx_bb = 0u;

            this->initializeVertices();
            this->update_bb();
            this->reinit_buffers();
        }

        void reserve_vertices (std::size_t n_vertices)
        {
            this->vertexPositions.reserve (3u * n_vertices);
            this->vertexNormals.reserve (3u * n_vertices);
            this->vertexColors.reserve (3u * n_vertices);
            this->indices.reserve (6u * n_vertices);
        }

        // Get a single position from vertexPositions, using the index into the vector<vec>
        // interpretation of vertexPositions
        sm::vec<float, 3> get_position (const uint32_t vec_idx) const
        {
            auto vp = reinterpret_cast<const std::vector<sm::vec<float, 3>>*>(&this->vertexPositions);
            return (*vp)[vec_idx];
        }

        // Get a single normal from vertexNormals, using the index into the vector<vec>
        // interpretation of vertexNormals
        sm::vec<float, 3> get_normal (const uint32_t vec_idx) const
        {
            auto vn = reinterpret_cast<const std::vector<sm::vec<float, 3>>*>(&this->vertexNormals);
            return (*vn)[vec_idx];
        }

        /**
         * Neighbour vertex mesh code.
         */

        // Our navigation mesh data struct
        std::unique_ptr<mplot::NavMesh> navmesh;

        /*!
         * Post-process vertices to generate a neighbour relationship mesh. The usual vertices and
         * indices may not be useful to help an agent to navigate the surface defined by the
         * mesh. This is because vertices may be duplicated at any location, so that adjacent faces
         * can have different normals and colours.
         *
         * To help guide movement across a mesh, it would be useful to have a mesh that always gives
         * neighbour relationships.
         */
        void make_navmesh()
        {
            constexpr bool debug_mn = false;
            if constexpr (debug_mn) { std::cout << "make_navmesh: Called" << std::endl; }

            if (this->navmesh) { return; } // already made it

            if (this->flags.test (vm_bools::compute_bb) == false) {
                throw std::runtime_error ("make_navmesh requires compute_bb flag to be true");
            }
            this->update_bb();

            // Create a new navmesh
            this->navmesh = std::make_unique<mplot::NavMesh>();

            // Copy the bounding box
            navmesh->bb = this->bb;

            // Treat vertexPositions as a vector of vec:
            auto vp = reinterpret_cast<const std::vector<sm::vec<float, 3>>*>(&this->vertexPositions);

            uint32_t vps = vp->size();
            std::unordered_map<sm::vec<float, 3>, std::set<uint32_t>, sm::vec<float, 3>::hash> equiv_v;
            uint32_t i = 0;
            for (auto p : *vp) { equiv_v[p].insert (i++); }
            std::map<uint32_t, std::set<uint32_t>> equiv;
            for (auto e : equiv_v) { equiv[*e.second.begin()] = e.second; }
            if constexpr (debug_mn) {
                for (auto e : equiv) {
                    std::cout << "make_navmesh: equiv[" << e.first << "] = ";
                    for (auto idx : e.second) {  std::cout << idx << ","; }
                    std::cout << std::endl;
                }
                std::cout << "make_navmesh: Populated equiv which has "
                          << equiv.size() << " vvecs" << std::endl;
            }

            // Make inverse of equiv to translate from original (indices, vertexPositions) index to
            // new topographic mesh index
            sm::vvec<uint32_t> navmesh_idx (vps, 0);
            navmesh->vertexidx_to_indices.resize (equiv.size());

            uint32_t vcount = 0;
            i = 0;
            for (auto eqs : equiv) {
                vcount += eqs.second.size();
                navmesh->vertexidx_to_indices[i].resize (eqs.second.size());
                std::copy (eqs.second.begin(), eqs.second.end(), navmesh->vertexidx_to_indices[i].begin());
                for (auto ev : eqs.second) {
                    if constexpr (debug_mn) { std::cout << "make_navmesh: set navmesh_idx[" << ev << "] = " << i << std::endl; }
                    navmesh_idx[ev] = i;
                }
                ++i;
            }
            if constexpr (debug_mn) {
                std::cout << "make_navmesh: Created equiv inverse" << std::endl;
            }
            if (vcount != vps) {
                std::cout << "make_navmesh: WARNING: Vertex count from equiv is " << vcount
                          << " which should (but does not) equal " << vps << std::endl;
            }

            // Can now populate vertex, a vector of coordinates, if required, or simply access (*vp)
            // as needed using equiv.first
            navmesh->vertex.resize (equiv.size(), {0});
            i = 0;
            for (auto eq : equiv) { navmesh->vertex[i++] = (*vp)[eq.first]; } // FIXME?

            // Lastly, generate edges. For which we require use of indices, which is expressed in
            // terms of the old indices. That lookup is navmesh_idx.
            for (uint32_t i = 0; i < this->indices.size(); i += 3) {
                // Each three entries in indices is a triangle containing 3 edges. NB: Edges must be listed in ascending order!
                std::array<uint32_t, 2> e = { navmesh_idx[indices[i]], navmesh_idx[indices[i+1]] };
                if (e[0] > e[1]) {
                    uint32_t t = e[0];
                    e[0] = e[1];
                    e[1] = t;
                }
                navmesh->edges.insert (e);

                e = { navmesh_idx[indices[i]], navmesh_idx[indices[i+2]] };
                if (e[0] > e[1]) {
                    uint32_t t = e[0];
                    e[0] = e[1];
                    e[1] = t;
                }
                navmesh->edges.insert (e);

                e = { navmesh_idx[indices[i+1]], navmesh_idx[indices[i+2]] };
                if (e[0] > e[1]) {
                    uint32_t t = e[0];
                    e[0] = e[1];
                    e[1] = t;
                }
                navmesh->edges.insert (e);

                // Direct population of triangles. Three indices and a 4th number to hold flags (with bit0 meaning edge-triangle)
                std::array<uint32_t, 4> t = { navmesh_idx[indices[i]], navmesh_idx[indices[i+1]], navmesh_idx[indices[i+2]], 0 };

                // The normal vector for this triangle could be obtained from the mesh normals, but
                // we can't trust them (though they're easy to get, as we're dealing with indices
                // already). However, use this to ensure that our triangle indices order is in
                // agreement with mesh normal as far as direction goes.
                sm::vec<float> trinorm = this->get_normal (indices[i]) + this->get_normal (indices[i+1]) + this->get_normal (indices[i+2]) ;
                trinorm.renormalize();

                // Compute trinorm as well and compare with the one from the mesh - perhaps it's
                // different? We really want the right normal.
                const sm::vec<float>& tv0 = navmesh->vertex[t[0]];
                const sm::vec<float>& tv1 = navmesh->vertex[t[1]];
                const sm::vec<float>& tv2 = navmesh->vertex[t[2]];
                sm::vec<float> nx = (tv1 - tv0);
                sm::vec<float> ny = (tv2 - tv0);
                sm::vec<float> n = nx.cross (ny);
                n.renormalize();

                // Check rotational sense of triangles?
                if (n.dot (trinorm) < 0.0f) {
                    // need to swap order in t:
                    uint32_t ti = t[2];
                    t[2] = t[1];
                    t[1] = ti;
                    n = -n; // Also reverse n
                }

                navmesh->triangles.push_back ({t, n, nx, ny}); // n is computed normal
            }
            if constexpr (debug_mn) { std::cout << "make_navmesh: Created triangles" << std::endl; }

            //navmesh->mark_edge_triangles();
            //if constexpr (debug_mn) { std::cout << "make_navmesh: Marked edge triangles and done." << std::endl; }
        }

        /**
         * End neighbour vertex mesh code
         */

        /*!
         * A function to call initialiseVertices and postVertexInit after any necessary attributes
         * have been set (see, for example, setting the colour maps up in VisualDataModel).
         */
        void finalize()
        {
            if (this->setContext != nullptr) { this->setContext (this->parentVis); }
            this->initializeVertices();
            this->update_bb();
            this->flags.set (vm_bools::postVertexInitRequired, true);
            // Release context after creating and finalizing this VisualModel. On Visual::render(),
            // context will be re-acquired.
            if (this->releaseContext != nullptr) { this->releaseContext (this->parentVis); }

            this->bindComponents();
            this->finalizeComponents();
        }

        //! Render the VisualModel. Note that it is assumed that the OpenGL context has been
        //! obtained by the parent Visual::render call.
        virtual void render() = 0;

        //! Setter for the viewmatrix
        void setViewMatrix (const sm::mat<float, 4>& mv) { this->viewmatrix = mv; }
        //! And a getter
        sm::mat<float, 4> getViewMatrix() const { return this->viewmatrix; }
        //! Pre or post-multiply
        void postmultViewMatrix (const sm::mat<float, 4>& m) { this->viewmatrix = this->viewmatrix * m; }
        void premultViewMatrix (const sm::mat<float, 4>& m) { this->viewmatrix = m * this->viewmatrix; }

        void scaleViewMatrix (const float by) { this->viewmatrix.scale (by); }

        virtual void setSceneMatrixTexts (const sm::mat<float, 4>& sv) = 0;

        void setSceneMatrixComponents (const sm::mat44<float>& sv)
        {
            for (auto& cmp : this->components) { cmp->setSceneMatrix (sv); }
        }

        //! When setting the scene matrix, also have to set the text's scene matrices.
        void setSceneMatrix (const sm::mat<float, 4>& sv)
        {
            this->scenematrix = sv;
            this->setSceneMatrixComponents (sv);
            this->setSceneMatrixTexts (sv);
        }

        virtual void setSceneTranslationTexts (const sm::vec<float>& v0) = 0;

        //! Set a translation into the scene and into any child texts
        template <std::size_t N = 3> requires (N == 3) || (N == 4)
        void setSceneTranslation (const sm::vec<float, N>& v0)
        {
            this->scenematrix.set_identity();
            this->scenematrix.translate (v0);
            if constexpr (N == 4) {
                this->setSceneTranslationTexts (v0.less_one_dim());
            } else {
                this->setSceneTranslationTexts (v0);
            }
        }

        //! Set a translation (only) into the scene view matrix
        template <std::size_t N = 3> requires (N == 3) || (N == 4)
        void addSceneTranslation (const sm::vec<float, N>& v0) { this->scenematrix.pretranslate (v0); }

        //! Set a rotation (only) into the scene view matrix
        void setSceneRotation (const sm::quaternion<float>& r)
        {
            this->scenematrix.set_identity();
            this->scenematrix.rotate (r);
        }

        //! Add a rotation to the scene view matrix
        void addSceneRotation (const sm::quaternion<float>& r) { this->scenematrix.rotate (r); }

        //! Set a translation to the model view matrix
        template <std::size_t N = 3> requires (N == 3) || (N == 4)
        void setViewTranslation (const sm::vec<float, N>& v0)
        {
            this->viewmatrix.set_identity();
            this->viewmatrix.translate (v0);
        }

        //! Add a translation to the model view matrix
        template <std::size_t N = 3> requires (N == 3) || (N == 4)
        void addViewTranslation (const sm::vec<float, N>& v0) { this->viewmatrix.pretranslate (v0); }

        //! Set a rotation (only) into the view, but keep texts fixed
        void setViewRotationFixTexts (const sm::quaternion<float>& r)
        {
            sm::vec<> os = this->viewmatrix.translation();
            this->viewmatrix.set_identity();
            this->viewmatrix.translate (os);
            this->viewmatrix.rotate (r);
        }

        virtual void setViewRotationTexts (const sm::quaternion<float>& r) = 0;

        //! Set a rotation (only) into the view
        void setViewRotation (const sm::quaternion<float>& r)
        {
            sm::vec<> os = this->viewmatrix.translation();
            this->viewmatrix.set_identity();
            this->viewmatrix.translate (os);
            this->viewmatrix.rotate (r);
            this->setViewRotationTexts (r);
        }

        virtual void addViewRotationTexts (const sm::quaternion<float>& r) = 0;

        //! Apply a further rotation to the model view matrix
        void addViewRotation (const sm::quaternion<float>& r)
        {
            this->viewmatrix.rotate (r);
            this->addViewRotationTexts (r);
        }

        //! Apply a further rotation to the model view matrix, but keep texts fixed
        void addViewRotationFixTexts (const sm::quaternion<float>& r)
        {
            this->viewmatrix.rotate (r);
        }

        // The alpha attribute accessors
        void setAlpha (const float _a) { this->alpha = _a; }
        float getAlpha() const { return this->alpha; }
        void incAlpha()
        {
            this->alpha += 0.1f;
            this->alpha = this->alpha > 1.0f ? 1.0f : this->alpha;
        }
        void decAlpha()
        {
            this->alpha -= 0.1f;
            this->alpha = this->alpha < 0.0f ? 0.0f : this->alpha;
        }

        // The hide attribute accessors
        void setHide (const bool _h = true) { this->flags.set (vm_bools::hide, _h); }
        void toggleHide() { this->flags.flip (vm_bools::hide); }
        float hidden() const { return this->flags.test (vm_bools::hide); }

        /*
         * Methods used by Visual::savegltf()
         */

        //! Get model translation in a json-friendly string
        std::string translation_str() { return this->viewmatrix.translation().str_mat(); }
        //! A getter for the viewmatrix translation of the origin (would be same as viewmatrix.translation)
        sm::vec<float> get_viewmatrix_origin() const
        {
            return (this->viewmatrix * sm::vec<float, 3>{0,0,0}).less_one_dim();
        }
        //! The centre of mass of the bounding box may not be the VisualModel's origin
        sm::vec<float> get_viewmatrix_bb_centre() const
        {
            return (this->viewmatrix * this->bb.mid()).less_one_dim();
        }

        //! Apply the viewmatrix to the model's bounding box and return it
        sm::range<sm::vec<float>> get_viewmatrix_modelbb() const
        {
            sm::range<sm::vec<float>> vmbb;
            vmbb.min = (this->viewmatrix * this->bb.min).less_one_dim();
            vmbb.max = (this->viewmatrix * this->bb.max).less_one_dim();
            return vmbb;
        }

        //! Return the number of elements in this->indices
        std::size_t indices_size() { return this->indices.size(); }
        float indices_max() { return this->idx_max; }
        float indices_min() { return this->idx_min; }
        std::size_t indices_bytes() { return this->indices.size() * sizeof (GLuint); }
        //! Return base64 encoded version of indices
        std::string indices_base64()
        {
            std::vector<std::uint8_t> idx_bytes (this->indices.size() << 2, 0);
            std::size_t b = 0u;
            for (auto i : this->indices) {
                idx_bytes[b++] = i & 0xff;
                idx_bytes[b++] = i >> 8 & 0xff;
                idx_bytes[b++] = i >> 16 & 0xff;
                idx_bytes[b++] = i >> 24 & 0xff;
            }
            return base64::encode (idx_bytes);
        }

        /*!
         * Find the extents of this VisualModel, returning it as the x range, the y range and the z range.
         */
        sm::vec<sm::range<float>, 3> extents()
        {
            sm::vec<sm::range<float>, 3> axis_extents;
            for (unsigned int i = 0; i < 3; ++i) { axis_extents[i].search_init(); }
            for (unsigned int j = 0; j < static_cast<unsigned int>(this->vertexPositions.size() - 2); j += 3) {
                for (unsigned int i = 0; i < 3; ++i) { axis_extents[i].update (this->vertexPositions[j+i]); }
            }
            return axis_extents;
        }

        /*!
         * Compute the max and min values of indices and vertexPositions/Colors/Normals for use
         * when saving gltf files
         */
        void computeVertexMaxMins()
        {
            // Compute index maxmins
            for (std::size_t i = 0u; i < this->indices.size(); ++i) {
                idx_max = this->indices[i] > idx_max ? this->indices[i] : idx_max;
                idx_min = this->indices[i] < idx_min ? this->indices[i] : idx_min;
            }
            // Check every 0th entry in vertex Positions, every 1st, etc for max in the

            if (this->vertexPositions.size() != this->vertexColors.size()
                ||this->vertexPositions.size() != this->vertexNormals.size()) {
                throw std::runtime_error ("Expect vertexPositions, Colors and Normals vectors all to have same size");
            }

            for (std::size_t i = 0u; i < this->vertexPositions.size(); i+=3u) {
                vpos_maxes[0] =  (vertexPositions[i] > vpos_maxes[0]) ? vertexPositions[i] : vpos_maxes[0];
                vpos_maxes[1] =  (vertexPositions[i+1] > vpos_maxes[1]) ? vertexPositions[i+1] : vpos_maxes[1];
                vpos_maxes[2] =  (vertexPositions[i+2] > vpos_maxes[2]) ? vertexPositions[i+2] : vpos_maxes[2];
                vcol_maxes[0] =  (vertexColors[i] > vcol_maxes[0]) ? vertexColors[i] : vcol_maxes[0];
                vcol_maxes[1] =  (vertexColors[i+1] > vcol_maxes[1]) ? vertexColors[i+1] : vcol_maxes[1];
                vcol_maxes[2] =  (vertexColors[i+2] > vcol_maxes[2]) ? vertexColors[i+2] : vcol_maxes[2];
                vnorm_maxes[0] =  (vertexNormals[i] > vnorm_maxes[0]) ? vertexNormals[i] : vnorm_maxes[0];
                vnorm_maxes[1] =  (vertexNormals[i+1] > vnorm_maxes[1]) ? vertexNormals[i+1] : vnorm_maxes[1];
                vnorm_maxes[2] =  (vertexNormals[i+2] > vnorm_maxes[2]) ? vertexNormals[i+2] : vnorm_maxes[2];

                vpos_mins[0] =  (vertexPositions[i] < vpos_mins[0]) ? vertexPositions[i] : vpos_mins[0];
                vpos_mins[1] =  (vertexPositions[i+1] < vpos_mins[1]) ? vertexPositions[i+1] : vpos_mins[1];
                vpos_mins[2] =  (vertexPositions[i+2] < vpos_mins[2]) ? vertexPositions[i+2] : vpos_mins[2];
                vcol_mins[0] =  (vertexColors[i] < vcol_mins[0]) ? vertexColors[i] : vcol_mins[0];
                vcol_mins[1] =  (vertexColors[i+1] < vcol_mins[1]) ? vertexColors[i+1] : vcol_mins[1];
                vcol_mins[2] =  (vertexColors[i+2] < vcol_mins[2]) ? vertexColors[i+2] : vcol_mins[2];
                vnorm_mins[0] =  (vertexNormals[i] < vnorm_mins[0]) ? vertexNormals[i] : vnorm_mins[0];
                vnorm_mins[1] =  (vertexNormals[i+1] < vnorm_mins[1]) ? vertexNormals[i+1] : vnorm_mins[1];
                vnorm_mins[2] =  (vertexNormals[i+2] < vnorm_mins[2]) ? vertexNormals[i+2] : vnorm_mins[2];
            }
        }

        std::size_t vpos_size() { return this->vertexPositions.size(); }
        std::string vpos_max() { return this->vpos_maxes.str_mat(); }
        std::string vpos_min() { return this->vpos_mins.str_mat(); }
        std::size_t vpos_bytes() { return this->vertexPositions.size() * sizeof (float); }
        std::string vpos_base64()
        {
            std::vector<std::uint8_t> _bytes (this->vertexPositions.size() << 2, 0);
            std::size_t b = 0u;
            float_bytes fb;
            for (auto i : this->vertexPositions) {
                fb.f = i;
                _bytes[b++] = fb.bytes[0];
                _bytes[b++] = fb.bytes[1];
                _bytes[b++] = fb.bytes[2];
                _bytes[b++] = fb.bytes[3];
            }
            return base64::encode (_bytes);
        }
        std::size_t next_vpos_idx = 0;
        void init_vpos_accessor() { this->next_vpos_idx = 0; }
        sm::vec<float, 3> get_next_vpos()
        {
            sm::vec<float, 3> pos;
            pos.set_from (std::numeric_limits<float>::max());
            if (this->next_vpos_idx < this->vertexPositions.size()) {
                sm::vec<float, 4> tmp = {
                    this->vertexPositions[this->next_vpos_idx],
                    this->vertexPositions[this->next_vpos_idx + 1],
                    this->vertexPositions[this->next_vpos_idx + 2]
                };

                pos = (this->viewmatrix * tmp).less_one_dim();
                this->next_vpos_idx += 3;
            }
            return pos;
        }

        std::size_t vcol_size() { return this->vertexColors.size(); }
        std::string vcol_max() { return this->vcol_maxes.str_mat(); }
        std::string vcol_min() { return this->vcol_mins.str_mat(); }
        std::size_t vcol_bytes() { return this->vertexColors.size() * sizeof (float); }
        std::string vcol_base64()
        {
            std::vector<std::uint8_t> _bytes (this->vertexColors.size() << 2, 0);
            std::size_t b = 0u;
            float_bytes fb;
            for (auto i : this->vertexColors) {
                fb.f = i;
                _bytes[b++] = fb.bytes[0];
                _bytes[b++] = fb.bytes[1];
                _bytes[b++] = fb.bytes[2];
                _bytes[b++] = fb.bytes[3];
            }
            return base64::encode (_bytes);
        }
        std::size_t vnorm_size() { return this->vertexNormals.size(); }
        std::string vnorm_max() { return this->vnorm_maxes.str_mat(); }
        std::string vnorm_min() { return this->vnorm_mins.str_mat(); }
        std::size_t vnorm_bytes() { return this->vertexNormals.size() * sizeof (float); }
        std::string vnorm_base64()
        {
            std::vector<std::uint8_t> _bytes (this->vertexNormals.size()<<2, 0);
            std::size_t b = 0u;
            float_bytes fb;
            for (auto i : this->vertexNormals) {
                fb.f = i;
                _bytes[b++] = fb.bytes[0];
                _bytes[b++] = fb.bytes[1];
                _bytes[b++] = fb.bytes[2];
                _bytes[b++] = fb.bytes[3];
            }
            return base64::encode (_bytes);
        }
        std::size_t next_vnorm_idx = 0;
        void init_vnorm_accessor() { this->next_vnorm_idx = 0; }
        sm::vec<float, 3> get_next_vnorm()
        {
            sm::vec<float, 3> pos;
            pos.set_from (std::numeric_limits<float>::max());
            if (this->next_vnorm_idx < this->vertexPositions.size()) {
                pos = { this->vertexPositions[this->next_vnorm_idx],
                    this->vertexPositions[this->next_vnorm_idx + 1],
                    this->vertexPositions[this->next_vnorm_idx + 2] };
                this->next_vnorm_idx += 3;
            }
            return pos;
        }
        // end Visual::savegltf() methods

        // A VisualModel may be given a name
        std::string name = {};

        //! The current indices index
        GLuint idx = 0u;
        GLuint idx_bb = 0u;

        /*!
         * This is the upper limit for instance_count. We reserve max_isntances of space in the SSBO.
         *
         * Max number of instances in a model. Each *model* has to reserve space in the SSBOs which
         * are managed by VisualResources. VisualResources<glver>::max_instances must be larger than this.
         */
        unsigned int max_instances = 16 * 1024;
        //! The offset into the SSBOs for the instance data for this VisualModel
        unsigned int instance_start = 0;
        //! If drawing with instancing, how many instances? <= this->max_instances
        unsigned int instance_count = 0;
        //! If drawing with instancing, how many params will be used (these will be cycled through
        //! per-instance and there may be fewer than instance_count parameters)
        unsigned int instparam_count = 0;

        /*!
         * A function that will be runtime defined to get_shaderprogs from a pointer to
         * Visual (saving a boilerplate argument and avoiding that killer circular
         * dependency at the cost of one line of boilerplate in client programs)
         */
        std::function<mplot::visgl::visual_shaderprogs(mplot::VisualBase<glver>*)> get_shaderprogs;
        //! Get the graphics shader prog id
        std::function<GLuint(mplot::VisualBase<glver>*)> get_gprog;
        //! Get the text shader prog id
        std::function<GLuint(mplot::VisualBase<glver>*)> get_tprog;

        //! Set OpenGL context. Should call parentVis->setContext().
        std::function<void(mplot::VisualBase<glver>*)> setContext;
        //! Release OpenGL context. Should call parentVis->releaseContext().
        std::function<void(mplot::VisualBase<glver>*)> releaseContext;

        //! Init the SSBOs for instanced rendering
        std::function<unsigned int(mplot::VisualBase<glver>*, const unsigned int)> init_instance_data;
        std::function<void(const unsigned int, const sm::vec<float, 3>&)> insert_instance_data;
        std::function<void(const unsigned int, const std::array<float, 3>&, const float, const float)> insert_instparam_data;
        std::function<void(mplot::VisualBase<glver>*)> instanced_needs_update;

        //! Set up the instance positions (with default params for colour, rotn, scale)
        virtual void set_instance_data (const sm::vvec<sm::vec<float, 3>>& position) = 0;

        //! Set instance positions, with a single colour, alpha and scale to apply to all points
        virtual void set_instance_data (const sm::vvec<sm::vec<float, 3>>& position,
                                        const std::array<float, 3>& colour,
                                        const float alpha,
                                        const float scale) = 0;

        //! Set instance positions, with an array containing colour, alpha and scale values to apply
        //! to the instances. The size of colour, alpha and scale must match, but it may be of a
        //! different size to position.
        virtual void set_instance_data (const sm::vvec<sm::vec<float, 3>>& position,
                                        const sm::vvec<std::array<float, 3>>& colour,
                                        const sm::vvec<float>& alpha,
                                        const sm::vvec<float>& scale) = 0;

#if 0
        //! Update the instance positions and params (colour, rotn, scale) in a range
        virtual void update_instance_data (const sm::vvec<sm::vec<float, 3>>& points, const sm::vvec<float>& data,
                                           std::size_t i_s, std::size_t i_e) = 0;
#endif
        //! Setter for the parent pointer, parentVis
        void set_parent (mplot::VisualBase<glver>* _vis)
        {
            if (this->parentVis != nullptr) { throw std::runtime_error ("VisualModel: Set the parent pointer once only!"); }
            this->parentVis = _vis;
        }

        // Flags defaults.
        constexpr sm::flags<vm_bools> flags_defaults()
        {
            sm::flags<vm_bools> _flags;
            _flags.set (vm_bools::postVertexInitRequired, false);
            _flags.set (vm_bools::twodimensional, false);
            _flags.set (vm_bools::hide, false);
            _flags.set (vm_bools::wireframe, false);
            _flags.set (vm_bools::show_bb, false);
            _flags.set (vm_bools::compute_bb, true);
            return _flags;
        }

        // State/options flags
        sm::flags<vm_bools> flags = flags_defaults();

        // Setters for flags
        void show_bb (const bool val) { this->flags.set (vm_bools::show_bb, val); }
        void compute_bb (const bool val) { this->flags.set (vm_bools::compute_bb, val); }

        //! A range can be used for a bounding box for this VisualModel
        sm::range<sm::vec<float>> bb;
        std::array<float, 3> colour_bb = mplot::colour::grey90;

        void twodimensional (const bool val) { this->flags.set (vm_bools::twodimensional, val); }
        bool twodimensional() const { return this->flags.test (vm_bools::twodimensional); }

        void wireframe (const bool val) { this->flags.set (vm_bools::wireframe, val); }
        bool wireframe() const { return this->flags.test (vm_bools::wireframe); }

        void instanced (const bool val) { this->flags.set (vm_bools::instanced, val); }
        bool instanced() const
        {
            bool cmpts_inst = this->flags.test (vm_bools::instanced);
            for (auto& cmp : this->components) {
                if (cmp->instanced()) { cmpts_inst = true; }
            }
            return cmpts_inst;
        }

        //! Getter for vertex positions (for mplot::NormalsVisual)
        std::vector<float> getVertexPositions() { return this->vertexPositions; }
        //! Getter for vertex normals (for mplot::NormalsVisual)
        std::vector<float> getVertexNormals() { return this->vertexNormals; }
        std::vector<float> getVertexColors() { return this->vertexColors; }

    protected:

        //! The model-specific view matrix. Used to transform the pose of the model in the scene.
        sm::mat<float, 4> viewmatrix = {};
        /*!
         * The scene view matrix. Each VisualModel has a copy of the scenematrix. It's set in
         * Visual::render. Different VisualModels may have different scenematrices (for example, the
         * CoordArrows has a different scenematrix from other VisualModels, and models marked
         * 'twodimensional' also have a different scenematrix).
         */
        sm::mat<float, 4> scenematrix = {};

        //! Contains the positions within the vbo array of the different vertex buffer objects
        enum VBOPos { posnVBO, normVBO, colVBO, idxVBO, numVBO };

        /*
         * Compute positions and colours of vertices for the hexes and store in these:
         */

        //! The OpenGL Vertex Array Object
        GLuint vao = 0;

        //! Vertex Buffer Objects stored in an array
        std::unique_ptr<GLuint[]> vbos;

        //! CPU-side data for indices
        std::vector<GLuint> indices = {};
        //! CPU-side data for vertex positions
        std::vector<float> vertexPositions = {};
        //! CPU-side data for vertex normals
        std::vector<float> vertexNormals = {};
        //! CPU-side data for vertex colours
        std::vector<float> vertexColors = {};

        // OpenGL arrays for the bounding box, if needed
        GLuint vao_bb = 0;
        std::unique_ptr<GLuint[]> vbos_bb;
        std::vector<GLuint> indices_bb = {};
        std::vector<float> vpos_bb = {};
        std::vector<float> vnorm_bb = {};
        std::vector<float> vcol_bb = {};

        static constexpr float _max = std::numeric_limits<float>::max();
        static constexpr float _low = std::numeric_limits<float>::lowest();

        // The max and min values in the next 8 attributes are only computed if gltf files are going
        // to be output by Visual::savegltf()

        //! Max values of 0th, 1st and 2nd coordinates in vertexPositions
        sm::vec<float, 3> vpos_maxes = { _low, _low, _low }; // fixme: same as bounding box!
        //! Min values in vertexPositions
        sm::vec<float, 3> vpos_mins = { _max, _max, _max };
        sm::vec<float, 3> vcol_maxes = { _low, _low, _low };
        sm::vec<float, 3> vcol_mins = { _max, _max, _max };
        sm::vec<float, 3> vnorm_maxes = { _low, _low, _low };
        sm::vec<float, 3> vnorm_mins = { _max, _max, _max };
        //! Max value in indices
        GLuint idx_max = 0u;
        //! Min value in indices.
        GLuint idx_min = std::numeric_limits<GLuint>::max();

        //! A model-wide alpha value for the shader
        float alpha = 1.0f;

        // The mplot::VisualBase in which this model exists.
        mplot::VisualBase<glver>* parentVis = nullptr;

        //! Push three floats onto the vector of floats \a vp
        void vertex_push (const float& x, const float& y, const float& z, std::vector<float>& vp)
        {
            vp.emplace_back (x);
            vp.emplace_back (y);
            vp.emplace_back (z);
        }
        //! Push array of 3 floats onto the vector of floats \a vp
        template<std::size_t N = 3> requires (N == 3 || N == 4)
        void vertex_push (const std::array<float, N>& arr, std::vector<float>& vp)
        {
            vp.emplace_back (arr[0]);
            vp.emplace_back (arr[1]);
            vp.emplace_back (arr[2]);
        }
        //! Push sm::vec of 3 floats onto the vector of floats \a vp
        template<std::size_t N = 3> requires (N == 3 || N == 4)
        void vertex_push (const sm::vec<float, N>& vec, std::vector<float>& vp)
        {
            vp.emplace_back (vec[0]);
            vp.emplace_back (vec[1]);
            vp.emplace_back (vec[2]);
        }

        //! Set up a vertex buffer object - bind, buffer and set vertex array object attribute
        virtual void setupVBO (GLuint& buf, std::vector<float>& dat, unsigned int bufferAttribPosition) = 0;

    protected:
        /*!
         * Add the given meshgroup to this VisualModel
         */
        void computeMeshgroup (const mplot::meshgroup& mg)
        {
            mg.validate();

            bool single_colr = mg.colours.empty();
            for (uint32_t i = 0; i < mg.positions.size(); ++i) {
                // We apply mg.transform *here*, rather than writing it into the viewmatrix. This is
                // because other elements of this visual model may be added with the assumption of an
                // identity viewmatrix.
                this->vertex_push (mg.transform * mg.positions[i], this->vertexPositions);
                this->vertex_push (mg.transform * mg.normals[i], this->vertexNormals);
                if (single_colr) {
                    this->vertex_push (mg.single_colour, this->vertexColors);
                } else {
                    this->vertex_push (mg.colours[i], this->vertexColors);
                }
            }
            for (uint32_t i = 0; i < mg.indices.size(); ++i) {
                this->indices.push_back (mg.indices[i] + this->idx);
            }
            this->idx += mg.positions.size();
        }

        /**
         * START vertex/index computation code
         *
         * ALL methods below this point are for computing vertices.
         *
         * The metheds compute vertexPositions/Normals/Colors along with indices in a form suitable
         * for use with GL's DrawElements (or DrawElementsInstanced) drawing call.
         */

        /*!
         * Create a tube from \a start to \a end, with radius \a r and a colour which
         * transitions from the colour \a colStart to \a colEnd.
         *
         * This version simply sub-calls into computeFlaredTube which will randomly choose the angle
         * of the vertices around the centre of each end cap.
         *
         * \param idx The index into the 'vertex array'
         * \param start The start of the tube
         * \param end The end of the tube
         * \param colStart The tube starting colour
         * \param colEnd The tube's ending colour
         * \param r Radius of the tube
         * \param segments Number of segments used to render the tube
         */
        void computeTube (sm::vec<float> start, sm::vec<float> end,
                          std::array<float, 3> colStart, std::array<float, 3> colEnd,
                          float r = 1.0f, int segments = 12)
        {
            this->computeFlaredTube (start, end, colStart, colEnd, r, r, segments);
        }

        /*!
         * Compute a tube. This version requires unit vectors for orientation of the
         * tube end faces/vertices (useful for graph markers). The other version uses a
         * randomly chosen vector to do this.
         *
         * Create a tube from \a start to \a end, with radius \a r and a colour which
         * transitions from the colour \a colStart to \a colEnd.
         *
         * \param start The start of the tube
         * \param end The end of the tube
         * \param _ux a vector in the x axis direction for the end face
         * \param _uy a vector in the y axis direction. _ux ^ _uy gives the end cap normal
         * \param colStart The tube starting colour
         * \param colEnd The tube's ending colour
         * \param r Radius of the tube
         * \param segments Number of segments used to render the tube
         * \param rotation A rotation in the _ux/_uy plane to orient the vertices of the
         * tube. Useful if this is to be a short tube used as a graph marker.
         * \param bb If true, write into the bounding box arrays, not the main ones
         */
        void computeTube (sm::vec<float> start, sm::vec<float> end,
                          sm::vec<float> _ux, sm::vec<float> _uy,
                          std::array<float, 3> colStart, std::array<float, 3> colEnd,
                          float r = 1.0f, int segments = 12, float rotation = 0.0f, bool bb = false)
        {
            // The vector from start to end defines direction of the tube
            sm::vec<float> vstart = start;
            sm::vec<float> vend = end;

            // v is a face normal
            sm::vec<float> v = _ux.cross(_uy);
            v.renormalize();

            // If bounding box, populate different buffers:
            std::vector<float>& vp = bb ? this->vpos_bb : this->vertexPositions;
            std::vector<float>& vn = bb ? this->vnorm_bb : this->vertexNormals;
            std::vector<float>& vc = bb ? this->vcol_bb : this->vertexColors;
            std::vector<GLuint>& idcs = bb ? this->indices_bb : this->indices;
            GLuint& _idx = bb ? this->idx_bb : this->idx;

            // Push the central point of the start cap - this is at location vstart
            this->vertex_push (vstart, vp);
            this->vertex_push (-v, vn);
            this->vertex_push (colStart, vc);

            // Start cap vertices (a triangle fan)
            for (int j = 0; j < segments; j++) {
                // t is the angle of the segment
                float t = rotation + j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = _ux * std::sin(t) * r + _uy * std::cos(t) * r;
                this->vertex_push (vstart+c, vp);
                this->vertex_push (-v, vn);
                this->vertex_push (colStart, vc);
            }

            // Intermediate, near start cap. Normals point in direction c
            for (int j = 0; j < segments; j++) {
                float t = rotation + j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = _ux * std::sin(t) * r + _uy * std::cos(t) * r;
                this->vertex_push (vstart+c, vp);
                c.renormalize();
                this->vertex_push (c, vn);
                this->vertex_push (colStart, vc);
            }

            // Intermediate, near end cap. Normals point in direction c
            for (int j = 0; j < segments; j++) {
                float t = rotation + (float)j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = _ux * std::sin(t) * r + _uy * std::cos(t) * r;
                this->vertex_push (vend+c, vp);
                c.renormalize();
                this->vertex_push (c, vn);
                this->vertex_push (colEnd, vc);
            }

            // Bottom cap vertices
            for (int j = 0; j < segments; j++) {
                float t = rotation + (float)j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = _ux * std::sin(t) * r + _uy * std::cos(t) * r;
                this->vertex_push (vend+c, vp);
                this->vertex_push (v, vn);
                this->vertex_push (colEnd, vc);
            }

            // Bottom cap. Push centre vertex as the last vertex.
            this->vertex_push (vend, vp);
            this->vertex_push (v, vn);
            this->vertex_push (colEnd, vc);

            // Number of vertices = segments * 4 + 2.
            int nverts = (segments * 4) + 2;

            // After creating vertices, push all the indices.
            GLuint capMiddle = _idx;
            GLuint capStartIdx = _idx + 1u;
            GLuint endMiddle = _idx + static_cast<GLuint>(nverts) - 1u;
            GLuint endStartIdx = capStartIdx + (3u * segments);

            // Start cap indices
            for (int j = 0; j < segments-1; j++) {
                idcs.push_back (capMiddle);
                idcs.push_back (capStartIdx + j);
                idcs.push_back (capStartIdx + 1 + j);
            }
            // Last one
            idcs.push_back (capMiddle);
            idcs.push_back (capStartIdx + segments - 1);
            idcs.push_back (capStartIdx);

            // Middle sections
            for (int lsection = 0; lsection < 3; ++lsection) {
                capStartIdx = _idx + 1 + lsection*segments;
                endStartIdx = capStartIdx + segments;
                for (int j = 0; j < segments; j++) {
                    idcs.push_back (capStartIdx + j);
                    if (j == (segments-1)) {
                        idcs.push_back (capStartIdx);
                    } else {
                        idcs.push_back (capStartIdx + 1 + j);
                    }
                    idcs.push_back (endStartIdx + j);
                    idcs.push_back (endStartIdx + j);
                    if (j == (segments-1)) {
                        idcs.push_back (endStartIdx);
                    } else {
                        idcs.push_back (endStartIdx + 1 + j);
                    }
                    if (j == (segments-1)) {
                        idcs.push_back (capStartIdx);
                    } else {
                        idcs.push_back (capStartIdx + j + 1);
                    }
                }
            }

            // bottom cap
            for (int j = 0; j < segments-1; j++) {
                idcs.push_back (endMiddle);
                idcs.push_back (endStartIdx + j);
                idcs.push_back (endStartIdx + 1 + j);
            }
            idcs.push_back (endMiddle);
            idcs.push_back (endStartIdx + segments - 1);
            idcs.push_back (endStartIdx);

            // Update idx
            _idx += nverts;
        } // end computeTube with ux/uy vectors for faces

        /*!
         * A 'draw an arrow' primitive. This is a 3D, tubular arrow made of a tube and a cone.
         *
         * \param start Start coordinate of the arrow
         *
         * \param end End coordinate of the arrow
         *
         * \param clr The colour for the arrow
         *
         * \param tube_radius Radius of arrow shaft. If < 0, then set from (end-start).length()
         *
         * \param arrowhead_prop The proportion of the arrow length that the head should take up
         *
         * \param cone_radius Radisu of cone that make the arrow head. If < 0, then set from
         * tube_radius
         *
         * \param shapesides How many facets to draw tube/cone with
         */
        void computeArrow (const sm::vec<float>& start, const sm::vec<float>& end,
                           const std::array<float, 3> clr,
                           float tube_radius = -1.0f,
                           float arrowhead_prop = -1.0f,
                           float cone_radius = -1.0f,
                           const int shapesides = 18)
        {
            // The right way to draw an arrow.
            sm::vec<float> arrow_line = end - start;
            float len = arrow_line.length();
            // Unless client code specifies, compute tube radius from length of arrow
            if (tube_radius < 0.0f) { tube_radius = len / 40.0f; }
            if (arrowhead_prop < 0.0f) { arrowhead_prop = 0.15f; }
            if (cone_radius < 0.0f) { cone_radius = 1.75f * tube_radius; }
            // We don't draw the full tube
            sm::vec<float> cone_start = arrow_line.shorten (len * arrowhead_prop);
            cone_start += start;
            this->computeTube (start, cone_start, clr, clr, tube_radius, shapesides);
            float conelen = (end-cone_start).length();
            if (arrow_line.length() > conelen) {
                this->computeCone (cone_start, end, 0.0f, clr, cone_radius, shapesides);
            }
        }

        /*!
         * Create a flared tube from \a start to \a end, with radius \a r at the start and a colour
         * which transitions from the colour \a colStart to \a colEnd. The radius of the end is
         * determined by the given angle, flare, in radians.
         *
         * \param idx The index into the 'vertex array'
         * \param start The start of the tube
         * \param end The end of the tube
         * \param colStart The tube starting colour
         * \param colEnd The tube's ending colour
         * \param r Radius of the tube
         * \param segments Number of segments used to render the tube
         * \param flare The angle, measured wrt the direction of the tube in radians, by which the
         * tube 'flares'
         */
        void computeFlaredTube (sm::vec<float> start, sm::vec<float> end,
                                std::array<float, 3> colStart, std::array<float, 3> colEnd,
                                float r = 1.0f, int segments = 12, float flare = 0.0f)
        {
            // Find the length of the tube
            sm::vec<float> v = end - start;
            float l = v.length();
            // Compute end radius from the length and the flare angle:
            float r_add = l * std::tan (std::abs(flare)) * (flare > 0.0f ? 1.0f : -1.0f);
            float r_end = r + r_add;
            // Now call into the other overload:
            this->computeFlaredTube (start, end, colStart, colEnd, r, r_end, segments);
        }

        /*!
         * Create a flared tube from \a start to \a end, with radius \a r at the start and a colour
         * which transitions from the colour \a colStart to \a colEnd. The radius of the end is
         * r_end, given as a function argument.
         *
         * \param start The start of the tube
         * \param end The end of the tube
         * \param colStart The tube starting colour
         * \param colEnd The tube's ending colour
         * \param r Radius of the tube's start cap
         * \param r_end radius of the end cap
         * \param segments Number of segments used to render the tube
         */
        void computeFlaredTube (sm::vec<float> start, sm::vec<float> end,
                                std::array<float, 3> colStart, std::array<float, 3> colEnd,
                                float r = 1.0f, float r_end = 1.0f, int segments = 12)
        {
            // The vector from start to end defines a vector and a plane. Find a
            // 'circle' of points in that plane.
            sm::vec<float> vstart = start;
            sm::vec<float> vend = end;
            sm::vec<float> v = vend - vstart;
            v.renormalize();

            // circle in a plane defined by a point (v0 = vstart or vend) and a normal
            // (v) can be found: Choose random vector vr. A vector inplane = vr ^ v. The
            // unit in-plane vector is inplane.normalise. Can now use that vector in the
            // plan to define a point on the circle. Note that this starting point on
            // the circle is at a random position, which means that this version of
            // computeTube is useful for tubes that have quite a few segments.
            sm::vec<float> rand_vec;
            rand_vec.randomize();
            sm::vec<float> inplane = rand_vec.cross(v);
            inplane.renormalize();

            // Now use parameterization of circle inplane = p1-x1 and
            // c1(t) = ( (p1-x1).normalized std::sin(t) + v.normalized cross (p1-x1).normalized * std::cos(t) )
            // c1(t) = ( inplane std::sin(t) + v * inplane * std::cos(t)
            sm::vec<float> v_x_inplane = v.cross(inplane);

            // Push the central point of the start cap - this is at location vstart
            this->vertex_push (vstart, this->vertexPositions);
            this->vertex_push (-v, this->vertexNormals);
            this->vertex_push (colStart, this->vertexColors);

            // Start cap vertices. Draw as a triangle fan, but record indices so that we
            // only need a single call to glDrawElements.
            for (int j = 0; j < segments; j++) {
                // t is the angle of the segment
                float t = j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = inplane * std::sin(t) * r + v_x_inplane * std::cos(t) * r;
                this->vertex_push (vstart+c, this->vertexPositions);
                this->vertex_push (-v, this->vertexNormals);
                this->vertex_push (colStart, this->vertexColors);
            }

            // Intermediate, near start cap. Normals point in direction c
            for (int j = 0; j < segments; j++) {
                float t = j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = inplane * std::sin(t) * r + v_x_inplane * std::cos(t) * r;
                this->vertex_push (vstart+c, this->vertexPositions);
                c.renormalize();
                this->vertex_push (c, this->vertexNormals);
                this->vertex_push (colStart, this->vertexColors);
            }

            // Intermediate, near end cap. Normals point in direction c
            for (int j = 0; j < segments; j++) {
                float t = (float)j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = inplane * std::sin(t) * r_end + v_x_inplane * std::cos(t) * r_end;
                this->vertex_push (vend+c, this->vertexPositions);
                c.renormalize();
                this->vertex_push (c, this->vertexNormals);
                this->vertex_push (colEnd, this->vertexColors);
            }

            // Bottom cap vertices
            for (int j = 0; j < segments; j++) {
                float t = (float)j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = inplane * std::sin(t) * r_end + v_x_inplane * std::cos(t) * r_end;
                this->vertex_push (vend+c, this->vertexPositions);
                this->vertex_push (v, this->vertexNormals);
                this->vertex_push (colEnd, this->vertexColors);
            }

            // Bottom cap. Push centre vertex as the last vertex.
            this->vertex_push (vend, this->vertexPositions);
            this->vertex_push (v, this->vertexNormals);
            this->vertex_push (colEnd, this->vertexColors);

            // Note: number of vertices = segments * 4 + 2.
            int nverts = (segments * 4) + 2;

            // After creating vertices, push all the indices.
            GLuint capMiddle = this->idx;
            GLuint capStartIdx = this->idx + 1u;
            GLuint endMiddle = this->idx + static_cast<GLuint>(nverts) - 1u;
            GLuint endStartIdx = capStartIdx + (3u * segments);

            // Start cap
            for (int j = 0; j < segments-1; j++) {
                this->indices.push_back (capMiddle);
                this->indices.push_back (capStartIdx + j);
                this->indices.push_back (capStartIdx + 1 + j);
            }
            // Last one
            this->indices.push_back (capMiddle);
            this->indices.push_back (capStartIdx + segments - 1);
            this->indices.push_back (capStartIdx);

            // Middle sections
            for (int lsection = 0; lsection < 3; ++lsection) {
                capStartIdx = this->idx + 1 + lsection*segments;
                endStartIdx = capStartIdx + segments;
                // This does sides between start and end. I want to do this three times.
                for (int j = 0; j < segments; j++) {
                    // Triangle 1
                    this->indices.push_back (capStartIdx + j);
                    if (j == (segments-1)) {
                        this->indices.push_back (capStartIdx);
                    } else {
                        this->indices.push_back (capStartIdx + 1 + j);
                    }
                    this->indices.push_back (endStartIdx + j);
                    // Triangle 2
                    this->indices.push_back (endStartIdx + j);
                    if (j == (segments-1)) {
                        this->indices.push_back (endStartIdx);
                    } else {
                        this->indices.push_back (endStartIdx + 1 + j);
                    }
                    if (j == (segments-1)) {
                        this->indices.push_back (capStartIdx);
                    } else {
                        this->indices.push_back (capStartIdx + j + 1);
                    }
                }
            }

            // Bottom cap
            for (int j = 0; j < segments-1; j++) {
                this->indices.push_back (endMiddle);
                this->indices.push_back (endStartIdx + j);
                this->indices.push_back (endStartIdx + 1 + j);
            }
            // Last one
            this->indices.push_back (endMiddle);
            this->indices.push_back (endStartIdx + segments - 1);
            this->indices.push_back (endStartIdx);

            // Update idx
            this->idx += nverts;
        } // end computeFlaredTube with randomly initialized end vertices

        /*!
         * Create an open (no end caps) flared tube from \a start to \a end, with radius
         * \a r at the start and a colour which transitions from the colour \a colStart
         * to \a colEnd. The radius of the end is r_end, given as a function argument.
         *
         * This has a normal vector for the start and end of the tube, so that the
         * circles can be angled.
         *
         * \param start The start of the tube
         * \param end The end of the tube
         * \param colStart The tube starting colour
         * \param colEnd The tube's ending colour
         * \param n_start The normal of the start 'face'
         * \param n_end The normal of the end 'face'
         *
         * \param z_start A vector pointing to the first vertex on the tube. allows
         * orientation of tube faces for connected tubes (which is what this primitive
         * is all about)
         *
         * \param r Radius of the tube's start circle
         * \param r_end radius of the end circle
         * \param segments Number of segments used to render the tube
         */
        void computeOpenFlaredTube (sm::vec<float> start, sm::vec<float> end,
                                    sm::vec<float> n_start, sm::vec<float> n_end,
                                    std::array<float, 3> colStart, std::array<float, 3> colEnd,
                                    float r = 1.0f, float r_end = 1.0f, int segments = 12)
        {
            // The vector from start to end defines a vector and a plane. Find a
            // 'circle' of points in that plane.
            sm::vec<float> vstart = start;
            sm::vec<float> vend = end;
            sm::vec<float> v = vend - vstart;
            v.renormalize();

            // Two rotations about our face normals
            sm::quaternion<float> rotn_start (n_start, sm::mathconst<float>::pi_over_2);
            sm::quaternion<float> rotn_end (-n_end, sm::mathconst<float>::pi_over_2);

            sm::vec<float> inplane = v.cross (n_start);
            // The above is no good if n_start and v are colinear. In that case choose random inplane:
            if (inplane.length() < std::numeric_limits<float>::epsilon()) {
                sm::vec<float> rand_vec;
                rand_vec.randomize();
                inplane = rand_vec.cross(v);
            }
            inplane.renormalize();

            // inplane defines a plane, n_start defines a plane. Our first point is the
            // intersection of the two planes and the circle of the end.
            sm::vec<float> v_x_inplane = n_start.cross (inplane);// rotn_start * inplane;
            v_x_inplane.renormalize();

            // If r == r_end we want a circular cross section tube (and not an elliptical cross section).
            float r_mod = r / v_x_inplane.cross (v).length();

            // Start ring of vertices. Normals point in direction c
            // Now use parameterization of circle inplane = p1-x1 and
            // c1(t) = ( (p1-x1).normalized std::sin(t) + v.normalized cross (p1-x1).normalized * std::cos(t) )
            // c1(t) = ( inplane std::sin(t) + v * inplane * std::cos(t)
            for (int j = 0; j < segments; j++) {
                float t = j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = inplane * std::sin(t) * r + v_x_inplane * std::cos(t) * r_mod;
                this->vertex_push (vstart+c, this->vertexPositions);
                c.renormalize();
                this->vertex_push (c, this->vertexNormals);
                this->vertex_push (colStart, this->vertexColors);
            }

            // end ring of vertices. Normals point in direction c
            v_x_inplane = inplane.cross (n_end);
            v_x_inplane.renormalize();
            r_mod = r_end / v_x_inplane.cross (v).length();

            for (int j = 0; j < segments; j++) {
                float t = (float)j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = inplane * std::sin(t) * r_end + v_x_inplane * std::cos(t) * r_mod;
                this->vertex_push (vend+c, this->vertexPositions);
                c.renormalize();
                this->vertex_push (c, this->vertexNormals);
                this->vertex_push (colEnd, this->vertexColors);
            }

            // Number of vertices
            int nverts = (segments * 2);

            // After creating vertices, push all the indices.
            GLuint sIdx = this->idx;
            GLuint eIdx = sIdx + segments;
            // This does sides between start and end
            for (int j = 0; j < segments; j++) {
                // Triangle 1
                this->indices.push_back (sIdx + j);
                if (j == (segments-1)) {
                    this->indices.push_back (sIdx);
                } else {
                    this->indices.push_back (sIdx + 1 + j);
                }
                this->indices.push_back (eIdx + j);
                // Triangle 2
                this->indices.push_back (eIdx + j);
                if (j == (segments-1)) {
                    this->indices.push_back (eIdx);
                } else {
                    this->indices.push_back (eIdx + 1 + j);
                }
                if (j == (segments-1)) {
                    this->indices.push_back (sIdx);
                } else {
                    this->indices.push_back (sIdx + j + 1);
                }
            }

            // Update idx
            this->idx += nverts;
        } // end computeOpenFlaredTube

        // An open, but un-flared tube with no end caps
        void computeOpenTube (sm::vec<float> start, sm::vec<float> end,
                              sm::vec<float> n_start, sm::vec<float> n_end,
                              std::array<float, 3> colStart, std::array<float, 3> colEnd,
                              float r = 1.0f, int segments = 12)
        {
            this->computeOpenFlaredTube (start, end, n_start, n_end, colStart, colEnd, r, r, segments);
        }


        //! Compute a Quad from 4 arbitrary corners which must be ordered clockwise around the quad.
        void computeFlatQuad (sm::vec<float> c1, sm::vec<float> c2,
                              sm::vec<float> c3, sm::vec<float> c4,
                              std::array<float, 3> col)
        {
            // v is the face normal
            sm::vec<float> u1 = c1-c2;
            sm::vec<float> u2 = c2-c3;
            sm::vec<float> v = u2.cross(u1);
            v.renormalize();

            // Push corner vertices
            size_t vpsz = this->vertexPositions.size();
            this->vertexPositions.resize (vpsz + 12);
            for (unsigned int i = 0; i < 3u; ++i) { this->vertexPositions[vpsz++] = c1[i]; }
            for (unsigned int i = 0; i < 3u; ++i) { this->vertexPositions[vpsz++] = c2[i]; }
            for (unsigned int i = 0; i < 3u; ++i) { this->vertexPositions[vpsz++] = c3[i]; }
            for (unsigned int i = 0; i < 3u; ++i) { this->vertexPositions[vpsz++] = c4[i]; }

            // Colours/normals
            size_t vcsz = this->vertexColors.size();
            size_t vnsz = this->vertexNormals.size();
            this->vertexColors.resize (vcsz + 12);
            this->vertexNormals.resize (vnsz + 12);
            for (unsigned int i = 0; i < 4u; ++i) {
                for (unsigned int j = 0; j < 3u; ++j) {
                    this->vertexColors[vcsz++] = col[j];
                    this->vertexNormals[vnsz++] = v[j];
                }
            }

            size_t i0 = this->indices.size();
            this->indices.resize (i0 + 6, 0);
            this->indices[i0++] = this->idx;
            this->indices[i0++] = this->idx + 1;
            this->indices[i0++] = this->idx + 2;
            this->indices[i0++] = this->idx;
            this->indices[i0++] = this->idx + 2;
            this->indices[i0++] = this->idx + 3;

            this->idx += 4;
        }

        /*!
         * Compute a tube. This version requires unit vectors for orientation of the
         * tube end faces/vertices (useful for graph markers). The other version uses a
         * randomly chosen vector to do this.
         *
         * Create a tube from \a start to \a end, with radius \a r and a colour which
         * transitions from the colour \a colStart to \a colEnd.
         *
         * \param idx The index into the 'vertex array'
         * \param vstart The centre of the polygon
         * \param _ux a vector in the x axis direction for the end face
         * \param _uy a vector in the y axis direction
         * \param col The polygon colour
         * \param r Radius of the tube
         * \param segments Number of segments used to render the tube
         * \param rotation A rotation in the ux/uy plane to orient the vertices of the
         * tube. Useful if this is to be a short tube used as a graph marker.
         */
        void computeFlatPoly (sm::vec<float> vstart,
                              sm::vec<float> _ux, sm::vec<float> _uy,
                              std::array<float, 3> col,
                              float r = 1.0f, int segments = 12, float rotation = 0.0f)
        {
            // v is a face normal
            sm::vec<float> v = _uy.cross(_ux);
            v.renormalize();

            // Push the central point of the start cap - this is at location vstart
            this->vertex_push (vstart, this->vertexPositions);
            this->vertex_push (-v, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            // Polygon vertices (a triangle fan)
            for (int j = 0; j < segments; j++) {
                // t is the angle of the segment
                float t = rotation + j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = _ux * std::sin(t) * r + _uy * std::cos(t) * r;
                this->vertex_push (vstart+c, this->vertexPositions);
                this->vertex_push (-v, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);
            }

            // Number of vertices
            int nverts = segments + 1;

            // After creating vertices, push all the indices.
            GLuint capMiddle = this->idx;
            GLuint capStartIdx = this->idx + 1;

            // Start cap indices
            for (int j = 0; j < segments-1; j++) {
                this->indices.push_back (capMiddle);
                this->indices.push_back (capStartIdx + j);
                this->indices.push_back (capStartIdx + 1 + j);
            }
            // Last one
            this->indices.push_back (capMiddle);
            this->indices.push_back (capStartIdx + segments - 1);
            this->indices.push_back (capStartIdx);

            // Update idx
            this->idx += nverts;
        } // end computeFlatPloy with ux/uy vectors for faces

        /*!
         * Make a ring of radius r, comprised of flat segments
         *
         * \param ro position of the centre of the ring
         * \param rc The ring colour.
         * \param r Radius of the ring
         * \param t Thickness of the ring
         * \param segments Number of tube segments used to render the ring
         */
        void computeRing (sm::vec<float> ro, std::array<float, 3> rc, float r = 1.0f,
                          float t = 0.1f, int segments = 12)
        {
            float r_in = r - (t * 0.5f);
            float r_out = r + (t * 0.5f);
            this->computeRingInOut (ro, rc, r_in, r_out, segments);
        }

        /*!
         * Make a ring of radius r, comprised of flat segments, specifying inner and outer radii
         *
         * \param ro position of the centre of the ring
         * \param rc The ring colour.
         * \param r_in Inner radius of the ring
         * \param r_out Outer radius of the ring
         * \param segments Number of tube segments used to render the ring
         */
        void computeRingInOut (sm::vec<float> ro, std::array<float, 3> rc,
                               float r_in = 1.0f, float r_out = 2.0f, int segments = 12)
        {
            for (int j = 0; j < segments; j++) {
                float segment = sm::mathconst<float>::two_pi * static_cast<float>(j) / segments;
                // x and y of inner point
                float xin = r_in * std::cos (segment);
                float yin = r_in * std::sin (segment);
                float xout = r_out * std::cos (segment);
                float yout = r_out * std::sin (segment);
                int segjnext = (j + 1) % segments;
                float segnext = sm::mathconst<float>::two_pi * static_cast<float>(segjnext) / segments;
                float xin_n = r_in * std::cos (segnext);
                float yin_n = r_in * std::sin (segnext);
                float xout_n = r_out * std::cos (segnext);
                float yout_n = r_out * std::sin (segnext);

                // Now draw a quad
                sm::vec<float> c4 = { xin, yin, 0.0f };
                sm::vec<float> c3 = { xout, yout, 0.0f };
                sm::vec<float> c2 = { xout_n, yout_n, 0.0f };
                sm::vec<float> c1 = { xin_n, yin_n, 0.0f };
                this->computeFlatQuad (ro + c1, ro + c2, ro + c3, ro + c4, rc);
            }
        }

        /*!
         * Sphere, geodesic polygon version.
         *
         * This function creates an object with exactly one OpenGL vertex per 'geometric
         * vertex of the polyhedron'. That means that colouring this object must be
         * achieved by colouring the vertices and faces cannot be coloured
         * distinctly. Pass in a single colour for the initial object. To recolour,
         * modify the content of vertexColors.
         *
         * \tparam F The type used for the polyhedron computation. Use float or double.
         *
         * \param so The sphere offset. Where to place this sphere...
         * \param sc The sphere colour.
         * \param r Radius of the sphere
         * \param iterations how many iterations of the geodesic polygon algo to go
         * through. Determines faces:
         *
         * For 0 iterations, get a geodesic with 20 faces        *0
         * For 1 iterations, get a geodesic with 80 faces
         * For 2 iterations, get a geodesic with 320 faces       *1
         * For 3 iterations, get a geodesic with 1280 faces      *2
         * For 4 iterations, get a geodesic with 5120 faces      *3
         * For 5 iterations, get a geodesic with 20480 faces     *4
         * For 6 iterations, get a geodesic with 81920 faces
         * For 7 iterations, get a geodesic with 327680 faces
         * For 8 iterations, get a geodesic with 1310720 faces
         * For 9 iterations, get a geodesic with 5242880 faces
         *
         * *0: You'll get an icosahedron
         * *1: decent graphical results
         * *2: excellent graphical results
         * *3: You can *just about* see a difference between 4 iterations and 3, but not
         *  between 4 and 5.
         * *4: The iterations limit if F is float (you'll get a runtime error 'vertices
         *  has wrong size' for iterations>5)
         *
         * \return The number of vertices in the generated geodesic sphere
         */
        template<typename F=float>
        int computeSphereGeo (sm::vec<float> so, std::array<float, 3> sc, float r = 1.0f, int iterations = 2)
        {
            if (iterations < 0) { throw std::runtime_error ("computeSphereGeo: iterations must be positive"); }
            // test if type F is float
            if constexpr (std::is_same<std::decay_t<F>, float>::value == true) {
                if (iterations > 5) {
                    throw std::runtime_error ("computeSphereGeo: For iterations > 5, F needs to be double precision");
                }
            } else {
                if (iterations > 10) {
                    throw std::runtime_error ("computeSphereGeo: This is an abitrary iterations limit (10 gives 20971520 faces)");
                }
            }
            // Note that we need double precision to compute higher iterations of the geodesic (iterations > 5)
            sm::geometry::icosahedral_geodesic<F> geo = sm::geometry::make_icosahedral_geodesic<F> (iterations);

            // Now essentially copy geo into vertex buffers
            for (auto v : geo.poly.vertices) {
                this->vertex_push (v.as_float() * r + so, this->vertexPositions);
                this->vertex_push (v.as_float(), this->vertexNormals);
                this->vertex_push (sc, this->vertexColors);
            }
            for (auto f : geo.poly.faces) {
                this->indices.push_back (this->idx + f[0]);
                this->indices.push_back (this->idx + f[1]);
                this->indices.push_back (this->idx + f[2]);
            }
            // idx is the *vertex index* and should be incremented by the number of vertices in the polyhedron
            int n_verts = static_cast<int>(geo.poly.vertices.size());
            this->idx += n_verts;

            return n_verts;
        }

        /*!
         * Sphere, geodesic polygon version with coloured faces
         *
         * To colour the faces of this polyhedron, update this->vertexColors (for an
         * example see mplot::GeodesicVisual). To make faces distinctly colourizable, we
         * have to generate 3 OpenGL vertices for each of the geometric vertices in the
         * polyhedron.
         *
         * \tparam F The type used for the polyhedron computation. Use float or double.
         *
         * \param so The sphere offset. Where to place this sphere...
         * \param sc The default colour
         * \param r Radius of the sphere
         * \param iterations how many iterations of the geodesic polygon algo to go
         * through. Determines number of faces
         */
        template<typename F=float>
        int computeSphereGeoFaces (sm::vec<float> so, std::array<float, 3> sc, float r = 1.0f, int iterations = 2)
        {
            if (iterations < 0) { throw std::runtime_error ("computeSphereGeo: iterations must be positive"); }
            // test if type F is float
            if constexpr (std::is_same<std::decay_t<F>, float>::value == true) {
                if (iterations > 5) {
                    throw std::runtime_error ("computeSphereGeo: For iterations > 5, F needs to be double precision");
                }
            } else {
                if (iterations > 10) {
                    throw std::runtime_error ("computeSphereGeo: This is an abitrary iterations limit (10 gives 20971520 faces)");
                }
            }
            // Note that we need double precision to compute higher iterations of the geodesic (iterations > 5)
            sm::geometry::icosahedral_geodesic<F> geo = sm::geometry::make_icosahedral_geodesic<F> (iterations);
            int n_faces = static_cast<int>(geo.poly.faces.size());

            for (int i = 0; i < n_faces; ++i) { // For each face in the geodesic...
                sm::vec<F, 3> norm = { F{0}, F{0}, F{0} };
                for (auto vtx : geo.poly.faces[i]) { // For each vertex in face...
                    norm += geo.poly.vertices[vtx]; // Add to the face norm
                    this->vertex_push (geo.poly.vertices[vtx].as_float() * r + so, this->vertexPositions);
                }
                sm::vec<float, 3> nf = (norm / F{3}).as_float();
                for (int j = 0; j < 3; ++j) { // Faces all have size 3
                    this->vertex_push (nf, this->vertexNormals);
                    this->vertex_push (sc, this->vertexColors); // A default colour
                    this->indices.push_back (this->idx + (3 * i) + j); // indices is vertex index
                }
            }
            // An index for each vertex of each face.
            this->idx += 3 * n_faces;

            return n_faces;
        }

        //! Fast computeSphereGeo, which uses constexpr make_icosahedral_geodesic. The
        //! resulting vertices and faces are NOT in any kind of order, but ok for
        //! plotting, e.g. scatter graph spheres.
        template<typename F=float, int iterations = 2>
        int computeSphereGeoFast (sm::vec<float> so, std::array<float, 3> sc, float r = 1.0f)
        {
            // test if type F is float
            if constexpr (std::is_same<std::decay_t<F>, float>::value == true) {
                static_assert (iterations <= 5, "computeSphereGeoFast: For iterations > 5, F needs to be double precision");
            } else {
                static_assert (iterations <= 10, "computeSphereGeoFast: This is an abitrary iterations limit (10 gives 20971520 faces)");
            }
            // Note that we need double precision to compute higher iterations of the geodesic (iterations > 5)
            constexpr sm::geometry_ce::icosahedral_geodesic<F, iterations>  geo = sm::geometry_ce::make_icosahedral_geodesic<F, iterations>();

            // Now essentially copy geo into vertex buffers
            for (auto v : geo.poly.vertices) {
                this->vertex_push (v.as_float() * r + so, this->vertexPositions);
                this->vertex_push (v.as_float(), this->vertexNormals);
                this->vertex_push (sc, this->vertexColors);
            }
            for (auto f : geo.poly.faces) {
                this->indices.push_back (this->idx + f[0]);
                this->indices.push_back (this->idx + f[1]);
                this->indices.push_back (this->idx + f[2]);
            }
            // idx is the *vertex index* and should be incremented by the number of vertices in the polyhedron
            int n_verts = static_cast<int>(geo.poly.vertices.size());
            this->idx += n_verts;

            return n_verts;
        }

        /*!
         * Sphere, 1 colour version.
         *
         * Code for creating a sphere as part of this model. I'll use a sphere at the centre of the arrows.
         *
         * \param so The sphere offset. Where to place this sphere...
         * \param sc The sphere colour.
         * \param r Radius of the sphere
         * \param rings Number of rings used to render the sphere
         * \param segments Number of segments used to render the sphere
         *
         * Number of faces should be (2 + rings) * segments
         */
        void computeSphere (sm::vec<float> so, std::array<float, 3> sc,
                            float r = 1.0f, int rings = 10, int segments = 12)
        {
            // First cap, draw as a triangle fan, but record indices so that
            // we only need a single call to glDrawElements.
            float rings0 = -sm::mathconst<float>::pi_over_2;
            float _z0  = std::sin(rings0);
            float z0  = r * _z0;
            float r0 =  std::cos(rings0);
            float rings1 = sm::mathconst<float>::pi * (-0.5f + 1.0f / rings);
            float _z1 = std::sin(rings1);
            float z1 = r * _z1;
            float r1 = std::cos(rings1);
            // Push the central point
            this->vertex_push (so[0]+0.0f, so[1]+0.0f, so[2]+z0, this->vertexPositions);
            this->vertex_push (0.0f, 0.0f, -1.0f, this->vertexNormals);
            this->vertex_push (sc, this->vertexColors);

            GLuint capMiddle = this->idx++;
            GLuint ringStartIdx = this->idx;
            GLuint lastRingStartIdx = this->idx;

            bool firstseg = true;
            for (int j = 0; j < segments; j++) {
                float segment = sm::mathconst<float>::two_pi * static_cast<float>(j) / segments;
                float x = std::cos(segment);
                float y = std::sin(segment);

                float _x1 = x*r1;
                float x1 = _x1*r;
                float _y1 = y*r1;
                float y1 = _y1*r;

                this->vertex_push (so[0]+x1, so[1]+y1, so[2]+z1, this->vertexPositions);
                this->vertex_push (_x1, _y1, _z1, this->vertexNormals);
                this->vertex_push (sc, this->vertexColors);

                if (!firstseg) {
                    this->indices.push_back (capMiddle);
                    this->indices.push_back (this->idx-1);
                    this->indices.push_back (this->idx++);
                } else {
                    this->idx++;
                    firstseg = false;
                }
            }
            this->indices.push_back (capMiddle);
            this->indices.push_back (this->idx-1);
            this->indices.push_back (capMiddle+1);

            // Now add the triangles around the rings
            for (int i = 2; i < rings; i++) {

                rings0 = sm::mathconst<float>::pi * (-0.5f + static_cast<float>(i) / rings);
                _z0  = std::sin(rings0);
                z0  = r * _z0;
                r0 =  std::cos(rings0);

                for (int j = 0; j < segments; j++) {

                    // "current" segment
                    float segment = sm::mathconst<float>::two_pi * static_cast<float>(j) / segments;
                    float x = std::cos(segment);
                    float y = std::sin(segment);

                    // One vertex per segment
                    float _x0 = x*r0;
                    float x0 = _x0*r;
                    float _y0 = y*r0;
                    float y0 = _y0*r;

                    // NB: Only add ONE vertex per segment. ALREADY have the first ring!
                    this->vertex_push (so[0]+x0, so[1]+y0, so[2]+z0, this->vertexPositions);
                    // The vertex normal of a vertex that makes up a sphere is
                    // just a normal vector in the direction of the vertex.
                    this->vertex_push (_x0, _y0, _z0, this->vertexNormals);
                    this->vertex_push (sc, this->vertexColors);

                    if (j == segments - 1) {
                        // Last vertex is back to the start
                        this->indices.push_back (ringStartIdx++);
                        this->indices.push_back (this->idx);
                        this->indices.push_back (lastRingStartIdx);
                        this->indices.push_back (lastRingStartIdx);
                        this->indices.push_back (this->idx++);
                        this->indices.push_back (lastRingStartIdx+segments);
                    } else {
                        this->indices.push_back (ringStartIdx++);
                        this->indices.push_back (this->idx);
                        this->indices.push_back (ringStartIdx);
                        this->indices.push_back (ringStartIdx);
                        this->indices.push_back (this->idx++);
                        this->indices.push_back (this->idx);
                    }
                }
                lastRingStartIdx += segments;
            }

            // bottom cap
            rings0 = sm::mathconst<float>::pi_over_2;
            _z0  = std::sin(rings0);
            z0  = r * _z0;
            r0 =  std::cos(rings0);
            // Push the central point of the bottom cap
            this->vertex_push (so[0]+0.0f, so[1]+0.0f, so[2]+z0, this->vertexPositions);
            this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
            this->vertex_push (sc, this->vertexColors);
            capMiddle = this->idx++;
            firstseg = true;
            // No more vertices to push, just do the indices for the bottom cap
            ringStartIdx = lastRingStartIdx;
            for (int j = 0; j < segments; j++) {
                if (j != segments - 1) {
                    this->indices.push_back (capMiddle);
                    this->indices.push_back (ringStartIdx++);
                    this->indices.push_back (ringStartIdx);
                } else {
                    // Last segment
                    this->indices.push_back (capMiddle);
                    this->indices.push_back (ringStartIdx);
                    this->indices.push_back (lastRingStartIdx);
                }
            }
        } // end of sphere calculation

        /*!
         * Sphere, two colour version.
         *
         * Code for creating a sphere as part of this model. I'll use a sphere at the
         * centre of the arrows.
         *
         * \param so The sphere offset. Where to place this sphere...
         * \param sc The sphere colour.
         * \param sc2 The sphere's second colour - used for cap and first ring
         * \param r Radius of the sphere
         * \param rings Number of rings used to render the sphere
         * \param segments Number of segments used to render the sphere
         */
        void computeSphere (sm::vec<float> so, std::array<float, 3> sc, std::array<float, 3> sc2,
                            float r = 1.0f, int rings = 10, int segments = 12)
        {
            // First cap, draw as a triangle fan, but record indices so that
            // we only need a single call to glDrawElements.
            float rings0 = -sm::mathconst<float>::pi_over_2;
            float _z0  = std::sin(rings0);
            float z0  = r * _z0;
            float r0 =  std::cos(rings0);
            float rings1 = sm::mathconst<float>::pi * (-0.5f + 1.0f / rings);
            float _z1 = std::sin(rings1);
            float z1 = r * _z1;
            float r1 = std::cos(rings1);
            // Push the central point
            this->vertex_push (so[0]+0.0f, so[1]+0.0f, so[2]+z0, this->vertexPositions);
            this->vertex_push (0.0f, 0.0f, -1.0f, this->vertexNormals);
            this->vertex_push (sc2, this->vertexColors);

            GLuint capMiddle = this->idx++;
            GLuint ringStartIdx = this->idx;
            GLuint lastRingStartIdx = this->idx;

            bool firstseg = true;
            for (int j = 0; j < segments; j++) {
                float segment = sm::mathconst<float>::two_pi * static_cast<float>(j) / segments;
                float x = std::cos(segment);
                float y = std::sin(segment);

                float _x1 = x*r1;
                float x1 = _x1*r;
                float _y1 = y*r1;
                float y1 = _y1*r;

                this->vertex_push (so[0]+x1, so[1]+y1, so[2]+z1, this->vertexPositions);
                this->vertex_push (_x1, _y1, _z1, this->vertexNormals);
                this->vertex_push (sc2, this->vertexColors);

                if (!firstseg) {
                    this->indices.push_back (capMiddle);
                    this->indices.push_back (this->idx-1);
                    this->indices.push_back (this->idx++);
                } else {
                    this->idx++;
                    firstseg = false;
                }
            }
            this->indices.push_back (capMiddle);
            this->indices.push_back (this->idx-1);
            this->indices.push_back (capMiddle+1);

            // Now add the triangles around the rings
            for (int i = 2; i < rings; i++) {

                rings0 = sm::mathconst<float>::pi * (-0.5f + static_cast<float>(i) / rings);
                _z0  = std::sin(rings0);
                z0  = r * _z0;
                r0 =  std::cos(rings0);

                for (int j = 0; j < segments; j++) {

                    // "current" segment
                    float segment = sm::mathconst<float>::two_pi * static_cast<float>(j) / segments;
                    float x = std::cos(segment);
                    float y = std::sin(segment);

                    // One vertex per segment
                    float _x0 = x*r0;
                    float x0 = _x0*r;
                    float _y0 = y*r0;
                    float y0 = _y0*r;

                    // NB: Only add ONE vertex per segment. ALREADY have the first ring!
                    this->vertex_push (so[0]+x0, so[1]+y0, so[2]+z0, this->vertexPositions);
                    // The vertex normal of a vertex that makes up a sphere is
                    // just a normal vector in the direction of the vertex.
                    this->vertex_push (_x0, _y0, _z0, this->vertexNormals);
                    if (i == 2 || i > (rings-2)) {
                        this->vertex_push (sc2, this->vertexColors);
                    } else {
                        this->vertex_push (sc, this->vertexColors);
                    }
                    if (j == segments - 1) {
                        // Last vertex is back to the start
                        this->indices.push_back (ringStartIdx++);
                        this->indices.push_back (this->idx);
                        this->indices.push_back (lastRingStartIdx);
                        this->indices.push_back (lastRingStartIdx);
                        this->indices.push_back (this->idx++);
                        this->indices.push_back (lastRingStartIdx+segments);
                    } else {
                        this->indices.push_back (ringStartIdx++);
                        this->indices.push_back (this->idx);
                        this->indices.push_back (ringStartIdx);
                        this->indices.push_back (ringStartIdx);
                        this->indices.push_back (this->idx++);
                        this->indices.push_back (this->idx);
                    }
                }
                lastRingStartIdx += segments;
            }

            // bottom cap
            rings0 = sm::mathconst<float>::pi_over_2;
            _z0  = std::sin(rings0);
            z0  = r * _z0;
            r0 =  std::cos(rings0);
            // Push the central point of the bottom cap
            this->vertex_push (so[0]+0.0f, so[1]+0.0f, so[2]+z0, this->vertexPositions);
            this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
            this->vertex_push (sc2, this->vertexColors);
            capMiddle = this->idx++;
            firstseg = true;
            // No more vertices to push, just do the indices for the bottom cap
            ringStartIdx = lastRingStartIdx;
            for (int j = 0; j < segments; j++) {
                if (j != segments - 1) {
                    this->indices.push_back (capMiddle);
                    this->indices.push_back (ringStartIdx++);
                    this->indices.push_back (ringStartIdx);
                } else {
                    // Last segment
                    this->indices.push_back (capMiddle);
                    this->indices.push_back (ringStartIdx);
                    this->indices.push_back (lastRingStartIdx);
                }
            }
        }

        /*!
         * Compute an ellipsoid surface of the form x*x/a*a + y*y/b*b + z*z/c*c = 1
         *
         * so is the offset position for the ellipsoid
         *
         * sc is the colour at one end of the z axis
         *
         * sc2 is the colour at the other end
         *
         * abc are the three ellipsoid parameters
         *
         * rings is the number of rings along the z axis
         *
         * segments is the number of segments in each ring along the z axis
         *
         * tr transform matrix to apply to each point in the ellipse (applied before so is applied
         * as a transform)
         */
        void computeEllipsoid (sm::vec<float> so,
                               std::array<float, 3> sc,
                               std::array<float, 3> sc2,
                               sm::vec<float> abc,
                               int rings = 10, int segments = 12,
                               sm::mat<float, 4> tr = sm::mat<float, 4>{})
        {
            // We have two angular parameters t and t2. t in range 0-2pi and t2 in range 0-pi. t
            // gives the 'xy' ellipse; t2 gives the change in size of the xy ellipse as the z axis
            // is traversed.
            float t = 0.0f;
            float t2 = 0.0f;
            // used computing normals
            sm::vec<float> two_over_abcsq = 2.0f / abc.sq();
            // Holding the coordinates of each point on the ellipsoid as we compute it
            sm::vec<float> p = {};
            // The normal vector
            sm::vec<float> n = {};

            // sm::vec versions of the colours
            sm::vec<float> _sc = {};
            _sc.set_from (sc);
            sm::vec<float> _sc2 = {};
            _sc2.set_from (sc2);

            // Push the central point
            p = { 0, 0, abc[2] };
            this->vertex_push (so + (tr * p).less_one_dim(), this->vertexPositions);
            n = { 0, 0, 1 };
            this->vertex_push (n, this->vertexNormals);
            this->vertex_push (_sc, this->vertexColors);

            GLuint capMiddle = this->idx++;
            GLuint ringStartIdx = this->idx;
            GLuint lastRingStartIdx = this->idx;

            t2 = sm::mathconst<float>::pi / rings;
            p[2] = abc[2] * std::cos(t2);

            bool firstseg = true;
            for (int j = 0; j < segments; j++) {

                t = sm::mathconst<float>::two_pi * static_cast<float>(j) / segments;
                p[0] = abc[0] * std::cos(t) * std::sin(t2);
                p[1] = abc[1] * std::sin(t) * std::sin(t2);

                sm::vec<> pp = (tr * p).less_one_dim();
                this->vertex_push (so + pp, this->vertexPositions);

                n = (tr * (p * two_over_abcsq)).less_one_dim();
                n.renormalize();
                this->vertex_push (n, this->vertexNormals);

                sm::vec<float> sc_ring = _sc * (1.0f - 1.0f / rings)  + _sc2 * 1.0f / rings;
                this->vertex_push (sc_ring, this->vertexColors);

                if (!firstseg) {
                    this->indices.push_back (capMiddle);
                    this->indices.push_back (this->idx-1);
                    this->indices.push_back (this->idx++);
                } else {
                    this->idx++;
                    firstseg = false;
                }
            }
            this->indices.push_back (capMiddle);
            this->indices.push_back (this->idx-1);
            this->indices.push_back (capMiddle+1);

            // Now add the triangles around the rings
            for (int i = 2; i < rings; i++) {

                t2 = sm::mathconst<float>::pi * (static_cast<float>(i) / rings);
                p[2] = abc[2] * std::cos(t2);

                sm::vec<float> sc_ring = _sc * (1.0f - static_cast<float>(i) / rings)  + _sc2 * static_cast<float>(i) / rings;

                for (int j = 0; j < segments; j++) {

                    // "current" segment
                    t = sm::mathconst<float>::two_pi * static_cast<float>(j) / segments;
                    p[0] = abc[0] * std::cos(t) * std::sin(t2);
                    p[1] = abc[1] * std::sin(t) * std::sin(t2);

                    // NB: Only add ONE vertex per segment. ALREADY have the first ring!
                    sm::vec<> pp = (tr * p).less_one_dim();
                    this->vertex_push (so + pp, this->vertexPositions);
                    // The vertex normal of a vertex that makes up a sphere is
                    // just a normal vector in the direction of the vertex.
                    n = (tr * (p * two_over_abcsq)).less_one_dim();
                    n.renormalize();
                    this->vertex_push (n, this->vertexNormals);

                    this->vertex_push (sc_ring, this->vertexColors);

                    if (j == segments - 1) {
                        // Last vertex is back to the start
                        this->indices.push_back (ringStartIdx++);
                        this->indices.push_back (this->idx);
                        this->indices.push_back (lastRingStartIdx);
                        this->indices.push_back (lastRingStartIdx);
                        this->indices.push_back (this->idx++);
                        this->indices.push_back (lastRingStartIdx+segments);
                    } else {
                        this->indices.push_back (ringStartIdx++);
                        this->indices.push_back (this->idx);
                        this->indices.push_back (ringStartIdx);
                        this->indices.push_back (ringStartIdx);
                        this->indices.push_back (this->idx++);
                        this->indices.push_back (this->idx);
                    }
                }
                lastRingStartIdx += segments;
            }

            // bottom cap

            // Push the central point of the bottom cap
            p = { 0, 0, -abc[2] };
            this->vertex_push (so + (tr * p).less_one_dim(), this->vertexPositions);
            n = { 0, 0, -1 };
            this->vertex_push (n, this->vertexNormals);
            this->vertex_push (_sc2, this->vertexColors);
            capMiddle = this->idx++;
            firstseg = true;
            // No more vertices to push, just do the indices for the bottom cap
            ringStartIdx = lastRingStartIdx;
            for (int j = 0; j < segments; j++) {
                if (j != segments - 1) {
                    this->indices.push_back (capMiddle);
                    this->indices.push_back (ringStartIdx++);
                    this->indices.push_back (ringStartIdx);
                } else {
                    // Last segment
                    this->indices.push_back (capMiddle);
                    this->indices.push_back (ringStartIdx);
                    this->indices.push_back (lastRingStartIdx);
                }
            }
        }

        /*!
         * Compute vertices for an icosahedron.
         */
        void computeIcosahedron (sm::vec<float> centre,
                                 std::array<std::array<float, 3>, 20> face_colours,
                                 float r = 1.0f) // radius or side length?
        {
            sm::geometry::polyhedron<float> ico = sm::geometry::icosahedron<float>();

            for (int j = 0; j < 20; ++j) {
                // Compute the face normal
                sm::vec<float, 3> norml = (ico.vertices[ico.faces[j][0]] + ico.vertices[ico.faces[j][1]] + ico.vertices[ico.faces[j][2]])/3.0f;
                this->vertex_push (centre + (ico.vertices[ico.faces[j][0]] * r), this->vertexPositions);
                this->vertex_push (centre + (ico.vertices[ico.faces[j][1]] * r), this->vertexPositions);
                this->vertex_push (centre + (ico.vertices[ico.faces[j][2]] * r), this->vertexPositions);
                for (int i = 0; i < 3; ++i) {
                    this->vertex_push (norml, this->vertexNormals);
                    this->vertex_push (face_colours[j], this->vertexColors);
                }
                // Indices...
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+1);
                this->indices.push_back (this->idx+2);
                this->idx += 3;
            }
        }

        /*!
         * Create a cone.
         *
         * \param centre The centre of the cone - would be the end of the line
         *
         * \param tip The tip of the cone
         *
         * \param ringoffset Move the ring forwards or backwards along the vector from
         * \a centre to \a tip. This is positive or negative proportion of tip - centre.
         *
         * \param col The cone colour
         *
         * \param r Radius of the ring
         *
         * \param segments Number of segments used to render the tube
         */
        void computeCone (sm::vec<float> centre,
                          sm::vec<float> tip,
                          float ringoffset,
                          std::array<float, 3> col,
                          float r = 1.0f, int segments = 12)
        {
            // Cone is drawn as a base ring around a centre-of-the-base vertex, an
            // intermediate ring which is on the base ring, but has different normals, a
            // 'ring' around the tip (with suitable normals) and a 'tip' vertex

            sm::vec<float> vbase = centre;
            sm::vec<float> vtip = tip;
            sm::vec<float> v = vtip - vbase;
            v.renormalize();

            // circle in a plane defined by a point and a normal
            sm::vec<float> rand_vec;
            rand_vec.randomize();
            sm::vec<float> inplane = rand_vec.cross(v);
            inplane.renormalize();
            sm::vec<float> v_x_inplane = v.cross(inplane);

            // Push the central point of the start cap - this is at location vstart
            this->vertex_push (vbase, this->vertexPositions);
            this->vertex_push (-v, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            // Base ring with normals in direction -v
            for (int j = 0; j < segments; j++) {
                float t = j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = inplane * std::sin(t) * r + v_x_inplane * std::cos(t) * r;
                // Subtract the vector which makes this circle
                c = c + (v * ringoffset);
                this->vertex_push (vbase+c, this->vertexPositions);
                this->vertex_push (-v, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);
            }

            // Intermediate ring of vertices around/aligned with the base ring with normals in direction c
            for (int j = 0; j < segments; j++) {
                float t = j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = inplane * std::sin(t) * r + v_x_inplane * std::cos(t) * r;
                c = c + (v * ringoffset);
                this->vertex_push (vbase+c, this->vertexPositions);
                c.renormalize();
                this->vertex_push (c, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);
            }

            // Intermediate ring of vertices around the tip with normals direction c
            for (int j = 0; j < segments; j++) {
                float t = j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c = inplane * std::sin(t) * r + v_x_inplane * std::cos(t) * r;
                c = c + (v * ringoffset);
                this->vertex_push (vtip, this->vertexPositions);
                c.renormalize();
                this->vertex_push (c, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);
            }

            // Push tip vertex as the last vertex, normal is in direction v
            this->vertex_push (vtip, this->vertexPositions);
            this->vertex_push (v, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            // Number of vertices = segments * 3 + 2.
            int nverts = segments * 3 + 2;

            // After creating vertices, push all the indices.
            GLuint capMiddle = this->idx;
            GLuint capStartIdx = this->idx + 1;
            GLuint endMiddle = this->idx + static_cast<GLuint>(nverts) - 1u;
            GLuint endStartIdx = capStartIdx;

            // Base of the cone
            for (int j = 0; j < segments-1; j++) {
                this->indices.push_back (capMiddle);
                this->indices.push_back (capStartIdx + j);
                this->indices.push_back (capStartIdx + 1 + j);
            }
            // Last tri of base
            this->indices.push_back (capMiddle);
            this->indices.push_back (capStartIdx + segments - 1);
            this->indices.push_back (capStartIdx);

            // Middle sections
            for (int lsection = 0; lsection < 2; ++lsection) {
                capStartIdx = this->idx + 1 + lsection*segments;
                endStartIdx = capStartIdx + segments;
                for (int j = 0; j < segments; j++) {
                    // Triangle 1:
                    this->indices.push_back (capStartIdx + j);
                    if (j == (segments-1)) {
                        this->indices.push_back (capStartIdx);
                    } else {
                        this->indices.push_back (capStartIdx + 1 + j);
                    }
                    this->indices.push_back (endStartIdx + j);
                    // Triangle 2:
                    this->indices.push_back (endStartIdx + j);
                    if (j == (segments-1)) {
                        this->indices.push_back (endStartIdx);
                    } else {
                        this->indices.push_back (endStartIdx + 1 + j);
                    }
                    if (j == (segments-1)) {
                        this->indices.push_back (capStartIdx);
                    } else {
                        this->indices.push_back (capStartIdx + j + 1);
                    }
                }
            }

            // tip
            for (int j = 0; j < segments-1; j++) {
                this->indices.push_back (endMiddle);
                this->indices.push_back (endStartIdx + j);
                this->indices.push_back (endStartIdx + 1 + j);
            }
            // Last triangle of tip
            this->indices.push_back (endMiddle);
            this->indices.push_back (endStartIdx + segments - 1);
            this->indices.push_back (endStartIdx);

            // Update idx
            this->idx += nverts;
        } // end of cone calculation

        //! Compute a line with a single colour
        void computeLine (sm::vec<float> start, sm::vec<float> end,
                          sm::vec<float> _uz,
                          std::array<float, 3> col,
                          float w = 0.1f, float thickness = 0.01f, float shorten = 0.0f)
        {
            this->computeLine (start, end, _uz, col, col, w, thickness, shorten);
        }

        /*!
         * Create a line from \a start to \a end, with width \a w and a colour which
         * transitions from the colour \a colStart to \a colEnd. The thickness of the
         * line in the z direction is \a thickness
         *
         * \param start The start of the tube
         * \param end The end of the tube
         * \param _uz Dirn of z (up) axis for end face of line. Should be normalized.
         * \param colStart The tube staring colour
         * \param colEnd The tube's ending colour
         * \param w width of line
         * \param thickness The thickness/depth of the line in uy direction
         * \param shorten An amount by which to shorten the length of the line at each end.
         */
        void computeLine (sm::vec<float> start, sm::vec<float> end,
                          sm::vec<float> _uz,
                          std::array<float, 3> colStart, std::array<float, 3> colEnd,
                          float w = 0.1f, float thickness = 0.01f, float shorten = 0.0f)
        {
            // There are always 8 segments for this line object, 2 at each of 4 corners
            const int segments = 8;

            // The vector from start to end defines direction of the tube
            sm::vec<float> vstart = start;
            sm::vec<float> vend = end;
            sm::vec<float> v = vend - vstart;
            v.renormalize();

            // If shorten is not 0, then modify vstart and vend
            if (shorten > 0.0f) {
                vstart = start + v * shorten;
                vend = end - v * shorten;
            }

            // vv is normal to v and _uz
            sm::vec<float> vv = v.cross(_uz);
            vv.renormalize();

            // Push the central point of the start cap - this is at location vstart
            this->vertex_push (vstart, this->vertexPositions);
            this->vertex_push (-v, this->vertexNormals);
            this->vertex_push (colStart, this->vertexColors);

            // Compute the 'face angles' that will give the correct width and thickness for the line
            std::array<float, 8> angles;
            float w_ = w * 0.5f;
            float d_ = thickness * 0.5f;
            float r = std::sqrt (w_ * w_ + d_ * d_);
            angles[0] = std::acos (w_ / r);
            angles[1] = angles[0];
            angles[2] = sm::mathconst<float>::pi - angles[0];
            angles[3] = angles[2];
            angles[4] = sm::mathconst<float>::pi + angles[0];
            angles[5] = angles[4];
            angles[6] = sm::mathconst<float>::two_pi - angles[0];
            angles[7] = angles[6];
            // The normals for the vertices around the line
            std::array<sm::vec<float>, 8> norms = { vv, _uz, _uz, -vv, -vv, -_uz, -_uz, vv };

            // Start cap vertices (a triangle fan)
            for (int j = 0; j < segments; j++) {
                sm::vec<float> c = _uz * std::sin(angles[j]) * r + vv * std::cos(angles[j]) * r;
                this->vertex_push (vstart+c, this->vertexPositions);
                this->vertex_push (-v, this->vertexNormals);
                this->vertex_push (colStart, this->vertexColors);
            }

            // Intermediate, near start cap. Normals point outwards. Need Additional vertices
            for (int j = 0; j < segments; j++) {
                sm::vec<float> c = _uz * std::sin(angles[j]) * r + vv * std::cos(angles[j]) * r;
                this->vertex_push (vstart+c, this->vertexPositions);
                this->vertex_push (norms[j], this->vertexNormals);
                this->vertex_push (colStart, this->vertexColors);
            }

            // Intermediate, near end cap. Normals point in direction c
            for (int j = 0; j < segments; j++) {
                sm::vec<float> c = _uz * std::sin(angles[j]) * r + vv * std::cos(angles[j]) * r;
                this->vertex_push (vend+c, this->vertexPositions);
                this->vertex_push (norms[j], this->vertexNormals);
                this->vertex_push (colEnd, this->vertexColors);
            }

            // Bottom cap vertices
            for (int j = 0; j < segments; j++) {
                sm::vec<float> c = _uz * std::sin(angles[j]) * r + vv * std::cos(angles[j]) * r;
                this->vertex_push (vend+c, this->vertexPositions);
                this->vertex_push (v, this->vertexNormals);
                this->vertex_push (colEnd, this->vertexColors);
            }

            // Bottom cap. Push centre vertex as the last vertex.
            this->vertex_push (vend, this->vertexPositions);
            this->vertex_push (v, this->vertexNormals);
            this->vertex_push (colEnd, this->vertexColors);

            // Number of vertices = segments * 4 + 2.
            int nverts = (segments * 4) + 2;

            // After creating vertices, push all the indices.
            GLuint capMiddle = this->idx;
            GLuint capStartIdx = this->idx + 1u;
            GLuint endMiddle = this->idx + static_cast<GLuint>(nverts) - 1u;
            GLuint endStartIdx = capStartIdx + (3u * segments);

            // Start cap indices
            for (int j = 0; j < segments-1; j++) {
                this->indices.push_back (capMiddle);
                this->indices.push_back (capStartIdx + j);
                this->indices.push_back (capStartIdx + 1 + j);
            }
            // Last one
            this->indices.push_back (capMiddle);
            this->indices.push_back (capStartIdx + segments - 1);
            this->indices.push_back (capStartIdx);

            // Middle sections
            for (int lsection = 0; lsection < 3; ++lsection) {
                capStartIdx = this->idx + 1 + lsection*segments;
                endStartIdx = capStartIdx + segments;
                for (int j = 0; j < segments; j++) {
                    this->indices.push_back (capStartIdx + j);
                    if (j == (segments-1)) {
                        this->indices.push_back (capStartIdx);
                    } else {
                        this->indices.push_back (capStartIdx + 1 + j);
                    }
                    this->indices.push_back (endStartIdx + j);
                    this->indices.push_back (endStartIdx + j);
                    if (j == (segments-1)) {
                        this->indices.push_back (endStartIdx);
                    } else {
                        this->indices.push_back (endStartIdx + 1 + j);
                    }
                    if (j == (segments-1)) {
                        this->indices.push_back (capStartIdx);
                    } else {
                        this->indices.push_back (capStartIdx + j + 1);
                    }
                }
            }

            // bottom cap
            for (int j = 0; j < segments-1; j++) {
                this->indices.push_back (endMiddle);
                this->indices.push_back (endStartIdx + j);
                this->indices.push_back (endStartIdx + 1 + j);
            }
            this->indices.push_back (endMiddle);
            this->indices.push_back (endStartIdx + segments - 1);
            this->indices.push_back (endStartIdx);

            // Update idx
            this->idx += nverts;
        } // end computeLine

        // Like computeLine, but this line has no thickness.
        void computeFlatLine (sm::vec<float> start, sm::vec<float> end,
                              sm::vec<float> _uz,
                              std::array<float, 3> col,
                              float w = 0.1f, float shorten = 0.0f)
        {
            // The vector from start to end defines direction of the tube
            sm::vec<float> vstart = start;
            sm::vec<float> vend = end;
            sm::vec<float> v = vend - vstart;
            v.renormalize();

            // If shorten is not 0, then modify vstart and vend
            if (shorten > 0.0f) {
                vstart = start + v * shorten;
                vend = end - v * shorten;
            }

            // vv is normal to v and _uz
            sm::vec<float> vv = v.cross(_uz);
            vv.renormalize();

            // corners of the line, and the start angle is determined from vv and w
            sm::vec<float> ww = vv * w * 0.5f;
            sm::vec<float> c1 = vstart + ww;
            sm::vec<float> c2 = vstart - ww;
            sm::vec<float> c3 = vend - ww;
            sm::vec<float> c4 = vend + ww;

            this->vertex_push (c1, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->vertex_push (c2, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->vertex_push (c3, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->vertex_push (c4, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            // Number of vertices = segments * 4 + 2.
            int nverts = 4;

            // After creating vertices, push all the indices.
            this->indices.push_back (this->idx);
            this->indices.push_back (this->idx+1);
            this->indices.push_back (this->idx+2);

            this->indices.push_back (this->idx);
            this->indices.push_back (this->idx+2);
            this->indices.push_back (this->idx+3);

            // Update idx
            this->idx += nverts;

        } // end computeFlatLine

        // Like computeFlatLine but with option to add rounded start/end caps (I lazily
        // draw a whole circle around start/end to achieve this, rather than figuring
        // out a semi-circle).
        void computeFlatLineRnd (sm::vec<float> start, sm::vec<float> end,
                                 sm::vec<float> _uz,
                                 std::array<float, 3> col,
                                 float w = 0.1f, float shorten = 0.0f, bool startcaps = true, bool endcaps = true)
        {
            // The vector from start to end defines direction of the tube
            sm::vec<float> vstart = start;
            sm::vec<float> vend = end;
            sm::vec<float> v = vend - vstart;
            v.renormalize();

            // If shorten is not 0, then modify vstart and vend
            if (shorten > 0.0f) {
                vstart = start + v * shorten;
                vend = end - v * shorten;
            }

            // vv is normal to v and _uz
            sm::vec<float> vv = v.cross(_uz);
            vv.renormalize();

            // corners of the line, and the start angle is determined from vv and w
            sm::vec<float> ww = vv * w * 0.5f;
            sm::vec<float> c1 = vstart + ww;
            sm::vec<float> c2 = vstart - ww;
            sm::vec<float> c3 = vend - ww;
            sm::vec<float> c4 = vend + ww;

            int segments = 12;
            float r = 0.5f * w;
            unsigned int startvertices = 0u;
            if (startcaps) {
                // Push the central point of the start cap - this is at location vstart
                this->vertex_push (vstart, this->vertexPositions);
                this->vertex_push (_uz, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);
                ++startvertices;
                // Start cap vertices (a triangle fan)
                for (int j = 0; j < segments; j++) {
                    float t = j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                    sm::vec<float> c = { std::sin(t) * r, std::cos(t) * r, 0.0f };
                    this->vertex_push (vstart+c, this->vertexPositions);
                    this->vertex_push (_uz, this->vertexNormals);
                    this->vertex_push (col, this->vertexColors);
                    ++startvertices;
                }
            }

            this->vertex_push (c1, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->vertex_push (c2, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->vertex_push (c3, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->vertex_push (c4, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            unsigned int endvertices = 0u;
            if (endcaps) {
                // Push the central point of the end cap - this is at location vend
                this->vertex_push (vend, this->vertexPositions);
                this->vertex_push (_uz, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);
                ++endvertices;
                // End cap vertices (a triangle fan)
                for (int j = 0; j < segments; j++) {
                    float t = j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                    sm::vec<float> c = { std::sin(t) * r, std::cos(t) * r, 0.0f };
                    this->vertex_push (vend+c, this->vertexPositions);
                    this->vertex_push (_uz, this->vertexNormals);
                    this->vertex_push (col, this->vertexColors);
                    ++endvertices;
                }
            }

            // After creating vertices, push all the indices.

            if (startcaps) { // prolly startcaps, for flexibility
                GLuint topcap = this->idx;
                for (int j = 0; j < segments; j++) {
                    int inc1 = 1+j;
                    int inc2 = 1+((j+1)%segments);
                    this->indices.push_back (topcap);
                    this->indices.push_back (topcap+inc1);
                    this->indices.push_back (topcap+inc2);
                }
                this->idx += startvertices;
            }

            // The line itself
            this->indices.push_back (this->idx);
            this->indices.push_back (this->idx+1);
            this->indices.push_back (this->idx+2);
            this->indices.push_back (this->idx);
            this->indices.push_back (this->idx+2);
            this->indices.push_back (this->idx+3);
            // Update idx
            this->idx += 4;

            if (endcaps) {
                GLuint botcap = this->idx;
                for (int j = 0; j < segments; j++) {
                    int inc1 = 1+j;
                    int inc2 = 1+((j+1)%segments);
                    this->indices.push_back (botcap);
                    this->indices.push_back (botcap+inc1);
                    this->indices.push_back (botcap+inc2);
                }
                this->idx += endvertices;
            }
        } // end computeFlatLine

        /*!
         * Like computeFlatLine, but this line has no thickness and you can provide the
         * previous and next data points so that this line, the previous line and the
         * next line can line up perfectly without drawing a circular rounded 'end cap'!
         *
         * This code assumes that the coordinates prev, start, end, next all lie on a 2D
         * plane normal to _uz. In fact, the 3D coordinates start, end, prev and next
         * will all be projected onto the plane defined by _uz, so that they can be
         * reduced to 2D coordinates. This then allows crossing points of lines to be
         * computed.
         *
         * If you want to make a ribbon between points that do *not* lie on a 2D plane,
         * you'll need to write another graphics primitive function.
         */
        void computeFlatLine (sm::vec<float> start, sm::vec<float> end,
                              sm::vec<float> prev, sm::vec<float> next,
                              sm::vec<float> _uz,
                              std::array<float, 3> col,
                              float w = 0.1f)
        {
            // Corner coordinates for this line section
            sm::vec<float> c1 = { 0.0f };
            sm::vec<float> c2 = { 0.0f };
            sm::vec<float> c3 = { 0.0f };
            sm::vec<float> c4 = { 0.0f };

            // Ensure _uz is a unit vector
            sm::vec<float> __uz = _uz;
            __uz.renormalize();

            // First find the rotation to make __uz into the actual unit z dirn
            sm::quaternion<float> rotn;
            sm::vec<float> basis_rotn_axis = __uz.cross (sm::vec<>::uz());
            if (basis_rotn_axis.length() > 0.0f) {
                float basis_rotn_angle = __uz.angle (sm::vec<>::uz(), basis_rotn_axis);
                rotn.rotate (basis_rotn_axis, basis_rotn_angle);
            } // else nothing to do  - basis rotn is null

            // Transform so that start is the origin
            // sm::vec<float> s_o = { 0.0f }; // by defn
            sm::vec<float> e_o = end - start;
            sm::vec<float> p_o = prev - start;
            sm::vec<float> n_o = next - start;

            // Apply basis rotation just to the end point. e_b: 'end point in rotated basis'
            sm::vec<float> e_b = rotn * e_o;

            // Use the vector from start to end as the in-plane x dirn. Do this AFTER
            // first coord rotn.  In other words: find the rotation about the new unit z
            // direction to force the end point to be on the x axis
            sm::vec<float> plane_x = e_b; // - s_b but s_b is (0,0,0) by defn
            plane_x.renormalize();
            sm::vec<float> plane_y = sm::vec<>::uz().cross (plane_x);
            plane_y.renormalize();
            // Find the in-plane coordinates in the rotated plane system
            sm::vec<float> e_p = { plane_x.dot (e_b), plane_y.dot (e_b), sm::vec<>::uz().dot (e_b) };

            // One epsilon is exacting
            if (std::abs(e_p[2]) > std::numeric_limits<float>::epsilon()) {
                throw std::runtime_error ("uz not orthogonal to the line start -> end?");
            }

            // From e_p and e_b (which should both be in a 2D plane) figure out what
            // angle of rotation brings e_b into the x axis
            float inplane_rotn_angle = e_b.angle (e_p, sm::vec<>::uz());
            sm::quaternion<float> inplane_rotn (sm::vec<>::uz(), inplane_rotn_angle);

            // Apply the in-plane rotation to the basis rotation
            rotn.premultiply (inplane_rotn);

            // Transform points
            sm::vec<float> p_p = rotn * p_o;
            sm::vec<float> n_p = rotn * n_o;
            //vec<float> s_p = rotn * s_o; // not necessary, s_p = (0,0,0) by defn

            // Line crossings time.
            sm::vec<float, 2> c1_p = { 0.0f }; // 2D crossing coords that we're going to find
            sm::vec<float, 2> c2_p = { 0.0f };
            sm::vec<float, 2> c3_p = e_p.less_one_dim();
            sm::vec<float, 2> c4_p = e_p.less_one_dim();

            // 3 lines on each side. l_p, l_c (current) and l_n. Each has two ends. l_p_1, l_p_2 etc.

            // 'prev' 'cur' and 'next' vectors
            sm::vec<float, 2> p_vec = (/*s_p*/ -p_p).less_one_dim();
            sm::vec<float, 2> c_vec = e_p.less_one_dim();
            sm::vec<float, 2> n_vec = (n_p - e_p).less_one_dim();

            sm::vec<float, 2> p_ortho = (/*s_p*/ - p_p).cross (sm::vec<>::uz()).less_one_dim();
            p_ortho.renormalize();
            sm::vec<float, 2> c_ortho = (e_p /*- s_p*/).cross (sm::vec<>::uz()).less_one_dim();
            c_ortho.renormalize();
            sm::vec<float, 2> n_ortho = (n_p - e_p).cross (sm::vec<>::uz()).less_one_dim();
            n_ortho.renormalize();

            const float hw = w / 2.0f;

            sm::vec<float, 2> l_p_1 = p_p.less_one_dim() + (p_ortho * hw) - p_vec; // makes it 3 times as long as the line.
            sm::vec<float, 2> l_p_2 = /*s_p.less_one_dim() +*/ (p_ortho * hw) + p_vec;
            sm::vec<float, 2> l_c_1 = /*s_p.less_one_dim() +*/ (c_ortho * hw) - c_vec;
            sm::vec<float, 2> l_c_2 = e_p.less_one_dim() + (c_ortho * hw) + c_vec;
            sm::vec<float, 2> l_n_1 = e_p.less_one_dim() + (n_ortho * hw) - n_vec;
            sm::vec<float, 2> l_n_2 = n_p.less_one_dim() + (n_ortho * hw) + n_vec;

            std::bitset<2> isect = sm::geometry::segments_intersect<float> (l_p_1, l_p_2, l_c_1, l_c_2);
            if (isect.test(0) == true && isect.test(1) == false) { // test for intersection but not colinear
                c1_p = sm::geometry::crossing_point (l_p_1, l_p_2, l_c_1, l_c_2);
            } else if (isect.test(0) == true && isect.test(1) == true) {
                c1_p = /*s_p.less_one_dim() +*/ (c_ortho * hw);
            } else { // no intersection. prev could have been start
                c1_p = /*s_p.less_one_dim() +*/ (c_ortho * hw);
            }
            isect = sm::geometry::segments_intersect<float> (l_c_1, l_c_2, l_n_1, l_n_2);
            if (isect.test(0) == true && isect.test(1) == false) {
                c4_p = sm::geometry::crossing_point (l_c_1, l_c_2, l_n_1, l_n_2);
            } else if (isect.test(0) == true && isect.test(1) == true) {
                c4_p = e_p.less_one_dim() + (c_ortho * hw);
            } else { // no intersection, prev could have been end
                c4_p = e_p.less_one_dim() + (c_ortho * hw);
            }

            // o for 'other side'. Could re-use vars in future version. Or just subtract (*_ortho * w) from each.
            sm::vec<float, 2> o_l_p_1 = p_p.less_one_dim() - (p_ortho * hw) - p_vec; // makes it 3 times as long as the line.
            sm::vec<float, 2> o_l_p_2 = /*s_p.less_one_dim()*/ - (p_ortho * hw) + p_vec;
            sm::vec<float, 2> o_l_c_1 = /*s_p.less_one_dim()*/ - (c_ortho * hw) - c_vec;
            sm::vec<float, 2> o_l_c_2 = e_p.less_one_dim() - (c_ortho * hw) + c_vec;
            sm::vec<float, 2> o_l_n_1 = e_p.less_one_dim() - (n_ortho * hw) - n_vec;
            sm::vec<float, 2> o_l_n_2 = n_p.less_one_dim() - (n_ortho * hw) + n_vec;

            isect = sm::geometry::segments_intersect<float> (o_l_p_1, o_l_p_2, o_l_c_1, o_l_c_2);
            if (isect.test(0) == true && isect.test(1) == false) { // test for intersection but not colinear
                c2_p = sm::geometry::crossing_point (o_l_p_1, o_l_p_2, o_l_c_1, o_l_c_2);
            } else if (isect.test(0) == true && isect.test(1) == true) {
                c2_p = /*s_p.less_one_dim()*/ - (c_ortho * hw);
            } else { // no intersection. prev could have been start
                c2_p = /*s_p.less_one_dim()*/ - (c_ortho * hw);
            }

            isect = sm::geometry::segments_intersect<float> (o_l_c_1, o_l_c_2, o_l_n_1, o_l_n_2);
            if (isect.test(0) == true && isect.test(1) == false) {
                c3_p = sm::geometry::crossing_point (o_l_c_1, o_l_c_2, o_l_n_1, o_l_n_2);
            } else if (isect.test(0) == true && isect.test(1) == true) {
                c3_p = e_p.less_one_dim() - (c_ortho * hw);
            } else { // no intersection. next could have been end
                c3_p = e_p.less_one_dim() - (c_ortho * hw);
            }

            // Transform and rotate back into c1-c4
            sm::quaternion<float> rotn_inv = rotn.invert();
            c1 = rotn_inv * c1_p.plus_one_dim() + start;
            c2 = rotn_inv * c2_p.plus_one_dim() + start;
            c3 = rotn_inv * c3_p.plus_one_dim() + start;
            c4 = rotn_inv * c4_p.plus_one_dim() + start;

            // Now create the vertices from these four corners, c1-c4
            this->vertex_push (c1, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->vertex_push (c2, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->vertex_push (c3, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->vertex_push (c4, this->vertexPositions);
            this->vertex_push (_uz, this->vertexNormals);
            this->vertex_push (col, this->vertexColors);

            this->indices.push_back (this->idx);
            this->indices.push_back (this->idx+1);
            this->indices.push_back (this->idx+2);

            this->indices.push_back (this->idx);
            this->indices.push_back (this->idx+2);
            this->indices.push_back (this->idx+3);

            // Update idx
            this->idx += 4;
        } // end computeFlatLine that joins perfectly

        //! Make a joined up line with previous.
        void computeFlatLineP (sm::vec<float> start, sm::vec<float> end,
                               sm::vec<float> prev,
                               sm::vec<float> _uz,
                               std::array<float, 3> col,
                               float w = 0.1f)
        {
            this->computeFlatLine (start, end, prev, end, _uz, col, w);
        } // end computeFlatLine that joins perfectly with prev

        //! Flat line, joining up with next
        void computeFlatLineN (sm::vec<float> start, sm::vec<float> end,
                               sm::vec<float> next,
                               sm::vec<float> _uz,
                               std::array<float, 3> col,
                               float w = 0.1f)
        {
            this->computeFlatLine (start, end, start, next, _uz, col, w);
        }

        // Like computeLine, but this line has no thickness and it's dashed.
        // dashlen: the length of dashes
        // gap prop: The proportion of dash length used for the gap
        void computeFlatDashedLine (sm::vec<float> start, sm::vec<float> end,
                                    sm::vec<float> _uz,
                                    std::array<float, 3> col,
                                    float w = 0.1f, float shorten = 0.0f,
                                    float dashlen = 0.1f, float gapprop = 0.3f)
        {
            if (dashlen == 0.0f) { return; }

            // The vector from start to end defines direction of the line
            sm::vec<float> vstart = start;
            sm::vec<float> vend = end;

            sm::vec<float> v = vend - vstart;
            float linelen = v.length();
            v.renormalize();

            // If shorten is not 0, then modify vstart and vend
            if (shorten > 0.0f) {
                vstart = start + v * shorten;
                vend = end - v * shorten;
                linelen = v.length() - shorten * 2.0f;
            }

            // vv is normal to v and _uz
            sm::vec<float> vv = v.cross(_uz);
            vv.renormalize();

            // Loop, creating the dashes
            sm::vec<float> dash_s = vstart;
            sm::vec<float> dash_e = dash_s + v * dashlen;
            sm::vec<float> dashes = dash_e - vstart;

            while (dashes.length() < linelen) {

                // corners of the line, and the start angle is determined from vv and w
                sm::vec<float> ww = vv * w * 0.5f;
                sm::vec<float> c1 = dash_s + ww;
                sm::vec<float> c2 = dash_s - ww;
                sm::vec<float> c3 = dash_e - ww;
                sm::vec<float> c4 = dash_e + ww;

                this->vertex_push (c1, this->vertexPositions);
                this->vertex_push (_uz, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);

                this->vertex_push (c2, this->vertexPositions);
                this->vertex_push (_uz, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);

                this->vertex_push (c3, this->vertexPositions);
                this->vertex_push (_uz, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);

                this->vertex_push (c4, this->vertexPositions);
                this->vertex_push (_uz, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);

                // Number of vertices = segments * 4 + 2.
                int nverts = 4;

                // After creating vertices, push all the indices.
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+1);
                this->indices.push_back (this->idx+2);

                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+2);
                this->indices.push_back (this->idx+3);

                // Update idx
                this->idx += nverts;

                // Next dash
                dash_s = dash_e + v * dashlen * gapprop;
                dash_e = dash_s + v * dashlen;
                dashes = dash_e - vstart;
            }

        } // end computeFlatDashedLine

        // Compute a flat line circle outline
        void computeFlatCircleLine (sm::vec<float> centre, sm::vec<float> norm, sm::vec<float> inplane, float radius,
                                    float linewidth, std::array<float, 3> col, int segments = 128)
        {
            inplane.renormalize();
            sm::vec<float> norm_x_inplane = norm.cross(inplane);

            float half_lw = linewidth / 2.0f;
            float r_in = radius - half_lw;
            float r_out = radius + half_lw;
            // Inner ring at radius radius-linewidth/2 with normals in direction norm;
            // Outer ring at radius radius+linewidth/2 with normals also in direction norm
            for (int j = 0; j < segments; j++) {
                float t = j * sm::mathconst<float>::two_pi / static_cast<float>(segments);
                sm::vec<float> c_in = inplane * std::sin(t) * r_in + norm_x_inplane * std::cos(t) * r_in;
                this->vertex_push (centre+c_in, this->vertexPositions);
                this->vertex_push (norm, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);
                sm::vec<float> c_out = inplane * std::sin(t) * r_out + norm_x_inplane * std::cos(t) * r_out;
                this->vertex_push (centre+c_out, this->vertexPositions);
                this->vertex_push (norm, this->vertexNormals);
                this->vertex_push (col, this->vertexColors);
            }
            // Added 2*segments vertices to vertexPositions

            // After creating vertices, push all the indices.
            for (int j = 0; j < segments; j++) {
                int jn = (segments + ((j+1) % segments)) % segments;
                this->indices.push_back (this->idx+(2*j));
                this->indices.push_back (this->idx+(2*jn));
                this->indices.push_back (this->idx+(2*jn+1));
                this->indices.push_back (this->idx+(2*j));
                this->indices.push_back (this->idx+(2*jn+1));
                this->indices.push_back (this->idx+(2*j+1));
            }
            this->idx += 2 * segments; // nverts

        } // end computeFlatCircleLine

        // Compute a flat line circle outline
        void computeFlatCircleLine (sm::vec<float> centre, sm::vec<float> norm, float radius,
                                    float linewidth, std::array<float, 3> col, int segments = 128)
        {
            // circle in a plane defined by a point (v0 = vstart or vend) and a normal
            // (v) can be found: Choose random vector vr. A vector inplane = vr ^ v. The
            // unit in-plane vector is inplane.normalise. Can now use that vector in the
            // plan to define a point on the circle. Note that this starting point on
            // the circle is at a random position, which means that this version of
            // computeFlatCircleLine is useful for tubes that have quite a few segments.
            sm::vec<float> rand_vec;
            rand_vec.randomize();
            sm::vec<float> inplane = rand_vec.cross(norm);
            // Sub call to the method that takes a normal vector AND an inplane vector:
            this->computeFlatCircleLine (centre, norm, inplane, radius, linewidth, col, segments);
        }

        // Compute triangles to form a true cuboid from 8 corners.
        void computeCuboid (const std::array<sm::vec<float>, 8>& v, const std::array<float, 3>& clr)
        {
            this->computeFlatQuad (v[0], v[1], v[2], v[3], clr);
            this->computeFlatQuad (v[0], v[4], v[5], v[1], clr);
            this->computeFlatQuad (v[1], v[5], v[6], v[2], clr);
            this->computeFlatQuad (v[2], v[6], v[7], v[3], clr);
            this->computeFlatQuad (v[3], v[7], v[4], v[0], clr);
            this->computeFlatQuad (v[7], v[6], v[5], v[4], clr);
        }

        // Compute a rhombus using the four defining coordinates. The coordinates are named as if
        // they were the origin, x, y and z of a right-handed 3D coordinate system. These define three edges
        void computeRhombus (const sm::vec<float>& o, const sm::vec<float>& x, const sm::vec<float>& y, const sm::vec<float>& z,
                             const std::array<float, 3>& clr)
        {
            // Edge vectors
            sm::vec<float> edge1 = x - o;
            sm::vec<float> edge2 = y - o;
            sm::vec<float> edge3 = z - o;

            // Compute the face normals
            sm::vec<float> _n1 = edge1.cross (edge2);
            _n1.renormalize();
            sm::vec<float> _n2 = edge2.cross (edge3);
            _n2.renormalize();
            sm::vec<float> _n3 = edge1.cross (edge3);
            _n3.renormalize();

            // Push positions and normals for 24 vertices to make up the rhombohedron; 4 for each face.
            // Front face
            this->vertex_push (o,                        this->vertexPositions);
            this->vertex_push (o + edge1,                this->vertexPositions);
            this->vertex_push (o + edge3,                this->vertexPositions);
            this->vertex_push (o + edge1 + edge3,        this->vertexPositions);
            for (unsigned short i = 0U; i < 4U; ++i) { this->vertex_push (_n3, this->vertexNormals); }
            // Top face
            this->vertex_push (o + edge3,                 this->vertexPositions);
            this->vertex_push (o + edge1 + edge3,         this->vertexPositions);
            this->vertex_push (o + edge2 + edge3,         this->vertexPositions);
            this->vertex_push (o + edge2 + edge1 + edge3, this->vertexPositions);
            for (unsigned short i = 0U; i < 4U; ++i) { this->vertex_push (_n1, this->vertexNormals); }
            // Back face
            this->vertex_push (o + edge2 + edge3,         this->vertexPositions);
            this->vertex_push (o + edge2 + edge1 + edge3, this->vertexPositions);
            this->vertex_push (o + edge2,                 this->vertexPositions);
            this->vertex_push (o + edge2 + edge1,         this->vertexPositions);
            for (unsigned short i = 0U; i < 4U; ++i) { this->vertex_push (-_n3, this->vertexNormals); }
            // Bottom face
            this->vertex_push (o + edge2,                 this->vertexPositions);
            this->vertex_push (o + edge2 + edge1,         this->vertexPositions);
            this->vertex_push (o,                         this->vertexPositions);
            this->vertex_push (o + edge1,                 this->vertexPositions);
            for (unsigned short i = 0U; i < 4U; ++i) { this->vertex_push (-_n1, this->vertexNormals); }
            // Left face
            this->vertex_push (o + edge2,                 this->vertexPositions);
            this->vertex_push (o,                         this->vertexPositions);
            this->vertex_push (o + edge2 + edge3,         this->vertexPositions);
            this->vertex_push (o + edge3,                 this->vertexPositions);
            for (unsigned short i = 0U; i < 4U; ++i) { this->vertex_push (-_n2, this->vertexNormals); }
            // Right face
            this->vertex_push (o + edge1,                 this->vertexPositions);
            this->vertex_push (o + edge1 + edge2,         this->vertexPositions);
            this->vertex_push (o + edge1 + edge3,         this->vertexPositions);
            this->vertex_push (o + edge1 + edge2 + edge3, this->vertexPositions);
            for (unsigned short i = 0U; i < 4U; ++i) { this->vertex_push (_n2, this->vertexNormals); }

            // Vertex colours are all the same
            for (unsigned short i = 0U; i < 24U; ++i) { this->vertex_push (clr, this->vertexColors); }

            // Indices for 6 faces
            for (unsigned short i = 0U; i < 6U; ++i) {
                this->indices.push_back (this->idx++);
                this->indices.push_back (this->idx++);
                this->indices.push_back (this->idx--);
                this->indices.push_back (this->idx++);
                this->indices.push_back (this->idx++);
                this->indices.push_back (this->idx++);
            }
        } // computeCuboid

        // Compute a rectangular cuboid of width (in x), height (in y) and depth (in z).
        void computeRectCuboid (const sm::vec<float>& o, const float wx, const float hy, const float dz,
                                const std::array<float, 3>& clr)
        {
            sm::vec<float> px = o + sm::vec<float>{wx, 0, 0};
            sm::vec<float> py = o + sm::vec<float>{0, hy, 0};
            sm::vec<float> pz = o + sm::vec<float>{0, 0, dz};
            this->computeRhombus (o, px, py, pz, clr);
        }

        // Compute the bounding box frame
        void computeBoundingBox()
        {
            // Draw a frame of tubes from bb.min to bb.max
            const float& x0 = this->bb.min[0];
            const float& y0 = this->bb.min[1];
            const float& z0 = this->bb.min[2];

            const float& x1 = this->bb.max[0];
            const float& y1 = this->bb.max[1];
            const float& z1 = this->bb.max[2];

            const sm::vec<float>& c0 = this->bb.min;
            sm::vec<float> c1 = { x1, y0, z0 };
            sm::vec<float> c2 = { x1, y1, z0 };
            sm::vec<float> c3 = { x0, y1, z0 };

            sm::vec<float> c4 = { x0, y0, z1 };
            sm::vec<float> c5 = { x1, y0, z1 };
            const sm::vec<float>& c6 = this->bb.max;
            sm::vec<float> c7 = { x0, y1, z1 };

            constexpr int segs = 4;
            constexpr float zrot = 0.0f;
            auto cl = this->colour_bb;

            // Frame tube radius
            float r = this->bb.span().length() / 500.0f;

            // Base
            this->computeTube (c0, c1, sm::vec<float>::uy(), sm::vec<float>::uz(), cl, cl, r, segs, zrot, true);
            this->computeTube (c1, c2, -sm::vec<float>::ux(), sm::vec<float>::uz(), cl, cl, r, segs, zrot, true);
            this->computeTube (c2, c3, -sm::vec<float>::uy(), sm::vec<float>::uz(), cl, cl, r, segs, zrot, true);
            this->computeTube (c3, c0, sm::vec<float>::ux(), sm::vec<float>::uz(), cl, cl, r, segs, zrot, true);
            // Top
            this->computeTube (c4, c5, sm::vec<float>::uy(), sm::vec<float>::uz(), cl, cl, r, segs, zrot, true);
            this->computeTube (c5, c6, -sm::vec<float>::ux(), sm::vec<float>::uz(), cl, cl, r, segs, zrot, true);
            this->computeTube (c6, c7, -sm::vec<float>::uy(), sm::vec<float>::uz(), cl, cl, r, segs, zrot, true);
            this->computeTube (c7, c4, sm::vec<float>::ux(), sm::vec<float>::uz(), cl, cl, r, segs, zrot, true);
            // Sides
            this->computeTube (c0, c4, sm::vec<float>::uy(), -sm::vec<float>::ux(), cl, cl, r, segs, zrot, true);
            this->computeTube (c1, c5, sm::vec<float>::uy(), -sm::vec<float>::ux(), cl, cl, r, segs, zrot, true);
            this->computeTube (c2, c6, sm::vec<float>::uy(), -sm::vec<float>::ux(), cl, cl, r, segs, zrot, true);
            this->computeTube (c3, c7, sm::vec<float>::uy(), -sm::vec<float>::ux(), cl, cl, r, segs, zrot, true);
        }
    };

} // namespace mplot
