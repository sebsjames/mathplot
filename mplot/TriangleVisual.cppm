module;

#include <cstdint>
#include <array>
#include <iostream>

export module mplot.trianglevisual;

import sm.vec;
import mplot.visualmodel;

export namespace mplot
{
    //! This class creates the vertices for a simple triangle in a 3D scene.
    template<std::int32_t glver = mplot::gl::version_4_1>
    class TriangleVisual : public VisualModel<glver>
    {
    public:
        TriangleVisual() {}

        //! Initialise with offset, three coordinates and a single colour.
        TriangleVisual(const sm::vec<float, 3> _offset,
                       const sm::vec<float, 3> _coord1, const sm::vec<float, 3> _coord2, const sm::vec<float, 3> _coord3,
                       const std::array<float, 3> _col)
        {
            this->init<3u> (_offset, _coord1, _coord2, _coord3, _col);
        }

        TriangleVisual(const sm::vec<float, 3> _offset,
                       const sm::vec<float, 3> _coord1, const sm::vec<float, 3> _coord2, const sm::vec<float, 3> _coord3,
                       const std::array<float, 4> _col)
        {
            this->init<4u> (_offset, _coord1, _coord2, _coord3, _col);
        }

        template <std::uint32_t NC = 3u> requires (NC == 3u) || (NC == 4u)
        void init (const sm::vec<float, 3> _offset,
                   const sm::vec<float, 3> _coord1, const sm::vec<float, 3> _coord2, const sm::vec<float, 3> _coord3,
                   const std::array<float, NC> _col)
        {
            this->ncol = NC;
            // Set up...
            this->viewmatrix.translate (_offset);
            this->coord1 = _coord1;
            this->coord2 = _coord2;
            this->coord3 = _coord3;
            if constexpr (NC == 3u) {
                this->col = { _col[0], _col[1], _col[2] };
            } else {
                this->col = _col;
            }
        }

        //! Compute a triangle from 3 arbitrary corners
        void computeTriangle (sm::vec<float> c1, sm::vec<float> c2, sm::vec<float> c3, std::array<float, 4> colr)
        {
            // v is the face normal
            sm::vec<float> u1 = c1 - c2;
            sm::vec<float> u2 = c2 - c3;
            sm::vec<float> v = u1.cross (u2);
            v.renormalize();
            // Push corner vertices
            this->vertex_push (c1, this->vertexPositions);
            this->vertex_push (c2, this->vertexPositions);
            this->vertex_push (c3, this->vertexPositions);
            // Colours/normals
            for (std::uint32_t i = 0; i < 3U; ++i) {
                this->vertex_push_colour (colr, this->vertexColors);
                this->vertex_push (v, this->vertexNormals);
            }
            this->indices.push_back (this->idx++);
            this->indices.push_back (this->idx++);
            this->indices.push_back (this->idx++);
        }

        //! Initialize vertex buffer objects and vertex array object.
        void initializeVertices()
        {
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();

            // Draw a triangle. That's it.
            this->computeTriangle (this->coord1, this->coord2, this->coord3, this->col);
        }

        //! The position of the vertices of the triangle
        sm::vec<float, 3> coord1 = {0.0f, 0.0f, 0.0f};
        sm::vec<float, 3> coord2 = {0.0f, 0.0f, 0.0f};
        sm::vec<float, 3> coord3 = {0.0f, 0.0f, 0.0f};

        //! The colour of the triangle
        std::array<float, 4> col = { 0.0f, 0.0f, 1.0f, 1.0f };
    };

} // namespace mplot
