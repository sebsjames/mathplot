/*!
 * \file
 *
 * Declares a base class for VisualTextModels.
 *
 * \author Seb James
 * \date March 2025
 */

#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <functional>

#include <mplot/gl/version.h>

import sm.quaternion;
import sm.mat;
import sm.vec;
#include <sm/mathconst>

#include <mplot/VisualCommon.h>
#include <mplot/unicode.h>
#include <mplot/TextGeometry.h>
#include <mplot/TextFeatures.h>
#include <mplot/colour.h>

namespace mplot
{
    //! Forward declaration of a VisualBase class
    template <int> class VisualBase;

    /*!
     * This is the base class for VisualTextModel containing common code, but no GL function calls.
     */
    template <int glver = mplot::gl::version_4_1>
    struct VisualTextModelBase
    {
        //! Pass just the TextFeatures. parentVis, tshader etc, accessed by callbacks
        VisualTextModelBase (mplot::TextFeatures _tfeatures)
        {
            this->tfeatures = _tfeatures;
            this->fontscale = tfeatures.fontsize / static_cast<float>(tfeatures.fontres);
        }

        virtual ~VisualTextModelBase() {}

        //! Render the VisualTextModel
        virtual void render() = 0;

        //! Set clr_text to a value suitable to be visible on the background colour bgcolour
        void setVisibleOn (const std::array<float, 4>& bgcolour)
        {
            constexpr float factor = 0.85f;
            this->clr_text = {1.0f - bgcolour[0] * factor, 1.0f - bgcolour[1] * factor, 1.0f - bgcolour[2] * factor};
        }

        //! Setter for VisualTextModel::viewmatrix, the model view
        void setViewMatrix (const sm::mat<float, 4>& mv) { this->viewmatrix = mv; }

        //! Setter for VisualTextModel::scenematrix, the scene view
        void setSceneMatrix (const sm::mat<float, 4>& sv) { this->scenematrix = sv; }

        //! Set the translation specified by \a v0 into the scene translation
        template <std::size_t N = 3> requires (N == 3) || (N == 4)
        void setSceneTranslation (const sm::vec<float, N>& v0)
        {
            this->scenematrix.set_identity();
            this->scenematrix.translate (v0);
        }

        //! Set a translation (only) into the scene view matrix
        template <std::size_t N = 3> requires (N == 3) || (N == 4)
        void addSceneTranslation (const sm::vec<float, N>& v0) { this->scenematrix.pretranslate (v0); }

        //! Set a rotation (only) into the scene view matrix
        void setSceneRotation (const sm::quaternion<float>& r)
        {
            auto _offset = this->scenematrix.translation();
            this->scenematrix.set_identity();
            this->scenematrix.translate (_offset);
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
        void addViewTranslation (const sm::vec<float>& v0) { this->viewmatrix.pretranslate (v0); }

        //! Set a rotation (only) into the model view matrix
        void setViewRotation (const sm::quaternion<float>& r)
        {
            auto tr = this->viewmatrix.translation();
            this->viewmatrix.set_identity();
            this->viewmatrix.translate (tr);
            this->viewmatrix.rotate (r);
        }

        //! Apply a further rotation to the model view matrix
        void addViewRotation (const sm::quaternion<float>& r) { this->viewmatrix.rotate (r); }

        //! Compute the geometry for a sample text.
        virtual mplot::TextGeometry getTextGeometry (const std::string& _txt) = 0;

        //! Return the geometry for the stored txt
        virtual mplot::TextGeometry getTextGeometry() = 0;

        float width() const { return this->extents[1] - this->extents[0]; }
        float height() const { return this->extents[3] - this->extents[2]; }

        std::string getText() const
        {
            std::string s = {};
            for (auto c : txt) { s += unicode::toUtf8 (c); }
            return s;
        }

        std::string debugText() const
        {
            std::stringstream ss;
            for (auto c : txt) { ss << unicode::toUtf8 (c); }
            ss << "--->\n"
               << "parent_rotation= " << this->parent_rotation << "\n"
               << "viewmatrix=\n" << this->viewmatrix << "\n"
               << "scenematrix=\n" << this->scenematrix << "\n"
               << "----------------------\n";
            return ss.str();
        }

    protected:
        static constexpr bool debug_textquads = false;

        //! Initialize the vertices that will represent the Quads.
        void initializeVertices() {

            unsigned int nquads = static_cast<unsigned int>(this->quads.size());

            for (unsigned int qi = 0; qi < nquads; ++qi) {

                std::array<float, 12> quad = this->quads[qi];

                if constexpr (debug_textquads == true) {
                    std::cout << "Quad box from (" << quad[0] << "," << quad[1] << "," << quad[2]
                              << ") to (" << quad[3] << "," << quad[4] << "," << quad[5]
                              << ") to (" << quad[6] << "," << quad[7] << "," << quad[8]
                              << ") to (" << quad[9] << "," << quad[10] << "," << quad[11] << ")" << std::endl;
                }

                this->vertex_push (quad[0], quad[1],  quad[2],  this->vertexPositions); //1
                this->vertex_push (quad[3], quad[4],  quad[5],  this->vertexPositions); //2
                this->vertex_push (quad[6], quad[7],  quad[8],  this->vertexPositions); //3
                this->vertex_push (quad[9], quad[10], quad[11], this->vertexPositions); //4

                // Add the info for drawing the textures on the quads
                this->vertex_push (0.0f, 1.0f, 0.0f, this->vertexTextures);
                this->vertex_push (0.0f, 0.0f, 0.0f, this->vertexTextures);
                this->vertex_push (1.0f, 0.0f, 0.0f, this->vertexTextures);
                this->vertex_push (1.0f, 1.0f, 0.0f, this->vertexTextures);

                // All same colours
                this->vertex_push (this->clr_backing, this->vertexColors);
                this->vertex_push (this->clr_backing, this->vertexColors);
                this->vertex_push (this->clr_backing, this->vertexColors);
                this->vertex_push (this->clr_backing, this->vertexColors);

                // All same normals
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);

                // Two triangles per quad
                // qi * 4 + 1, 2 3 or 4
                GLuint ib = (GLuint)qi*4;
                this->indices.push_back (ib++); // 0
                this->indices.push_back (ib++); // 1
                this->indices.push_back (ib);   // 2

                this->indices.push_back (ib++); // 2
                this->indices.push_back (ib);   // 3
                ib -= 3;
                this->indices.push_back (ib);   // 0
            }
        }

        //! Common code to call after the vertices have been set up.
        virtual void postVertexInit() = 0;

    public:
        // A VisualTextModel may be given a name
        std::string name = "VisualTextModel";

        //! The colour of the text
        std::array<float, 3> clr_text = {0.0f, 0.0f, 0.0f};
        //! Line spacing, in multiples of the height of an 'h'
        float line_spacing = 1.4f;
        //! Parent Visual
        mplot::VisualBase<glver>* parentVis = nullptr;

        /*!
         * Callbacks are analogous to those in VisualModel
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

        //! SSBOs are unused in VisualTextModels, but these functions have to be present
        std::function<unsigned int(mplot::VisualBase<glver>*, const unsigned int)> init_instance_data;
        std::function<void(const unsigned int, const sm::vec<float, 3>&)> insert_instance_data;
        std::function<void(const unsigned int, const std::array<float, 3>&, const float, const float)> insert_instparam_data;
        std::function<void(mplot::VisualBase<glver>*)> instanced_needs_update;

        //! Setter for the parent pointer, parentVis
        void set_parent (mplot::VisualBase<glver>* _vis)
        {
            //if (this->parentVis != nullptr) { throw std::runtime_error ("VisualTextModel: Set the parent pointer once only!"); }
            this->parentVis = _vis;
        }

    protected:
        // The text features for this VisualTextModel
        mplot::TextFeatures tfeatures;

        // face is in derived class

        //! The colour of the backing quad's vertices. Doesn't have any effect.
        std::array<float, 3> clr_backing = {1.0f, 1.0f, 0.0f};

        //! A scaling factor based on the desired width of an 'm'
        float fontscale = 1.0f; //  fontscale = tfeatures.fontsize/(float)tfeatures.fontres;

        //! A rotation of the parent model
        sm::quaternion<float> parent_rotation = {};

        //! The text-model-specific view matrix and a scene matrix
        sm::mat<float, 4> viewmatrix = {};
        //! Before, I wrote: We protect the scene matrix as updating it with the parent
        //! model's scene matrix likely involves also adding an additional
        //! translation. Now, I'm still slightly confused as to whether I *need* to have a
        //! copy of the scenematrix *here*.
        sm::mat<float, 4> scenematrix = {};

        //! The text string stored for debugging
        std::basic_string<char32_t> txt;
        //! The Quads that form the 'medium' for the text textures. 12 float = 4 corners
        std::vector<std::array<float,12>> quads = {};
        //! left, right, top and bottom extents of the text for this
        //! VisualTextModel. setupText should modify these as it sets up quads. Order of
        //! numbers is left, right, bottom, top
        sm::vec<float, 4> extents = { 1e7, -1e7, 1e7, -1e7 };
        //! The texture ID for each quad - so that we draw the right texture image over each quad.
        std::vector<unsigned int> quad_ids = {};
        //! Position within vertex buffer object (if I use an array of VBO)
        enum VBOPos { posnVBO, normVBO, colVBO, idxVBO, textureVBO, numVBO };
        //! The OpenGL Vertex Array Object
        GLuint vao = 0;
        //! Single vbo to use as in example
        GLuint vbo = 0;
        //! Vertex Buffer Objects stored in an array
        std::unique_ptr<GLuint[]> vbos;
        //! CPU-side data for indices
        std::vector<GLuint> indices = {};
        //! CPU-side data for quad vertex positions
        std::vector<float> vertexPositions = {};
        //! CPU-side data for quad vertex normals
        std::vector<float> vertexNormals = {};
        //! CPU-side data for vertex colours
        std::vector<float> vertexColors = {};
        //! data for textures
        std::vector<float> vertexTextures = {};
        //! A model-wide alpha value for the shader
        float alpha = 1.0f;
        //! If true, then calls to VisualModel::render should return
        bool hide = false;

        //! Set up a vertex buffer object - bind, buffer and set vertex array object attribute
        virtual void setupVBO (GLuint& buf, std::vector<float>& dat, unsigned int bufferAttribPosition) = 0;

        //! Push three floats onto the vector of floats \a vp
        void vertex_push (const float& x, const float& y, const float& z, std::vector<float>& vp)
        {
            vp.push_back (x);
            vp.push_back (y);
            vp.push_back (z);
        }
        //! Push array of 3 floats onto the vector of floats \a vp
        void vertex_push (const std::array<float, 3>& arr, std::vector<float>& vp)
        {
            vp.push_back (arr[0]);
            vp.push_back (arr[1]);
            vp.push_back (arr[2]);
        }
        //! Push mplot::vec of 3 floats onto the vector of floats \a vp
        void vertex_push (const sm::vec<float>& vec, std::vector<float>& vp)
        {
            std::copy (vec.begin(), vec.end(), std::back_inserter (vp));
        }
    };

} // namespace mplot
