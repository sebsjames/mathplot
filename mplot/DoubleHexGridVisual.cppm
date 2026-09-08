module;

#include <array>
#include <stdexcept>
#include <iostream>
#include <cmath>

export module mplot.doublehexgridvisual;

export import sm.vec;
export import sm.hexgrid;
export import mplot.hexgridvisual;

export namespace mplot
{
    /*!
     * A 'double' HexGridVisual.
     *
     * Display a grid that has two sections, each of which uses the same, single hexgrid, but with
     * separate coordinates. Thus, the scalarData or vectorData provided to this class should have
     * twice the number of elements as the hexgrid. If dataCoords is provided, this should also
     * contain twice the number of elements as the hexgrid.
     */
    template <class T, int glver = mplot::gl::version_4_1>
    class DoubleHexGridVisual : public mplot::HexGridVisual<T, glver>
    {
    public:
        // If true, show the flat representation of the hexgrids, ignoring the info in ommatidia
        bool show_flat = false;

        // When showing flat, should we flip the hex positions in the x (left-right) direction?
        bool second_grid_flip_lr = false;

        // When showing flat, should we flip the hex positions in the y (up-down) direction?
        bool second_grid_flip_ud = false;

        // When showing flat, this is the translation that is applied to the hex vertices of the
        // second grid and first grid to move them apart. -half the offset is applied to the first
        // grid and +half to the second grid.
        sm::vec<float, 2> grid_offset = {};

        DoubleHexGridVisual(const sm::hexgrid<float>* _hg, const sm::vec<float> _offset)
            : mplot::HexGridVisual<T, glver>(_hg, _offset) {}

        void reinitColours() // Originally coded as RGB only
        {
            if (this->scalarData == nullptr && this->vectorData == nullptr) { return; }

            size_t n_verts = this->vertexColors.size(); // should be tube_vertices * n_omm
            if (n_verts == 0u) { return; } // model doesn't exist yet

            // Need this? Yes, think so. Then use dcolour for the colours. Simples.
            this->setupScaling();

            if (this->hexVisMode == mplot::HexVisMode::Triangles) {

                if (this->scalarData != nullptr) {
                    // Update from scalarData via scaling
                    //size_t n_omm = this->scalarData->size();
                    throw std::runtime_error ("DoubleHexGridVisual::reinitColours: Write logic for Triangle visMode and scalarData");

                } else { // this->vectorData != nullptr
                    // Update from vectorData direct
                    size_t n_omm = this->vectorData->size();

                    if (n_verts < 3 * n_omm) {
                        throw std::runtime_error ("DoubleHexGridVisual: n_verts/n_omm sizes mismatch!");
                    }

                    for (size_t i = 0u; i < n_omm; ++i) {
                        // Update the 3 RGB values in vertexColors
                        std::array<float, 3> clr = this->cm.convert ((*this->vectorData)[i][0]/255.0f, (*this->vectorData)[i][1]/255.0f);
                        this->vertexColors[i*3] = clr[0];
                        this->vertexColors[i*3+1] = clr[1];
                        this->vertexColors[i*3+2] = clr[2];
                        //this->vertex_push (clr, this->vertexColors);
                    }
                }

            } else {
                if (this->scalarData != nullptr) {
                    // Update from scalarData via scaling
                    //size_t n_omm = this->scalarData->size();
                    throw std::runtime_error ("DoubleHexGridVisual::reinitColours: Write logic for HexInterp visMode and scalarData");

                } else { // this->vectorData != nullptr
                    // Update from vectorData direct
                    size_t n_omm = this->vectorData->size();

                    if (n_verts < 3 * 7 * n_omm) {
                        std::stringstream ee;
                        ee << "DoubleHexGridVisual: n_verts["<<n_verts<<"] vs. n_omm["<<n_omm<<"] sizes mismatch (HexInterp)!";
                        throw std::runtime_error (ee.str());
                    }

                    for (size_t i = 0u; i < n_omm; ++i) {
                        // Update the 3 * 7 RGB values in vertexColors
                        std::array<float, 3> clr = this->setColour (i);
                        for (size_t j = 0u; j < 7; ++j) {

                            size_t base = i * (3 * 7) + j * 3;

                            // Hacky! FIXME when more awake
                            if (this->cm.getType() == mplot::ColourMapType::HSV) {
                                this->vertexColors[base] = clr[0];
                                this->vertexColors[base+1] = clr[1];
                                this->vertexColors[base+2] = clr[2];
                            } else {
                                // convert?
                                this->vertexColors[base] = (*this->vectorData)[i][0];
                                this->vertexColors[base+1] = (*this->vectorData)[i][1];
                                this->vertexColors[base+2] = (*this->vectorData)[i][2];
                            }
                        }
                    }
                }
            }

            // Lastly, this call copies vertexColors (etc) into the OpenGL memory space
            this->reinit_colour_buffer();
        }

        //! Do the computations to initialize the vertices that will represent the
        //! HexGrid.
        void initializeVertices()
        {
            this->idx = 0;
            this->determine_datasize();
            if (this->datasize == 0) {
                std::cout << "No data to show; return" << std::endl;
                return;
            }

            switch (this->hexVisMode) {
            case mplot::HexVisMode::Triangles:
            {
                this->initializeVerticesTris();
                break;
            }
            case mplot::HexVisMode::HexInterp:
            default:
            {
                this->initializeVerticesHexesInterpolated();
                break;
            }
            }
        }

        // Initialize vertex buffer objects and vertex array object.

        //! Initialize as triangled. Gives a smooth surface with much
        //! less compute than initializeVerticesHexesInterpolated.
        void initializeVerticesTris()
        {
            unsigned int nhex = this->hg->num();

            this->setupScaling();

            std::array<float, 3> blkclr = {0,0,0};

            // DoubleHexGridVisual has two 'sections' of data
            for (unsigned int section = 0; section < 2; ++section) {
                unsigned int sdo = section * nhex; // section data offset
                for (unsigned int hi = 0; hi < nhex; ++hi) {
                    unsigned int dhi = hi + sdo;
                    std::array<float, 3> clr = this->setColour (hi);
                    // If dataCoords has been populated, use these for hex positions, allowing for
                    // mapping of the 2D HexGrid onto a 3D manifold.
                    if (this->dataCoords == nullptr) {
                        this->vertex_push (this->zoom*this->hg->d_x[hi],
                                           this->zoom*this->hg->d_y[hi],
                                           this->zoom*this->dcopy[hi], this->vertexPositions);

                    } else { // Otherwise use the positions directly in the HexGrid:
                        this->vertex_push ((*this->dataCoords)[dhi], this->vertexPositions);
                    }
                    if (this->markedHexes.count(hi)) {
                        this->vertex_push (blkclr, this->vertexColors);
                    } else {
                        this->vertex_push (clr, this->vertexColors);
                    }
                    this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                }

                // Build indices based on neighbour relations in the HexGrid
                for (unsigned int hi = 0; hi < nhex; ++hi) {
                    if (this->hg->has_nne(hi) && this->hg->has_ne(hi)) {
                        //std::cout << "1st triangle " << hi << "->" << NNE(hi) << "->" << NE(hi) << std::endl;
                        this->indices.push_back (sdo + hi);
                        this->indices.push_back (sdo + this->hg->nne(hi));
                        this->indices.push_back (sdo + this->hg->ne(hi));
                    }

                    if (this->hg->has_nw(hi) && this->hg->has_nsw(hi)) {
                        //std::cout << "2nd triangle " << hi << "->" << NW(hi) << "->" << NSW(hi) << std::endl;
                        this->indices.push_back (sdo + hi);
                        this->indices.push_back (sdo + this->hg->nw(hi));
                        this->indices.push_back (sdo + this->hg->nsw(hi));
                    }
                }
                this->idx += nhex;
            }
        }

        //! Initialize as hexes, with z position of each of the 6
        //! outer edges of the hexes interpolated, but a single colour
        //! for each hex. Gives a smooth surface.
        void initializeVerticesHexesInterpolated()
        {
            this->computeHexes();
            // To show origin in model frame:
            //this->computeSphere (sm::vec<float>{0,0,0}, mplot::colour::navy, 0.003f);
        }

        // Apply the transforms in grid_offset and second_grid_flip_lr (etc)
        void apply_grid_transforms (const std::uint32_t sdo, sm::vec<float>& _vtx)
        {
            if (this->show_flat == false) { return; }
            if (sdo == 0) {
                // First hexgrid
                _vtx[0] -= this->grid_offset[0] / 2.0f;
                _vtx[1] -= this->grid_offset[1] / 2.0f;
            } else {
                // Second hexgrid
                if (this->second_grid_flip_lr) { _vtx[0] *= -1; }
                if (this->second_grid_flip_ud) { _vtx[1] *= -1; }
                _vtx[0] += this->grid_offset[0] / 2.0f;
                _vtx[1] += this->grid_offset[1] / 2.0f;
            }
        }

        // Compute vertices for the patchwork quilt of hexes
        void computeHexes()
        {
            // Here's a complication. In a transformed grid, we can't rely on these. Should be able
            // to *compute* them though.
            float sr = this->hg->get_sr();
            float vne = this->hg->get_v_to_ne();
            float lr = this->hg->get_lr();

            unsigned int nhex = this->hg->num();

            // We have a double grid and use the hexgrid twice on the first half and second half.
            if (this->datasize != nhex * 2u) {
                throw std::runtime_error ("datasize is not twice nhex");
            }

            this->setupScaling();

            // x and y coords on the HexGrid. May be replaced if dataCoords has been set.
            float _x = 0.0f;
            float _y = 0.0f;
            // These Ts are all floats, right?
            float datumC = 0.0f;   // datum at the centre
            float datumNE = 0.0f;  // datum at the hex to the east.
            float datumNNE = 0.0f; // etc
            float datumNNW = 0.0f;
            float datumNW = 0.0f;
            float datumNSW = 0.0f;
            float datumNSE = 0.0f;

            float datum = 0.0f;
            float third = 0.3333333f;
            float half = 0.5f;
            sm::vec<float> vtx_0, vtx_1, vtx_2, vtx_3, vtx_4, vtx_5, vtx_6;

            sm::vec<float> coordC = { 0.0f, 0.0f, 0.0f };
            sm::vec<float> coordNE = coordC;
            sm::vec<float> coordNNE = coordC;
            sm::vec<float> coordNNW = coordC;
            sm::vec<float> coordNW = coordC;
            sm::vec<float> coordNSW = coordC;
            sm::vec<float> coordNSE = coordC;

            // Figure out an offset to centre the eyes about the current mv_offset. This
            // is the centroid off dataCoords
            sm::vec<float> coffs = {0,0,0};

            if (this->dataCoords != nullptr) {
                // dataCoords is ptr to vector<vec<float>>. The mean vector will work
                coffs = -reinterpret_cast<sm::vvec<sm::vec<float, 3>>*>(this->dataCoords)->mean();
            }

            // The rotation from the transformation in the hexgrid (if any)
            sm::mat<float, 3> lt = this->hg->tfm.linear();

            for (unsigned int section = 0; section < 2; ++section) {
                unsigned int sdo = section * nhex; // section data offset
                for (unsigned int hi = 0; hi < nhex; ++hi) {

                    vtx_1.zero();
                    vtx_2.zero();

                    unsigned int dhi = hi + sdo;

                    if (this->dataCoords == nullptr) {
                        _x = this->hg->d_x[hi];
                        _y = this->hg->d_y[hi];
                        // Use the linear scaled copy of the data, dcopy.
                        datumC   = this->dcopy[hi]; // '_z'
                        datumNE  = this->hg->has_ne(hi)  ? this->dcopy[this->hg->ne(hi)]  : datumC; // datum Neighbour East
                        datumNNE = this->hg->has_nne(hi) ? this->dcopy[this->hg->nne(hi)] : datumC; // datum Neighbour North East
                        datumNNW = this->hg->has_nnw(hi) ? this->dcopy[this->hg->nnw(hi)] : datumC; // etc
                        datumNW  = this->hg->has_nw(hi)  ? this->dcopy[this->hg->nw(hi)]  : datumC;
                        datumNSW = this->hg->has_nsw(hi) ? this->dcopy[this->hg->nsw(hi)] : datumC;
                        datumNSE = this->hg->has_nse(hi) ? this->dcopy[this->hg->nse(hi)] : datumC;
                    } else {
                        // Get coordinates from dataCoords
                        coordC = coffs + (*this->dataCoords)[dhi];
                        _x = (*this->dataCoords)[dhi][0];
                        _y = (*this->dataCoords)[dhi][1];
                        datumC = (*this->dataCoords)[dhi][2];

                        coordNE  = this->hg->has_ne(hi)  ? coffs + (*this->dataCoords)[sdo + this->hg->ne(hi)]  : coordC; // datum Neighbour East
                        coordNNE = this->hg->has_nne(hi) ? coffs + (*this->dataCoords)[sdo + this->hg->nne(hi)] : coordC; // datum Neighbour North East
                        coordNNW = this->hg->has_nnw(hi) ? coffs + (*this->dataCoords)[sdo + this->hg->nnw(hi)] : coordC; // etc
                        coordNW  = this->hg->has_nw(hi)  ? coffs + (*this->dataCoords)[sdo + this->hg->nw(hi)]  : coordC;
                        coordNSW = this->hg->has_nsw(hi) ? coffs + (*this->dataCoords)[sdo + this->hg->nsw(hi)] : coordC;
                        coordNSE = this->hg->has_nse(hi) ? coffs + (*this->dataCoords)[sdo + this->hg->nse(hi)] : coordC;

                        datumNE = coordNE[2];
                        datumNNE = coordNNE[2];
                        datumNNW = coordNNW[2];
                        datumNW = coordNW[2];
                        datumNSW = coordNSW[2];
                        datumNSE = coordNSE[2];
                    }

                    // Use a single colour for each hex, even though hex z positions are
                    // interpolated. Do the _colour_ scaling:
                    std::array<float, 3> clr = this->setColour (hi);
                    if (this->showboundary && (this->hg->vhexen[hi])->boundary_hex() == true) {
                        this->markHex (hi);
                    }
                    if (this->showcentre && _x == 0.0f && _y == 0.0f) { this->markHex (hi); }
                    std::array<float, 3> blkclr = {0,0,0};

                    // First push the 7 positions of the triangle vertices, starting with the centre

                    // Use the centre position as the first location for finding the normal vector
                    vtx_0 = this->dataCoords == nullptr ? sm::vec<float>{ _x, _y, datumC } : coordC;
                    this->apply_grid_transforms (sdo, vtx_0);
                    this->vertex_push (this->zoom * vtx_0, this->vertexPositions);

                    // NE vertex
                    if (this->dataCoords == nullptr) {
                        if (this->hg->has_nne(hi) && this->hg->has_ne(hi)) {
                            // Compute mean of this->data[hi] and NE and E hexes
                            datum = third * (datumC + datumNNE + datumNE);
                        } else if (this->hg->has_nne(hi) || this->hg->has_ne(hi)) {
                            if (this->hg->has_nne(hi)) {
                                datum = half * (datumC + datumNNE);
                            } else {
                                datum = half * (datumC + datumNE);
                            }
                        } else {
                            datum = datumC;
                        }
                        sm::vec<float> crnr = lt * sm::vec<float>{ sr, vne, 0 };
                        vtx_1 = crnr + sm::vec<float>{ _x, _y, datum };
                    } else {
                        // Similar logic, but for the coordinate, not just the data value
                        if (this->hg->has_nne(hi) && this->hg->has_ne(hi)) {
                            // Compute mean of coordC and NE and E hexes
                            vtx_1 = third * (coordC + coordNNE + coordNE);
                        } else if (this->hg->has_nne(hi) || this->hg->has_ne(hi)) {
                            if (this->hg->has_nne(hi)) {
                                vtx_1 = half * (coordC + coordNNE);
                            } else {
                                vtx_1 = half * (coordC + coordNE);
                            }
                        } else {
                            vtx_1 = coordC;
                        }
                    }
                    this->apply_grid_transforms (sdo, vtx_1);
                    this->vertex_push (this->zoom * vtx_1, this->vertexPositions);

                    // SE vertex
                    if (this->dataCoords == nullptr) {
                        if (this->hg->has_ne(hi) && this->hg->has_nse(hi)) {
                            datum = third * (datumC + datumNE + datumNSE);
                        } else if (this->hg->has_ne(hi) || this->hg->has_nse(hi)) {
                            if (this->hg->has_ne(hi)) {
                                datum = half * (datumC + datumNE);
                            } else {
                                datum = half * (datumC + datumNSE);
                            }
                        } else {
                            datum = datumC;
                        }
                        sm::vec<float> crnr = lt * sm::vec<float>{ sr, -vne, 0 };
                        vtx_2 = crnr + sm::vec<float>{ _x, _y, datum };
                    } else {
                        if (this->hg->has_ne(hi) && this->hg->has_nse(hi)) {
                            vtx_2 = third * (coordC + coordNE + coordNSE);
                        } else if (this->hg->has_ne(hi) || this->hg->has_nse(hi)) {
                            if (this->hg->has_ne(hi)) {
                                vtx_2 = half * (coordC + coordNE);
                            } else {
                                vtx_2 = half * (coordC + coordNSE);
                            }
                        } else {
                            vtx_2 = coordC;
                        }
                    }
                    this->apply_grid_transforms (sdo, vtx_2);
                    this->vertex_push (this->zoom * vtx_2, this->vertexPositions);


                    // S
                    if (this->dataCoords == nullptr) {
                        if (this->hg->has_nse(hi) && this->hg->has_nsw(hi)) {
                            datum = third * (datumC + datumNSE + datumNSW);
                        } else if (this->hg->has_nse(hi) || this->hg->has_nsw(hi)) {
                            if (this->hg->has_nse(hi)) {
                                datum = half * (datumC + datumNSE);
                            } else {
                                datum = half * (datumC + datumNSW);
                            }
                        } else {
                            datum = datumC;
                        }
                        sm::vec<float> crnr = lt * sm::vec<float>{ 0, -lr, 0 };
                        vtx_3 = crnr + sm::vec<float>{ _x, _y, datum };
                    } else {
                        if (this->hg->has_nse(hi) && this->hg->has_nsw(hi)) {
                            vtx_3 = third * (coordC + coordNSE + coordNSW);
                        } else if (this->hg->has_nse(hi) || this->hg->has_nsw(hi)) {
                            if (this->hg->has_nse(hi)) {
                                vtx_3 = half * (coordC + coordNSE);
                            } else {
                                vtx_3 = half * (coordC + coordNSW);
                            }
                        } else {
                            vtx_3 = coordC;
                        }
                    }
                    this->apply_grid_transforms (sdo, vtx_3);
                    this->vertex_push (this->zoom * vtx_3, this->vertexPositions);

                    // SW
                    if (this->dataCoords == nullptr) {
                        if (this->hg->has_nw(hi) && this->hg->has_nsw(hi)) {
                            datum = third * (datumC + datumNW + datumNSW);
                        } else if (this->hg->has_nw(hi) || this->hg->has_nsw(hi)) {
                            if (this->hg->has_nw(hi)) {
                                datum = half * (datumC + datumNW);
                            } else {
                                datum = half * (datumC + datumNSW);
                            }
                        } else {
                            datum = datumC;
                        }
                        sm::vec<float> crnr = lt * sm::vec<float>{ -sr, -vne, 0 };
                        vtx_4 = crnr + sm::vec<float>{ _x, _y, datum };
                    } else {
                        if (this->hg->has_nw(hi) && this->hg->has_nsw(hi)) {
                            vtx_4 = third * (coordC + coordNW + coordNSW);
                        } else if (this->hg->has_nw(hi) || this->hg->has_nsw(hi)) {
                            if (this->hg->has_nw(hi)) {
                                vtx_4 = half * (coordC + coordNW);
                            } else {
                                vtx_4 = half * (coordC + coordNSW);
                            }
                        } else {
                            vtx_4 = coordC;
                        }
                    }
                    this->apply_grid_transforms (sdo, vtx_4);
                    this->vertex_push (this->zoom * vtx_4, this->vertexPositions);

                    // NW
                    if (this->dataCoords == nullptr) {
                        if (this->hg->has_nnw(hi) && this->hg->has_nw(hi)) {
                            datum = third * (datumC + datumNNW + datumNW);
                        } else if (this->hg->has_nnw(hi) || this->hg->has_nw(hi)) {
                            if (this->hg->has_nnw(hi)) {
                                datum = half * (datumC + datumNNW);
                            } else {
                                datum = half * (datumC + datumNW);
                            }
                        } else {
                            datum = datumC;
                        }
                        sm::vec<float> crnr = lt * sm::vec<float>{ -sr, vne, 0 };
                        vtx_5 = crnr + sm::vec<float>{ _x, _y, datum };
                    } else {
                        if (this->hg->has_nnw(hi) && this->hg->has_nw(hi)) {
                            vtx_5 = third * (coordC + coordNNW + coordNW);
                        } else if (this->hg->has_nnw(hi) || this->hg->has_nw(hi)) {
                            if (this->hg->has_nnw(hi)) {
                                vtx_5 = half * (coordC + coordNNW);
                            } else {
                                vtx_5 = half * (coordC + coordNW);
                            }
                        } else {
                            vtx_5 = coordC;
                        }
                    }
                    this->apply_grid_transforms (sdo, vtx_5);
                    this->vertex_push (this->zoom * vtx_5, this->vertexPositions);

                    // N
                    if (this->dataCoords == nullptr) {
                        if (this->hg->has_nnw(hi) && this->hg->has_nne(hi)) {
                            datum = third * (datumC + datumNNW + datumNNE);
                        } else if (this->hg->has_nnw(hi) || this->hg->has_nne(hi)) {
                            if (this->hg->has_nnw(hi)) {
                                datum = half * (datumC + datumNNW);
                            } else {
                                datum = half * (datumC + datumNNE);
                            }
                        } else {
                            datum = datumC;
                        }
                        sm::vec<float> crnr = lt * sm::vec<float>{ 0, lr, 0 };
                        vtx_6 = crnr + sm::vec<float>{ _x, _y, datum };
                    } else {
                        if (this->hg->has_nnw(hi) && this->hg->has_nne(hi)) {
                            vtx_6 = third * (coordC + coordNNW + coordNNE);
                        } else if (this->hg->has_nnw(hi) || this->hg->has_nne(hi)) {
                            if (this->hg->has_nnw(hi)) {
                                vtx_6 = half * (coordC + coordNNW);
                            } else {
                                vtx_6 = half * (coordC + coordNNE);
                            }
                        } else {
                            vtx_6 = coordC;
                        }
                    }
                    this->apply_grid_transforms (sdo, vtx_6);
                    this->vertex_push (this->zoom * vtx_6, this->vertexPositions);

                    // From vtx_0, and any two of vtx_1 to vtx_6, compute two planes and thus the normal vector.
                    sm::vec<float> plane1 = {0,0,0};
                    sm::vec<float> plane2 = {0,0,0};

                    // First get the first plane
                    int plane1_vtx = -1;
                    if ((vtx_1 - vtx_0).length() > 0.0f) {
                        plane1 = vtx_1 - vtx_0;
                        plane1_vtx = 1;
                    } else if ((vtx_2 - vtx_0).length() > 0.0f) {
                        plane1 = vtx_2 - vtx_0;
                        plane1_vtx = 2;
                    } else if ((vtx_3 - vtx_0).length() > 0.0f) {
                        plane1 = vtx_3 - vtx_0;
                        plane1_vtx = 3;
                    } else if ((vtx_4 - vtx_0).length() > 0.0f) {
                        plane1 = vtx_4 - vtx_0;
                        plane1_vtx = 4;
                    } else if ((vtx_5 - vtx_0).length() > 0.0f) {
                        plane1 = vtx_5 - vtx_0;
                        plane1_vtx = 5;
                    } else if ((vtx_6 - vtx_0).length() > 0.0f) {
                        plane1 = vtx_6 - vtx_0;
                        plane1_vtx = 6;
                    } else {
                        throw std::runtime_error ("DoubleHexGridVisual: vtx_0 has no neighbour?!");
                    }

                    // Now select a second plane
                    if (plane1_vtx != 1 && (vtx_1 - vtx_0).length() > 0.0f) {
                        plane2 = vtx_1 - vtx_0;
                    } else if (plane1_vtx != 2 && (vtx_2 - vtx_0).length() > 0.0f) {
                        plane2 = vtx_2 - vtx_0;
                    } else if (plane1_vtx != 3 && (vtx_3 - vtx_0).length() > 0.0f) {
                        plane2 = vtx_3 - vtx_0;
                    } else if (plane1_vtx != 4 && (vtx_4 - vtx_0).length() > 0.0f) {
                        plane2 = vtx_4 - vtx_0;
                    } else if (plane1_vtx != 5 && (vtx_5 - vtx_0).length() > 0.0f) {
                        plane2 = vtx_5 - vtx_0;
                    } else if (plane1_vtx != 6 && (vtx_6 - vtx_0).length() > 0.0f) {
                        plane2 = vtx_6 - vtx_0;
                    } else {
                        throw std::runtime_error ("Can't do planes with only 1 neighbour to a hex"); // or gracefully handle? Can select a random vector!
                    }

                    // The normal is the cross product of the planes.
                    sm::vec<float> vnorm = plane2.cross (plane1);
                    vnorm.renormalize();

                    this->vertex_push (vnorm, this->vertexNormals);
                    this->vertex_push (vnorm, this->vertexNormals);
                    this->vertex_push (vnorm, this->vertexNormals);
                    this->vertex_push (vnorm, this->vertexNormals);
                    this->vertex_push (vnorm, this->vertexNormals);
                    this->vertex_push (vnorm, this->vertexNormals);
                    this->vertex_push (vnorm, this->vertexNormals);

                    // Usually seven vertices with the same colour, but if the hex is
                    // marked, then three of the vertices are given the colour black,
                    // marking the hex out visually.
                    if (std::isnan(this->dcolour[hi])) {
                        this->vertex_push (clr, this->vertexColors);
                        this->vertex_push (blkclr, this->vertexColors);
                        this->vertex_push (blkclr, this->vertexColors);
                        this->vertex_push (blkclr, this->vertexColors);
                        this->vertex_push (blkclr, this->vertexColors);
                        this->vertex_push (blkclr, this->vertexColors);
                        this->vertex_push (blkclr, this->vertexColors);
                    } else {
                        this->vertex_push (clr, this->vertexColors);
                        if (this->markedHexes.count(hi)) {
                            this->vertex_push (blkclr, this->vertexColors);
                        } else {
                            this->vertex_push (clr, this->vertexColors);
                        }

                        this->vertex_push (clr, this->vertexColors);

                        if (this->markedHexes.count(hi)) {
                            this->vertex_push (blkclr, this->vertexColors);
                        } else {
                            this->vertex_push (clr, this->vertexColors);
                        }
                        this->vertex_push (clr, this->vertexColors);
                        if (this->markedHexes.count(hi)) {
                            this->vertex_push (blkclr, this->vertexColors);
                        } else {
                            this->vertex_push (clr, this->vertexColors);
                        }
                        this->vertex_push (clr, this->vertexColors);
                    }

                    // Define indices now to produce the 6 triangles in the hex
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
                    this->indices.push_back (this->idx+5);

                    this->indices.push_back (this->idx+5);
                    this->indices.push_back (this->idx);
                    this->indices.push_back (this->idx+6);

                    this->indices.push_back (this->idx+6);
                    this->indices.push_back (this->idx);
                    this->indices.push_back (this->idx+1);

                    this->idx += 7; // 7 vertices (each of 3 floats for x/y/z), 18 indices.
                }
            } // section
        }
    };

} // namespace mplot
