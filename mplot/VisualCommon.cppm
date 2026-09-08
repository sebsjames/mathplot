/*
 * Common code for GL functionality in mathplot programs.
 *
 * Author: Seb James.
 */
module;

#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <string>
#include <array>

export module mplot.visualcommon;

import sm.vec;
import sm.interval;
import sm.vvec;
import sm.mat;
import mplot.colour;
import mplot.tools;

export namespace mplot
{
    // State/options flags for VisualModels
    enum class vm_bools : std::uint32_t
    {
        postVertexInitRequired,
        twodimensional,         // If true, then this VisualModel should always be viewed in a plane - it's a 2D model
        hide,                   // If true, then calls to VisualModel::render should return
        wireframe,              // If true, draw in GL's polygon GL_LINES mode (instead of GL_FILL)
        greyscale,              // If true, drain the colour from the model
        instanced,              // If true, draw this VisualModel with 'instancing' 1 or more times
        show_bb,                // If true, draw vertices/indices for the bounding box frame
        label_bb,               // If true, show the bounding box's name label
        compute_bb              // For some models, it's not useful to compute the bounding box (e.g. coordinate arrows)
    };

    // A very simple mesh struct. No textures, materials or owt
    struct meshgroup
    {
        std::string name;
        sm::mat<float, 4> transform;
        sm::vvec<std::uint32_t> indices;
        sm::vvec<sm::vec<float>> positions;
        sm::vvec<sm::vec<float>> normals;
        sm::vvec<sm::vec<float>> colours;
        sm::interval<sm::vec<float>> object_aabb;
        sm::interval<sm::vec<float>> world_aabb;
        // Single colour is used if colours is empty
        std::array<float, 3> single_colour = mplot::colour::grey50;
        void validate() const
        {
            if (this->positions.size() != this->normals.size()) {
                throw std::runtime_error ("meshgroup has different numbers of positions and normals");
            }
            if (!this->colours.empty() && this->colours.size() != this->positions.size()) {
                throw std::runtime_error ("meshgroup has different numbers of positions and colours");
            }
        }
    };
}

export namespace mplot::visgl
{
    // A container struct for the shader program identifiers used in a mplot::Visual. Separate
    // from mplot::Visual so that it can be used in mplot::VisualModel as well, which does not
    // #include mplot/Visual.h.
    struct visual_shaderprogs
    {
        //! An OpenGL shader program for graphical objects
        std::uint32_t gprog = 0;
        //! A text shader program, which uses textures to draw text on quads.
        std::uint32_t tprog = 0;
    };

    // This defines different graphics shader types, as used in mplot::Visual. The essential
    // difference between the current shaders is that they render different projection types
    enum class graphics_shader_type
    {
        none,         // Unset/unknown graphics shader type
        projection2d, // both orthographic and perspective projections to a 2D surface
        cylindrical,  // cylindrical projections. Used to be implemented, but removed for code simplicity
        spherical     // not implemented, but we could have a spherical projection
    };

    //! The locations for the position, normal and colour vertex attributes in the
    //! mplot::Visual GLSL programs
    enum AttribLocn { posnLoc = 0, normLoc = 1, colLoc = 2, textureLoc = 3 };

    //! A struct to hold information about font glyph properties
    struct CharInfo
    {
        //! ID handle of the glyph texture
        std::uint32_t textureID;
        //! Size of glyph
        sm::vec<std::int32_t, 2>  size;
        //! Offset from baseline to left/top of glyph
        sm::vec<std::int32_t, 2>  bearing;
        //! Offset to advance to next glyph
        std::uint32_t advance;
    };
} // namespace
