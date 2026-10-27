module;

#include <cstdint>
#include <vector>
#include <array>
#include <stdexcept>
#include <string>
#include <sstream>
#include <map>
#include <unordered_map>
#include <algorithm> // transform()

export module mplot.healpixvisual;

import sm.scale;
import sm.vec;
import sm.vvec;
import mplot.visualmodel;
import mplot.colourmap;
import hp.bare;

export namespace mplot
{
    /*
     * Type T for the data. A HEALPix VisualModel which visualizes the values in
     * pixeldata, which should be indexed with the Healpix NEST index scheme.
     */
    template <typename T, std::int32_t glver = mplot::gl::version_4_1>
    struct HealpixVisual : public mplot::VisualModel<glver>
    {
    public:
        HealpixVisual(const sm::vec<float> _offset) : mplot::VisualModel<glver> (_offset)
        {
            this->colourScale.reset();
            this->colourScale.do_autoscale = true;
            this->reliefScale.reset();
            this->reliefScale.do_autoscale = true;
        }

        // Update the VisualModel, changing only colours if that's enough, or doing a
        // full rebuild if we're displaying relief.
        void reinit()
        {
            if (this->relief == true) { mplot::VisualModel<glver>::reinit(); }
            else { this->reinitColours(); }
        }

        void setColourData (std::vector<std::array<float, 3>>* cdata) { this->colourdata = cdata; }

        void reinitColours()
        {
            size_t n_data = this->n_pixels();

            if (this->vertexColors.size() < n_data * this->ncol) {
                throw std::runtime_error ("vertexColors is not big enough to reinitColours()");
            }

            if (this->colourdata == nullptr) {
                // Scale data
                sm::vvec<float> scaled_data (this->pixeldata.size());
                if (this->colourScale.do_autoscale == true) { this->colourScale.reset(); }
                this->colourScale.transform (this->pixeldata, scaled_data);

                // Re-colour
                for (size_t i = 0u; i < n_data; ++i) {
                    auto c = this->cm.convert (scaled_data[i]);
                    this->vertexColors[this->ncol * i] = c[0];
                    this->vertexColors[this->ncol * i + 1] = c[1];
                    this->vertexColors[this->ncol * i + 2] = c[2];
                    if (this->ncol == 4) { this->vertexColors[this->ncol * i + 3] = 1.0f; }
                }
            } else {
                // Use colour in colourdata directly, assuming it is in correct range (0->1 for each channel)
                for (size_t i = 0u; i < n_data; ++i) {
                    this->vertexColors[this->ncol * i] = (*colourdata)[i][0];
                    this->vertexColors[this->ncol * i + 1] = (*colourdata)[i][1];
                    this->vertexColors[this->ncol * i + 2] = (*colourdata)[i][2];
                    if (this->ncol == 4) { this->vertexColors[this->ncol * i + 3] = 1.0f; }
                }
            }

            // Lastly, this call copies vertexColors (etc) into the OpenGL memory space
            this->reinit_colour_buffer();
        }

        // Draw spheres at vertex locations (for debugging the geometry)
        void vertex_spheres()
        {
            std::int64_t n_p = this->n_pixels();

            // Determine a good sphere size
            std::int64_t p0 = 0;
            hp::t_ang ang = hp::ring2ang (this->nside, p0);
            hp::t_vec pv = hp::loc2vec (hp::ang2loc (ang));
            sm::vec<float> vpf0 = sm::vec<double>({pv.x, pv.y, pv.z}).as_float();
            ang = hp::ring2ang (this->nside, p0+1);
            pv = hp::loc2vec (hp::ang2loc (ang));
            sm::vec<float> vpf1 = sm::vec<double>({pv.x, pv.y, pv.z}).as_float();
            float vvdist = (vpf0-vpf1).length();

            for (std::int64_t p = 0; p < n_p; ++p) {
                // Convert ring index p to angle for this pixel
                hp::t_ang ang = hp::ring2ang (this->nside, p);
                // Find the location of the pixel
                hp::t_vec pv = hp::loc2vec (hp::ang2loc (ang));
                // Convert it into a sm::vec
                sm::vec<float> vpf = sm::vec<double>({pv.x, pv.y, pv.z}).as_float();

                this->computeSphere (vpf, mplot::colour::black, vvdist * 0.05f, 18, 18);
            }
        }

        // Draw one sphere at each face start vertex (the lowest NEST index for each face)
        void face_spheres()
        {
            for (std::int32_t f = 0; f < 12; ++f) {
                std::int64_t p = f << (k+k); // gets nested face start index
                // nest2ang and ring2ang return same angle for 12 faces in a zeroth order healpix
                hp::t_ang ang = hp::nest2ang (this->nside, p);
                hp::t_vec pv = hp::loc2vec (hp::ang2loc (ang));
                sm::vec<float> vpf = (sm::vec<double>({pv.x, pv.y, pv.z}) * this->r).as_float();
                std::array<float, 3> sc = this->cm.convert (this->pixeldata[p]);
                this->computeSphere (vpf, sc, this->r/30.0f, 18, 18);
                this->addLabel (std::string("face ") + std::to_string(f), (vpf  * this->r * 1.15f),
                                mplot::TextFeatures(0.03f, mplot::colour::black) );
            }
        }

        /*
         * Find a neighbour either 'forwards' or 'across' within a patch of HEALPix
         * NESTed indices.
         *
         * This function uses the feature of the nested index scheme that flipping odd
         * or even bits of an index can obtain the neighbour, along with sometimes
         * degrading to a lower order index, then prograding back.
         *
         * \param x input The index in one axis along the patch (x or y) for the quads
         * of interest
         *
         * \param neighbxor input The value to XOR an index to find the neighbour
         * candidate. 0x5 for forwards, 0xa for across.
         *
         * \param i_up input The four HEALpix indices for which we are finding two
         * neighbours
         *
         * \param i_nb output The two neighbours forwards or across
         */
        void find_quad_neighbour (const std::int64_t& x, const std::int64_t neighbxor,
                                  const sm::vec<std::int64_t, 4>& i_up, sm::vec<std::int64_t, 2>& i_nb)
        {
            if (i_up > -1LL == false) { return; }
            std::int64_t nside_down = 1LL << (this->k - 1);

            // Invert bottom four odd bits of i_nb for the 'neighbours forward'
            std::transform (i_nb.begin(), i_nb.end(), i_nb.begin(), [neighbxor](std::int64_t ii){ return ii ^ neighbxor; });

            if (i_nb[0] > i_up[0]) { // Accept
            } else if ((x+1) % nside_down != 0) { // Not at end of patch; can't accept
                std::int64_t i_pgrd = 0;       // prograded index
                std::int64_t i_dgrd = i_up[1]; // degraded index
                std::uint32_t rtn_steps = 0;
                std::int64_t fwd_mask = 0x3;
                bool found_neighbour = false;
                while (found_neighbour == false) {
                    // Degrade. and increment rtn_steps
                    i_dgrd >>= 2;
                    rtn_steps += 2;
                    fwd_mask |= 0x3 << rtn_steps;
                    // Every 2 loops, we have to apply 4 more bits of neighbour relationship to i_nb:
                    if (rtn_steps % 4 == 0) {
                        std::transform (i_nb.begin(), i_nb.end(), i_nb.begin(), [neighbxor, rtn_steps](std::int64_t ii){return ii ^ (neighbxor << rtn_steps);});
                    }
                    // do fwd neighbour on i_dgrd and check if it can be used
                    std::int64_t i_dgrd_neighb = i_dgrd ^ neighbxor;
                    if (i_dgrd_neighb > i_dgrd) {
                        // i_dgrd_neighb is good, Prograde it.
                        i_pgrd = i_dgrd_neighb << rtn_steps;
                        found_neighbour = true;
                    } // else next loop
                }
                // Now apply i_pgrd in a transformation of i_nb
                std::transform (i_nb.begin(), i_nb.end(), i_nb.begin(), [i_pgrd, fwd_mask](std::int64_t ii){return (ii & fwd_mask) | i_pgrd;});
            } else { // End of patch
                i_nb.set_from (-1);
            }
        }

        // Map face index to {NE, SE} face indices. Face 0 has Face 1 to the NE and Face 5 to the SE and so on.
        // A direction is ORed into the index, shifted 8 bits
        // 1, 2, 4, 8 means NE, NW, SW, SE.
        // 1 | 2<<8 means the neighbour is face 1 and it joins on its NW edge.
        // 0 | 4<<8 means the neighbour is face 0 and it joins on its SW edge.
        std::map<std::int32_t, sm::vec<std::int32_t, 4>> face_map{ {0,  {1 | 2<<8,  5  | 2<<8 }},
                                                                   {1,  {2 | 2<<8,  6  | 2<<8 }},
                                                                   {2,  {3 | 2<<8,  7  | 2<<8 }},
                                                                   {3,  {0 | 2<<8,  4  | 2<<8 }},

                                                                   {4,  {0 | 4<<8,  8  | 2<<8 }},
                                                                   {5,  {1 | 4<<8,  9  | 2<<8 }},
                                                                   {6,  {2 | 4<<8,  10 | 2<<8 }},
                                                                   {7,  {3 | 4<<8,  11 | 2<<8 }},

                                                                   {8,  {5 | 4<<8,  9  | 4<<8 }},
                                                                   {9,  {6 | 4<<8,  10 | 4<<8 }},
                                                                   {10, {7 | 4<<8,  11 | 4<<8 }},
                                                                   {11, {4 | 4<<8,  8  | 4<<8 }}  };

        // corners to be in rotated order
        void fill_square (const std::array<hp::t_hpd, 4>& corners)
        {
            std::int64_t c0 = hp::hpd2nest (this->nside, corners[0]);
            std::int64_t c2 = hp::hpd2nest (this->nside, corners[2]);
            this->indices.push_back (this->idx + c0);
            this->indices.push_back (this->idx + hp::hpd2nest (this->nside, corners[1]));
            this->indices.push_back (this->idx + c2);
            this->indices.push_back (this->idx + c0);
            this->indices.push_back (this->idx + c2);
            this->indices.push_back (this->idx + hp::hpd2nest (this->nside, corners[3]));
        }

        void fill_triangle (const std::array<hp::t_hpd, 3>& corners)
        {
            this->indices.push_back (this->idx + hp::hpd2nest (this->nside, corners[0]));
            this->indices.push_back (this->idx + hp::hpd2nest (this->nside, corners[1]));
            this->indices.push_back (this->idx + hp::hpd2nest (this->nside, corners[2]));
        }

        // corners to be in raster order
        void fill_square (const sm::vec<std::int64_t, 4>& corners_nest)
        {
            this->indices.push_back (this->idx + corners_nest[0]);
            this->indices.push_back (this->idx + corners_nest[1]);
            this->indices.push_back (this->idx + corners_nest[2]);
            this->indices.push_back (this->idx + corners_nest[1]);
            this->indices.push_back (this->idx + corners_nest[3]);
            this->indices.push_back (this->idx + corners_nest[2]);
        }
        // corners to be in raster order
        void fill_square (const std::int64_t c0, const std::int64_t c1, const std::int64_t c2, const std::int64_t c3)
        {
            this->indices.push_back (this->idx + c0);
            this->indices.push_back (this->idx + c1);
            this->indices.push_back (this->idx + c2);
            this->indices.push_back (this->idx + c1);
            this->indices.push_back (this->idx + c3);
            this->indices.push_back (this->idx + c2);
        }

        // Fill the channel between faces and their neighbour to the NE.
        //
        // What's x and y indices for start_ne and start_se?
        // start_ne is      (xmax, 0) and increase y
        // start_se is just (0,    0) and increase x
        //
        // target_nw edge is from (0, ymax) increasing x
        // target_sw edge         (0,    0) increasing y
        void fill_channels_ne()
        {
            std::int64_t x = this->nside - 1, x2 = 0, y2 = this->nside - 1;
            std::array<hp::t_hpd, 4> corners;

            for (std::int32_t f = 0; f < 12; ++f) {
                std::int32_t ne_face = face_map[f][0] & 0xff;
                std::int32_t ne_dirn = face_map[f][0] >> 8;
                if (ne_dirn == 2) { // NW edge on neighbour
                    for (std::int64_t y = 0; y < y2; y++) {
                        corners = {hp::t_hpd{x, y,   f},  hp::t_hpd{x, y+1, f}, hp::t_hpd{y+1, y2, ne_face}, hp::t_hpd{y, y2, ne_face} };
                        this->fill_square (corners);
                    }
                } else if (ne_dirn == 4) { // SW edge on neighbour
                    for (std::int64_t y = 0; y < y2; y++) {
                        corners = {hp::t_hpd{x, y,   f},  hp::t_hpd{x, y+1, f},  hp::t_hpd{x2, y+1, ne_face}, hp::t_hpd{x2, y, ne_face} };
                        this->fill_square (corners);
                    }
                }
            }
        }

        // Fill the channel between faces and their neighbour to the SE.
        //
        // What's x and y indices for start_ne and start_se?
        // start_ne is      (xmax, 0) and increase y
        // start_se is just (0,    0) and increase x
        //
        // target_nw edge is from (0, ymax) increasing x
        // target_sw edge         (0,    0) increasing y
        void fill_channels_se()
        {
            std::int64_t y = 0, x2 = 0, y2 = this->nside - 1;
            std::array<hp::t_hpd, 4> corners;

            for (std::int32_t f = 0; f < 12; ++f) {
                std::int32_t se_face = face_map[f][1] & 0xff;
                std::int32_t se_dirn = face_map[f][1] >> 8;
                if (se_dirn == 2) { // NW edge on neighbour
                    for (std::int64_t x = 0; x < this->nside - 1; x++) {
                        corners = {hp::t_hpd{x, y, f},  hp::t_hpd{x+1, y, f}, hp::t_hpd{x+1, y2, se_face}, hp::t_hpd{x, y2, se_face} };
                        this->fill_square (corners);
                    }
                } else if (se_dirn == 4) { // SW edge on neighbour
                    for (std::int64_t x = 0; x < this->nside - 1; x++) {
                        corners = {hp::t_hpd{x, y, f},  hp::t_hpd{x+1, y, f}, hp::t_hpd{x2, x+1, se_face}, hp::t_hpd{x2, x, se_face} };
                        this->fill_square (corners);
                    }
                }
            }
        }

        void fill_six_squares()
        {
            std::int64_t max = this->nside-1;
            std::array<hp::t_hpd, 4> corners = { hp::t_hpd{0, 0, 1}, hp::t_hpd{0, max, 6}, hp::t_hpd{max, max, 9}, hp::t_hpd{max, 0, 5} } ;
            this->fill_square (corners);
            corners = { hp::t_hpd{max, max, 0}, hp::t_hpd{max, max, 1}, hp::t_hpd{max, max, 2}, hp::t_hpd{max, max, 3} } ;
            this->fill_square (corners);
            corners = { hp::t_hpd{0, 0, 8}, hp::t_hpd{0, 0, 9}, hp::t_hpd{0, 0, 10}, hp::t_hpd{0, 0, 11} } ;
            this->fill_square (corners);
            corners = { hp::t_hpd{0, 0, 0}, hp::t_hpd{0, max, 5}, hp::t_hpd{max, max, 8}, hp::t_hpd{max, 0, 4} } ;
            this->fill_square (corners);
            corners = { hp::t_hpd{0, 0, 2}, hp::t_hpd{0, max, 7}, hp::t_hpd{max, max, 10}, hp::t_hpd{max, 0, 6} } ;
            this->fill_square (corners);
            corners = { hp::t_hpd{0, 0, 3}, hp::t_hpd{0, max, 4}, hp::t_hpd{max, max, 11}, hp::t_hpd{max, 0, 7} } ;
            this->fill_square (corners);
        }

        void fill_eight_triangles()
        {
            std::int64_t max = this->nside-1;
            std::array<hp::t_hpd, 3> corners = { hp::t_hpd{0, 0, 6}, hp::t_hpd{max, 0, 9}, hp::t_hpd{0, max, 10} } ;
            this->fill_triangle (corners);
            corners = { hp::t_hpd{0, 0, 7}, hp::t_hpd{max, 0, 10}, hp::t_hpd{0, max, 11} } ;
            this->fill_triangle (corners);
            corners = { hp::t_hpd{0, 0, 4}, hp::t_hpd{max, 0, 11}, hp::t_hpd{0, max, 8} } ;
            this->fill_triangle (corners);
            corners = { hp::t_hpd{0, 0, 5}, hp::t_hpd{max, 0, 8}, hp::t_hpd{0, max, 9} } ;
            this->fill_triangle (corners);
            corners = { hp::t_hpd{max, max, 4}, hp::t_hpd{max, 0, 3}, hp::t_hpd{0, max, 0} } ;
            this->fill_triangle (corners);
            corners = { hp::t_hpd{max, max, 5}, hp::t_hpd{max, 0, 0}, hp::t_hpd{0, max, 1} } ;
            this->fill_triangle (corners);
            corners = { hp::t_hpd{max, max, 6}, hp::t_hpd{max, 0, 1}, hp::t_hpd{0, max, 2} } ;
            this->fill_triangle (corners);
            corners = { hp::t_hpd{max, max, 7}, hp::t_hpd{max, 0, 2}, hp::t_hpd{0, max, 3} } ;
            this->fill_triangle (corners);

        }

        // Add triangle indices for the channels between the 12 base HEALPix faces.
        void fill_channels()
        {
            this->fill_channels_ne();
            this->fill_channels_se();
            this->fill_six_squares();
            this->fill_eight_triangles();
        }

        /*
         * This function creates OpenGL vertices from the HEALPix (exactly one for each
         * HEALPix pixel), in NEST order. It sets the location of each pixel to the location
         * on the 3D sphere surface, using this->r to set the radius and modulating the
         * radius with relief generated from pixeldata if this->relief is true. It sets the
         * vertex colours from pixeldata using a ColourMap (this->cm).
         *
         * After creating the vertices, it then computes the OpenGL indices that will form
         * trangles between the vertices to make the spherical surface.
         */
        void healpix_triangles_by_nest()
        {
            // For colours and relief, we scale data
            sm::vvec<float> scaled_colours (this->pixeldata.size());
            if (this->colourScale.do_autoscale == true) { this->colourScale.reset(); }
            this->colourScale.transform (this->pixeldata, scaled_colours);
            sm::vvec<float> scaled_relief (this->pixeldata.size());
            if (this->reliefScale.do_autoscale == true) { this->reliefScale.reset(); }
            this->reliefScale.transform (this->pixeldata, scaled_relief);

            // The first loop creates all the *vertices* using nest scheme.
            std::int64_t n_p = this->n_pixels();

            // If colourdata is set, then use those RGB values directly, rather than scaled colours
            bool use_colourdata = false;
            bool anticipate_colourdata = false;
            if (this->colourdata != nullptr) {
                anticipate_colourdata = true;
                if (this->colourdata->size() >= static_cast<size_t>(n_p)) { use_colourdata = true; }
            }

            for (std::int64_t p = 0; p < n_p; ++p) {
                // Convert nest index p to angle for this pixel
                hp::t_ang ang = hp::nest2ang (this->nside, p);

                // Find the location of the pixel
                hp::t_vec pv = hp::loc2vec (hp::ang2loc (ang));
                // Convert it into a sm::vec and modify according to radius and relief
                float _r = this->r;
                if (this->relief == true) { _r += scaled_relief[p]; }
                sm::vec<float> vpf = (sm::vec<double>({pv.x, pv.y, pv.z}) * _r).as_float();

                // Make a colour from the pixeldata
                std::array<float, 3> sc = mplot::colour::black;
                if (use_colourdata == true) {
                    sc = (*this->colourdata)[p];
                } else if (anticipate_colourdata == true) {
                    // Do nothing
                } else {
                    sc = this->cm.convert (scaled_colours[p]);
                }

                if (this->show_nest_labels) {
                    this->addLabel (std::to_string(p), (vpf  * this->r * 1.03f),
                                    mplot::TextFeatures(0.025f, mplot::colour::black) );
                }
                // Add the vertex info for pixel p
                this->vertex_push (vpf * this->r, this->vertexPositions);
                this->vertex_push_colour (sc, this->vertexColors);
                vpf.renormalize();
                this->vertex_push (vpf, this->vertexNormals);
            }

            // Now draw indices
            std::int64_t k_down = this->k - 1;
            std::int64_t nside_down = 1LL << k_down;
            for (std::int32_t f = 0; f < 12; ++f) { // 12 'faces' of the HEALPix

                // i are the nested indices of the order down
                // Iterate through nside_down * nside_down quads for each face, unless k == 1
                for (std::int64_t i = f * nside_down * nside_down; i < (f+1) * nside_down * nside_down; ++i) {

                    // i_up are the indices of the order up. Draw the first two triangles with these indices (the main quad)
                    sm::vec<std::int64_t, 4> i_up = { i * 4, i * 4 + 1, i * 4 + 2, i * 4 + 3 };
                    this->fill_square (i_up);

                    // In the simplest case we draw just one triangle pair for each face before we
                    // then fill in the 'channels between'. If nside_down > 1 then we have to do adjacent neighbours
                    if (nside_down > 1) {

                        // find_quad_neighbour need one of the 'x' 'y' values relating to this p
                        hp::t_hpd xyf = hp::nest2hpd (nside_down, i);

                        // Find the neighbour quad 'forwards'
                        sm::vec<std::int64_t, 2> i_fwd = { i_up[1], i_up[3] };
                        find_quad_neighbour (xyf.x, 0x5, i_up, i_fwd);
                        if (i_fwd[0] > -1) { this->fill_square (i_fwd[0], i_fwd[1], i_up[1], i_up[3]); }

                        // And neighbour 'across'
                        sm::vec<std::int64_t, 2> i_across = { i_up[2], i_up[3] };
                        find_quad_neighbour (xyf.y, 0xa, i_up, i_across);
                        if (i_across[0] > -1) { this->fill_square (i_across[0], i_across[1], i_up[2], i_up[3]); }

                        // pass two elements in even though we only need one to use same find_quad_neighbour() function
                        sm::vec<std::int64_t, 2> i_fwdagain = { i_across[1], i_across[1] };
                        sm::vec<std::int64_t, 4> i_up2 = { i_across[1], i_across[0], 0, 0};
                        find_quad_neighbour (xyf.x, 0x5, i_up2, i_fwdagain);

                        if (i_up[3] > -1 && i_fwd[1] > -1 && i_across[1] > -1 && i_fwdagain[0] > -1) {
                            this->fill_square (i_up[3], i_fwd[1], i_across[1], i_fwdagain[0]);
                        }
                    }
                }
            }

            // Last job is to fill in the channels. Maybe use xy indexing for this task.
            this->fill_channels();

            this->idx += n_p;
        }

        void initializeVertices()
        {
            if (this->pixeldata.size() != static_cast<std::uint64_t>(this->n_pixels())) {
                this->pixeldata.resize (this->n_pixels(), 0.0f);
            }
            if (this->k == 0 || this->show_face_spheres) { this->face_spheres(); }
            if (this->k == 0) { return; }
            this->healpix_triangles_by_nest();
            if (this->show_spheres == true) { this->vertex_spheres(); }
            if (this->indicate_axes == true) { this->draw_coordaxes(); }

            // If required, populated the angles lookup map
            if (this->enable_angles_map && this->angles.empty()) { this->populate_angles(); }
        }

        // Draw a small set of coordinate arrows with origin at pixel 0
        void draw_coordaxes()
        {
            sm::vec<float> vpf0 = {0, 0, this->r};

            // draw tubes
            float tlen = this->r * 0.1f;
            float tlen2 = this->r * 0.05f;
            float tthk = this->r * 0.005f;

            this->computeCone (vpf0 + (sm::vec<>::uz() * tthk/2),
                               vpf0 + (sm::vec<>::uz() * tlen),
                               0.0f, mplot::colour::blue2, tthk);

            this->computeCone (vpf0 + sm::vec<>::ux() * tthk * 1.1f + sm::vec<>::uz() * tthk,
                               vpf0 + sm::vec<>::ux() * tlen2 + sm::vec<>::uz() * tthk,
                               0.0f, mplot::colour::crimson, tthk/2);

            this->computeCone (vpf0 + sm::vec<>::uy() * tthk * 1.1f + sm::vec<>::uz() * tthk,
                               vpf0 + sm::vec<>::uy() * tlen2 + sm::vec<>::uz() * tthk,
                               0.0f, mplot::colour::springgreen2, tthk/2);
        }

        std::int64_t n_pixels() const { return 12 * this->nside * this->nside; }

        static constexpr std::int64_t k_limit = 11;

        void set_order (std::int64_t _k)
        {
            if (_k < 0) {
                std::stringstream ee;
                ee << "Set order in the range [0, " << k_limit << "].";
                throw std::runtime_error (ee.str());
            }
            if (_k > k_limit) {
                std::stringstream ee;
                ee << "If you want the healpix order >" << k_limit << " then change HealpixVisual::k_limit "
                   << "in the code and make sure you have >8GB RAM and a powerful GPU.";
                throw std::runtime_error (ee.str());
            }
            this->k = _k;
            this->nside = 1 << _k;
            if (this->pixeldata.size() != static_cast<std::uint64_t>(this->n_pixels())) {
                this->pixeldata.resize (this->n_pixels(), T{0});
            }
        }
        std::int64_t get_order() const { return this->k; }

        void set_nside (std::int64_t _nside)
        {
            if (_nside < 0) { throw std::runtime_error ("nside must be positive"); }
            // Count bits set in _nside. Should be 1 only.
            std::int64_t n = _nside;
            std::int64_t c = 0; // c will be the count of bits
            while (n) { n &= n--, ++c; } // Kernighan's algorithm
            if (c != 1) { throw std::runtime_error ("That nside is not a power of 2"); }
            // Find k, then call set_order, which does re-sizing
            std::int64_t _k = -1;
            while (_nside) { _nside >>= 1, ++_k; }
            this->set_order (_k);
        }
        std::int64_t get_nside() const { return this->nside; }

        // Wrapper around nest2ang. Convert nest_index to angle for this pixel
        hp::t_ang get_angles (std::int64_t nest_index) { return hp::nest2ang (this->nside, nest_index); }

        // Get the angles for the index nest_index from the map angles.
        hp::t_ang lookup_angles (std::int64_t nest_index) const { return this->angles.at(nest_index); }

        // Populate the angles lookup map (at end of initialize vertices)
        void populate_angles()
        {
            for (std::int64_t i = 0; i < this->n_pixels(); ++i) {
                this->angles[i] = this->get_angles (i);
            }
        }

        // It's faster to have a lookup map of the angles, rather than call get_angles_slow() each time.
        std::unordered_map<std::int64_t, hp::t_ang> angles;

        // Set true to make use of the angles lookup map, which is populated at the end
        // of initializeVertices
        bool enable_angles_map = false;

        // Sphere radius
        float r = 1.0f;

        // What data to show on the healpix? Indexed by NEST index
        sm::vvec<T> pixeldata;

        // If this is non-null, it'll be used to colour the Fourpi instead of pixeldata.
        std::vector<std::array<float, 3>>* colourdata = nullptr;

        // A colour scaling
        sm::scale<T> colourScale;

        // A colourmap to translate pixeldata into colours
        mplot::ColourMap<T> cm;

        // Use relief to indicate function value - i.e. add a scaled pixeldata value to the radius of the sphere
        bool relief = false;

        // A scaling for pixeldata -> additional radius for relief
        sm::scale<T> reliefScale;

        // Show spheres at vertex locations? (mainly for debug)
        bool show_spheres = false;

        // Show vertex NEST index labels?
        bool show_nest_labels = false;

        // Show spheres at face locations? (mainly for debug)
        bool show_face_spheres = false;

        // Show a little coordinate axes set indicating directions?
        bool indicate_axes = false;

    private:
        // How many sides for the healpix? This is a choice of the user. Default to 3.
        std::int64_t k = 3; // k is the 'order'
        std::int64_t nside = 1 << k;
    };

} // namespace mplot
