module;

#include <cstdint>
#include <array>
#include <string>
#include <cstdlib>

export module mplot.scalarvisual;

import sm.vec;
export import sm.scale;
import mplot.visualmodel;
export import mplot.colourmap;

export namespace mplot
{
    //! This class creates the vertices for a square scalar value indicator. The indicator has a tubular border
    template<typename T, std::int32_t glver = mplot::gl::version_4_1>
    class ScalarVisual : public VisualModel<glver>
    {
    public:
        ScalarVisual() {}

        //! Initialise with offset
        ScalarVisual(const sm::vec<float, 3> _offset)
        {
            this->colourScale.identity_scaling(); // [0, 1] maps to [0, 1] by default
            this->viewmatrix.translate (_offset);
        }

        void computeRectangle (const float w, const std::array<float, 3> col, const sm::vec<float, 2> os = {})
        {
            // Corners of the rectangle - make sure they're clockwise in order
            sm::vec<float, 2> c1 = { w, w };
            sm::vec<float, 2> c2 = { w, -w };
            sm::vec<float, 2> c3 = { -w, -w };
            sm::vec<float, 2> c4 = { -w, w };

            c1 /= 2.0f;
            c2 /= 2.0f;
            c3 /= 2.0f;
            c4 /= 2.0f;
            c1 += os;
            c2 += os;
            c3 += os;
            c4 += os;

            this->computeFlatQuad (c1.plus_one_dim(), c2.plus_one_dim(), c3.plus_one_dim(), c4.plus_one_dim(), col);
        }

        void tubularBorder (const sm::vec<float, 4>& r_extents, // xmin xmax ymin ymax
                            const float bz, const float tubedia,
                            const std::array<float, 3>& clr)
        {
            sm::vec<float> lb = { r_extents[0], r_extents[2], bz };
            sm::vec<float> lt = { r_extents[0], r_extents[3], bz };
            sm::vec<float> rt = { r_extents[1], r_extents[3], bz };
            sm::vec<float> rb = { r_extents[1], r_extents[2], bz };

            sm::vec<float> lblt = lt - lb;
            lblt.renormalize();
            sm::vec<float> ltrt = rt - lt;
            ltrt.renormalize();
            sm::vec<float> rtrb = rb - rt;
            rtrb.renormalize();
            sm::vec<float> rblb = lb - rb;
            rblb.renormalize();

            float rad = tubedia * 0.5f;
            this->computeOpenTube (lb, lt,  -(rblb + lblt), (lblt + ltrt),  clr, clr, rad, 20);
            this->computeOpenTube (lt, rt,  -(lblt + ltrt), (ltrt + rtrb),  clr, clr, rad, 20);
            this->computeOpenTube (rt, rb,  -(ltrt + rtrb), (rtrb + rblb),  clr, clr, rad, 20);
            this->computeOpenTube (rb, lb,  -(rtrb + rblb), (rblb + lblt),  clr, clr, rad, 20);
        }

        //! Initialize vertex buffer objects and vertex array object.
        void initializeVertices()
        {
            if (value_ptr != nullptr) { this->value = *value_ptr; }

            // Draw a rectangle in the x-y plane. That's it.
            this->computeRectangle (width, this->cm.convert (this->colourScale.transform_one (this->value))); // 4 indices

            // Draw a border around it
            float t_rad = width / 10.0f;
            float t_rad_2 = 2.0f * t_rad;
            sm::vec<float, 4> r_extents = { -width - t_rad_2, width + t_rad_2, -width - t_rad_2, width + t_rad_2};
            this->tubularBorder (r_extents / 2.0f, 0.0f, t_rad, mplot::colour::black);
            if (!lbl.empty()) {
                // Draw label. These positions could be better determined (using TextGeometry)
                float od = width / 2.0f + 2.0f * width / 10.0f; // outer dimension
                this->addLabel (this->lbl, sm::vec<float>{-od + 0.02f, od + width / 30.0f, 0}, mplot::TextFeatures(0.05f));
            }
        }

        // reinitColours to change the last rectangles worth of colour
        void reinitColours()
        {
            if (value_ptr != nullptr) { this->value = *value_ptr; }
            // Change colour of just the rectangle
            auto c = this->cm.convert (this->colourScale.transform_one (this->value));
            for (std::size_t i = 0u; i < 4u; ++i) {
                this->vertexColors[this->ncol * i] = c[0];
                this->vertexColors[this->ncol * i + 1] = c[1];
                this->vertexColors[this->ncol * i + 2] = c[2];
                if (this->ncol == 4) { this->vertexColors[this->ncol * i + 3] = 1.0f; }
            }
            this->reinit_colour_buffer();
        }

        // The scalar value to indicate
        T value = T{0};
        // If a pointer is set, use that instead
        T* value_ptr = nullptr;
        // The colour map to use
        ColourMap<float> cm;
        // A Scaling function for the colour map.
        sm::scale<T, float> colourScale;
        // width of the square indicator
        float width = 0.3f;
        //! The label
        std::string lbl = "";
    };

} // namespace mplot
