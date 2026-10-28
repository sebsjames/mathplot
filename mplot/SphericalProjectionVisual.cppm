module;

#include <cstdint>
#include <iostream>
#include <array>
#include <vector>
#include <stdexcept>

export module mplot.sphericalprojectionvisual;

import sm.mathconst;
import sm.vec;
import sm.geometry;
import sm.quaternion;

import mplot.gl.version;
import mplot.visualmodel;
import sm.jcv;

export namespace mplot
{
    //! This class creates a flat projection of spherical data provided as vvecs of
    //! latitude-longitude pairs and scalar or vector values. Use VisualDataModel?
    template<typename T, std::int32_t glver = mplot::gl::version_4_1> requires std::is_floating_point_v<T>
    struct SphericalProjectionVisual : public mplot::VisualModel<glver>
    {
        SphericalProjectionVisual() {}
        SphericalProjectionVisual (const sm::vec<float, 3> _offset) : mplot::VisualModel<glver>(_offset) {}

        sm::geometry::spherical_projection::type proj_type = sm::geometry::spherical_projection::type::mercator;

        sm::vec<T, 2> project (sm::vec<T, 2> ll, const T radius)
        {
            // Apply this->rotation to lat-long coordinates, ll
            sm::vec<T, 3> llv = sm::geometry::spherical_projection::latlong_to_xyz (ll, T{1});
            ll = sm::geometry::spherical_projection::xyz_to_latlong (this->rotation * llv);
            if (ll.has_nan()) { throw std::runtime_error ("nan in ll"); }
            if (this->proj_type == sm::geometry::spherical_projection::type::equirectangular) {
                return sm::geometry::spherical_projection::equirectangular (ll, radius, this->lambda0, this->phi0, this->phi1);
            } else if (this->proj_type == sm::geometry::spherical_projection::type::cassini) {
                return sm::geometry::spherical_projection::cassini (ll, radius, this->lambda0);
            } else {
                return sm::geometry::spherical_projection::mercator (ll, radius, this->lambda0);
            }
        }

        void initializeVertices()
        {
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();
            this->xy.resize (this->latlong.size());
            for (std::uint32_t i = 0; i < this->latlong.size(); ++i) {
                this->xy[i] = this->project (this->latlong[i], this->radius).plus_one_dim().template as<double>();
            }
            this->voronoi2d();
        }

        //! Compute a triangle from 3 arbitrary corners
        void computeTriangle (sm::vec<double> c1, sm::vec<double> c2, sm::vec<double> c3, const std::array<float, 3>& colr)
        {
            // v is the face normal
            sm::vec<double> u1 = c1-c2;
            sm::vec<double> u2 = c2-c3;
            sm::vec<double> v = u1.cross(u2);
            v.renormalize();
            // Push corner vertices
            this->vertex_push (c1.as_float(), this->vertexPositions);
            this->vertex_push (c2.as_float(), this->vertexPositions);
            this->vertex_push (c3.as_float(), this->vertexPositions);
            // Colours/normals
            for (std::uint32_t i = 0; i < 3U; ++i) {
                this->vertex_push_colour (colr, this->vertexColors);
                this->vertex_push (v.as_float(), this->vertexNormals);
            }
            this->indices.push_back (this->idx++);
            this->indices.push_back (this->idx++);
            this->indices.push_back (this->idx++);
        }

        void voronoi2d()
        {
            // Find the extents of dataCoords. From these create a rectangle to pass to
            // diagram_generate.
            std::int32_t ncoords = static_cast<std::int32_t>(this->xy.size());

            sm::jcv::manager<double> vorman; // we need double precision for projections, float may run into trouble
            vorman.border_width = this->border_width;
            vorman.diagram_generate (this->xy);

            std::int32_t diag_nsites = vorman.diagram_numsites();
            if (diag_nsites != ncoords) {
                std::cout << "WARNING: diagram's ncoords (" << diag_nsites << ") != ncoords (" << ncoords << ")?!?!\n";
            }

            // We obtain access to the Voronoi cell sites:
            const sm::jcv::site<double>* sites = vorman.diagram_get_sites();

            for (std::int32_t i = 0; i < vorman.diagram_numsites() && i < ncoords; ++i) {
                const sm::jcv::site<double>* site = &sites[i];
                sm::jcv::graphedge<double>* e = site->edges; // The very first edge
                while (e) {
                    // Set z. Should be done in jcvoronoi, but haven't found out how
                    e->pos[0][2] = this->xy[i][2];
                    e->pos[1][2] = e->pos[0][2];
                    e = e->next;
                }
            }

            // To draw triangles iterate over the 'sites' and draw triangles
            for (std::int32_t i = 0; i < vorman.diagram_numsites() && i < ncoords; ++i) {
                const sm::jcv::site<double>* site = &sites[i];
                const sm::jcv::graphedge<double>* e = site->edges;
                std::array<float, 3> c = mplot::colour::black;
                if (static_cast<std::size_t>(site->index) < this->colour.size()) { c = this->colour[site->index]; }
                while (e) {
                    this->computeTriangle (site->p, e->pos[0], e->pos[1], c);
                    if constexpr (show_centres) {
                        auto sphc = (site->p +  e->pos[0] + e->pos[1]) / T{3};
                        this->computeSphere (sphc, this->centre_col, this->centre_rad, 4, 4);
                    }
                    e = e->next;
                }
            }
        }

        // latlong, supplied by user in radians
        sm::vvec<sm::vec<T, 2>> latlong;
        // Colour, supplied by user
        sm::vvec<std::array<float, 3>> colour;
        // xy, result of projection, but in 3D. double precision always
        sm::vvec<sm::vec<double, 3>> xy;
        // The radius of our sphere
        T radius = T{1};
        // The longitudinal offset
        T lambda0 = T{0};
        // Params for equirectangular projection
        T phi0 = T{0};
        T phi1 = T{0};
        // A border width for the Voronoi cells
        T border_width = T{0.001};

        // To debug the centres of the Voronoi cells, set show_centres true
        static constexpr bool show_centres = false;
        static constexpr std::array<float, 3> centre_col = mplot::colour::black;
        static constexpr T centre_rad = T{0.005};

        void set_rotation (const sm::quaternion<T>& r)
        {
            this->rotation = r;
            this->rotation.renormalize();
        }
        sm::quaternion<T> get_rotation () const { return this->rotation; }

    private:
        // A rotation to apply to each latitude-longitude before feeding it to the projection.
        sm::quaternion<T> rotation;
    };
}
