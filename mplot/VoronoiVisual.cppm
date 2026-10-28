/*!
 * Visualize an arbitrary surface defined by values at points in 3D space (similar to
 * ScatterVisual). Using code from https://github.com/JCash/voronoi compute a 2D Voronoi
 * diagram around the data points (using their x/y values) to create the 'panels' to
 * colourize. interpolate the z values of the data points to determine the z values of
 * the edges in the Voronoi diagram. Colourize the panels based on the scalarData or
 * vectorData of the underlying VisualDataModel.
 *
 * \author Seb James
 * \date 2024
 */
module;

#include <iostream>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <array>
#include <map>
#include <set>

export module mplot.voronoivisual;

import sm.vec;
import sm.quaternion;
import sm.geometry;
import sm.centroid;

import mplot.colourmap;
import mplot.tools;
import mplot.visualdatamodel;
import sm.jcv;

export namespace mplot
{
    //! The template argument F is the type of the data which this VoronoiVisual
    //! will visualize.
    template <typename F, std::int32_t n_epsilons = 0, std::int32_t glver = mplot::gl::version_4_1>
    class VoronoiVisual : public VisualDataModel<F, glver>
    {
        // Need a vec comparison function for a std::set of sm::vec or a std::map with a sm::vec key. See:
        // https://sebsjames.github.io/maths/ref/vec/#using-smvec-as-a-key-in-stdmap-or-within-an-stdset
        struct veccmp
        {
            bool operator()(sm::vec<float> a, sm::vec<float> b) const
            {
                return a.lexical_lessthan_beyond_epsilon(b, n_epsilons);
            }
        };

    public:

        // The shape of the domain to draw around the points in the Voronoi diagram Each kind of
        // boundary shape is auto-fit to the data points, although the user can set an additional
        // border_width.
        enum class domain_shape : std::uint32_t
        {
            rectangular,
            ellipsoid,
            circular,
            traced      // trace around the points - arbitrary polygon
        };

        VoronoiVisual (const sm::vec<float> _offset)
        {
            this->viewmatrix.translate (_offset);
            this->zScale.set_params (1, 0);
            this->colourScale.do_autoscale = true;
            this->colourScale2.do_autoscale = true;
            this->colourScale3.do_autoscale = true;
        }

        //! Compute 2.5D Voronoi diagram using code adapted from
        // https://github.com/JCash/voronoi. The adaptation is to add a third dimension
        // to point.
        void initializeVertices()
        {
            std::uint32_t ncoords = this->dataCoords == nullptr ? 0 : this->dataCoords->size();
            if (ncoords == 0) { return; }

            this->determine_datasize();
            if (this->datasize == 0) { return; }

            if (ncoords != this->datasize) {
                std::cerr << "VoronoiVisual Error: ncoords (" << ncoords << ") != datasize ("<< this->datasize
                          << "), return (no model)." << std::endl;
                return;
            }

            this->setupScaling();

            sm::quaternion<float> rq;
            if (this->data_z_direction != sm::vec<>::uz()) {
                // Find the rotation between data_z_direction and uz
                this->dcoords.resize (ncoords);
                sm::vec<float> r_axis = this->data_z_direction.cross (sm::vec<>::uz());
                r_axis.renormalize();
                float r_angle = this->data_z_direction.angle (sm::vec<>::uz(), r_axis);
                rq.rotate(r_axis, r_angle);
                for (size_t i = 0; i < ncoords; ++i) {
                    this->dcoords[i] = rq * (*this->dataCoords)[i];
                }
                this->dcoords_ptr = &this->dcoords;
            } else {
                this->dcoords_ptr = this->dataCoords;
            }

            // Generate the 2D Voronoi diagram
            sm::jcv::manager<float> vorman;
            if (this->border_width == -1) { this->border_width = 4.0f / std::sqrt (ncoords); }
            vorman.border_width = this->border_width;

            if (this->dom_shape == domain_shape::ellipsoid || this->dom_shape == domain_shape::circular) {

                sm::vec<float> cent = this->coordsCentroid();
                sm::vec<float, 2> cent2 = {cent[0], cent[1]};
                sm::vvec<float> lengths (this->dcoords_ptr->size(), 0.0f);
                for (std::uint32_t i = 0; i < this->dcoords_ptr->size(); ++i) {
                    sm::vec<float, 2> c = (*this->dcoords_ptr)[i].less_one_dim() - cent2;
                    lengths[i] = c.length();
                }
                float max_len = lengths.max();
                // Points MUST be in anti-clockwise order!!!!!
                this->boundary.clear();
                this->boundary.resize (this->num_boundary_points, cent2.plus_one_dim());

                if (this->dom_shape == domain_shape::ellipsoid) {
                    throw std::runtime_error ("Elliptic domain shapes not supported");
                    /*
                     * Here is an approach using arma::princomp, but I don't want the arma
                     * dependency, as elliptic boundaries are not as useful to me as traced
                     * boundaries. This code awaits sm::pca::compute().
                     */
#if 0
                    // Find ellipse parameters for the data. First place data in an arma::mat, offsetting by the centroid
                    arma::Mat<float> x (dcoords_ptr->size(), 2);
                    for (std::uint32_t i = 0; i < dcoords_ptr->size(); ++i) {
                        x(i, 0) = (*this->dcoords_ptr)[i][0] - cent2[0];
                        x(i, 1) = (*this->dcoords_ptr)[i][1] - cent2[1];
                    }
                    // From PCA determine ellipsoid axes. Angle of coefficient vector and length of
                    // eigen values gives the ellipse parameters
                    arma::Mat<float> co, sc;
                    arma::Col<float> lat, tsq;
                    arma::princomp (co, sc, lat, tsq, x);
                    // Mat access is (r, c)
                    sm::vec<float, 2> pc1vec = { co(0, 0), co(1, 0) };
                    sm::mat<float, 2> el_rotn;
                    el_rotn.rotate (pc1vec.angle());
                    float a = this->n_sigma * std::sqrt (lat(0));
                    float b = this->n_sigma * std::sqrt (lat(1));
                    // Create the elliptic boundary
                    for (std::uint32_t i = 0; i < this->num_boundary_points; ++i) {
                        float phi = i * sm::mathconst<float>::two_pi / this->num_boundary_points;
                        sm::vec<float, 2> bp = el_rotn * sm::vec<float, 2>{ a * std::cos (phi), b * std::sin (phi) }  + cent2;
                        this->boundary[i] = bp.plus_one_dim();
                    }
#endif
                } else { // circular boundary
                    float l = max_len + this->border_width;
                    for (std::uint32_t i = 0; i < this->num_boundary_points; ++i) {
                        float phi = i * sm::mathconst<float>::two_pi / this->num_boundary_points;
                        this->boundary[i] += { l * std::cos (phi), l * std::sin (phi) };
                    }
                }
            } else if (this->dom_shape == domain_shape::traced) {
                // Copy 3D points to 2D
                sm::vvec<sm::vec<float, 2>> coords2 (dcoords_ptr->size());
                for (std::uint32_t i = 0; i < dcoords_ptr->size(); ++i) {
                    coords2[i] = (*dcoords_ptr)[i].less_one_dim();
                }
                auto bnd2centr = sm::algo::centroid (coords2);
                // Find convex hull
                sm::vvec<sm::vec<float, 2>> bnd2 = sm::geometry::graham_scan (coords2);
                this->boundary.resize (bnd2.size());
                // Copy 2D to 3D boundary
                for (std::uint32_t i = 0; i < bnd2.size(); ++i) {
                    this->boundary[i] = bnd2[i].plus_one_dim();
                    // Add border
                    sm::vec<float, 2> brd = bnd2[i] - bnd2centr; // border vector from centroid to point
                    brd.renormalize();
                    brd *= this->border_width;
                    this->boundary[i] += brd.plus_one_dim();
                }
            } // else rectangular default does not populate this->boundary

            if (this->boundary.size() > 0) {
                vorman.diagram_generate (*(dcoords_ptr), this->boundary);
            } else {
                // default rectangular box boundary
                vorman.diagram_generate (*(dcoords_ptr));
            }

            if (static_cast<std::uint32_t>(vorman.diagram_numsites()) == 0) {
                if (this->boundary.empty()) {
                    throw std::runtime_error ("numsites == 0.");
                } else {
                    throw std::runtime_error ("numsites == 0. Make sure your boundary points appear in anti-clockwise order");
                }
            }

            // We obtain access to the Voronoi cell sites:
            const sm::jcv::site<float>* sites = vorman.diagram_get_sites();

            // Now scan through the Voronoi cell 'sites' and 'edges' to re-assign z
            // values in the edges. This is not going to be particularly efficient, but
            // it will be fine for grids of the size that we generally visualize.
            //
            // For each site, we have a set of edges around that site. We examine each
            // edge in turn, considering the sites that cluster around each end of the
            // edge. We assign the mean z of the sites in the cluster to the edge's
            // position at that end of the edge.
            //
            // This is complicated by the fact that there may be multiple (2?) edges between pairs
            // of sites. So, need a map of edge location to z_sums

            // Mapping edge end-point locations to a container of adjacent cell centres
            std::map<sm::vec<float, 3>, std::set<sm::vec<float, 3>, veccmp>, veccmp> edge_pos_centres;
            // Mapping same edge end-point locations to the averate z value of the adjacent cell centres
            std::map<sm::vec<float, 3>, float, veccmp> edge_end_zsums;

            for (std::int32_t i = 0; i < vorman.diagram_numsites(); ++i) {

                // We have the current edge_1, the next edge_2 and the previous edge_0
                const sm::jcv::site<float>* site = &sites[i];
                sm::jcv::graphedge<float>* edge_first = site->edges; // The very first edge
                sm::jcv::graphedge<float>* edge_1 = edge_first;
                sm::jcv::graphedge<float>* edge_2 = edge_first;
                sm::jcv::graphedge<float>* edge_0 = edge_first;
                while (edge_0->next) { edge_0 = edge_0->next; }

                while (edge_1) {

                    // Set z to 0. Should be done in jcvoronoi, but haven't found out how
                    edge_1->pos[0][2] = 0.0f;
                    edge_1->pos[1][2] = 0.0f;

                    edge_2 = edge_1->next ? edge_1->next : edge_first;
                    // edge_0 already set

                    // populate the edge_pos_centres map. Known issue: In some cases, outer edges
                    // have only 1 site at 1 of their ends. This makes it a problem to
                    // compute the correct z value for the end with no site. The
                    // solution would be to modify the jcvoronoi algorithm to populate
                    // both ends of all edges with additional logic.
                    for (std::uint32_t j = 0; j < 2; ++j) {
                        if (edge_1->edge_->sites[j]) {
                            //std::cout << "insert edge_1 " << edge_1->edge->sites[j]->p << " into edge_pos_centres[" << edge_1->pos[1] << "]\n";
                            edge_pos_centres[edge_1->pos[1]].insert (edge_1->edge_->sites[j]->p);
                            //std::cout << "insert edge_1 " << edge_1->edge->sites[j]->p << " into edge_pos_centres[" << edge_1->pos[0] << "]\n";
                            edge_pos_centres[edge_1->pos[0]].insert (edge_1->edge_->sites[j]->p);
                        }
                        // By definition, cellcentres_1 also gets edge_2 sites...
                        if (edge_2->edge_->sites[j]) {
                            //std::cout << "insert edge_2 " << edge_2->edge->sites[j]->p << " into edge_pos_centres[" << edge_1->pos[1] << "]\n";
                            edge_pos_centres[edge_1->pos[1]].insert (edge_2->edge_->sites[j]->p);
                        }
                        // and cellcentres_0 gets edge_0 sites
                        if (edge_0->edge_->sites[j]) {
                            //std::cout << "insert edge_0 " << edge_0->edge->sites[j]->p << " into edge_pos_centres[" << edge_1->pos[0] << "]\n";
                            edge_pos_centres[edge_1->pos[0]].insert (edge_0->edge_->sites[j]->p);
                        }
                    }

                    // Prepare for next loop
                    edge_0 = edge_1;
                    edge_1 = edge_1->next;
                }
            } // finished reassignment of z values

            // From edge_pos_centres, compute zsums for edge_end_zsums
            for (auto epc : edge_pos_centres) {
                float zsum = 0.0f;
                // cce: centres clustered-around edge
                for (auto cce : epc.second) {
                    zsum += cce[2];
                }
                if (epc.second.size() != 0) {
                    edge_end_zsums[epc.first] = zsum / epc.second.size();
                } // else do nothing
            }

            // Now go through edge_end_zsums and edges and update z values
            for (std::int32_t i = 0; i < vorman.diagram_numsites(); ++i) {
                const sm::jcv::site<float>* site = &sites[i];
                sm::jcv::graphedge<float>* edge_1 = site->edges; // The very first edge
                while (edge_1) {
                    // For each edge, set z from the map
                    float zsum0 = 0.0f;
                    float zsum1 = 0.0f;
                    try {
                        zsum0 = edge_end_zsums.at(edge_1->pos[0]);
                    } catch (const std::out_of_range& e) {
                        std::cout << "For edge_1: " << edge_1->pos[0] << " --- " <<  edge_1->pos[1] << std::endl;
                        std::cout << "(zsum0) no edge_end_zsums.at (" << edge_1->pos[0] << ")\n";
                    }
                    try {
                        zsum1 = edge_end_zsums.at(edge_1->pos[1]);
                    } catch (const std::out_of_range& e) {
                        std::cout << "For edge_1: " << edge_1->pos[0] << " --- " <<  edge_1->pos[1] << std::endl;
                        std::cout << "(zsum1) no edge_end_zsums.at (" << edge_1->pos[1] << ")\n";
                    }
                    //std::cout << "zsum0 = " << zsum0 << " and zsum1 = " << zsum1 << std::endl;
                    edge_1->pos[0][2] = zsum0;
                    edge_1->pos[1][2] = zsum1;
                    edge_1 = edge_1->next;
                }
            }

            // To draw triangles iterate over the 'sites' and get the edges
            this->triangle_counts.resize (ncoords, 0);
            this->site_indices.resize (ncoords, 0);
            this->triangle_count_sum = 0;

            if (this->data_z_direction != sm::vec<>::uz()) {
                // inverse rotate
                sm::vec<float> t0 = {0.0f};
                sm::vec<float> t1 = {0.0f};
                sm::vec<float> t2 = {0.0f};
                sm::quaternion<float> rqinv = rq.invert();
                for (std::int32_t i = 0; i < vorman.diagram_numsites(); ++i) {
                    const sm::jcv::site<float>* site = &sites[i];
                    const sm::jcv::graphedge<float>* e = site->edges;
                    std::uint32_t site_triangles = 0;
                    while (e) {
                        // NB: There are 3 each of pos/col/norm vertices (and 3 indices) per
                        // triangle. Could be reduced in principle. For a random map, it
                        // comes out as about about 17*4 vertices per coordinate.
                        t0 = rqinv * site->p;
                        t1 = rqinv * e->pos[0];
                        t2 = rqinv * e->pos[1];
                        this->computeTriangle (t0, t1, t2, this->setColour(site->index));
                        ++site_triangles;
                        e = e->next;
                    }
                    this->triangle_counts[i] = site_triangles;
                    this->site_indices[i] = site->index;
                    this->triangle_count_sum += site_triangles;
                }
            } else {
                // No need to inverse rotate
                for (std::int32_t i = 0; i < vorman.diagram_numsites(); ++i) {
                    const sm::jcv::site<float>* site = &sites[i];
                    const sm::jcv::graphedge<float>* e = site->edges;
                    std::uint32_t site_triangles = 0;
                    while (e) {
                        this->computeTriangle (site->p, e->pos[0], e->pos[1], this->setColour(site->index));
                        ++site_triangles;
                        e = e->next;
                    }
                    this->triangle_counts[i] = site_triangles;
                    this->site_indices[i] = site->index;
                    this->triangle_count_sum += site_triangles;
                }
            }
            if (static_cast<std::uint32_t>(vorman.diagram_numsites()) != ncoords) {
                std::cout << "WARNING: numsites (" << vorman.diagram_numsites()
                          << ") != ncoords (" << ncoords << ")?!?!\n";
            }

            // Draw optional objects
            if (this->debug_edges) {
                // Now scan through the edges drawing tubes for debug
                if (this->data_z_direction != sm::vec<>::uz()) {
                    // Apply rotations then compute
                    sm::vec<float> t0 = {0.0f};
                    sm::vec<float> t1 = {0.0f};
                    sm::quaternion<float> rqinv = rq.invert();
                    for (std::int32_t i = 0; i < vorman.diagram_numsites(); ++i) {
                        const sm::jcv::site<float>* site = &sites[i];
                        const sm::jcv::graphedge<float>* e = site->edges;
                        while (e) {
                            t0 = rqinv * (e->pos[0] * this->zoom);
                            t1 = rqinv * (e->pos[1] * this->zoom);
                            this->computeTube (t0, t1, mplot::colour::royalblue, mplot::colour::goldenrod2, this->voronoi_grid_thickness, 12);
                            e = e->next;
                        }
                    }

                } else {
                    // No rotations required
                    for (std::int32_t i = 0; i < vorman.diagram_numsites(); ++i) {
                        const sm::jcv::site<float>* site = &sites[i];
                        const sm::jcv::graphedge<float>* e = site->edges;
                        while (e) {
                            this->computeTube (e->pos[0] * this->zoom, e->pos[1] * this->zoom,
                                               mplot::colour::royalblue, mplot::colour::goldenrod2, this->voronoi_grid_thickness, 12);
                            e = e->next;
                        }
                    }
                }
            }

            if (this->show_voronoi2d) {
                sm::vec<float> t0 = {0.0f};
                sm::vec<float> t1 = {0.0f};
                sm::quaternion<float> rqinv = {};
                if (this->data_z_direction != sm::vec<>::uz()) {
                    // Apply rotations then compute
                    rqinv = rq.invert();
                } // else show the 2D Voronoi diagram's edges at z=0

                for (std::int32_t i = 0; i < vorman.diagram_numsites(); ++i) {
                    const sm::jcv::site<float>* site = &sites[i];
                    const sm::jcv::graphedge<float>* e = site->edges;
                    while (e) {
                        t0 = rqinv * sm::vec<float>{ e->pos[0].x() * this->zoom, e->pos[0].y() * this->zoom, 0.0f };
                        t1 = rqinv * sm::vec<float>{ e->pos[1].x() * this->zoom, e->pos[1].y() * this->zoom, 0.0f };
                        this->computeTube (t0, t1, mplot::colour::black, mplot::colour::black, this->voronoi_grid_thickness, 6);
                        e = e->next;
                    }
                }
            }

            if (this->show_delaunay2d) {
                sm::vec<float> t0 = {0.0f};
                sm::vec<float> t1 = {0.0f};
                sm::quaternion<float> rqinv = {};
                if (this->data_z_direction != sm::vec<>::uz()) { rqinv = rq.invert(); }
                vorman.delaunay_begin();
                sm::jcv::delaunay_edge<float> de;
                while (vorman.delaunay_next (&de) != 0) {
                    t0 = rqinv * sm::vec<float>{ de.pos[0].x() * this->zoom, de.pos[0].y() * this->zoom, 0.0f };
                    t1 = rqinv * sm::vec<float>{ de.pos[1].x() * this->zoom, de.pos[1].y() * this->zoom, 0.0f };
                    this->computeTube (t0, t1, mplot::colour::indigo, mplot::colour::indigo, this->voronoi_grid_thickness, 6);
                }
            }

            if (this->debug_dataCoords) {
                // Add some spheres at the original data points for debugging
                for (std::uint32_t i = 0; i < ncoords; ++i) {
                    this->computeSphere ((*this->dataCoords)[i] * this->zoom, mplot::colour::black, this->dataCoord_sphere_size);
                }
                // Polygonal boundary (if used)
                for (auto b : this->boundary) {
                    this->computeSphere (b * this->zoom, mplot::colour::dodgerblue2, 3.0f * this->dataCoord_sphere_size);
                    // Show text of boundary vertex position like this
                    // this->addLabel (b.str(), b * this->zoom + labelOffset, mplot::TextFeatures(labelSize) );
                }
            }
        }

        void reinitColoursScalar()
        {
            if (this->colourScale.do_autoscale == true) { this->colourScale.reset(); }
            this->dcolour.resize (this->scalarData->size());
            this->colourScale.transform (*(this->scalarData), this->dcolour);

            // Replace elements of vertexColors
            std::uint32_t tcounts = 0;
            for (std::size_t i = 0u; i < this->triangle_counts.size(); ++i) {
                auto c = this->cm.convert (this->dcolour[this->site_indices[i]]);
                std::size_t d_idx = tcounts * 3 * this->ncol; // ncol floats per vtx, 3 vtxs per tri
                for (std::size_t j = 0; j < 3 * this->triangle_counts[i]; ++j) {
                    // This is ONE colour vertex. Need 3 per triangle.
                    this->vertexColors[d_idx + this->ncol * j]     = c[0];
                    this->vertexColors[d_idx + this->ncol * j + 1] = c[1];
                    this->vertexColors[d_idx + this->ncol * j + 2] = c[2];
                    if (this->ncol == 4) { this->vertexColors[d_idx + this->ncol * j + 3] = 1.0f; }
                }
                tcounts += this->triangle_counts[i];
            }

            // Lastly, this call copies vertexColors (etc) into the OpenGL memory space
            this->reinit_colour_buffer();
        }

        //! Called by reinitColours when vectorData is not null (vectors are probably RGB colour)
        void reinitColoursVector()
        {
            if (this->colourScale.do_autoscale == true) { this->colourScale.reset(); }
            if (this->colourScale2.do_autoscale == true) { this->colourScale.reset(); }
            if (this->colourScale3.do_autoscale == true) { this->colourScale.reset(); }

            for (std::uint32_t i = 0; i < this->vectorData->size(); ++i) {
                this->dcolour[i] = (*this->vectorData)[i][0];
                this->dcolour2[i] = (*this->vectorData)[i][1];
                this->dcolour3[i] = (*this->vectorData)[i][2];
            }
            if (this->cm.getType() != mplot::ColourMapType::RGB
                && this->cm.getType() != mplot::ColourMapType::RGBMono
                && this->cm.getType() != mplot::ColourMapType::RGBGrey) {
                this->colourScale.transform (this->dcolour, this->dcolour);
                this->colourScale2.transform (this->dcolour2, this->dcolour2);
                this->colourScale3.transform (this->dcolour3, this->dcolour3);
            } // else assume dcolour/dcolour2/dcolour3 are all in range 0->1 (or 0-255) already

            // Replace elements of vertexColors
            std::uint32_t tcounts = 0;
            for (std::size_t i = 0u; i < this->triangle_counts.size(); ++i) {
                std::array<float, 3> c = this->setColour (this->site_indices[i]);
                std::size_t d_idx = tcounts * 3 * this->ncol; // ncol floats per vtx, 3 vtxs per tri
                for (std::size_t j = 0; j < 3 * this->triangle_counts[i]; ++j) {
                    // This is ONE colour vertex. Need 3 per triangle.
                    this->vertexColors[d_idx + this->ncol * j]     = c[0];
                    this->vertexColors[d_idx + this->ncol * j + 1] = c[1];
                    this->vertexColors[d_idx + this->ncol * j + 2] = c[2];
                    if (this->ncol == 4) { this->vertexColors[d_idx + this->ncol * j + 3] = 1.0f; }
                }
                tcounts += this->triangle_counts[i];
            }

            // Lastly, this call copies vertexColors (etc) into the OpenGL memory space
            this->reinit_colour_buffer();
        }

        void reinitColours()
        {
            if (this->vertexColors.size() < this->triangle_count_sum * this->ncol) {
                throw std::runtime_error ("vertexColors is not big enough to reinitColours()");
            }

            // now sub-call the scalar or vector reinit colours function
            if (this->scalarData != nullptr) {
                this->reinitColoursScalar();
            } else if (this->vectorData != nullptr) {
                this->reinitColoursVector();
            } else {
                throw std::runtime_error ("No data to reinitColours()");
            }
        }

        //! Zoom in? To zoom in (make bigger) choose a value >1
        float zoom = 1.0f;

        //! If true, show 2.5D Voronoi edges
        bool debug_edges = false;
        //! If true, show 2D Voronoi edges
        bool show_voronoi2d = false;
        //! If true, show 2D Delaunay Triangulation
        bool show_delaunay2d = false;
        //! The thickness of the lines (tubes) used to draw the 2D Voronoi grid (if show_voronoi2d is true)
        float voronoi_grid_thickness = 0.01f;
        //! If true, show black spheres at dataCoord locations
        bool debug_dataCoords = false;
        //! The size of the black spheres are dataCoord locations
        float dataCoord_sphere_size = 0.008f;
        //! What direction should be considered 'z' when converting the data into a voronoi diagram?
        //! The data values will be rotated before the Voronoi pass, then rotated back.
        sm::vec<float> data_z_direction = sm::vec<>::uz();

        //! You can add a little extra to the rectangle that is auto-detected from the
        //! datacoordinate ranges. This defaults to epsilon to give the best possible
        //! surface with a rectangular grid.
        float border_width = -1;
        // Create a rectangular domain or use a smoother (circular) boundary?
        domain_shape dom_shape = domain_shape::rectangular;
        // When making the boundary, how many points? Or use some f (dcoords.size())?
        std::uint32_t num_boundary_points = 30;
        // When finding ellipsoid shape from PCA, how many sigmas to multiply the principle axes length by?
        float n_sigma = 3.0f;

        // Do we add index labels?
        bool labelIndices = false;
        sm::vec<float, 3> labelOffset = { 0.04f, 0.0f, 0.0f };
        float labelSize = 0.03f;

    protected:
        //! Compute a triangle from 3 arbitrary corners
        void computeTriangle (sm::vec<float> c1, sm::vec<float> c2, sm::vec<float> c3, const std::array<float, 3>& colr)
        {
            c1 *= this->zoom;
            c2 *= this->zoom;
            c3 *= this->zoom;
            // v is the face normal
            sm::vec<float> u1 = c1-c2;
            sm::vec<float> u2 = c2-c3;
            sm::vec<float> v = u1.cross(u2);
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

        //! Have to record the number of triangles in each cell in order to update the colours
        sm::vvec<std::uint32_t> triangle_counts;
        //! Record the data index for each Voronoi cell index
        sm::vvec<std::uint32_t> site_indices;
        std::uint32_t triangle_count_sum = 0;
        // Polygon domain coordinates
        std::vector<sm::vec<float>> boundary;
        //! Internally owned version of dataCoords after rotation
        std::vector<sm::vec<float>> dcoords;
        //! A pointer to dcoords
        const std::vector<sm::vec<float>>* dcoords_ptr;
    };

} // namespace mplot
