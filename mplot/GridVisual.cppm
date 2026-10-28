module;

#include <cstdint>
#include <iostream>
#include <vector>
#include <array>
#include <unordered_map>
#include <format>

export module mplot.gridvisual;

import sm.grid;
export import sm.vec;
import sm.flags;

export import mplot.gl.version;
export import mplot.colourmap;
export import mplot.colour;
export import mplot.visualmodel;
export import mplot.visualdatamodel;

export namespace mplot
{
    /*!
     * How to visualize a grid. You could draw a triangle map with vertices at the centres of the
     * elements or you could draw a rectangular pixel for each element. Triangles is
     * faster. RectInterp gives a nice pixellated rendering.
     */
    enum class GridVisMode
    {
        Triangles,  // Render triangles with a triangle vertex at the centre of each Rect.
        RectInterp, // Render each rect as an actual rectangle made of 4 triangles, interpolating heights with neighbours
        Pixels,     // Render each rect as a rectangular pixel, with all z values the same
        Columns     // Render each rect as a rectangular column, with sides
    };

    /*!
     * Boolean options used in GridVisual. For meanings, see the setters of the same
     * names in GridVisual. E.g. GridVisual::centralize(bool)
     */
    enum class gridvisual_flags : std::uint32_t
    {
        centralize,
        showorigin,
        showgrid,
        implygrid,
        grid_thickness_fixed,
        showborder,
        border_thickness_fixed,
        border_tubular,
        showselectedpixborder,
        showselectedpixborder_enclosing,
        selected_pix_thickness_fixed,
        interpolate_colour_sides,
    };

    /*!
     * GridVisual a visualizer for the sm::grid class
     *
     * \tparam T the type of the *data* which this GridVisual will visualize.
     *
     * \tparam I The type for the Grid indexing (defaults to std::uint32_t)
     *
     * \tparam C The type for the Grid coordinates (default float, must be a signed type)
     *
     * \tparam glver The OpenGL version in use in your program
     */
    template <typename T, typename I = std::uint32_t, typename C = float, std::int32_t glver = mplot::gl::version_4_1>
    struct GridVisual : public VisualDataModel<T, glver>
    {
        GridVisual(const sm::grid<I, C>* _grid, const sm::vec<float> _offset)
        {
            // Set up...
            this->options_defaults();
            sm::vec<float> pixel_offset = _grid->get_dx().plus_one_dim (0.0f);
            this->viewmatrix.translate (_offset + pixel_offset);
            // Defaults for z and colourScale
            this->zScale.set_params (1, 0);
            this->colourScale.do_autoscale = true;
            this->colourScale2.do_autoscale = true;
            this->colourScale3.do_autoscale = true;
            this->grid = _grid;
            // Note: VisualModel::finalize() should be called before rendering
        }

        // If you only need to change the colours in your GridVisual (for example, if you are
        // visualizing it flat), then it is about 4 times faster to only update the colours.
        void reinitColours()
        {
            if (this->grid == nullptr) {
                throw std::runtime_error ("grid is nullptr in reinitColours()");
            }

            std::size_t n_data = static_cast<std::size_t>(this->grid->n());
            std::size_t n_cvertices_per_datum = 0;
            // Different gridVisModes will have generated different numbers of OpenGL colour vertices
            switch (this->gridVisMode) {
            case GridVisMode::Triangles:
            {
                n_cvertices_per_datum = 1; // initializeVertices used initializeVerticesTris
                break;
            }
            case GridVisMode::Columns:
            {
                n_cvertices_per_datum = 13; // used initializeVerticesCols
                break;
            }
            case GridVisMode::Pixels:
            case GridVisMode::RectInterp:
            default:
            {
                n_cvertices_per_datum = 5; // used initializeVerticesRectsInterpolated or initializeVerticesPixels
                break;
            }
            }
            if (this->vertexColors.size() < n_data * n_cvertices_per_datum * this->ncol) {
                throw std::runtime_error ("vertexColors is not big enough to reinitColours()");
            }

            // now sub-call the scalar or vector reinit colours function
            if (this->scalarData != nullptr) {
                this->reinitColoursScalar (n_data, n_cvertices_per_datum);
            } else if (this->vectorData != nullptr) {
                this->reinitColoursVector (n_data, n_cvertices_per_datum);
            } else {
                throw std::runtime_error ("No data to reinitColours()");
            }
        }

    public:
        // function that draws a border around the whole image
        void drawBorder()
        {
            sm::vec<float, 2> gridline_ht = this->get_gridline_ht();
            // Draw around the outside. Find grid extents
            sm::vec<float, 4> cg_extents = this->grid->extents(); // {xmin, xmax, ymin, ymax}
            // Adjust for any implied or shown interpixel grid
            cg_extents[0] -= gridline_ht[0];
            cg_extents[1] += gridline_ht[0];
            cg_extents[2] -= gridline_ht[1];
            cg_extents[3] += gridline_ht[1];
            sm::vec<float, 2> dx = this->grid->get_dx();
            float bthick    = this->options.test (gridvisual_flags::border_thickness_fixed) ? this->border_thickness : dx[0] * this->border_thickness;
            float left  = cg_extents[0] + this->centering_offset[0];
            float right = cg_extents[1] + this->centering_offset[0];
            float bot   = cg_extents[2] + this->centering_offset[1];
            float top   = cg_extents[3] + this->centering_offset[1];
            if (this->gridVisMode != GridVisMode::Triangles) {
                // These account for the size of the pixels representing each element of the grid
                left  -= (dx[0]/2.0f);
                right += (dx[0]/2.0f);
                bot   -= (dx[1]/2.0f);
                top   += (dx[1]/2.0f);
            }
            sm::vec<float, 4> r_extents = { left, right, bot, top };
            if (this->options.test (gridvisual_flags::border_tubular)) {
                // Have to add a bit for the tubular border
                r_extents[0] -= bthick/2.0f;
                r_extents[1] += bthick/2.0f;
                r_extents[2] -= bthick/2.0f;
                r_extents[3] += bthick/2.0f;
                this->tubularBorder (r_extents, this->border_z_offset, bthick, this->border_colour);
            } else {
                this->rectangularOuterBorder (r_extents, this->border_z_offset, bthick, this->border_colour);
            }
        }

        //! function to draw the grid (border around each pixel)
        void drawGrid()
        {
            // Draw around all pixels
            sm::vec<float, 4> cg_extents = this->grid->extents(); // {xmin, xmax, ymin, ymax}
            sm::vec<float, 2> dx = this->grid->get_dx();
            float gridthick_x = this->options.test (gridvisual_flags::grid_thickness_fixed) ? this->grid_thickness : dx[0] * this->grid_thickness;
            float gridthick_y = this->options.test (gridvisual_flags::grid_thickness_fixed) ? this->grid_thickness : dx[1] * this->grid_thickness;

            // loop through each pixel
            for (float left = cg_extents[0] - (dx[0]/2.0f); left < cg_extents[1]; left += dx[0]) {
                for (float bot = cg_extents[2] - (dx[1]/2.0f); bot < cg_extents[3]; bot += dx[1]) {
                    float right = left + dx[0];
                    float top = bot + dx[1];

                    sm::vec<float> lb = { left + this->centering_offset[0],  bot + this->centering_offset[0], this->grid_z_offset };
                    sm::vec<float> lt = { left + this->centering_offset[0],  top + this->centering_offset[0], this->grid_z_offset };
                    sm::vec<float> rt = { right + this->centering_offset[0], top + this->centering_offset[0], this->grid_z_offset };
                    sm::vec<float> rb = { right + this->centering_offset[0], bot + this->centering_offset[0], this->grid_z_offset };

                    // draw the vertical from bottom left to top left
                    this->computeFlatLine (lb, lt, rb, rt, sm::vec<>::uz(), this->grid_colour, gridthick_y);
                    // draw the horizontal from bottom left to bottom right
                    this->computeFlatLine (rb, lb, rt, lt, sm::vec<>::uz(), this->grid_colour, gridthick_x);

                    // complete the last right border (from bottom right to top right)
                    if (right >= cg_extents[1]) {
                        this->computeFlatLine (rt, rb, lt, lb, sm::vec<>::uz(), this->grid_colour, gridthick_y);
                    }
                    // complete the last top border (from top left to top right)
                    if (top >= cg_extents[3]) {
                        this->computeFlatLine (lt, rt, lb, rb, sm::vec<>::uz(), this->grid_colour, gridthick_x);
                    }
                }
            }
        }

        void rectangularInnerBorder (const sm::vec<float, 4>& r_extents,
                                     const float bz, const sm::vec<float, 2> linethickness,
                                     const std::array<float, 3>& clr)
        {
            sm::vec<float> lb = { r_extents[0], r_extents[2], bz };
            sm::vec<float> lt = { r_extents[0], r_extents[3], bz };
            sm::vec<float> rt = { r_extents[1], r_extents[3], bz };
            sm::vec<float> rb = { r_extents[1], r_extents[2], bz };
            sm::vec<float> lbin = { r_extents[0] + linethickness[0], r_extents[2] + linethickness[1], bz };
            sm::vec<float> ltin = { r_extents[0] + linethickness[0], r_extents[3] - linethickness[1], bz };
            sm::vec<float> rtin = { r_extents[1] - linethickness[0], r_extents[3] - linethickness[1], bz };
            sm::vec<float> rbin = { r_extents[1] - linethickness[0], r_extents[2] + linethickness[1], bz };
            this->computeFlatQuad (lb, lt, ltin, lbin, clr);
            this->computeFlatQuad (lt, rt, rtin, ltin, clr);
            this->computeFlatQuad (rt, rb, rbin, rtin, clr);
            this->computeFlatQuad (rb, lb, lbin, rbin, clr);
        }

        // Draw a GridVisual border *outside* r_extents. Used for a border around the entire grid.
        // r_extents: rectangular extents of the Grid
        void rectangularOuterBorder (const sm::vec<float, 4>& r_extents,
                                     const float bz, const float linethickness,
                                     const std::array<float, 3>& clr)
        {
            sm::vec<float> lb = { r_extents[0], r_extents[2], bz };
            sm::vec<float> lt = { r_extents[0], r_extents[3], bz };
            sm::vec<float> rt = { r_extents[1], r_extents[3], bz };
            sm::vec<float> rb = { r_extents[1], r_extents[2], bz };
            sm::vec<float> lbout = { r_extents[0] - linethickness, r_extents[2] - linethickness, bz };
            sm::vec<float> ltout = { r_extents[0] - linethickness, r_extents[3] + linethickness, bz };
            sm::vec<float> rtout = { r_extents[1] + linethickness, r_extents[3] + linethickness, bz };
            sm::vec<float> rbout = { r_extents[1] + linethickness, r_extents[2] - linethickness, bz };
            this->computeFlatQuad (lbout, ltout, lt, lb, clr);
            this->computeFlatQuad (lt, ltout, rtout, rt, clr);
            this->computeFlatQuad (rt, rtout, rbout, rb, clr);
            this->computeFlatQuad (rb, rbout, lbout, lb, clr);
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

        //! function to draw the border around selected pixels
        void drawSelectedPixBorder()
        {
            // Draw around all pixels
            sm::vec<float, 4> cg_extents = this->grid->extents(); // {xmin, xmax, ymin, ymax}
            sm::vec<float, 2> dx = this->grid->get_dx();

            // Take into account any inter-pixel gridlines
            sm::vec<float, 2> gridline_ht = this->get_gridline_ht();

            // The grid width in pixels
            I gw = this->grid->get_w();

            // Thickness of spacing for selected pixels
            sm::vec<float, 2> selth = dx * this->selected_pix_thickness;
            if (this->options.test (gridvisual_flags::selected_pix_thickness_fixed)) {
                selth.set_from (this->selected_pix_thickness);
            }

            float grid_left  = cg_extents[0] - (dx[0]/2.0f) + this->centering_offset[0];
            float grid_bot   = cg_extents[2] - (dx[1]/2.0f) + this->centering_offset[1];

            // loop through each pixel
            for (auto sp : this->selected_pix) {
                // sp.first is index, sp.second is colour
                I r = sp.first % gw;
                I c = sp.first / gw;
                // {xmin, xmax, ymin, ymax}
                sm::vec<float, 4> r_extents =
                {
                    (grid_left + r     * dx[0] + gridline_ht[0]),
                    (grid_left + (r+1) * dx[0] - gridline_ht[0]),
                    (grid_bot  + c     * dx[1] + gridline_ht[1]),
                    (grid_bot  + (c+1) * dx[1] - gridline_ht[1])
                };
                this->rectangularInnerBorder (r_extents, this->grid_z_offset, selth, sp.second);
            }
        }

        //! Draw a border around the selected pixels, using the first selected pix colour
        void drawSelectedPixBorderEnclosing()
        {
            if (this->selected_pix.empty()) { return; }

            // Draw around all pixels using a tubular frame
            sm::vec<float, 4> cg_extents = this->grid->extents(); // {xmin, xmax, ymin, ymax}

            // Take into account any inter-pixel gridlines
            sm::vec<float, 2> gridline_ht = this->get_gridline_ht();

            // The grid width in pixels
            I gw = this->grid->get_w();

            // Thickness of spacing for selected pixels
            sm::vec<float, 2> dx = this->grid->get_dx();
            sm::vec<float, 2> selth = dx * this->selected_pix_thickness;
            if (this->options.test (gridvisual_flags::selected_pix_thickness_fixed)) {
                selth.set_from (this->selected_pix_thickness);
            }
            float grid_left  = cg_extents[0] - (dx[0]/2.0f) + this->centering_offset[0];
            float grid_bot   = cg_extents[2] - (dx[1]/2.0f) + this->centering_offset[1];

            sm::interval<float> l_r; // left extent range
            l_r.search_init();
            sm::interval<float> r_r;
            r_r.search_init();
            sm::interval<float> b_r;
            b_r.search_init();
            sm::interval<float> t_r;
            t_r.search_init();

            // Find extents of our selected pixels
            for (auto sp : this->selected_pix) {
                // sp.first is index, sp.second is colour (unused in this function)
                I r = sp.first % gw;
                I c = sp.first / gw;
                // {xmin, xmax, ymin, ymax}
                float left  = grid_left + r * dx[0] + gridline_ht[0];
                float right = left      +     dx[0] - gridline_ht[0];
                float bot   = grid_bot  + c * dx[1] + gridline_ht[0];
                float top   = bot       +     dx[1] - gridline_ht[0];
                l_r.update (left);
                r_r.update (right);
                b_r.update (bot);
                t_r.update (top);
            }

            // xmin xmax ymin ymax
            sm::vec<float, 4> r_extents = { l_r.min, r_r.max, b_r.min, t_r.max };
            this->tubularBorder (r_extents, this->grid_z_offset, selth[0], this->enclosing_border_colour);
        }

        // Common function to setup scaling. Called by all initializeVertices subroutines. Also
        // checks size of scalar/vectorData and the Grid match.
        void setupScaling()
        {
            // Grid specific checks
            if (this->grid == nullptr) {
                throw std::runtime_error ("GridVisual error: grid is a nullptr");
            }

            if (this->scalarData != nullptr) {
                // Check scalar data has same size as Grid
                if (this->scalarData->size() != static_cast<std::size_t>(this->grid->n())) {
                    throw std::runtime_error (std::format("GridVisual error: grid size ({}) does not match scalarData size ({})",
                                                          this->grid->n(), this->scalarData->size()));
                }
            } else if (this->vectorData != nullptr) {
                // Check vector data matches grid
                if (this->vectorData->size() != static_cast<std::size_t>(this->grid->n())) {
                    throw std::runtime_error (std::format("GridVisual error: grid size ({}) does not match vectorData size ({})",
                                                          this->grid->n(), this->vectorData->size()));
                }
            }
            // Now call the parent function
            mplot::VisualDataModel<T, glver>::setupScaling();
        }

        //! Do the computations to initialize the vertices that will represent the Grid.
        void initializeVertices()
        {
            this->determine_datasize();
            if (this->datasize == 0) { return; }

            // Optionally compute an offset to ensure that the cartgrid is centred about the viewmatrix translation
            if (this->options.test(gridvisual_flags::centralize) == true) {
                this->centering_offset = -this->grid->centre().plus_one_dim();
            }

            switch (this->gridVisMode) {
            case GridVisMode::Triangles:
            {
                if (this->options.test (gridvisual_flags::showgrid) == true
                    || this->options.test (gridvisual_flags::implygrid) == true) {
                    throw std::runtime_error ("GridVisual: Can't draw an inter-pixel grid in gridVisMode == Triangles");
                }
                this->initializeVerticesTris();
                break;
            }
            case GridVisMode::Columns:
            {
                if (this->options.test (gridvisual_flags::showgrid) == true
                    || this->options.test (gridvisual_flags::implygrid) == true) {
                    throw std::runtime_error ("GridVisual: Can't (currently) draw an inter-pixel grid in gridVisMode == Columns");
                }
                this->initializeVerticesCols();
                break;
            }
            case GridVisMode::Pixels:
            {
                this->initializeVerticesPixels();
                break;
            }
            case GridVisMode::RectInterp:
            default:
            {
                this->initializeVerticesRectsInterpolated();
                break;
            }
            }

            // Note: For reinitColours to work, it's important to do all border/grid drawing AFTER
            // the initializeVerticesTris/Cols/Pixels etc
            if (this->options.test (gridvisual_flags::showborder) == true) {
                this->drawBorder();
            }
            if (this->options.test (gridvisual_flags::showgrid) == true) {
                this->drawGrid();
            }
            if (this->options.test (gridvisual_flags::showselectedpixborder) == true) {
                this->drawSelectedPixBorder();
            }
            if (this->options.test (gridvisual_flags::showselectedpixborder_enclosing) == true) {
                this->drawSelectedPixBorderEnclosing();
            }
            if (this->options.test (gridvisual_flags::showorigin) == true) {
                this->computeSphere (sm::vec<float>{0, 0, 0}, mplot::colour::crimson, 0.25f * this->grid->get_dx()[0]);
            }
        }

        //! Initialize as a minimal, triangled surface
        void initializeVerticesTris()
        {
            this->idx = 0;
            this->setupScaling();

            I vpsz = static_cast<I>(this->vertexPositions.size());
            I vcsz = static_cast<I>(this->vertexColors.size());
            I vnsz = static_cast<I>(this->vertexNormals.size());
            // How many additional vertices?
            I add_v = this->grid->n() * I{3};
            I add_v_c = this->grid->n() * static_cast<I>(this->ncol);

            // Additional space in vertexPositions/Colors/Normals
            this->vertexPositions.resize (vpsz + add_v);
            this->vertexColors.resize (vcsz + add_v_c);
            this->vertexNormals.resize (vnsz + add_v);

            I vidx = 0;
            for (I ri = 0; ri < this->grid->n(); ++ri) {
                std::array<float, 3> clr = this->setColour (ri);

                vidx = vpsz + ri * 3;
                this->vertexPositions[vidx++] = (*this->grid)[ri][0] + centering_offset[0];
                this->vertexPositions[vidx++] = (*this->grid)[ri][1] + centering_offset[1];
                this->vertexPositions[vidx++] = this->dcopy[ri];

                vidx = vcsz + ri * this->ncol;
                this->vertexColors[vidx++] = clr[0];
                this->vertexColors[vidx++] = clr[1];
                this->vertexColors[vidx++] = clr[2];
                if (this->ncol == 4) { this->vertexColors[vidx++] = 1.0f; }

                vidx = vnsz + ri * 3;
                this->vertexNormals[vidx++] = 0.0f;
                this->vertexNormals[vidx++] = 0.0f;
                this->vertexNormals[vidx++] = 1.0f;
            }

            // Build indices row by row.
            auto dims = this->grid->get_dims();
            // We know how many indices will be created
            I ri_sz = static_cast<I>(dims[1]) - I{1};
            I ci_sz = static_cast<I>(dims[0]) - I{1};
            size_t add_i = (dims[1]-1) * (dims[0]-1) * 6u;
            size_t sz_start = this->indices.size();
            size_t sz_end = sz_start + add_i;
            this->indices.resize (sz_end, 0);
            I i0 = static_cast<I>(sz_start);
            I ind_idx = i0;
            I six_ci_sz = ci_sz * I{6};
            if (this->grid->get_order() == sm::gridorder::bottomleft_to_topright) {
                for (I ri = 0; ri < ri_sz; ++ri) {
                    ind_idx = i0 + ri * six_ci_sz;
                    for (I ci = 0; ci < ci_sz; ++ci) {
                        // Triangle 1
                        I ii = ri * dims[0] + ci;
                        this->indices[ind_idx++] = (ii);
                        this->indices[ind_idx++] = (ii + 1);            // NE
                        this->indices[ind_idx++] = (ii + dims[0] + 1);  // NNE
                        // Triangle 2
                        this->indices[ind_idx++] = (ii);
                        this->indices[ind_idx++] = (ii + dims[0] + 1);  // NNE
                        this->indices[ind_idx++] = (ii + dims[0]);      // NN
                    }
                }
            } else if (this->grid->get_order() == sm::gridorder::topleft_to_bottomright) {
                for (I ri = 0; ri < ri_sz; ++ri) {
                    ind_idx = i0 + ri * six_ci_sz;
                    for (I ci = 0; ci < ci_sz; ++ci) {
                        // Triangle 1
                        I ii = ri * dims[0] + ci;
                        this->indices[ind_idx++] = (ii);
                        this->indices[ind_idx++] = (ii + 1);
                        this->indices[ind_idx++] = (ii + dims[0] + 1); // NSE
                        // Triangle 2
                        this->indices[ind_idx++] = (ii);
                        this->indices[ind_idx++] = (ii + dims[0] + 1); // NSE
                        this->indices[ind_idx++] = (ii + dims[0]);     // NS
                    }
                }
            } else if (this->grid->get_order() == sm::gridorder::bottomleft_to_topright_colmaj) {
                for (I ri = 0; ri < ri_sz; ++ri) {
                    ind_idx = i0 + ri * six_ci_sz;
                    for (I ci = 0; ci < ci_sz; ++ci) {
                        // Triangle 1
                        I ii = ci * dims[1] + ri;
                        this->indices[ind_idx++] = (ii);
                        this->indices[ind_idx++] = (ii + dims[1] + 1); // NNE
                        this->indices[ind_idx++] = (ii + dims[1]);     // NE
                        // Triangle 2
                        this->indices[ind_idx++] = (ii);
                        this->indices[ind_idx++] = (ii + 1);           // NN
                        this->indices[ind_idx++] = (ii + dims[1] + 1); // NNE
                    }
                }
            } else if (this->grid->get_order() == sm::gridorder::topleft_to_bottomright_colmaj) {
                for (I ri = 0; ri < ri_sz; ++ri) {
                    ind_idx = i0 + ri * six_ci_sz;
                    for (I ci = 0; ci < ci_sz; ++ci) {
                        // Triangle 1
                        I ii = ci * dims[1] + ri;
                        this->indices[ind_idx++] = (ii);
                        this->indices[ind_idx++] = (ii + dims[1]);     // NE
                        this->indices[ind_idx++] = (ii + dims[1] + 1); // NSE
                        // Triangle 2
                        this->indices[ind_idx++] = (ii);
                        this->indices[ind_idx++] = (ii + dims[1] + 1); // NSE
                        this->indices[ind_idx++] = (ii + 1);           // NS
                    }
                }

            } else {
                throw std::runtime_error ("mplot::GridVisual: Unhandled sm::gridorder");
            }

            this->idx += this->grid->n();
        }

        //! Initialize as a rectangle made of 4 triangles for each rect, with z position
        //! of each of the 4 outer edges of the triangles interpolated, but a single colour
        //! for each rectangle. Gives a smooth surface in which you can see the pixels.
        void initializeVerticesRectsInterpolated()
        {
            sm::vec<float, 2> dx = this->grid->get_dx();
            float hx = 0.5f * dx[0];
            float vy = 0.5f * dx[1];

            sm::vec<float, 2> gridline_ht = this->get_gridline_ht();

            this->idx = 0;
            this->setupScaling();

            float datumC = 0.0f;   // datum at the centre
            float datumNE = 0.0f;  // datum at the hex to the east.
            float datumNNE = 0.0f;
            float datumNN = 0.0f;
            float datumNNW = 0.0f;
            float datumNW = 0.0f;
            float datumNSW = 0.0f;
            float datumNS = 0.0f;
            float datumNSE = 0.0f;

            float datum = 0.0f;

            sm::vec<float> vtx_0, vtx_1, vtx_2;

            // Thickness of spacing for selected pixels
            const float selth_x = this->options.test (gridvisual_flags::selected_pix_thickness_fixed) ? this->selected_pix_thickness : dx[0] * this->selected_pix_thickness;
            const float selth_y = this->options.test (gridvisual_flags::selected_pix_thickness_fixed) ? this->selected_pix_thickness : dx[1] * this->selected_pix_thickness;
            float sx = 0.0f;
            float sy = 0.0f;

            for (I ri = 0; ri < this->grid->n(); ++ri) {

                if (this->options.test (gridvisual_flags::showselectedpixborder) == true) {
                    if (this->selected_pix.contains(ri)) {
                        sx = selth_x;
                        sy = selth_y;
                    } else { // ri is not in selected_pix
                        sx = 0.0f;
                        sy = 0.0f;
                    }
                } // else sx = sy = 0

                // Use the linear scaled copy of the data, dcopy.
                datumC  = this->dcopy[ri];
                datumNE =  this->grid->has_ne(ri)  ? this->dcopy[this->grid->index_ne(ri)] : datumC;
                datumNN =  this->grid->has_nn(ri)  ? this->dcopy[this->grid->index_nn(ri)] : datumC;
                datumNW =  this->grid->has_nw(ri)  ? this->dcopy[this->grid->index_nw(ri)] : datumC;
                datumNS =  this->grid->has_ns(ri)  ? this->dcopy[this->grid->index_ns(ri)] : datumC;
                datumNNE = this->grid->has_nne(ri) ? this->dcopy[this->grid->index_nne(ri)] : datumC;
                datumNNW = this->grid->has_nnw(ri) ? this->dcopy[this->grid->index_nnw(ri)] : datumC;
                datumNSW = this->grid->has_nsw(ri) ? this->dcopy[this->grid->index_nsw(ri)] : datumC;
                datumNSE = this->grid->has_nse(ri) ? this->dcopy[this->grid->index_nse(ri)] : datumC;

                // Use a single colour for each rect, even though rectangle's z positions are
                // interpolated. Do the _colour_ scaling:
                std::array<float, 3> clr = this->setColour (ri);

                // First push the 5 positions of the triangle vertices, starting with the centre
                // Use the centre position as the first location for finding the normal vector
                vtx_0 = { (*this->grid)[ri][0] + centering_offset[0], (*this->grid)[ri][1] + centering_offset[1], datumC };
                this->vertex_push (vtx_0, this->vertexPositions);


                // NE vertex
                // Compute mean of this->data[ri] and N, NE and E elements
                if (this->grid->has_nn(ri) && this->grid->has_ne(ri) && this->grid->has_nne(ri)) {
                    datum = 0.25f * (datumC + datumNN + datumNE + datumNNE);
                } else if (this->grid->has_ne(ri)) {
                    // Assume no NN and no NNE
                    datum = 0.5f * (datumC + datumNE);
                } else if (this->grid->has_nn(ri)) {
                    // Assume no NE and no NNE
                    datum = 0.5f * (datumC + datumNN);
                } else {
                    datum = datumC;
                }
                vtx_1 = { (*this->grid)[ri][0] + hx + centering_offset[0] - gridline_ht[0] - sx, (*this->grid)[ri][1] + vy + centering_offset[1] - gridline_ht[1] - sy, datum };
                this->vertex_push (vtx_1, this->vertexPositions);

                // SE vertex
                if (this->grid->has_ns(ri) && this->grid->has_ne(ri) && this->grid->has_nse(ri)) {
                    datum = 0.25f * (datumC + datumNS + datumNE + datumNSE);
                } else if (this->grid->has_ne(ri)) {
                    // Assume no NS and no NSE
                    datum = 0.5f * (datumC + datumNE);
                } else if (this->grid->has_ns(ri)) {
                    // Assume no NE and no NSE
                    datum = 0.5f * (datumC + datumNS);
                } else {
                    datum = datumC;
                }
                vtx_2 = {{(*this->grid)[ri][0] + hx + centering_offset[0] - gridline_ht[0] - sx, (*this->grid)[ri][1] - vy + centering_offset[1] + gridline_ht[1] + sy, datum}};
                this->vertex_push (vtx_2, this->vertexPositions);


                // SW vertex
                if (this->grid->has_ns(ri) && this->grid->has_nw(ri) && this->grid->has_nsw(ri)) {
                    datum = 0.25f * (datumC + datumNS + datumNW + datumNSW);
                } else if (this->grid->has_nw(ri)) {
                    datum = 0.5f * (datumC + datumNW);
                } else if (this->grid->has_ns(ri)) {
                    datum = 0.5f * (datumC + datumNS);
                } else {
                    datum = datumC;
                }
                // vtx_3
                this->vertex_push ((*this->grid)[ri][0] - hx + centering_offset[0] + gridline_ht[0] + sx, (*this->grid)[ri][1] - vy + centering_offset[1] + gridline_ht[1] + sy, datum, this->vertexPositions);

                // NW vertex
                if (this->grid->has_nn(ri) && this->grid->has_nw(ri) && this->grid->has_nnw(ri)) {
                    datum = 0.25f * (datumC + datumNN + datumNW + datumNNW);
                } else if (this->grid->has_nw(ri)) {
                    datum = 0.5f * (datumC + datumNW);
                } else if (this->grid->has_nn(ri)) {
                    datum = 0.5f * (datumC + datumNN);
                } else {
                    datum = datumC;
                }
                // vtx_4
                this->vertex_push ((*this->grid)[ri][0] - hx + centering_offset[0] + gridline_ht[0] + sx, (*this->grid)[ri][1] + vy + centering_offset[1] - gridline_ht[1] - sy, datum, this->vertexPositions);

                // From vtx_0,1,2 compute normal. This sets the correct normal, but note that there
                // is only one 'layer' of vertices; the back of the GridVisual will be coloured the
                // same as the front. To get lighting effects to look really good, the back of the
                // surface could need the opposite normal.
                sm::vec<float> plane1 = vtx_1 - vtx_0;
                sm::vec<float> plane2 = vtx_2 - vtx_0;
                sm::vec<float> vnorm = plane2.cross (plane1);
                vnorm.renormalize();
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);

                // Five vertices with the same colour
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);

                // Define indices now to produce the 4 triangles in the pixel
                this->indices.push_back (this->idx+1);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+2);

                this->indices.push_back (this->idx+2);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+3);

                this->indices.push_back (this->idx+3);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+4);

                this->indices.push_back (this->idx+4);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+1);

                this->idx += 5; // 5 vertices (each of 3 floats for x/y/z), 15 indices.
            }
        }

        void initializeVerticesCols()
        {
            sm::vec<float, 2> dx = this->grid->get_dx();
            float hx = 0.5f * dx[0];
            float vy = 0.5f * dx[1];

            // Note: No handling of an implied grid here, but it is possible. It does
            // mean drawing complete columns for each pixel, and so until there's a
            // need, I'll leave it unimplemented.

            this->idx = 0;
            this->setupScaling();

            float datumC = 0.0f;   // datum at the centre
            float datumNE = 0.0f;  // datum at the hex to the east.
            float datumNN = 0.0f;

            sm::vec<float> vtx_0, vtx_1, vtx_2, vtx_3, vtx_4;

            for (I ri = 0; ri < this->grid->n(); ++ri) {

                // Use the linear scaled copy of the data, dcopy.
                datumC  = this->dcopy[ri];
                datumNE =  this->grid->has_ne(ri)  ? this->dcopy[this->grid->index_ne(ri)] : datumC;
                datumNN =  this->grid->has_nn(ri)  ? this->dcopy[this->grid->index_nn(ri)] : datumC;

                // Use a single colour for each rect, even though rectangle's z positions are
                // interpolated. Do the _colour_ scaling:
                std::array<float, 3> clr = this->setColour (ri);
                std::array<float, 3> clr_e;
                std::array<float, 3> clr_n;
                if (this->options.test (gridvisual_flags::interpolate_colour_sides) == true) {
                    clr_e = this->setColour (this->grid->has_ne(ri) ? this->grid->index_ne(ri) : ri);
                    clr_n = this->setColour (this->grid->has_nn(ri) ? this->grid->index_nn(ri) : ri);
                }

                // First push the 5 positions of the pixel top face, starting with the centre
                // Use the centre position as the first location for finding the normal vector
                vtx_0 = { (*this->grid)[ri][0] + centering_offset[0], (*this->grid)[ri][1] + centering_offset[1], datumC };
                this->vertex_push (vtx_0, this->vertexPositions);

                // NE vertex
                vtx_1 = { (*this->grid)[ri][0] + hx + centering_offset[0], (*this->grid)[ri][1] + vy + centering_offset[1], datumC };
                this->vertex_push (vtx_1, this->vertexPositions);

                // SE vertex
                vtx_2 = { (*this->grid)[ri][0] + hx + centering_offset[0], (*this->grid)[ri][1] - vy + centering_offset[1], datumC };
                this->vertex_push (vtx_2, this->vertexPositions);

                // SW vertex
                this->vertex_push ((*this->grid)[ri][0] - hx + centering_offset[0], (*this->grid)[ri][1] - vy + centering_offset[1], datumC, this->vertexPositions);

                // NW vertex
                this->vertex_push ((*this->grid)[ri][0] - hx + centering_offset[0], (*this->grid)[ri][1] + vy + centering_offset[1], datumC, this->vertexPositions);

                // 4 Neighbour East vertices
                // NE
                this->vertex_push ((*this->grid)[ri][0] + hx + centering_offset[0], (*this->grid)[ri][1] + vy + centering_offset[1], datumC, this->vertexPositions);
                // SE
                this->vertex_push ((*this->grid)[ri][0] + hx + centering_offset[0], (*this->grid)[ri][1] - vy + centering_offset[1], datumC, this->vertexPositions);

                // NE
                vtx_3 = { (*this->grid)[ri][0] + hx + centering_offset[0], (*this->grid)[ri][1] + vy + centering_offset[1], datumNE };
                this->vertex_push (vtx_3, this->vertexPositions);
                // SE
                this->vertex_push ((*this->grid)[ri][0] + hx + centering_offset[0], (*this->grid)[ri][1] - vy + centering_offset[1], datumNE, this->vertexPositions);

                // 4 Neighbour North vertices
                // NW high
                this->vertex_push ((*this->grid)[ri][0] - hx + centering_offset[0], (*this->grid)[ri][1] + vy + centering_offset[1], datumC, this->vertexPositions);
                // NE high
                this->vertex_push ((*this->grid)[ri][0] + hx + centering_offset[0], (*this->grid)[ri][1] + vy + centering_offset[1], datumC, this->vertexPositions);
                // NW low
                vtx_4 = { (*this->grid)[ri][0] - hx + centering_offset[0], (*this->grid)[ri][1] + vy + centering_offset[1], datumNN };
                this->vertex_push (vtx_4, this->vertexPositions);
                // NE low
                this->vertex_push ((*this->grid)[ri][0] + hx + centering_offset[0], (*this->grid)[ri][1] + vy + centering_offset[1], datumNN, this->vertexPositions);

                // From vtx_0,1,2 compute normal. This sets the correct normal, but note that there
                // is only one 'layer' of vertices; the back of the GridVisual will be coloured the
                // same as the front. To get lighting effects to look really good, the back of the
                // surface could need the opposite normal.
                sm::vec<float> plane1 = vtx_1 - vtx_0;
                sm::vec<float> plane2 = vtx_2 - vtx_0;
                sm::vec<float> vnorm = plane2.cross (plane1);

                plane1 = vtx_1 - vtx_2;
                plane2 = vtx_3 - vtx_2;
                sm::vec<float> vnorm_e = plane2.cross (plane1);
                if (datumNE > datumC) { vnorm_e = -vnorm_e; }

                plane1 = vtx_1 - vtx_3;
                plane2 = vtx_4 - vtx_3;
                sm::vec<float> vnorm_n = plane2.cross (plane1);
                if (datumNN > datumC) { vnorm_n = -vnorm_n; }

                vnorm.renormalize();
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm_e, this->vertexNormals);
                this->vertex_push (vnorm_e, this->vertexNormals);
                this->vertex_push (vnorm_e, this->vertexNormals);
                this->vertex_push (vnorm_e, this->vertexNormals);
                this->vertex_push (vnorm_n, this->vertexNormals);
                this->vertex_push (vnorm_n, this->vertexNormals);
                this->vertex_push (vnorm_n, this->vertexNormals);
                this->vertex_push (vnorm_n, this->vertexNormals);

                // Five vertices with the same colour
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);

                if (this->options.test (gridvisual_flags::interpolate_colour_sides) == true) {
                    this->vertex_push_colour (clr, this->vertexColors);
                    this->vertex_push_colour (clr, this->vertexColors);
                    this->vertex_push_colour (clr_e, this->vertexColors);
                    this->vertex_push_colour (clr_e, this->vertexColors);

                    this->vertex_push_colour (clr, this->vertexColors);
                    this->vertex_push_colour (clr, this->vertexColors);
                    this->vertex_push_colour (clr_n, this->vertexColors);
                    this->vertex_push_colour (clr_n, this->vertexColors);
                } else {
                    this->vertex_push_colour (this->clr_east_column, this->vertexColors);
                    this->vertex_push_colour (this->clr_east_column, this->vertexColors);
                    this->vertex_push_colour (this->clr_east_column, this->vertexColors);
                    this->vertex_push_colour (this->clr_east_column, this->vertexColors);

                    this->vertex_push_colour (this->clr_north_column, this->vertexColors);
                    this->vertex_push_colour (this->clr_north_column, this->vertexColors);
                    this->vertex_push_colour (this->clr_north_column, this->vertexColors);
                    this->vertex_push_colour (this->clr_north_column, this->vertexColors);
                }

                // Define indices now to produce the 4 triangles in the pixel
                this->indices.push_back (this->idx+1);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+2);

                this->indices.push_back (this->idx+2);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+3);

                this->indices.push_back (this->idx+3);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+4);

                this->indices.push_back (this->idx+4);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+1);

                // East face
                this->indices.push_back (this->idx + 5);
                this->indices.push_back (this->idx + 6);
                this->indices.push_back (this->idx + 7);

                this->indices.push_back (this->idx + 6);
                this->indices.push_back (this->idx + 8);
                this->indices.push_back (this->idx + 7);

                // North face
                this->indices.push_back (this->idx + 9);
                this->indices.push_back (this->idx + 10);
                this->indices.push_back (this->idx + 11);

                this->indices.push_back (this->idx + 10);
                this->indices.push_back (this->idx + 12);
                this->indices.push_back (this->idx + 11);

                this->idx += 13;
            }
        }

        //! Floating pixels
        void initializeVerticesPixels()
        {
            sm::vec<float, 2> dx = this->grid->get_dx();
            float hx = 0.5f * dx[0];
            float vy = 0.5f * dx[1];

            this->idx = 0;
            this->setupScaling();

            sm::vec<float, 2> gridline_ht = this->get_gridline_ht();

            // Thickness of spacing for selected pixels
            const float selth_x = this->options.test (gridvisual_flags::selected_pix_thickness_fixed) ? this->selected_pix_thickness : dx[0] * this->selected_pix_thickness;
            const float selth_y = this->options.test (gridvisual_flags::selected_pix_thickness_fixed) ? this->selected_pix_thickness : dx[1] * this->selected_pix_thickness;
            float sx = 0.0f;
            float sy = 0.0f;

            float datumC = 0.0f;   // datum at the centre

            sm::vec<float> vtx_0, vtx_1, vtx_2;

            for (I ri = 0; ri < this->grid->n(); ++ri) {

                if (this->options.test (gridvisual_flags::showselectedpixborder) == true) {
                    if (this->selected_pix.contains (ri)) {
                        sx = selth_x;
                        sy = selth_y;
                    } else { // ri is not in selected_pix
                        sx = 0.0f;
                        sy = 0.0f;
                    }
                }

                // Use the linear scaled copy of the data, dcopy.
                datumC  = this->dcopy[ri];

                // Use a single colour for each rect, even though rectangle's z positions are
                // interpolated. Do the _colour_ scaling:
                std::array<float, 3> clr = this->setColour (ri);

                // First push the 5 positions of the triangle vertices, starting with the centre
                // Use the centre position as the first location for finding the normal vector
                vtx_0 = { (*this->grid)[ri][0] + centering_offset[0], (*this->grid)[ri][1] + centering_offset[1], datumC };
                this->vertex_push (vtx_0, this->vertexPositions);


                // NE vertex
                vtx_1 = { (*this->grid)[ri][0] + hx + centering_offset[0] - gridline_ht[0] - sx, (*this->grid)[ri][1] + vy + centering_offset[1] - gridline_ht[1] - sy, datumC };
                this->vertex_push (vtx_1, this->vertexPositions);

                // SE vertex
                vtx_2 = { (*this->grid)[ri][0] + hx + centering_offset[0] - gridline_ht[0] - sx, (*this->grid)[ri][1] - vy + centering_offset[1] + gridline_ht[1] + sy, datumC };
                this->vertex_push (vtx_2, this->vertexPositions);

                // SW vertex
                this->vertex_push ((*this->grid)[ri][0] - hx + centering_offset[0] + gridline_ht[0] + sx, (*this->grid)[ri][1] - vy + centering_offset[1] + gridline_ht[1] + sy, datumC, this->vertexPositions);

                // NW vertex
                this->vertex_push ((*this->grid)[ri][0] - hx + centering_offset[0] + gridline_ht[0] + sx, (*this->grid)[ri][1] + vy + centering_offset[1] - gridline_ht[1] - sy, datumC, this->vertexPositions);

                // From vtx_0,1,2 compute normal. This sets the correct normal, but note that there
                // is only one 'layer' of vertices; the back of the GridVisual will be coloured the
                // same as the front. To get lighting effects to look really good, the back of the
                // surface could need the opposite normal.
                sm::vec<float> plane1 = vtx_1 - vtx_0;
                sm::vec<float> plane2 = vtx_2 - vtx_0;
                sm::vec<float> vnorm = plane2.cross (plane1);
                vnorm.renormalize();
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);

                // Five vertices with the same colour
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);
                this->vertex_push_colour (clr, this->vertexColors);

                // Define indices now to produce the 4 triangles in the pixel
                this->indices.push_back (this->idx+1);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+2);

                this->indices.push_back (this->idx+2);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+3);

                this->indices.push_back (this->idx+3);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+4);

                this->indices.push_back (this->idx+4);
                this->indices.push_back (this->idx);
                this->indices.push_back (this->idx+1);

                this->idx += 5; // 5 vertices (each of 3 floats for x/y/z), 15 indices.
            }
        }

        /*!
         * Choice of method for rendering the elements. Triangles are fastest, this
         * places an OpenGL vertex at each grid centre and renders the surface with the
         * minimum number of triangles. RectInterp shows the area of each element. Other
         * options are Pixels and Columns.
         */
        GridVisMode gridVisMode = GridVisMode::RectInterp;

        // Option flags
        sm::flags<gridvisual_flags> options;
        void options_defaults()
        {
            this->options.reset();
            // Most options are false by default except:
            this->options.set (gridvisual_flags::border_tubular, true);
        }

        //! Set this to true to adjust the positions that the GridVisual uses to plot the grid so
        //! that the grid is centralised around the VisualModel's translation
        void centralize (bool flag_value = true)
        { this->options.set (gridvisual_flags::centralize, flag_value); }

        //! Show a sphere at the grid's origin? This can be useful when placing several grids with
        //! different sized pixels in a scene - it helps you to figure out the scene coordinates at
        //! which to place each grid.
        void showorigin (bool flag_value = true)
        { this->options.set (gridvisual_flags::showorigin, flag_value); }

        //! Set true to draw a grid (border around each pixels)
        void showgrid (bool flag_value = true)
        { this->options.set (gridvisual_flags::showgrid, flag_value); }


        //! showgrid overrides this, but if this is true and showgrid is false, then
        //! imply a grid by shrinking the pixels that are drawn.
        void implygrid (bool flag_value = true)
        { this->options.set (gridvisual_flags::implygrid, flag_value); }

        //! The colour used for the grid (default is grey)
        std::array<float, 3> grid_colour = mplot::colour::grey80;

        //! The grid thickness in multiples of a pixel in the grid
        float grid_thickness = 0.07f;

        //! If you need to override the pixels-relationship to the grid thickness, set it here
        void grid_thickness_fixed (bool flag_value = true)
        { this->options.set (gridvisual_flags::grid_thickness_fixed, flag_value); }

        //! How far in z to locate the grid lines?
        float grid_z_offset = 0.0f;

        //! Set true to draw a border around the outside
        void showborder (bool flag_value = true)
        { this->options.set (gridvisual_flags::showborder, flag_value); }

        //! The colour for the border
        std::array<float, 3> border_colour = mplot::colour::grey80;

        //! The border thickness in multiples of a pixel in the grid
        float border_thickness = 0.15f;

        //! If you need to override the pixels-relationship to the border thickness, set it here
        void border_thickness_fixed (bool flag_value = true)
        { this->options.set (gridvisual_flags::border_thickness_fixed, flag_value); }

        //! Where in z to locate the border lines?
        float border_z_offset = 0.0f;

        //! If true draw a tubular border, else draw a flat border
        void border_tubular (bool flag_value = true)
        { this->options.set (gridvisual_flags::border_tubular, flag_value); }

        /*!
         * If true, draw a border around selected pixels (with a full border around each selected
         * pixel). The selected pixels are chosen by the client code, which should populate
         * selected_pix_indexes.
         */
        void showselectedpixborder (bool flag_value = true)
        { this->options.set (gridvisual_flags::showselectedpixborder, flag_value); }

        /*!
         * A list of those pixel indices that should be drawn with an individual
         * border. The key is the pixel index within the grid. The value is the colour
         * for the border. This container may also be filled (with arbitrary colours) if
         * the 'enclosing' border is to be drawn.
         */
        std::unordered_map<I, std::array<float, 3>> selected_pix;

        //! Thickness of the inner border for each selected pixel
        float selected_pix_thickness = 0.1f;

        //! If non-zero, use this fixed value for the thickness of selected inner borders
        void selected_pix_thickness_fixed (bool flag_value = true)
        { this->options.set (gridvisual_flags::selected_pix_thickness_fixed, flag_value); }

        /*!
         * If true, draw a rectangular border enclosing all of the selected pixels. The
         * selected_pix_thickness(_fixed) value is used for the thickness of this
         * border.
         */
        void showselectedpixborder_enclosing (bool flag_value = true)
        { this->options.set (gridvisual_flags::showselectedpixborder_enclosing, flag_value); }

        //! The colour for the selected pixels enclosure border (showselectedpixborder_enclosing)
        std::array<float, 3> enclosing_border_colour = mplot::colour::grey60;

        // If true, interpolate the colour of the sides of columns on a column grid
        void interpolate_colour_sides (bool flag_value = true)
        { this->options.set (gridvisual_flags::interpolate_colour_sides, flag_value); }

        // User-modifiable colours for the columns if interpolated_colour_sides == false
        std::array<float, 3> clr_east_column = mplot::colour::black;
        std::array<float, 3> clr_north_column = mplot::colour::black;

    protected:

        //! GridVisual specific getter for grid-line half thickness, which is non-zero
        //! only if showgrid or implygrid are true.
        sm::vec<float, 2> get_gridline_ht() const
        {
            sm::vec<float, 2> gridline_ht = { 0.0f, 0.0f };
            if (this->options.test (gridvisual_flags::showgrid) == true
                || this->options.test (gridvisual_flags::implygrid) == true) {
                if (this->options.test (gridvisual_flags::grid_thickness_fixed) == false) {
                    gridline_ht = this->grid->get_dx() * this->grid_thickness * 0.5f;
                } else {
                    gridline_ht.set_from (this->grid_thickness * 0.5f);
                }
            }
            return gridline_ht;
        }

        //! Called by reinitColours when scalarData is not null
        void reinitColoursScalar (const std::size_t n_data, const std::size_t n_cvertices_per_datum)
        {
            if (this->colourScale.do_autoscale == true) { this->colourScale.reset(); }
            this->dcolour.resize (this->scalarData->size());
            this->colourScale.transform (*(this->scalarData), this->dcolour);

            // Replace elements of vertexColors
            for (std::size_t i = 0u; i < n_data; ++i) {
                auto c = this->cm.convert (this->dcolour[i]);
                std::size_t d_idx = this->ncol * i * n_cvertices_per_datum;
                for (std::size_t j = 0; j < n_cvertices_per_datum; ++j) {
                    this->vertexColors[d_idx + this->ncol * j] = c[0];
                    this->vertexColors[d_idx + this->ncol * j + 1] = c[1];
                    this->vertexColors[d_idx + this->ncol * j + 2] = c[2];
                    if (this->ncol == 4) { this->vertexColors[d_idx + this->ncol * j + 3] = 1.0f; }
                }
            }

            // Lastly, this call copies vertexColors (etc) into the OpenGL memory space
            this->reinit_colour_buffer();
        }

        //! Called by reinitColours when vectorData is not null (vectors are probably RGB colour)
        void reinitColoursVector (const std::size_t n_data, const std::size_t n_cvertices_per_datum)
        {
            if (this->colourScale.do_autoscale == true) { this->colourScale.reset(); }
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
            for (std::size_t i = 0u; i < n_data; ++i) {
                std::array<float, 3> c = this->setColour (i);
                std::size_t d_idx = 3 * i * n_cvertices_per_datum;
                for (std::size_t j = 0; j < n_cvertices_per_datum; ++j) {
                    this->vertexColors[d_idx + this->ncol * j] = c[0];
                    this->vertexColors[d_idx + this->ncol * j + 1] = c[1];
                    this->vertexColors[d_idx + this->ncol * j + 2] = c[2];
                    if (this->ncol == 4) { this->vertexColors[d_idx + this->ncol * j + 3] = 1.0f; }
                }
            }

            // Lastly, this call copies vertexColors (etc) into the OpenGL memory space
            this->reinit_colour_buffer();
        }

        //! The sm::grid<> to visualize
        const sm::grid<I, C>* grid;

        // A centering offset to make sure that the grid is centred on the VisualModels
        // translation. This is computed so that you *add* centering_offset to each
        // computed x/y/z position for a rectangle, and this means that the rectangle
        // will be centered.
        sm::vec<float, 3> centering_offset = { 0.0f, 0.0f, 0.0f };
    };

} // namespace mplot
