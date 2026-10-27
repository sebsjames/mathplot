module;

#include <cstdint>
#include <array>
#include <vector>
#include <utility>

export module mplot.rhombovisual;

export import mplot.visualmodel;
export import mplot.colourmap;
export import sm.vec;

export namespace mplot
{
    //! This class creates the vertices for a rhombohedron
    template<std::int32_t glver = mplot::gl::version_4_1>
    class RhomboVisual : public VisualModel<glver>
    {
    public:
        //! Initialise with offset, three edges and a single colour.
        RhomboVisual(const sm::vec<float, 3> _offset,
                     const sm::vec<float, 3> _edge1, const sm::vec<float, 3> _edge2, const sm::vec<float, 3> _edge3,
                     const std::array<float, 3> _col)
            : VisualModel<glver>(_offset)
            , edge1 (_edge1) , edge2 (_edge2) , edge3 (_edge3), col (_col) {}

        //! Initialize vertex buffer objects and vertex array object.
        void initializeVertices() override
        {
            if (this->facecm == mplot::ColourMapType::Fixed) {
                this->vertices_singlecolour();
            } else {
                this->vertices_multicolour();
                if (this->annotate) {
                    sm::vec<> v1;
                    sm::vec<> v2;
                    sm::vec<> v3;
                    sm::vec<> v4;
                    auto vpptr = reinterpret_cast< std::vector<sm::vec<float, 3>>* >(&this->vertexPositions);
                    // Show coordinates of vertices
                    // Front face
                    // 6 vertices of which 4 are our corners
                    v1 = (*vpptr)[0];
                    v2 = (*vpptr)[1];
                    v3 = (*vpptr)[2];
                    v4 = (*vpptr)[5];

                    auto lbl = this->makeVisualTextModel (this->tf);
                    mplot::TextGeometry geom = lbl->getTextGeometry (v1.str());
                    auto loffset = sm::vec<>{-geom.width(),0,0};
                    lbl->setupText (v1.str(), v1 + this->viewmatrix.translation() + loffset, this->tf.colour);
                    this->texts.push_back (std::move(lbl));

                    lbl = this->makeVisualTextModel (this->tf);
                    geom = lbl->getTextGeometry (v2.str());
                    lbl->setupText (v2.str(), v2 + this->viewmatrix.translation(), this->tf.colour);
                    this->texts.push_back (std::move(lbl));

                    lbl = this->makeVisualTextModel (this->tf);
                    geom = lbl->getTextGeometry (v3.str());
                    loffset = sm::vec<>{-geom.width(),0,0};
                    lbl->setupText (v3.str(), v3 + this->viewmatrix.translation() + loffset, this->tf.colour);
                    this->texts.push_back (std::move(lbl));

                    lbl = this->makeVisualTextModel (this->tf);
                    geom = lbl->getTextGeometry (v4.str());
                    lbl->setupText (v4.str(), v4 + this->viewmatrix.translation(), this->tf.colour);
                    this->texts.push_back (std::move(lbl));


                    // Back face
                    v1 = (*vpptr)[12 + 0];
                    v2 = (*vpptr)[12 + 1];
                    v3 = (*vpptr)[12 + 2];
                    v4 = (*vpptr)[12 + 5];

                    lbl = this->makeVisualTextModel (this->tf);
                    geom = lbl->getTextGeometry (v1.str());
                    loffset = sm::vec<>{-geom.width(),0,0};
                    lbl->setupText (v1.str(), v1 + this->viewmatrix.translation() + loffset, this->tf.colour);
                    this->texts.push_back (std::move(lbl));

                    lbl = this->makeVisualTextModel (this->tf);
                    geom = lbl->getTextGeometry (v2.str());
                    lbl->setupText (v2.str(), v2 + this->viewmatrix.translation(), this->tf.colour);
                    this->texts.push_back (std::move(lbl));

                    lbl = this->makeVisualTextModel (this->tf);
                    geom = lbl->getTextGeometry (v3.str());
                    loffset = sm::vec<>{-geom.width(),0,0};
                    lbl->setupText (v3.str(), v3 + this->viewmatrix.translation() + loffset, this->tf.colour);
                    this->texts.push_back (std::move(lbl));

                    lbl = this->makeVisualTextModel (this->tf);
                    geom = lbl->getTextGeometry (v4.str());
                    lbl->setupText (v4.str(), v4 + this->viewmatrix.translation(), this->tf.colour);
                    this->texts.push_back (std::move(lbl));

                }
            }
        }

        void vertices_singlecolour()
        {
            // Compute the face normals
            sm::vec<float> _n1 = this->edge1.cross (this->edge2);
            _n1.renormalize();
            sm::vec<float> _n2 = this->edge2.cross (this->edge3);
            _n2.renormalize();
            sm::vec<float> _n3 = this->edge1.cross (this->edge3);
            _n3.renormalize();

            // First corner of rhombohedron is at model-frame's origin
            sm::vec<float> o = {0,0,0};

            // Push positions and normals for 24 vertices to make up the rhombohedron; 4 for each face.
            // Front face
            this->vertex_push (o,                              this->vertexPositions);
            this->vertex_push (o + this->edge1,                this->vertexPositions);
            this->vertex_push (o + this->edge3,                this->vertexPositions);
            this->vertex_push (o + this->edge1 + this->edge3,  this->vertexPositions);
            for (std::uint16_t i = 0U; i < 4U; ++i) { this->vertex_push (_n3, this->vertexNormals); }
            // Top face
            this->vertex_push (o + this->edge3, this->vertexPositions);
            this->vertex_push (o + this->edge1 + this->edge3,               this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge3,               this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge1 + this->edge3, this->vertexPositions);
            for (std::uint16_t i = 0U; i < 4U; ++i) { this->vertex_push (_n1, this->vertexNormals); }
            // Back face
            this->vertex_push (o + this->edge2 + this->edge3,               this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge1 + this->edge3, this->vertexPositions);
            this->vertex_push (o + this->edge2,                             this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge1,               this->vertexPositions);
            for (std::uint16_t i = 0U; i < 4U; ++i) { this->vertex_push (-_n3, this->vertexNormals); }
            // Bottom face
            this->vertex_push (o + this->edge2,                this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge1,  this->vertexPositions);
            this->vertex_push (o,                              this->vertexPositions);
            this->vertex_push (o + this->edge1,                this->vertexPositions);
            for (std::uint16_t i = 0U; i < 4U; ++i) { this->vertex_push (-_n1, this->vertexNormals); }
            // Left face
            this->vertex_push (o + this->edge2,                this->vertexPositions);
            this->vertex_push (o,                              this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge3,  this->vertexPositions);
            this->vertex_push (o + this->edge3,                this->vertexPositions);
            for (std::uint16_t i = 0U; i < 4U; ++i) { this->vertex_push (-_n2, this->vertexNormals); }
            // Right face
            this->vertex_push (o + this->edge1,                             this->vertexPositions);
            this->vertex_push (o + this->edge1 + this->edge2,               this->vertexPositions);
            this->vertex_push (o + this->edge1 + this->edge3,               this->vertexPositions);
            this->vertex_push (o + this->edge1 + this->edge2 + this->edge3, this->vertexPositions);
            for (std::uint16_t i = 0U; i < 4U; ++i) { this->vertex_push (_n2, this->vertexNormals); }

            // Vertex colours are all the same
            for (std::uint16_t i = 0U; i < 24U; ++i) {
                this->vertex_push_colour (this->col, this->vertexColors);
            }

            // Indices for 6 faces
            for (std::uint16_t i = 0U; i < 6U; ++i) {
                this->indices.push_back (this->idx++);
                this->indices.push_back (this->idx++);
                this->indices.push_back (this->idx--);
                this->indices.push_back (this->idx++);
                this->indices.push_back (this->idx++);
                this->indices.push_back (this->idx++);
            }
        }

        // Allowing a colour for each face. May be useful for debugging
        void vertices_multicolour()
        {
            sm::vec<float> coroffs = {};
            if (this->raise_corner) { coroffs = { 0, 0.2, 0 }; }

            // Compute the face normals
            sm::vec<float> _n1 = this->edge1.cross (this->edge2);
            _n1.renormalize();
            sm::vec<float> _n2 = this->edge2.cross (this->edge3);
            _n2.renormalize();
            sm::vec<float> _n3 = this->edge1.cross (this->edge3);
            _n3.renormalize();

            // First corner of rhombohedron is at model-frame's origin
            sm::vec<float> o = {0,0,0};

            // Push positions and normals for 24 vertices to make up the rhombohedron; *6* for each face.
            // Front face (z up) needs 6 vertices
            this->vertex_push (o,                              this->vertexPositions);
            this->vertex_push (o + this->edge1,                this->vertexPositions);
            this->vertex_push (o + this->edge3,                this->vertexPositions);
            this->vertex_push (o + this->edge3,                this->vertexPositions); // extra
            this->vertex_push (o + this->edge1,                this->vertexPositions); // extra
            this->vertex_push (o + this->edge1 + this->edge3,  this->vertexPositions);
            for (std::uint16_t i = 0U; i < 6U; ++i) { this->vertex_push (_n3, this->vertexNormals); }
            // Top face
            this->vertex_push (o + this->edge3, this->vertexPositions);
            this->vertex_push (o + this->edge1 + this->edge3,               this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge3,               this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge3,               this->vertexPositions); // extra
            this->vertex_push (o + this->edge1 + this->edge3,               this->vertexPositions); // extra
            this->vertex_push (o + this->edge2 + this->edge1 + this->edge3, this->vertexPositions);
            for (std::uint16_t i = 0U; i < 6U; ++i) { this->vertex_push (_n1, this->vertexNormals); }
            // Back face
            this->vertex_push (o + this->edge2 + this->edge3,               this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge1 + this->edge3, this->vertexPositions);
            this->vertex_push (o + this->edge2,                             this->vertexPositions);
            this->vertex_push (o + this->edge2,                             this->vertexPositions); // extra
            this->vertex_push (o + this->edge2 + this->edge1 + this->edge3, this->vertexPositions); // extra
            this->vertex_push (o + this->edge2 + this->edge1 + coroffs,     this->vertexPositions);
            for (std::uint16_t i = 0U; i < 3U; ++i) { this->vertex_push (-_n3, this->vertexNormals); }
            if (this->raise_corner) {
                for (std::uint16_t i = 0U; i < 3U; ++i) { this->vertex_push (-_n3, this->vertexNormals); }
            } else {
                sm::vec<float> _n4 = (edge2 - (edge1 + edge2 + edge3)).cross (edge1 + edge2 + coroffs - (edge1 + edge2 + edge3));
                for (std::uint16_t i = 0U; i < 3U; ++i) { this->vertex_push (-_n4, this->vertexNormals); }
            }
            // Bottom face
            this->vertex_push (o + this->edge2,                this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge1 + coroffs,  this->vertexPositions);
            this->vertex_push (o,                              this->vertexPositions);
            this->vertex_push (o,                              this->vertexPositions); // extra
            this->vertex_push (o + this->edge2 + this->edge1 + coroffs,  this->vertexPositions); // extra
            this->vertex_push (o + this->edge1,                this->vertexPositions);
            for (std::uint16_t i = 0U; i < 6U; ++i) { this->vertex_push (-_n1, this->vertexNormals); }
            // Left face
            this->vertex_push (o + this->edge2,                this->vertexPositions);
            this->vertex_push (o,                              this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge3,  this->vertexPositions);
            this->vertex_push (o + this->edge2 + this->edge3,  this->vertexPositions); // extra
            this->vertex_push (o,                              this->vertexPositions); // extra
            this->vertex_push (o + this->edge3,                this->vertexPositions);
            for (std::uint16_t i = 0U; i < 6U; ++i) { this->vertex_push (-_n2, this->vertexNormals); }
            // Right face
            this->vertex_push (o + this->edge1,                             this->vertexPositions);
            this->vertex_push (o + this->edge1 + this->edge2 + coroffs,               this->vertexPositions);
            this->vertex_push (o + this->edge1 + this->edge3,               this->vertexPositions);
            this->vertex_push (o + this->edge1 + this->edge3,               this->vertexPositions); // extra
            this->vertex_push (o + this->edge1 + this->edge2 + coroffs,               this->vertexPositions); // extra
            this->vertex_push (o + this->edge1 + this->edge2 + this->edge3, this->vertexPositions);
            for (std::uint16_t i = 0U; i < 6U; ++i) { this->vertex_push (_n2, this->vertexNormals); }

            // Vertex colours are NOT all the same
            mplot::ColourMap<float> cm (this->facecm);
            for (std::uint16_t i = 0U; i < 36U; i += 3) {
                this->vertex_push_colour (cm.convert(static_cast<float>(i) / 35.0f), this->vertexColors);
                this->vertex_push_colour (cm.convert(static_cast<float>(i) / 35.0f), this->vertexColors);
                this->vertex_push_colour (cm.convert(static_cast<float>(i) / 35.0f), this->vertexColors);
            }

            // Indices for 6 faces.
            for (std::uint16_t i = 0U; i < 36U; ++i) { this->indices.push_back (this->idx++); }
        }

        //! Three vectors define the Rhombohedron and we use a single colour
        sm::vec<float, 3> edge1 = {0.0f, 0.0f, 0.0f};
        sm::vec<float, 3> edge2 = {0.0f, 0.0f, 0.0f};
        sm::vec<float, 3> edge3 = {0.0f, 0.0f, 0.0f};
        std::array<float, 3> col = {0.0f, 0.0f, 1.0f};
        mplot::ColourMapType facecm = mplot::ColourMapType::Fixed; // if so, use col
        bool annotate = false;
        mplot::TextFeatures tf; // for annotations if used
        bool raise_corner = false;
    };

} // namespace mplot
