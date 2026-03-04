#pragma once

#include <array>
#include <sm/mathconst>
import sm.vec;
#include <mplot/VisualModel.h>

namespace mplot
{
    //! This class creates the vertices for a cylindrical 'rod' in a 3D scene.
    template<int glver = mplot::gl::version_4_1>
    class RodVisual : public VisualModel<glver>
    {
    public:
        RodVisual() {}

        //! Initialise with offset, start and end coordinates, radius and a single colour.
        RodVisual(const sm::vec<float, 3> _offset,
                  const sm::vec<float, 3> _start_coord, const sm::vec<float, 3> _end_coord, const float _radius,
                  const std::array<float, 3> _col)
        {
            this->init (_offset, _start_coord, _end_coord, _radius, _col, _col);
        }

        //! Initialise with offset, start and end coordinates, radius and start and end colours.
        RodVisual(const sm::vec<float, 3> _offset,
                  const sm::vec<float, 3> _start_coord, const sm::vec<float, 3> _end_coord, const float _radius,
                  const std::array<float, 3> _start_col, const std::array<float, 3> _end_col)
        {
            this->init (_offset, _start_coord, _end_coord, _radius, _start_col, _end_col);
        }

        ~RodVisual () {}

        void init (const sm::vec<float, 3> _offset,
                   const sm::vec<float, 3> _start_coord, const sm::vec<float, 3> _end_coord, const float _radius,
                   const std::array<float, 3> _start_col, const std::array<float, 3> _end_col)
        {
            // Set up...
            this->viewmatrix.translate (_offset);

            this->start_coord = _start_coord;
            this->end_coord = _end_coord;
            this->radius = _radius;
            this->start_col = _start_col;
            this->end_col = _end_col;
        }

        //! Initialize vertex buffer objects and vertex array object.
        void initializeVertices()
        {
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();

            // Draw a tube. That's it!
            if (use_oriented_tube == false) {
                this->computeTube (this->start_coord, this->end_coord,
                                   this->start_col, this->end_col, this->radius, 12);
            } else {
                // Can alternatively use the 'oriented' tube
                this->computeTube (this->start_coord, this->end_coord,
                                   face_uy, face_uz,
                                   this->start_col, this->end_col, this->radius, 6, sm::mathconst<float>::pi_over_6);
            }
        }

        template <std::size_t N = 3> requires (N == 3) || (N == 4)
        void update (const sm::vec<float, N>& s, const sm::vec<float, N>& e)
        {
            for (std::size_t i = 0; i < 3; ++i) {
                this->start_coord[i] = s[i];
                this->end_coord[i] = e[i];
            }
            this->reinit();
        }

        //! The position of the start of the rod, given with respect to the parent's offset
        sm::vec<float, 3> start_coord = {0.0f, 0.0f, 0.0f};
        //! The position of the end of the rod, given with respect to the parent's offset
        sm::vec<float, 3> end_coord = {1.0f, 0.0f, 0.0f};
        //! The radius of the rod
        float radius = 1.0f;
        //! If true, use face_uz and face_uy to draw the tube, else get a square-ended tube
        bool use_oriented_tube = true;
        //! Face directions for the oriented tube's *end* cap. Choose carefully so that face_uy ^
        //! face_uz gives the normal for the end cap.
        sm::vec<float, 3> face_uy = sm::vec<float, 3>::uy();
        sm::vec<float, 3> face_uz = sm::vec<float, 3>::uz();

        //! The colours of the rod.
        std::array<float, 3> start_col = {1.0f, 0.0f, 0.0f};
        std::array<float, 3> end_col = {0.0f, 0.0f, 1.0f};
    };

} // namespace mplot
