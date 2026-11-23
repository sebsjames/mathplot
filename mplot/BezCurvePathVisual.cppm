/*
 * Visualize an sm::bezcurvepath
 */
module;

#include <cstdint>
#include <iostream>
#include <array>

export module mplot.bezcurvepathvisual;

export import mplot.gl.version;
export import mplot.visualmodel;
export import sm.bezcurvepath;
import sm.vec;
import sm.bezcoord;

export namespace mplot
{
    //! The template argument T is the type of the data which this HexGridVisual
    //! will visualize.
    template <class T, std::uint32_t N, std::int32_t glver = mplot::gl::version_4_1>
    struct BezCurvePathVisual : public VisualModel<glver>
    {
        BezCurvePathVisual(const sm::vec<float> _offset) { this->viewmatrix.translate (_offset); }

        // The path to visualize
        sm::bezcurvepath<T>* bcp = nullptr;

        // The step size for segments in the path (we visualize as a sequence of straight lines)
        T step = T{0.1};

        float width = 0.05f;

        float z = 0.0f;

        // Is the path a closed loop? If so, draw a line to close it
        bool path_is_loop = true;

        std::array<float, 3> colour = mplot::colour::black;

        void initializeVertices()
        {
            if (this->bcp == nullptr) {
                std::cerr << "BezCurvePathVisual: There is no path to visualize\n";
                return;
            }

            // For each curve in path, draw curve. Easy.
            this->bcp->compute_points (this->step);

            if (this->bcp->points.size() < 2) {
               std::cerr << "BezCurvePathVisual: There are no points in the path to visualize\n";
                return;
            }

            if (path_is_loop) {
                // Start the curve with the end->start segment
                const sm::bezcoord<T> pe = bcp->points[bcp->points.size() - 1]; // end
                const sm::bezcoord<T> ps = bcp->points[0];                      // start
                const sm::vec<float> ve = {static_cast<float>(pe.x()), static_cast<float>(pe.y()), this->z};
                const sm::vec<float> vs = {static_cast<float>(ps.x()), static_cast<float>(ps.y()), this->z};
                this->computeFlatLine (ve, vs, sm::vec<float>::uz(), this->colour, this->width);
            }
            // now have bcp->points, tangents and normals
            for (std::uint32_t i = 1; i < bcp->points.size(); ++i) {
                const sm::bezcoord<T> p0 = bcp->points[i - 1];
                const sm::bezcoord<T> p1 = bcp->points[i];
                const sm::vec<float> v0 = {static_cast<float>(p0.x()), static_cast<float>(p0.y()), this->z};
                const sm::vec<float> v1 = {static_cast<float>(p1.x()), static_cast<float>(p1.y()), this->z};
                // Draw line segment from p0 to p1
                this->computeFlatLine (v0, v1, sm::vec<float>::uz(), this->colour, this->width);
            }
        }
    };
}
