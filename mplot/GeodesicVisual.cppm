module;

#include <cstdint>
#include <array>
#include <stdexcept>

export module mplot.geodesicvisual;

export import sm.vec;
export import sm.vvec;
export import sm.scale;
import sm.geometry;
import sm.geometry_polyhedra;

export import mplot.visualmodel;
export import mplot.colourmap;

export namespace mplot
{
    /*!
     * This class creates the vertices for an geodesic polyhedron in a 3D scene.
     *
     * \tparam T the type for the data to be visualized as face (or maybe vertex) colours
     *
     * \tparam glver The usual OpenGL version code; match this to everything else in
     * your program.
     */
    template<typename T, std::int32_t glver = mplot::gl::version_4_1>
    class GeodesicVisual : public VisualModel<glver>
    {
    public:
        GeodesicVisual() { this->init ({0.0, 0.0, 0.0}, 1.0f); }

        //! Initialise with offset, start and end coordinates, radius and a single colour.
        GeodesicVisual(const sm::vec<float, 3> _offset, const float _radius)
        {
            this->init (_offset, _radius);
        }

        ~GeodesicVisual () {}

        void init (const sm::vec<float, 3> _offset, const float _radius)
        {
            this->viewmatrix.translate (_offset);
            this->radius = _radius;
            this->cm.setType (mplot::ColourMapType::Jet);
            this->colourScale.do_autoscale = true;
        }

        //! Initialize vertex buffer objects and vertex array object.
        void initializeVertices()
        {
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();

            sm::geometry::icosahedral_geodesic_info gi(this->iterations);
            this->n_faces = gi.n_faces;
            this->n_geo_verts = gi.n_vertices;
            this->n_verts = n_geo_verts; // n_verts is OpenGL vertices. May be different from n_geo_verts to colour the Geodesic faces.

            if (this->colourFaces == true) {
                // Resize our data.
                this->data.resize (this->n_faces, T{0});

                if (iterations > 5) {
                    this->n_verts = this->template computeSphereGeoFaces<double> (sm::vec<float, 3>{0.0f}, this->cm.convert(0.0f), this->radius, this->iterations);
                } else {
                    this->n_verts = this->computeSphereGeoFaces (sm::vec<float, 3>{0.0f}, this->cm.convert(0.0f), this->radius, this->iterations);
                }

            } else { // colour vertices

                if (iterations > 5) {
                    // Note odd necessity to stick in the 'template' keyword after this->
                    this->n_verts = this->template computeSphereGeo<double> (sm::vec<float, 3>{0.0f}, this->cm.convert(0.0f), this->radius, this->iterations);
                } else {
                    // computeSphereGeo F defaults to float
                    this->n_verts = this->computeSphereGeo (sm::vec<float, 3>{0.0f}, this->cm.convert(0.0f), this->radius, this->iterations);
                }
                // Resize our data.
                this->data.resize (n_verts, T{0});
            }
        }

        sm::vvec<sm::vec<float>> cart_centres;
        sm::vvec<sm::vec<float>> sph_centres;
        void vertexPositionsToFaces()
        {
            if (!this->n_faces) { throw std::runtime_error ("Call this after finalize()"); }
            // Iterate through this->vertexPositions and indices, to compute the centres of each
            // facet of the sphere, then project this onto the sphere. This gives an array of
            // coordinates the same size as this->data/this->cdata. From these compute coordinates
            // (r, theta, phi).
            this->cart_centres.resize (this->n_faces);
            this->sph_centres.resize (this->n_faces);
            for (size_t i = 0; i < this->indices.size(); i+=3) {
                std::int32_t _vtx1 = this->indices[i];
                std::int32_t _vtx2 = this->indices[i+1];
                std::int32_t _vtx3 = this->indices[i+2];
                sm::vec<float> v1 = {
                    this->vertexPositions[3 * _vtx1],
                    this->vertexPositions[3 * _vtx1 + 1],
                    this->vertexPositions[3 * _vtx1+ 2]
                };
                sm::vec<float> v2 = {
                    this->vertexPositions[3 * _vtx2],
                    this->vertexPositions[3 * _vtx2 + 1],
                    this->vertexPositions[3 * _vtx2+ 2]
                };
                sm::vec<float> v3 = {
                    this->vertexPositions[3 * _vtx3],
                    this->vertexPositions[3 * _vtx3 + 1],
                    this->vertexPositions[3 * _vtx3+ 2]
                };
                this->cart_centres[i/3] = (v1 + v2 + v3) / 3.0f;
                this->sph_centres[i/3] = this->cart_centres[i/3].cartesian_to_spherical();
            }
        }


        //! reinit just the colours based on vvec<T> data
        void reinitColours()
        {
            if (this->data.empty() && this->cdata.empty()) { return; }

            size_t n_cvals = this->vertexColors.size();
            if (n_cvals == 0u) { return; } // model doesn't exist yet

            if (!this->cdata.empty()) {

                this->vertexColors.clear(); // could potentially just replace values
                size_t n_data = this->cdata.size();

                // there are n_vertex colours, and n_data data points.
                if (this->colourFaces == true) {
                    if (n_cvals != 3 * 3 * n_data) { throw std::runtime_error ("GeodesicVisual: data is not right size");  }
                } else {
                    if (n_cvals != 3 * n_data) { throw std::runtime_error ("GeodesicVisual: data is not right size");  }
                }

                // Re-colour
                for (size_t i = 0u; i < n_data; ++i) {
                    // Update the 3 RGB values in vertexColors
                    for (std::int32_t ci = 0; ci < (this->colourFaces == true ? 3 : 1); ++ci) {
                        this->vertex_push_colour (cdata[i], this->vertexColors);
                    }
                }

            } else {

                this->vertexColors.clear(); // could potentially just replace values
                size_t n_data = this->data.size();

                // there are n_vertex colours, and n_data data points.
                if (this->colourFaces == true) {
                    if (n_cvals != 3 * 3 * n_data) { throw std::runtime_error ("GeodesicVisual: data is not right size");  }
                } else {
                    if (n_cvals != 3 * n_data) { throw std::runtime_error ("GeodesicVisual: data is not right size");  }
                }

                // Scale data
                sm::vvec<float> scaled_data (this->data);
                if (this->colourScale.do_autoscale == true) { this->colourScale.reset(); }
                this->colourScale.transform (this->data, scaled_data);
                //std::cout << "data range: " << data.range() << ", scaled_data range " << scaled_data.range() << std::endl;

                // Re-colour
                for (size_t i = 0u; i < n_data; ++i) {
                    // Update the 3 RGB values in vertexColors
                    auto c = this->cm.convert (scaled_data[i]);
                    for (std::int32_t ci = 0; ci < (this->colourFaces == true ? 3 : 1); ++ci) {
                        this->vertex_push_colour (c, this->vertexColors);
                    }
                }
            }
            // Lastly, this call copies vertexColors (etc) into the OpenGL memory space
            this->reinit_colour_buffer();
        }

        //! The radius of the geodesic
        float radius = 1.0f;
        //! The colour of the object. Can be resized to n_faces to colour each face
        //! independently, or possibly to size of vertices to colour the vertices. Fill
        //! this vvec with data *after* calling initialize.
        sm::vvec<T> data;
        //! Can also colour with direct colour data
        sm::vvec<std::array<float, 3>> cdata;
        //! Do we colour vertices or faces? Set before finalize()
        bool colourFaces = true;
        //! A colour map for data plotting
        mplot::ColourMap<float> cm;
        //! A scaling for data colour
        sm::scale<T, float> colourScale;
        //! The number of iterations in the geodesic sphere. Set before finalize() to change from the default.
        std::int32_t iterations = 2;
        //! This may be filled with the number of vertices in the geodesic shape. This is the
        //! 'maths' number of vertices.
        std::int32_t n_geo_verts = 0;
        //! This may be filled with the number of OpenGL vertices used to construct the
        //! geodesic. The OpenGL number of vertices >= n_geo_verts.
        std::int32_t n_verts = 0;
        //! This may be filled with the number of faces in the geodesic
        std::int32_t n_faces = 0;
    };

} // namespace mplot
