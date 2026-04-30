//
// A visualmodel to render a compound-ray compound eye.
//
module;

#include <array>
#include <vector>
#include <mplot/jcvoronoi/jc_voronoi.h>

export module mplot.compoundray.eyevisual;

export import sm.mathconst;
export import sm.vec;
import sm.mat;
import sm.range;
import sm.geometry;
import sm.centroid;

export import mplot.gl.version;
export import mplot.visualmodel;
export import mplot.compoundray.ommatidium;
import mplot.tools;

export namespace mplot::compoundray
{
    // Helper function. Read the compound-ray csv eye file into ommatidia. ommatidia should be a pointer to an allocate vector.
    [[maybe_unused]] std::vector<mplot::compoundray::Ommatidium>*
    readEye (std::vector<mplot::compoundray::Ommatidium>* ommatidia, const std::string& path)
    {
        if (ommatidia == nullptr) { return ommatidia; }

        std::cout << "Path: " << path << std::endl;

        ommatidia->clear();

        std::ifstream eyeDataFile (path, std::ifstream::in);
        if(!eyeDataFile.is_open()) {
            std::cout << "Failed to open eye data file " << path << "\n";
            return ommatidia;
        }

        std::string line;
        size_t ommCount = 0;
        while (std::getline (eyeDataFile, line)) {
            std::vector<std::string> splitData = mplot::tools::stringToVector (line, " ");
            if (splitData.size() < 8) {
                std::cout << "Malformed line, continue...\n";
                continue;
            }
            mplot::compoundray::Ommatidium o = {
                sm::vec<float, 3>{ std::stof(splitData[0]), std::stof(splitData[1]), std::stof(splitData[2]) },
                sm::vec<float, 3>{ std::stof(splitData[3]), std::stof(splitData[4]), std::stof(splitData[5]) },
                std::stof(splitData[6]),
                std::stof(splitData[7])
            };
            std::cout << "o: " << o.relativePosition << "; " << o.relativeDirection << "; " << o.acceptanceAngleRadians << "; " << o.focalPointOffset << std::endl;
            ommatidia->push_back (o);
            ommCount++;
        }
        std::cout <<  "  Loaded " << ommCount << " ommatidia." << std::endl;

        return ommatidia;
    }

    //! This class creates a visualization of a compound-ray format compound eye model
    template<int glver = mplot::gl::version_4_1>
    class EyeVisual : public mplot::VisualModel<glver>
    {
    public:
        EyeVisual() {}

        //! Initialise with offset, start and end coordinates, radius and a single colour.
        EyeVisual (const sm::vec<float, 3> _offset,
                   std::vector<std::array<float, 3>>* _ommData,
                   std::vector<mplot::compoundray::Ommatidium>* _ommatidia,
                   const mplot::meshgroup* _head_mesh = nullptr)
        {
            this->init (_offset, _ommData, _ommatidia, _head_mesh);
        }

        ~EyeVisual() {}

        void init (const sm::vec<float, 3> _offset,
                   std::vector<std::array<float, 3>>* _ommData,
                   std::vector<mplot::compoundray::Ommatidium>* _ommatidia,
                   const mplot::meshgroup* _head_mesh = nullptr)
        {
            this->viewmatrix.translate (_offset);
            this->ommData = _ommData;
            this->ommatidia = _ommatidia;
            this->head_mesh = _head_mesh;
        }

        void reinitColours()
        {
            if (ommData == nullptr) { return; }
            if (ommData->empty()) { return; }
            size_t n_verts = this->vertexColors.size(); // should be tube_vertices * n_omm
            if (n_verts == 0u) { return; } // model doesn't exist yet
            size_t n_omm = ommData->size();

            std::size_t i_3d = 0;
            if (show_3d) {
                // Replace colours for the 3D part of the model
                int num_vertices = disc_vertices;
                if (this->show_cones == true && this->cones_will_show) {
                    num_vertices = cone_vertices + disc_vertices;
                } // else num_vertices = disc_vertices;

                // Re-colour cones built from a focal point offset and acceptance angle
                for (size_t i = 0u; i < n_omm; ++i) {
                    // Update the 3 RGB values in vertexColors tube_vertices times
                    int j = 0;
                    for (; j < num_vertices; ++j) {
                        this->vertexColors[i * num_vertices * 3 + j * 3] =     (*ommData)[i][0];
                        this->vertexColors[i * num_vertices * 3 + j * 3 + 1] = (*ommData)[i][1];
                        this->vertexColors[i * num_vertices * 3 + j * 3 + 2] = (*ommData)[i][2];
                    }
                }
                // i_3d is the index offset for the 3D part
                i_3d = n_omm * num_vertices * 3;
            }

            // Replace colours in the 2D part of the model
            for (uint32_t pri = 0; pri < this->projections.size(); ++pri) {
                // Replace elements of vertexColors
                std::size_t tcounts = 0;
                std::size_t d_2d = 0;
                for (std::size_t i = 0u; i < this->projections[pri].triangle_counts.size(); ++i) {
                    auto c = (*ommData)[this->projections[pri].site_indices[i] + this->projections[pri].start_i];
                    std::size_t d_idx = i_3d + tcounts * 9; // 3 floats per vtx, 3 vtxs per tri
                    for (std::size_t j = 0; j < 3 * this->projections[pri].triangle_counts[i]; ++j) {
                        // This is ONE colour vertex. Need 3 per triangle.
                        this->vertexColors[d_idx + 3 * j]     = c[0];
                        this->vertexColors[d_idx + 3 * j + 1] = c[1];
                        this->vertexColors[d_idx + 3 * j + 2] = c[2];
                        d_2d += 3;
                    }
                    tcounts += this->projections[pri].triangle_counts[i];
                }
                i_3d += d_2d;
            }

            // Lastly, this call copies vertexColors (etc) into the OpenGL memory space
            this->reinit_colour_buffer();
        }

        // Available projections
        enum class projection_type : uint8_t
        {
            mercator,
            cassini,
            equirectangular,
            cylindrical
        };

        // Project latitude/longitude ll with projection type t and given radius
        sm::vec<float, 2> spherical_projection (const sm::vec<float, 2>& ll, projection_type t, const float radius)
        {
            sm::vec<float, 2> xy = {};
            if (t == projection_type::equirectangular) {
                //sm::mathconst<float>::pi_over_2
                xy = sm::geometry::spherical_projection::equirectangular (ll, radius);
            } else if (t == projection_type::cassini) {
                xy = sm::geometry::spherical_projection::cassini (ll, radius);
            } else if (t == projection_type::cylindrical) {
                throw std::runtime_error ("Not a spherical projection");
            } else {
                xy = sm::geometry::spherical_projection::mercator (ll, radius);
            }
            return xy;
        }

        // 2D positions for the ommatidia centres encoded in 3D vecs. Gets re-used for each projection
        sm::vvec<sm::vec<double, 3>> omm2d;

        /*
         * Possibly each of these need replication for each of multiple 2d projections
         */
        struct projection_data
        {
            // Use this to position the 2D map wrt the three D model. You can translate, scale and rotate
            sm::mat<float, 4> twod_transform;
            // The user-provided radius of the projection sphere. Will need to match the size of the compound ray eye
            float proj_radius = 0.0f;
            // The centre of the user-provided projection sphere or cylinder
            sm::vec<float> proj_centre = {};
            // The height of a projection cylinder
            sm::vec<float> proj_height = {};
            // A rotation to apply before projecting
            sm::quaternion<float> proj_rotation;
            // Which spherical to 2D projection to use?
            projection_type proj_type = projection_type::mercator;
            // Have to record the number of triangles in each cell in the 2D map in order to update the colours
            sm::vvec<uint32_t> triangle_counts;
            // Record the data index for each Voronoi cell index. For reinitColours
            sm::vvec<uint32_t> site_indices;
            // Sum of triangles used for reinitColours
            uint32_t triangle_count_sum = 0;
            // Starting ommatidium index for projection
            uint32_t start_i = 0;
            // End index
            uint32_t end_i = std::numeric_limits<uint32_t>::max();
        };

        // A compound eye visualization may require several projections to 2D
        std::vector<projection_data> projections;

        void add_spherical_projection (projection_type t, const sm::mat<float, 4>& _twod_transform,
                                       const sm::vec<float>& centre, const float radius,
                                       const sm::quaternion<float> rotn = sm::quaternion<float>(),
                                       const uint32_t _start_i = 0,
                                       const uint32_t _end_i = std::numeric_limits<uint32_t>::max())
        {
            projection_data d;
            d.proj_type = t;
            d.twod_transform = _twod_transform;
            d.proj_centre = centre;
            d.proj_radius = radius;
            d.proj_rotation = rotn;
            d.proj_rotation.renormalize();
            d.start_i = _start_i;
            d.end_i = _end_i;
            this->projections.push_back (d);
        }

        void add_spherical_projection (projection_type t, const sm::vec<float, 3>& _twod_offset,
                                       const sm::vec<float>& centre, const float radius,
                                       const sm::quaternion<float> rotn = sm::quaternion<float>(),
                                       const uint32_t _start_i = 0,
                                       const uint32_t _end_i = std::numeric_limits<uint32_t>::max())
        {
            sm::mat<float, 4> tr;
            tr.translate (_twod_offset);
            this->add_spherical_projection (t, tr, centre, radius, rotn, _start_i, _end_i);
        }

        void add_cylindrical_projection (const sm::mat<float, 4>& _twod_transform, const sm::vec<float>& centre,
                                         const float radius, const float height,
                                         const uint32_t _start_i = 0,
                                         const uint32_t _end_i = std::numeric_limits<uint32_t>::max())
        {
            projection_data d;
            d.proj_type = projection_data::cylindrical;
            d.twod_transform = _twod_transform;
            d.proj_centre = centre;
            d.proj_radius = radius;
            d.proj_height = height;
            d.start_i = _start_i;
            d.end_i = _end_i;
            this->projections.push_back (d);
        }

        //! Initialize vertex buffer objects and vertex array object.
        void initializeVertices()
        {
            this->vertexPositions.clear();
            this->vertexNormals.clear();
            this->vertexColors.clear();
            this->indices.clear();

            // Sanity check our data pointers and return or throw
            if (ommData == nullptr || ommatidia == nullptr) { return; }
            if (ommatidia != nullptr && ommatidia->empty()) { return; }
            if (ommData != nullptr && ommData->empty()) { return; }
            if (ommData->size() != ommatidia->size()) {
                throw std::runtime_error ("sizes mismatch!");
            }

            // Draw ommatidia
            size_t n_omm = ommData->size();

            // Determine eye dimensions
            sm::range<sm::vec<float, 3>> ommrng = sm::range<sm::vec<float, 3>>::search_initialized();
            for (size_t i = 0u; i < n_omm; ++i) { ommrng.update ((*ommatidia)[i].relativePosition); }
            float ray_radius = ommrng.span().max() / 500.0f;

            // Find mean minimum ommatidial distance
            if (this->min_dist_to_other.empty()) {
                sm::vvec<float> dist_to_other (n_omm, 0.0f);
                min_dist_to_other.resize (n_omm, 0.0f);
                for (size_t i = 0u; i < n_omm; ++i) {
                    for (size_t j = 0u; j < n_omm; ++j) {
                        if (i == j) {
                            dist_to_other[j] = 10000.0f;
                        } else {
                            dist_to_other[j] = ((*ommatidia)[i].relativePosition - (*ommatidia)[j].relativePosition).length();
                        }
                    }
                    min_dist_to_other[i] = dist_to_other.min();
                }
            }
            // std::cerr << "Mean ommatidial distance: " << this->min_dist_to_other.mean() << std::endl;

            // First find out if all focal points are 0
            this->focal_point_sum = 0.0f;
            for (size_t i = 0u; i < n_omm; ++i) {
                this->focal_point_sum += std::abs((*ommatidia)[i].focalPointOffset);
            }

            if (show_3d && this->focal_point_sum > 0.0f) {
                std::cout << "Stanza 1\n";
                // We have focal points, so draw with the relativePosition representing the centre
                // of the ommatidial lens - the base of a cone - which then extends back to the cone
                // tip, which can be thought of as the location of the ommatidial 'sensor'
                for (size_t i = 0u; i < n_omm; ++i) {
                    // Ommatidia colour, position/shape
                    std::array<float, 3> colour = (*ommData)[i];
                    float angle = (*ommatidia)[i].acceptanceAngleRadians;
                    float focal_point = std::abs((*ommatidia)[i].focalPointOffset);
                    sm::vec<float, 3> pos = (*ommatidia)[i].relativePosition;
                    sm::vec<float, 3> dir = (*ommatidia)[i].relativeDirection;
                    dir.renormalize();
                    // Tip of cone is 'behind' the position of the ommatidial face/lens
                    sm::vec<float, 3> ommatidial_detector_point = pos - dir * focal_point;
                    // work out a radius from acceptance angle and focal_point
                    // The discs are based on the inter-ommatidial distances in space, which have to have been computed
                    float dw = this->min_dist_to_other[i];
                    this->computeTube (pos, pos + (0.05f * dw * dir), colour, colour, dw * 0.5f, tube_faces);

                    // This visualizes the optical cones
                    if (this->show_cones == true && this->show_fov == false) {
                        float optical_radius = focal_point * std::tan (angle / 2.0f);
                        // Colour comes from ommData. ringoffset is 1.0f
                        this->computeCone (pos, ommatidial_detector_point, 0.0f, colour, optical_radius, tube_faces);
                        this->cones_will_show = true;

                    } else if (this->show_fov == true) {
                        // do a cone of angle 'acceptanceAngle' using user-supplied cone_length, starting FROM the disc to show field of view of the eye
                        sm::vec<float, 3> ommatidial_cone_pos = pos + dir * this->cone_length;
                        float radius = this->cone_length * std::tan (angle / 2.0f);
                        this->computeCone (ommatidial_cone_pos, pos, 0.0f, colour, radius, tube_faces);
                    }
                }

                if ((this->show_cones == true || this->show_fov == true) && n_omm > 0) {
                    this->cones_will_show = true;
                } else {
                    this->cones_will_show = false;
                }

            } else if (show_3d && this->focal_point_sum <= 0.0f) {
                // std::cout << "Stanza 2\n";
                // All our focal_points are 0. Don't have focal point offset to help define our
                // cones, only acceptance angle. Use manually specified tube_length (or computed
                // radius) to figure out the size of a cone, whose tip is the location of the
                // ommatidial sensor AND the centre of the ommatidial lens
                for (size_t i = 0u; i < n_omm; ++i) {
                    std::array<float, 3> colour = (*ommData)[i];
                    float angle = (*ommatidia)[i].acceptanceAngleRadians;
                    // pos will be the tip of the cone in this case, and the centre of the disc
                    sm::vec<float, 3> pos = (*ommatidia)[i].relativePosition;
                    sm::vec<float, 3> dir = (*ommatidia)[i].relativeDirection;
                    dir.renormalize();

                    float dw = this->min_dist_to_other[i];
                    this->computeTube (pos, pos + (0.05f * dw * dir), colour, colour, dw * 0.5f, tube_faces);
                    // We don't have a focal length to show cones, but we can still show the acceptance angle
                    if (this->show_fov == true) {
                        // do a cone of angle 'acceptanceAngle' using user-supplied cone_length
                        sm::vec<float, 3> ommatidial_cone_pos = pos + dir * this->cone_length;
                        float radius = this->cone_length * std::tan (angle / 2.0f);
                        this->computeCone (ommatidial_cone_pos, pos, 0.0f, colour, radius, tube_faces);
                    }
                }

                if (this->show_fov == true && n_omm > 0) {
                    this->cones_will_show = true;
                } else {
                    this->cones_will_show = false;
                }

            } else {
                this->cones_will_show = false;
            }

            // 2D projections
            for (uint32_t pri = 0; pri < this->projections.size(); ++pri) {
                this->omm2d.clear();
                if (this->projections[pri].proj_type == projection_type::cylindrical) {
                    std::cout << "Cylindrical projections are currently unimplemented\n";
                } else {
                    // Compute intersections between ommatidia direction vectors and our projection sphere.

                    // Rotate coordinates as the compound eye looks forwards along z, whereas 2D
                    // projections look forwards along x by convention.
                    sm::mat<float, 4> coord_rotn;
                    coord_rotn.rotate (sm::vec<>::uz(), sm::mathconst<float>::pi_over_2);
                    coord_rotn.rotate (sm::vec<>::ux(), sm::mathconst<float>::pi_over_2);

                    for (size_t i = this->projections[pri].start_i; i < this->ommatidia->size() && i < this->projections[pri].end_i; ++i) {
                        sm::vec<sm::vec<>, 2> sph_coord = sm::geometry::ray_sphere_intersection (this->projections[pri].proj_centre,
                                                                                                 this->projections[pri].proj_radius,
                                                                                                 (*ommatidia)[i].relativePosition,
                                                                                                 -(*ommatidia)[i].relativeDirection);
                        if (sph_coord[0][0] != std::numeric_limits<float>::max()) {
                            sph_coord[0] -= this->projections[pri].proj_centre; // offset by centre before rotation
                            // sph_coord[0] is the coordinate for the ommatidia pixel on the sphere
                            sm::vec<float, 3> rot_coord = (coord_rotn * sph_coord[0]).less_one_dim();
                            // Now apply our projection rotation quaternion
                            rot_coord = this->projections[pri].proj_rotation * rot_coord;
                            if (rot_coord.has_nan()) { throw std::runtime_error ("rot_coord has NaN"); }
                            sm::vec<float, 2> ll = sm::geometry::spherical_projection::xyz_to_latlong (rot_coord);
                            if (ll.has_nan()) { throw std::runtime_error ("latlong has NaN"); }
                            sm::vec<float, 2> xy = this->spherical_projection (ll, this->projections[pri].proj_type, this->projections[pri].proj_radius);
                            // Add xy as one of the points that we'll make a Voronoi diagram from.
                            this->omm2d.push_back (xy.plus_one_dim().as<double>());
                        }
                    }
                    // Make 2D Voronoi of omm2d.
                    this->voronoi2d (pri);
                }
            }

            // Sphere and rays for finding a suitable 2D projection
            for (uint32_t pri = 0; pri < this->projections.size(); ++pri) {

                if (this->show_sphere) {
                    this->computeSphere (this->projections[pri].proj_centre, mplot::colour::grey50,
                                         this->projections[pri].proj_radius, 18, 18);
                }

                if (this->show_rays) {
                    for (size_t i = 0; i < this->ommatidia->size(); ++i) {
                        // Can now find intersections on our sphere
                        sm::vec<> l0 = (*ommatidia)[i].relativePosition;
                        sm::vec<> l = -(*ommatidia)[i].relativeDirection;
                        // Make rays a sensible length based on projections.proj_radius
                        l.renormalize();
                        l *= this->projections[pri].proj_radius * 3.0f;
                        // Show direction vector from ommatidium position
                        this->computeArrow (l0, l0 + l, mplot::colour::grey80, ray_radius);
                        // Recompute intersections
                        sm::vec<sm::vec<>, 2> intersections = sm::geometry::ray_sphere_intersection (this->projections[pri].proj_centre,
                                                                                                     this->projections[pri].proj_radius, l0, l);
                        if (intersections[0][0] != std::numeric_limits<float>::max()) {
                            // intersections[0] is the coordinate for the ommatidia pixel on the sphere
                            this->computeSphere (intersections[0], mplot::colour::crimson, 0.006f * this->projections[pri].proj_radius);
                        }
                    }
                }
            }

            // Optional head
            if (this->head_mesh != nullptr) { this->computeMeshgroup (*this->head_mesh); }
        }

        void voronoi2d (uint32_t pri)
        {
            // Use mplot::range to find the extents of dataCoords. From these create a
            // rectangle to pass to diagram_generate.
            int ncoords = static_cast<int>(this->omm2d.size());

            jcv::manager<double> vorman; // we need double precision for projections, float may run into trouble
            vorman.border_width = this->border_width;

            std::vector<sm::vec<double, 3>> boundary;

            // Copy 3D points to 2D
            sm::vvec<sm::vec<double, 2>> coords2 (omm2d.size());
            for (unsigned int i = 0; i < omm2d.size(); ++i) {
                coords2[i] = omm2d[i].less_one_dim();
            }
            auto bnd2centr = sm::algo::centroid (coords2);
            // Find convex hull
            sm::vvec<sm::vec<double, 2>> bnd2 = sm::geometry::graham_scan (coords2);
            boundary.resize (bnd2.size());
            // Copy 2D to 3D boundary
            for (unsigned int i = 0; i < bnd2.size(); ++i) {
                boundary[i] = bnd2[i].plus_one_dim();
                // Add border
                sm::vec<double, 2> brd = bnd2[i] - bnd2centr; // border vector from centroid to point
                brd.renormalize();
                brd *= this->border_width;
                boundary[i] += brd.plus_one_dim();
            }

            vorman.diagram_generate (this->omm2d, boundary);

            int diag_nsites = vorman.diagram_numsites();
            if (diag_nsites != ncoords) {
                std::cout << "WARNING: diagram's ncoords (" << diag_nsites << ") != ncoords (" << ncoords << ")?!?!\n";
            }

            // We obtain access to the Voronoi cell sites:
            const jcv::site<double>* sites = vorman.diagram_get_sites();

            for (int i = 0; i < diag_nsites && i < ncoords; ++i) {
                const jcv::site<double>* site = &sites[i];
                jcv::graphedge<double>* e = site->edges; // The very first edge
                while (e) {
                    // Set z. Should be done in jcvoronoi, but haven't found out how
                    e->pos[0][2] = this->omm2d[i][2];
                    e->pos[1][2] = e->pos[0][2];
                    e = e->next;
                }
            }

            // To draw triangles iterate over the 'sites' and get the edges
            this->projections[pri].triangle_counts.resize (ncoords, 0);
            this->projections[pri].site_indices.resize (ncoords, 0);
            this->projections[pri].triangle_count_sum = 0;

            sm::vvec<sm::vec<>> flat_triangles;          // contains a sequence of triplets of vecs
            sm::vvec<std::array<float, 3>> flat_colours; // fewer elements than flat_triangles
            // To draw triangles iterate over the 'sites' and draw triangles
            for (int i = 0; i < diag_nsites && i < ncoords; ++i) {
                const jcv::site<double>* site = &sites[i];
                const jcv::graphedge<double>* e = site->edges;
                this->projections[pri].site_indices[i] = site->index;
                std::array<float, 3> colour = mplot::colour::black;
                if (site->index + this->projections[pri].start_i < ommData->size()) {
                    colour = (*ommData)[site->index + this->projections[pri].start_i];
                } else {
                    std::cout << "Uh oh, can't access colour [" << site->index << " + " << this->projections[pri].start_i << "]\n";
                }
                uint32_t site_triangles = 0;
                while (e) {
                    flat_triangles.push_back (site->p.as<float>());
                    flat_triangles.push_back (e->pos[0].as<float>());
                    flat_triangles.push_back (e->pos[1].as<float>());
                    flat_colours.push_back (colour);
                    ++site_triangles;
                    e = e->next;
                }
                this->projections[pri].triangle_counts[i] = site_triangles;
                this->projections[pri].triangle_count_sum += site_triangles;
            }

            // Can now computeTriangles
            for (uint32_t i = 0; i < flat_triangles.size(); i += 3) {
                sm::vec<float> t1 = (this->projections[pri].twod_transform * flat_triangles[i]).less_one_dim();
                sm::vec<float> t2 = (this->projections[pri].twod_transform * flat_triangles[i + 1]).less_one_dim();
                sm::vec<float> t3 = (this->projections[pri].twod_transform * flat_triangles[i + 2]).less_one_dim();
                this->computeTriangle (t1, t2, t3, flat_colours[i/3]);
            }
        }

        // If false, hide 3D representation (the ommatidial cones and discs)
        bool show_3d = true;
        // If true, show optical cones, if possible
        bool show_cones = false;
        // If true, show 'field of view' cones. Overrides show_cones. Control size of cones with
        // this->cone_length
        bool show_fov = false;
        // either show_cones or show_fov are enabled and cones have been drawn
        bool cones_will_show = false;
        // The colours detected by each ommatidium
        std::vector<std::array<float, 3>>* ommData = nullptr;
        // The position and orientation of each ommatidium
        std::vector<mplot::compoundray::Ommatidium>* ommatidia = nullptr;
        // Distances to the nearest ommatidia, for choosing disc size. Computed once only
        sm::vvec<float> min_dist_to_other = {};
        // An optional head mesh
        const mplot::meshgroup* head_mesh = nullptr;
        // If sum is 0, then we have a special case for rendering the eye as we have no focal point
        // offsets specified for this eye (and hence the optical radius of the ommatidium is not known)
        float focal_point_sum = 0.0f;
        // Hard-coded number of faces making up an ommatidial element (the higher this is, the more round it will look)
        static constexpr int tube_faces = 18;
        // Rendering as cone. This is the number of vertices per cone.
        static constexpr int cone_vertices = tube_faces * 3 + 2;
        static constexpr int disc_vertices = tube_faces * 4 + 2;
        // Setter for cone_length must reinit vertices
        void set_cone_length (float _cone_length)
        {
            if (this->focal_point_sum > 0.0f) {
                std::cout << "WARNING: manual cone length will be ignored because "
                          << "compound-ray eye file specifies focal offsets\n";
            }
            this->cone_length = _cone_length; this->reinit();
        }
        void pre_set_cone_length (float _cone_length) { this->cone_length = _cone_length; }
        float get_cone_length() { return this->cone_length; }

        // Should projection spheres be shown visually (maybe by external code?
        bool show_sphere = false;
        // Should we show arrows/intersection locations with the projection sphere(s)?
        bool show_rays = false;
        // Width of borders around 2D map(s)
        float border_width = std::numeric_limits<float>::epsilon();

    private:
        // User-modifiable ommatidial cone length which is used if there's no focal point offset
        float cone_length = 0.1f;

        //! Compute a triangle from 3 arbitrary corners
        void computeTriangle (sm::vec<float> c1, sm::vec<float> c2, sm::vec<float> c3, const std::array<float, 3>& colr)
        {
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
            for (uint32_t i = 0; i < 3U; ++i) {
                this->vertex_push (colr, this->vertexColors);
                this->vertex_push (v, this->vertexNormals);
            }
            this->indices.push_back (this->idx++);
            this->indices.push_back (this->idx++);
            this->indices.push_back (this->idx++);
        }
    };

} // namespace mplot::compoundray
