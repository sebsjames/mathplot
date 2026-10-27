module;

#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <stdexcept>

export module mplot.verticesvisual;

import sm.vec;
import sm.vvec;
import sm.mat;

export import mplot.visualmodel;

export namespace mplot
{
    /*!
     * Create a visual model directly from indices, vertices and normals, which might have been
     * harvested from a file (glTF, for example). This model is constructed with the data required
     * to specify it. Unlike most VisualModel derived classes, it doesn't need to implement
     * initializeVertices as the vertices are copied in by the constructor.
     */
    template<std::int32_t glver = mplot::gl::version_4_1>
    struct VerticesVisual : public VisualModel<glver>
    {
        VerticesVisual (sm::mat<float, 4>& _model_transform,
                        sm::vvec<std::uint32_t> _ind,
                        sm::vvec<sm::vec<float>> _posn,
                        sm::vvec<sm::vec<float>> _norm,
                        sm::vvec<sm::vec<float>> _colr)
        {
            this->viewmatrix = _model_transform;
            // Copy in the indices and vertices
            for (auto i : _ind) { this->indices.push_back (i); }
            for (auto p : _posn) { this->vertex_push (p, this->vertexPositions); }
            for (auto n : _norm) { this->vertex_push (n, this->vertexNormals); }
            for (auto c : _colr) { this->vertex_push_colour (c, this->vertexColors); }
        }
    };

} // namespace mplot
