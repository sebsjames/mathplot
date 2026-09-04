module;

#include <cstdint>
#include <cmath>
#include <iostream>
#include <vector>
#include <array>
#include <set>

export module mplot.hexgridvisual;

import sm.vec;
import sm.vvec;
export import sm.hexgrid;
import sm.scale;

export import mplot.gl.version;
export import mplot.colourmap;
import mplot.tools;
export import mplot.visualdatamodel;

export namespace mplot
{
    enum class HexVisMode
    {
        Triangles, // Render triangles with a triangle vertex at the centre of each Hex. Fast (x3.7 cf. HexInterp).
        HexInterp  // Render each hex as an actual hex made of 6 triangles.
        // Could add HexBars - like the Giant's Causeway in Co. Antrim
    };

    //! The template argument T is the type of the data which this HexGridVisual
    //! will visualize and the type for the hexgrid.
    template <class T, int32_t glver = mplot::gl::version_4_1>
    class HexGridVisual : public VisualDataModel<T,glver>
    {
    public:
        //! Simplest constructor. Use this in all new code!
        HexGridVisual(const sm::hexgrid<T>* _hg, const sm::vec<float> _offset)
        {
            this->viewmatrix.translate (_offset);
            this->zScale.set_params (1, 0);
            this->colourScale.do_autoscale = true;
            this->colourScale2.do_autoscale = true;
            this->colourScale3.do_autoscale = true;
            this->hg = _hg;
        }

        //! Hexes to mark out. There are hex iterators, that I can do if (markedHexes.count(hi)) {}
        std::set<uint32_t> markedHexes;

        //! Mark a hex at location r,g,b=0 - it should be outlined with a ring, or
        //! something, so that it is visible.
        void markHex (uint32_t hi) { this->markedHexes.insert(hi); }

        //! Zoom factor
        float zoom = 1.0f;

        //! Show a set of hexes at the zero?
        bool zerogrid = false;

        //! Show boundary as 'marked' hexes?
        bool showboundary = false;

        //! Show centre hex as a 'marked' hex?
        bool showcentre = false;

        //! Set false to omit the hexes
        bool showhexes = true;

        void initializeVertices() { this->initializeVertices (false); }
        //! Do the computations to initialize the vertices that will represent the
        //! hexgrid.
        void initializeVertices(const bool update)
        {
            this->idx = 0;
            this->determine_datasize();
            if (this->datasize == 0) { return; }

            switch (this->hexVisMode) {
            case HexVisMode::Triangles:
            {
                this->initializeVerticesTris (update);
                break;
            }
            case HexVisMode::HexInterp:
            default:
            {
                this->initializeVerticesHexesInterpolated();
                break;
            }
            }
        }

        // This locally defined reinit function knows that we don't want to clear vertexPositions/vertexNormals
        void reinit_on_update()
        {
            mplot::VisualResources<glver>::i().setContext (this->parentVis);
            // No need to set idx to 0 on an update, or clear/empty vertex/indices containers
            this->initializeVertices (true); // true for 'update' not 'initial build'
            this->reinit_buffers(); // could potentially be 'reinit_position_color_buffers_only()'
        }

        // Override the updateData method
        void updateData (const std::vector<T>* _data)
        {
            this->scalarData = _data;
            switch (this->hexVisMode) {
            case HexVisMode::Triangles:
            {
                this->reinit_on_update(); // instead of VisualDataModel<T,glver>::reinit().
                break;
            }
            default:
            {
                VisualDataModel<T,glver>::reinit();
                break;
            }
            }
        }

        // Initialize vertex buffer objects and vertex array object.

        /*!
         * Initialize as triangled. Gives a smooth surface with much less compute than
         * initializeVerticesHexesInterpolated.
         *
         * If update is true, then we are updating an existing HexGridVisual, and that
         * means we don't need to re-generate the indices OR change the normals.
         */
        void initializeVerticesTris (const bool update)
        {
            uint32_t nhex = this->hg->num();

            this->setupScaling();

            std::array<float, 3> blkclr = {0,0,0};

            if (update == false) {
                this->vertexPositions.resize (3u * nhex);
                this->vertexNormals.resize (3u * nhex);
                this->vertexColors.resize (3u * nhex);
                this->indices.reserve (6u * nhex);
            }

            for (uint32_t hi = 0; hi < nhex; ++hi) {
                std::array<float, 3> clr = this->setColour (hi);
                // If dataCoords has been populated, use these for hex positions, allowing for
                // mapping of the 2D hexgrid onto a 3D manifold.
                if (this->dataCoords == nullptr) {
                    if (update == false) {
                        this->vertexPositions[hi * 3] = this->zoom * this->hg->d_x[hi];
                        this->vertexPositions[hi * 3 + 1] = this->zoom * this->hg->d_y[hi];
                    }
                    this->vertexPositions[hi * 3 + 2] = this->zoom * this->dcopy[hi];

                } else { // Otherwise use the positions directly in the hexgrid:
                    if (update == false) {
                        this->vertexPositions[hi * 3] = (*this->dataCoords)[hi][0];
                        this->vertexPositions[hi * 3 + 1] = (*this->dataCoords)[hi][1];
                    }
                    this->vertexPositions[hi * 3 + 2] = (*this->dataCoords)[hi][2];
                }
                if (this->markedHexes.count(hi)) {
                    this->vertexColors[hi * 3] = blkclr[0];
                    this->vertexColors[hi * 3 + 1] = blkclr[1];
                    this->vertexColors[hi * 3 + 2] = blkclr[2];
                } else {
                    this->vertexColors[hi * 3] = clr[0];
                    this->vertexColors[hi * 3 + 1] = clr[1];
                    this->vertexColors[hi * 3 + 2] = clr[2];
                }
                if (update == false) {
                    this->vertexNormals[hi * 3] = 0.0f;
                    this->vertexNormals[hi * 3 + 1] = 0.0f;
                    this->vertexNormals[hi * 3 + 2] = 1.0f;
                }
            }

            // Build indices based on neighbour relations in the hexgrid
            // Only needs to happen *on init*. On update, this will not change :)
            if (update == false) {
                std::size_t ind_sz = 0;
                for (uint32_t hi = 0; hi < nhex; ++hi) {
                    if (this->hg->has_nne(hi) && this->hg->has_ne(hi)) {
                        //std::cout << "1st triangle " << hi << "->" << NNE(hi) << "->" << NE(hi) << std::endl;
                        this->indices.resize (ind_sz + 3);
                        this->indices[ind_sz++] = hi;
                        this->indices[ind_sz++] = this->hg->nne(hi);
                        this->indices[ind_sz++] = this->hg->ne(hi);
                    }

                    if (this->hg->has_nw(hi) && this->hg->has_nsw(hi)) {
                        //std::cout << "2nd triangle " << hi << "->" << NW(hi) << "->" << NSW(hi) << std::endl;
                        this->indices.resize (ind_sz + 3);
                        this->indices[ind_sz++] = hi;
                        this->indices[ind_sz++] = this->hg->nw(hi);
                        this->indices[ind_sz++] = this->hg->nsw(hi);
                    }
                }
                this->idx = nhex;
            }
        }

        //! Initialize as hexes, with z position of each of the 6
        //! outer edges of the hexes interpolated, but a single colour
        //! for each hex. Gives a smooth surface.
        void initializeVerticesHexesInterpolated()
        {
            if (this->showhexes == true) {
                this->computeHexes();
            }
            // Optionally show a Flat surface for the zero plane
            if (this->zerogrid == true) {
                this->computeZerogridIndices();
            }
            // End trial grid
        }

        // Compute vertices for the patchwork quilt of hexes
        void computeHexes()
        {
            // Here's a complication. In a transformed grid, we can't rely on these. Should be able
            // to *compute* them though.
            float sr = this->hg->get_sr();
            float vne = this->hg->get_v_to_ne();
            float lr = this->hg->get_lr();

            uint32_t nhex = this->hg->num();

            this->setupScaling();

            // x and y coords on the hexgrid. May be replaced if dataCoords has been set.
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
            sm::vec<float> vtx_0, vtx_1, vtx_2, /* vtx_3, vtx_4, vtx_5, vtx_6, */ vtx_tmp;

            sm::vec<float> coordC = { 0.0f, 0.0f, 0.0f };
            sm::vec<float> coordNE = coordC;
            sm::vec<float> coordNNE = coordC;
            sm::vec<float> coordNNW = coordC;
            sm::vec<float> coordNW = coordC;
            sm::vec<float> coordNSW = coordC;
            sm::vec<float> coordNSE = coordC;

            for (uint32_t hi = 0; hi < nhex; ++hi) {

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
                    _x = (*this->dataCoords)[hi][0];
                    _y = (*this->dataCoords)[hi][1];
                    datumC = (*this->dataCoords)[hi][2];
                    coordC = (*this->dataCoords)[hi];

                    coordNE  = this->hg->has_ne(hi)  ? (*this->dataCoords)[this->hg->ne(hi)]  : (*this->dataCoords)[hi]; // datum Neighbour East
                    coordNNE = this->hg->has_nne(hi) ? (*this->dataCoords)[this->hg->nne(hi)] : (*this->dataCoords)[hi]; // datum Neighbour North East
                    coordNNW = this->hg->has_nnw(hi) ? (*this->dataCoords)[this->hg->nnw(hi)] : (*this->dataCoords)[hi]; // etc
                    coordNW  = this->hg->has_nw(hi)  ? (*this->dataCoords)[this->hg->nw(hi)]  : (*this->dataCoords)[hi];
                    coordNSW = this->hg->has_nsw(hi) ? (*this->dataCoords)[this->hg->nsw(hi)] : (*this->dataCoords)[hi];
                    coordNSE = this->hg->has_nse(hi) ? (*this->dataCoords)[this->hg->nse(hi)] : (*this->dataCoords)[hi];

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
                if (this->showcentre && _x == 0.0f && _y == 0.0f) {
                    this->markHex (hi);
                }
                std::array<float, 3> blkclr = {0,0,0};

                // First push the 7 positions of the triangle vertices, starting with the centre

                // Use the centre position as the first location for finding the normal vector
                vtx_0 = this->dataCoords == nullptr ? sm::vec<float>{ _x, _y, datumC } : coordC;
                this->vertex_push (this->zoom * vtx_0, this->vertexPositions);

                // The rotation from the transformation in the hexgrid (if any)
                sm::mat<float, 3> lt = this->hg->tfm.linear().template as<float>();

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
                    // Have to rotate after subtracting the center.
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
                    vtx_tmp = crnr + sm::vec<float>{ _x, _y, datum };
                } else {
                    if (this->hg->has_nse(hi) && this->hg->has_nsw(hi)) {
                        vtx_tmp = third * (coordC + coordNSE + coordNSW);
                    } else if (this->hg->has_nse(hi) || this->hg->has_nsw(hi)) {
                        if (this->hg->has_nse(hi)) {
                            vtx_tmp = half * (coordC + coordNSE);
                        } else {
                            vtx_tmp = half * (coordC + coordNSW);
                        }
                    } else {
                        vtx_tmp = coordC;
                    }
                }
                this->vertex_push (this->zoom * vtx_tmp, this->vertexPositions);

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
                    vtx_tmp = crnr + sm::vec<float>{ _x, _y, datum };
                } else {
                    if (this->hg->has_nw(hi) && this->hg->has_nsw(hi)) {
                        vtx_tmp = third * (coordC + coordNW + coordNSW);
                    } else if (this->hg->has_nw(hi) || this->hg->has_nsw(hi)) {
                        if (this->hg->has_nw(hi)) {
                            vtx_tmp = half * (coordC + coordNW);
                        } else {
                            vtx_tmp = half * (coordC + coordNSW);
                        }
                    } else {
                        vtx_tmp = coordC;
                    }
                }
                this->vertex_push (this->zoom * vtx_tmp, this->vertexPositions);

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
                    vtx_tmp = crnr + sm::vec<float>{ _x, _y, datum };
                } else {
                    if (this->hg->has_nnw(hi) && this->hg->has_nw(hi)) {
                        vtx_tmp = third * (coordC + coordNNW + coordNW);
                    } else if (this->hg->has_nnw(hi) || this->hg->has_nw(hi)) {
                        if (this->hg->has_nnw(hi)) {
                            vtx_tmp = half * (coordC + coordNNW);
                        } else {
                            vtx_tmp = half * (coordC + coordNW);
                        }
                    } else {
                        vtx_tmp = coordC;
                    }
                }
                this->vertex_push (this->zoom * vtx_tmp, this->vertexPositions);

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
                    vtx_tmp = crnr + sm::vec<float>{ _x, _y, datum };
                } else {
                    if (this->hg->has_nnw(hi) && this->hg->has_nne(hi)) {
                        vtx_tmp = third * (coordC + coordNNW + coordNNE);
                    } else if (this->hg->has_nnw(hi) || this->hg->has_nne(hi)) {
                        if (this->hg->has_nnw(hi)) {
                            vtx_tmp = half * (coordC + coordNNW);
                        } else {
                            vtx_tmp = half * (coordC + coordNNE);
                        }
                    } else {
                        vtx_tmp = coordC;
                    }
                }
                this->vertex_push (this->zoom * vtx_tmp, this->vertexPositions);

                // From vtx_0,1,2 compute normal. This sets the correct normal, but note
                // that there is only one 'layer' of vertices; the back of the
                // HexGridVisual will be coloured the same as the front. To get lighting
                // effects to look really good, the back of the surface could need the
                // opposite normal.
                sm::vec<float> plane1 = vtx_1 - vtx_0;
                sm::vec<float> plane2 = vtx_2 - vtx_0;
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
        }

        // Show a Flat surface for the zero plane. Currently, this is expensively
        // plotting out all the hexes because that was easy. it could be simply a big
        // rectangle of two triangles.
        void computeZerogridIndices()
        {
            float sr = this->hg->get_sr();
            float vne = this->hg->get_v_to_ne();
            float lr = this->hg->get_lr();
            uint32_t nhex = this->hg->num();

            sm::vec<float> vtx_0, vtx_1, vtx_2;
            for (uint32_t hi = 0; hi < nhex; ++hi) {

                // z position is always 0
                float datum = 0.0f;
                // Use a single colour for the zero grid
                std::array<float, 3> clr = { .8f, .8f, .8f};

                // First push the 7 positions of the triangle vertices, starting with the centre
                this->vertex_push (this->hg->d_x[hi], this->hg->d_y[hi], datum, this->vertexPositions);

                // Use the centre position as the first location for finding the normal vector
                vtx_0 = sm::vec<float>{static_cast<float>(this->hg->d_x[hi]), static_cast<float>(this->hg->d_y[hi]), datum};
                // NE vertex
                this->vertex_push (this->hg->d_x[hi]+sr, this->hg->d_y[hi]+vne, datum, this->vertexPositions);
                vtx_1 = sm::vec<float>{static_cast<float>(this->hg->d_x[hi])+sr, static_cast<float>(this->hg->d_y[hi])+vne, datum};
                // SE vertex
                this->vertex_push (this->hg->d_x[hi]+sr, this->hg->d_y[hi]-vne, datum, this->vertexPositions);
                vtx_2 = sm::vec<float>{static_cast<float>(this->hg->d_x[hi])+sr, static_cast<float>(this->hg->d_y[hi])-vne, datum};
                // S
                this->vertex_push (this->hg->d_x[hi], this->hg->d_y[hi]-lr, datum, this->vertexPositions);
                // SW
                this->vertex_push (this->hg->d_x[hi]-sr, this->hg->d_y[hi]-vne, datum, this->vertexPositions);
                // NW
                this->vertex_push (this->hg->d_x[hi]-sr, this->hg->d_y[hi]+vne, datum, this->vertexPositions);
                // N
                this->vertex_push (this->hg->d_x[hi], this->hg->d_y[hi]+lr, datum, this->vertexPositions);

                // From vtx_0,1,2 compute normal. This sets the correct normal, but note
                // that there is only one 'layer' of vertices; the back of the
                // HexGridVisual will be coloured the same as the front. To get lighting
                // effects to look really good, the back of the surface could need the
                // opposite normal.
                sm::vec<float> plane1 = vtx_1 - vtx_0;
                sm::vec<float> plane2 = vtx_2 - vtx_0;
                sm::vec<float> vnorm = plane2.cross (plane1);
                vnorm.renormalize();
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);
                this->vertex_push (vnorm, this->vertexNormals);

                // Seven vertices with the same colour
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);

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
        }

        //! How to render the hexes. Triangles are faster, HexInterp allows you to see
        //! the scale of the hexes in your sim
        HexVisMode hexVisMode = HexVisMode::HexInterp;

    protected:
        //! The hexgrid to visualize. This is not expected to change (update methods may
        //! assume the hexgrid has remained unaltered)
        const sm::hexgrid<T>* hg;
    };

} // namespace mplot
