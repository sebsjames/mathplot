/*!
 * \file
 *
 * Defines a coordinate arrow class
 *
 * \author Seb James
 * \date 2019
 */
#pragma once

#include <array>
import sm.vec;
#include <mplot/gl/version.h>
#include <mplot/VisualModel.h>
#include <mplot/colour.h>

namespace mplot
{
    //! This class creates the vertices for a set of coordinate arrows to be rendered
    //! in a 3-D scene.
    template<int glver = mplot::gl::version_4_1>
    class CoordArrows : public VisualModel<glver>
    {
    public:
        CoordArrows() : mplot::VisualModel<glver>() {}
        CoordArrows (const sm::vec<float, 3>& offset) : mplot::VisualModel<glver>(offset) {}

        //! Must make the boilerplate bindmodel call before calling init() (for text handling)
        void init (const sm::vec<float, 3> _lengths, const float _thickness, const float _em)
        {
            this->compute_bb (false);
            this->lengths = _lengths;
            this->thickness = _thickness;
            this->em = _em;
        }

        //! You can call this AS well as the first init overload to set the axis vectors
        void init (const sm::vec<float, 3> _x, const sm::vec<float, 3> _y, const sm::vec<float, 3> _z)
        {
            this->compute_bb (false);
            this->x_axis = _x;
            this->y_axis = _y;
            this->z_axis = _z;
        }

        //! Make sure coord arrow colours are ok on the given background colour. Call this *after* finalize.
        void setColourForBackground (const std::array<float, 4>& bgcolour)
        {
            // For now, only worry about the centresphere:
            std::array<float, 3> cscol = { 1.0f - bgcolour[0], 1.0f - bgcolour[1], 1.0f - bgcolour[2] };
            if (cscol != this->centresphere_col) {
                this->centresphere_col = cscol;
                this->reinit(); // sets context, does not release it

                // Give the text labels a suitable, visible colour
                if (this->setContext != nullptr) { this->setContext (this->parentVis); }
                auto ti = this->texts.begin();
                while (ti != this->texts.end()) {
                    (*ti)->setVisibleOn (bgcolour);
                    ti++;
                }
                if (this->releaseContext != nullptr) { this->releaseContext (this->parentVis); }
            }
        }

        void initAxisLabels()
        {
            if (this->em > 0.0f) {

                if (this->setContext != nullptr) { this->setContext (this->parentVis); } // For VisualTextModel

                mplot::TextFeatures tfca(this->em, 48, false, mplot::colour::black, mplot::VisualFont::DVSansItalic);

                // These texts are black by default
                sm::vec<float> _offset = this->viewmatrix.translation();
                sm::vec<float> toffset = {};
                toffset = _offset + this->x_axis * this->lengths[0];
                toffset[0] += this->em;
                auto vtm1 = this->makeVisualTextModel (tfca);
                vtm1->setupText (this->x_label, toffset);
                this->texts.push_back (std::move(vtm1));
                toffset = _offset + this->y_axis * this->lengths[1];
                toffset[0] += this->em;
                auto vtm2 = this->makeVisualTextModel (tfca);
                vtm2->setupText (this->y_label, toffset);
                this->texts.push_back (std::move(vtm2));
                toffset = _offset + this->z_axis * this->lengths[2];
                toffset[0] += this->em;
                auto vtm3 = this->makeVisualTextModel (tfca);
                vtm3->setupText (this->z_label, toffset);
                this->texts.push_back (std::move(vtm3));

                if (this->releaseContext != nullptr) { this->releaseContext (this->parentVis); }
            }
        }

        //! Initialize vertex buffer objects and vertex array object.
        void initializeVertices()
        {
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();
            this->idx = 0;

            // Draw four spheres to make up the coord frame, with centre at 0,0,0
            sm::vec<float, 3> reloffset = {};
            static constexpr sm::vec<float, 3> zerocoord = { 0.0f, 0.0f, 0.0f };
            this->computeSphere (zerocoord, centresphere_col, this->thickness * this->lengths[0] / 20.0f);

            // x
            reloffset = this->x_axis * this->lengths[0];
            this->computeSphere (reloffset, x_axis_col, (this->thickness * this->lengths[0] / 40.0f) * endsphere_size);
            this->computeTube (zerocoord, reloffset, x_axis_col, x_axis_col, this->thickness * this->lengths[0] / 80.0f);
            if (showneg) {
                this->computeTube (zerocoord, -reloffset, x_axis_neg, x_axis_neg, this->thickness * this->lengths[0] / 80.0f);
            }

            // y
            reloffset = this->y_axis * this->lengths[1];
            this->computeSphere (reloffset, y_axis_col, (this->thickness * this->lengths[0] / 40.0f) * endsphere_size);
            this->computeTube (zerocoord, reloffset, y_axis_col, y_axis_col, this->thickness * this->lengths[0] / 80.0f);
            if (showneg) {
                this->computeTube (zerocoord, -reloffset, y_axis_neg, y_axis_neg, this->thickness * this->lengths[0] / 80.0f);
            }

            // z
            reloffset = this->z_axis * this->lengths[2];
            this->computeSphere (reloffset, z_axis_col, (this->thickness * this->lengths[0] / 40.0f) * endsphere_size);
            this->computeTube (zerocoord, reloffset, z_axis_col, z_axis_col, this->thickness * this->lengths[0] / 80.0f);
            if (showneg) {
                this->computeTube (zerocoord, -reloffset, z_axis_neg, z_axis_neg, this->thickness * this->lengths[0] / 80.0f);
            }

            this->initAxisLabels();
        }

        //! Length multipliers that can be applied to ux, uy and uz
        sm::vec<float, 3> lengths = { 1.0f, 1.0f, 1.0f };

        //! The axes for the coordinate arrows. A simple right handed coordinate system aligned with
        //! the 'real' world coordinate system by default.
        sm::vec<float, 3> x_axis = { 1.0f, 0.0f, 0.0f };
        sm::vec<float, 3> y_axis = { 0.0f, 1.0f, 0.0f };
        sm::vec<float, 3> z_axis = { 0.0f, 0.0f, 1.0f };

        //! A thickness scaling factor, to apply to the arrows.
        float thickness = 1.0f;
        //! a multiplier on the end spheres
        float endsphere_size = 1.0f;
        //! m size for text labels
        float em = 0.0f;

        //! The colours of the arrows, and of the centre sphere (where default of black is suitable
        //! for a white background)
        std::array<float, 3> centresphere_col = mplot::colour::black;
        std::array<float, 3> x_axis_col = mplot::colour::crimson;
        std::array<float, 3> y_axis_col = mplot::colour::springgreen2;
        std::array<float, 3> z_axis_col = mplot::colour::blue2;

        bool showneg = false;
        std::array<float, 3> x_axis_neg = mplot::colour::raspberry;
        std::array<float, 3> y_axis_neg = mplot::colour::darkseagreen3;
        std::array<float, 3> z_axis_neg = mplot::colour::steelblue3;

        std::string x_label = "X";
        std::string y_label = "Y";
        std::string z_label = "Z";
    };

} // namespace mplot
