#pragma once

/*
 * Common code for GL functionality in mathplot programs.
 *
 * Author: Seb James.
 */

#include <stdexcept>
#include <iostream>
#include <cstring>
#include <string>
#include <array>

import sm.vec;
import sm.range;
import sm.vvec;
import sm.mat;

#include <mplot/tools.h>
#include <mplot/colour.h>

namespace mplot
{
    // A very simple mesh struct. No textures, materials or owt
    struct meshgroup
    {
        std::string name;
        sm::mat<float, 4> transform;
        sm::vvec<uint32_t> indices;
        sm::vvec<sm::vec<float>> positions;
        sm::vvec<sm::vec<float>> normals;
        sm::vvec<sm::vec<float>> colours;
        sm::range<sm::vec<float>> object_aabb;
        sm::range<sm::vec<float>> world_aabb;
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

namespace mplot::visgl
{
    // A container struct for the shader program identifiers used in a mplot::Visual. Separate
    // from mplot::Visual so that it can be used in mplot::VisualModel as well, which does not
    // #include mplot/Visual.h.
    struct visual_shaderprogs
    {
        //! An OpenGL shader program for graphical objects
        unsigned int /*GLuint*/ gprog = 0;
        //! A text shader program, which uses textures to draw text on quads.
        unsigned int /*GLuint*/ tprog = 0;
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
        unsigned int textureID;
        //! Size of glyph
        sm::vec<int,2>  size;
        //! Offset from baseline to left/top of glyph
        sm::vec<int,2>  bearing;
        //! Offset to advance to next glyph
        unsigned int advance;
    };
} // namespace
