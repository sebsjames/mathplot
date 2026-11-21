module;

#include <cstdint>
#include <array>

export module mplot.rectanglevisual;

import sm.mathconst;
import sm.vec;
import sm.mat;
import mplot.visualmodel;

export namespace mplot
{
    //! This class creates the vertices for a simple flat rectangle in a 3D scene.
    template<std::int32_t glver = mplot::gl::version_4_1>
    class RectangleVisual : public VisualModel<glver>
    {
    public:
        RectangleVisual() {}

        //! Initialise with offset, three coordinates and a single colour.
        RectangleVisual(const sm::vec<float, 3> _offset,
                        const sm::vec<float, 2> _dims, const float _angle,
                        const std::array<float, 3> _col)
        {
            this->init (_offset, _dims, _angle, _col);
        }

        void init (const sm::vec<float, 3> _offset,
                   const sm::vec<float, 2> _dims, const float _angle,
                   const std::array<float, 3> _col)
        {
            this->viewmatrix.translate (_offset);
            this->dims = _dims;
            this->angle = _angle;
            this->col = _col;
        }

        void computeRectangle()
        {
            // Corners of the rectangle - make sure they're clockwise in order
            sm::vec<float, 2> c1 = dims;
            sm::vec<float, 2> c2 = { dims[0], -dims[1] };
            sm::vec<float, 2> c3 = -dims;
            sm::vec<float, 2> c4 = { -dims[0], dims[1] };

            c1 /= 2.0f;
            c2 /= 2.0f;
            c3 /= 2.0f;
            c4 /= 2.0f;

            // Apply rotational transformation
            sm::mat<float, 2> rotn = sm::mat<float, 2>::identity();
            rotn.rotate (this->angle * sm::mathconst<float>::deg2rad);
            c1 = rotn * c1;
            c2 = rotn * c2;
            c3 = rotn * c3;
            c4 = rotn * c4;

            this->computeFlatQuad (c1.plus_one_dim(), c2.plus_one_dim(), c3.plus_one_dim(), c4.plus_one_dim(), this->col);
        }

        //! Initialize vertex buffer objects and vertex array object.
        void initializeVertices()
        {
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();

            // Draw a rectangle in the x-y plane. That's it.
            this->computeRectangle();
        }

        //! The dimensions (width x height) of the rectangle, which is always centred on 0,0,0
        sm::vec<float, 2> dims = { 1.0f, 1.0f };
        float angle = 0.0f; // Angle of rectangle in degrees

        //! The colour of the rectangle
        std::array<float, 3> col = mplot::colour::black;
    };

} // namespace mplot
