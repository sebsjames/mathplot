/*!
 * \file
 *
 * A navigation mesh class. An instance of this navigation mesh may be owned by a VisualModel to aid
 * navigation across and around the model.
 *
 * \author Seb James
 * \date October 2025
 */

#pragma once

#include <cstdint>
#include <limits>
#include <tuple>
#include <array>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <string_view>

#include <sm/util>

import sm.vec;
import sm.vvec;
import sm.flags;
import sm.mat;
import sm.geometry;

namespace mplot
{
    namespace mesh
    {
        /*!
         * Half-edge data structures for ordered meshes
         */
        template<typename I = uint32_t> requires std::is_integral_v<I>
        struct halfedge
        {
            // two vertex indices for start and end of this halfedge
            sm::vec<I, 2> vi = { std::numeric_limits<I>::max(), std::numeric_limits<I>::max() };
            I twin = std::numeric_limits<I>::max(); // twin half edge
            I next = std::numeric_limits<I>::max(); // next half edge in face (or hole)
            I prev = std::numeric_limits<I>::max(); // prev half edge in face (or hole)
            I flags = 0; // 0x1: boundary halfedge
        };

        template<typename I = uint32_t, typename F=float, I N = 3> requires std::is_integral_v<I>
        struct vertex
        {
            // Coordinate position of vertex
            sm::vec<F, N> p = {};
            // A halfedge (hi: halfedge index) emanating from this vertex
            I hi = std::numeric_limits<I>::max();
        };

        template<typename I = uint32_t> requires std::is_integral_v<I>
        struct face
        {
            // The index of the starting halfedge that records the existence of this face
            I hi = std::numeric_limits<I>::max();
        };
    }

    /*!
     * Navigation mesh of triangles.
     *
     * This is built from an OpenGL vertex/indices set by VisualModel::make_navmesh()
     */
    struct NavMesh
    {
        /*!
         * Minimum set of vertices to generate a topological mesh. populated by
         * VisualModel::make_navmesh()
         */
        std::vector<mesh::vertex<>> vertex = {};

        //! The vector of half edges in the mesh
        std::vector<mesh::halfedge<>> halfedge = {};

        //! Triangle mesh faces. populated by VisualModel::make_navmesh()
        std::vector<mesh::face<>> triangles = {};

        //! Holds a copy of the bb of the parent model
        sm::range<sm::vec<float>> bb;

        //! When navigating, this is the 'current triangle' that you're located over/near
        uint32_t ti0 = std::numeric_limits<uint32_t>::max();

        /*!
         * Save this navmesh into a binary file.
         */
        void save (const std::string& filename) const
        {
            std::cout << "Save NavMesh to " << filename << std::endl;

            std::ofstream fout (filename, std::ios::binary | std::ios::out | std::ios::trunc);
            if (!fout.is_open()) {
                std::cerr << "NavMesh::save: Failed to open " << filename << " for writing, continue\n";
                return;
            }
            // fout is open
            uint64_t vertex_sz = this->vertex.size();
            uint64_t halfedge_sz = this->halfedge.size();
            uint64_t triangles_sz = this->triangles.size();

            // Write sizes at head of file, as the first thing
            sm::util::binary_write (fout, vertex_sz);
            sm::util::binary_write (fout, halfedge_sz);
            sm::util::binary_write (fout, triangles_sz); // 3 * 8 = 24 bytes

            // Write the bb range next.
            sm::util::binary_write (fout, this->bb.min);
            sm::util::binary_write (fout, this->bb.max); // 2 * 3 * 4 = 24 bytes

            // Now loop
            for (auto v : this->vertex) {
                sm::util::binary_write (fout, v.p);
                sm::util::binary_write (fout, v.hi);     // 3 * 4 + 4 = 16 bytes per line
            }

            for (auto he : this->halfedge) {
                sm::util::binary_write (fout, he.vi);
                sm::util::binary_write (fout, he.twin);
                sm::util::binary_write (fout, he.next);
                sm::util::binary_write (fout, he.prev);  // 5 * 4 = 20 bytes per line
                sm::util::binary_write (fout, he.flags);  // plus 4
            }

            for (auto t : this->triangles) { sm::util::binary_write (fout, t.hi); }
        }

        /*!
         * Load the navmesh from the binary format produced by NavMesh::save()
         */
        void load (const std::string& filename)
        {
            std::cout << "Load NavMesh from " << filename << std::endl;

            std::ifstream fin (filename, std::ios::binary | std::ios::in);
            if (!fin.is_open()) { throw std::runtime_error ("NavMesh::load: Failed to open file"); }

            uint64_t vertex_sz = 0;
            uint64_t halfedge_sz = 0;
            uint64_t triangles_sz = 0;

            sm::util::binary_read (fin, vertex_sz);
            sm::util::binary_read (fin, halfedge_sz);
            sm::util::binary_read (fin, triangles_sz); // 3 * 8 = 24 bytes

            sm::util::binary_read (fin, this->bb.min);
            sm::util::binary_read (fin, this->bb.max);

            this->vertex.resize (vertex_sz);
            this->halfedge.resize (halfedge_sz);
            this->triangles.resize (triangles_sz);

            for (auto& v : this->vertex) {
                sm::util::binary_read (fin, v.p);
                sm::util::binary_read (fin, v.hi);     // 3 * 4 + 4 = 16 bytes per line
            }

            for (auto& he : this->halfedge) {
                sm::util::binary_read (fin, he.vi);
                sm::util::binary_read (fin, he.twin);
                sm::util::binary_read (fin, he.next);
                sm::util::binary_read (fin, he.prev);  // 5 * 4 = 20 bytes per line
                sm::util::binary_read (fin, he.flags);
            }

            for (auto& t : this->triangles) { sm::util::binary_read (fin, t.hi); }
        }

        /*!
         * Return index of this->vertex that is closest to scene_coord. Can use vertexidx_to_indices
         * to find the indices into vertexPositions and vertexNormals that this index in the
         * topographic mesh relates to.
         *
         * \param scene_coord Supplied coordinate in scene frame of reference
         * \param viewmatrix The viewmatrix of the model which converts model frame coordinates to the scene frame
         */
        uint32_t find_vertex_nearest (const sm::vec<float>& scene_coord, const sm::mat<float, 4>& viewmatrix) const
        {
            uint32_t i = std::numeric_limits<uint32_t>::max();
            // Brute force it. (But we have a mesh; can this guarantee a faster search? I don't think so)
            float min_d = std::numeric_limits<float>::max();
            for (uint32_t j = 0; j < this->vertex.size(); ++j) {
                sm::vec<float> vcoord = (viewmatrix * this->vertex[j].p).less_one_dim();
                float d = (scene_coord - vcoord).length();
                if (d < min_d) {
                    min_d = d;
                    i = j;
                }
            }
            return i;
        }

        /*!
         * Performs the work required to verify a triangle or a boundary made of halfedges.
         */
        bool verify_halfedge_chain (const uint32_t _hi,
                                    const uint32_t chain_length = std::numeric_limits<uint32_t>::max(),
                                    const bool debug = false) const
        {
            constexpr uint32_t max = std::numeric_limits<uint32_t>::max();

            if (_hi >= this->halfedge.size()) { return false; }

            uint32_t fcount = 0u;
            uint32_t fhi = _hi;

            if (debug) {
                // Loop through once showing chain info (index, prev, next, twin)
                do {
                    std::cout << "(prev: " << this->halfedge[fhi].prev << ") halfedge["
                              << fhi << "] (next: " << this->halfedge[fhi].next << ") has twin "
                              << this->halfedge[fhi].twin << " and flags " << this->halfedge[fhi].flags << std::endl;
                    fhi = this->halfedge[fhi].next;
                    ++fcount;
                } while (fhi != _hi && fhi != max && fcount < this->halfedge.size());
                // Reset for forward again:
                fcount = 0u;
                fhi = _hi;
            }

            bool zlen_halfedge = false;
            bool zlen_halfedge_to_halfedge = false;
            bool approx_zlen_halfedge_to_halfedge = false;
            // Each boundary should be the same forwards and backwards from every possible start halfedge
            do {
                // No self-referrals please
                if (this->halfedge[fhi].next == fhi) { throw std::runtime_error ("self next referral!"); }
                // Make sure that the vertices are not the same (no triangles-that-are-lines)
                const uint32_t fhi_nx = this->halfedge[fhi].next;
                // Check the length of the edge
                auto hlen = (this->vertex[halfedge[fhi].vi[1]].p - this->vertex[halfedge[fhi_nx].vi[1]].p).length();
                if (halfedge[fhi].vi[0] == halfedge[fhi].vi[1]) { zlen_halfedge = true; }
                if (halfedge[fhi].vi[1] == halfedge[fhi_nx].vi[1]) { zlen_halfedge_to_halfedge = true; }
                if (hlen < (10.0f * std::numeric_limits<float>::epsilon())) {
                    approx_zlen_halfedge_to_halfedge = true;
                }
                // Move to the next one
                fhi = this->halfedge[fhi].next;
                ++fcount;
            } while (fhi != _hi && fhi != max && fcount < this->halfedge.size());

            if (debug) {
                std::cout << "From forwards loop: fcount = "  << fcount << " and fhi = " << fhi << " cf _hi = " << _hi << std::endl;
                if (zlen_halfedge == true) {
                    std::cout << "test fails because we have zlen_halfedge\n";
                }
                if (zlen_halfedge_to_halfedge == true) {
                    std::cout << "test fails because we have zlen_halfedge_to_halfedge\n";
                }
                if (approx_zlen_halfedge_to_halfedge == true) {
                    std::cout << "test fails because we have at least one zero or ~zero length halfedge\n";
                }
            }

            if (zlen_halfedge || zlen_halfedge_to_halfedge || approx_zlen_halfedge_to_halfedge) {
                return false;
            }

            // Check continuity in the reverse direction now
            uint32_t rcount = 0u;
            uint32_t rhi = _hi;
            do {
                if (this->halfedge[rhi].prev == rhi) { throw std::runtime_error ("self prev referral!"); }
                rhi = this->halfedge[rhi].prev;
                ++rcount;
            } while (rhi != _hi && rhi != max && rcount < this->halfedge.size());

            if (debug) {
                std::cout << "From reverse loop: rcount = "  << rcount << " and rhi = " << rhi << " cf _hi = " << _hi << std::endl;
            }

            if (fcount == rcount && rhi == _hi && fhi == _hi) {
                if (chain_length != max) {
                    if (fcount == chain_length) {
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    return true;
                }
            } else {
                return false;
            }
        }
        /*!
         * Verify a boundary that is not expected to be a triangle (its loop is likely to contain >3
         * halfedges)
         */
        bool verify_boundary (const uint32_t boundary_hi, const bool debug = false) const
        {
            return this->verify_halfedge_chain (boundary_hi, std::numeric_limits<uint32_t>::max(), debug);
        }

        /*!
         * Verify a halfedge loop that is expected to be a triangle with 3 halfedges
         */
        bool verify_triangle (const uint32_t tri_hi, const bool debug = false) const
        {
            return this->verify_halfedge_chain (tri_hi, 3, debug);
        }

        /*!
         * Return the three indices in the triangle containing halfedge hi
         *
         * A single max value in the set indicates an error
         */
        std::set<uint32_t> triangle_indices (uint32_t hi) const
        {
            std::set<uint32_t> indices;
            if (hi == std::numeric_limits<uint32_t>::max()) {
                indices.insert (hi);
                return indices;
            }
            for (uint32_t i = 0; i < 3; ++i) {
                indices.insert (hi);
                hi = this->halfedge[hi].next;
                if (hi >= this->halfedge.size()) { break; }
            }
            return indices;
        }

        /*!
         * Return the three vertex coordinates, in the NavMesh model frame for the triangle of which
         * the halfedge index tri_hi is a part.
         */
        sm::vec<sm::vec<float>, 3> triangle_vertices (uint32_t tri_hi) const
        {
            sm::vec<sm::vec<float>, 3> trivert = {};
            if (tri_hi == std::numeric_limits<uint32_t>::max()) {
                std::cout << "tri_hi is unset?\n";
                return trivert;
            }

            uint32_t hi = tri_hi;
            for (uint32_t i = 0; i < 3; ++i) {
                if (this->halfedge[hi].vi[0] < this->vertex.size()) {
                    trivert[i] = this->vertex[this->halfedge[hi].vi[0]].p;
                }
                hi = this->halfedge[hi].next;
                if (hi >= this->halfedge.size()) { break; }
            }
            if (hi != tri_hi) {
                // Triangle didn't close. This can occur at the edge of a flat model
                trivert[0][0] = std::numeric_limits<float>::max(); // to tell client code
            }

            return trivert;
        }

        /*!
         * Return the three vertex coordinates, transformed from the NavMesh model frame by
         * 'transform' for the triangle of which the halfedge index tri_hi is a part.
         */
        sm::vec<sm::vec<float>, 3> triangle_vertices (const uint32_t tri_hi, const sm::mat<float, 4>& transform) const
        {
            sm::vec<sm::vec<float>, 3> trivert = {};
            if (tri_hi == std::numeric_limits<uint32_t>::max()) {
                std::cout << "tri_hi is unset?\n";
                return trivert;
            }

            uint32_t hi = tri_hi;
            for (uint32_t i = 0; i < 3; ++i) {
                if (this->halfedge[hi].vi[0] < this->vertex.size()) {
                    trivert[i] = (transform * this->vertex[this->halfedge[hi].vi[0]].p).less_one_dim();
                }
                hi = this->halfedge[hi].next;
                if (hi >= this->halfedge.size()) { break; }
            }
            if (hi != tri_hi) {
                // Triangle didn't close. This can occur at the edge of a flat model
                trivert[0][0] = std::numeric_limits<float>::max(); // to tell client code
            }

            return trivert;
        }

        /*!
         * Compute the triangle normal for the ordered triplet of triangle vertices, tverts
         */
        sm::vec<float, 3> triangle_normal (const sm::vec<sm::vec<float>, 3>& tverts) const
        {
            sm::vec<float> n = (tverts[1] - tverts[0]).cross (tverts[2] - tverts[0]);
            n.renormalize();
            return n;
        }

        /*!
         * Retrieve the halfedge as a vector, transformed by the given transform
         */
        sm::vec<float> edge_vector (uint32_t hi, const sm::mat<float, 4>& transform) const
        {
            const sm::vec<float> v0 = (transform * this->vertex[this->halfedge[hi].vi[0]].p).less_one_dim();
            const sm::vec<float> v1 = (transform * this->vertex[this->halfedge[hi].vi[1]].p).less_one_dim();
            return v1 - v0;
        }

        /*!
         * Retrieve the coordinate of the start of the halfedge, transformed by the given transform
         */
        sm::vec<float> edge_start (uint32_t hi, const sm::mat<float, 4>& transform) const
        {
            return (transform * this->vertex[this->halfedge[hi].vi[0]].p).less_one_dim();
        }

        /*!
         * Find all the neighbours of the triangle *vertex* found at the start (position 0) of the
         * halfedge index a, throwing exceptions on errors.
         *
         * Returns the same vector of halfedge indices as find_neighbours, but without assuming that
         * the navmesh is good.
         *
         * \return vector of halfedge indices
         */
        std::vector<uint32_t> test_neighbours (const uint32_t a) const
        {
            uint32_t hi = a;
            std::vector<uint32_t> rtn = {};

            if (hi > this->halfedge.size()) { return rtn; }
            // We have to defensively check for repeated halfedges as we cycle around neighbours.
            std::set<uint32_t> repeat;
            do {
                if (repeat.count (hi)) {
                    // hi is a repeat. This means the mesh isn't perfect.
                    std::cout << "test_neighbours: Found a repeated halfedge that wasn't the first one\n";
                    for (auto h : rtn) {
                        std::cout << h << " flag: " << this->halfedge[h].flags
                                  << " prev: " << this->halfedge[h].prev
                                  << " prev.twin: " << this->halfedge[this->halfedge[h].prev].twin << std::endl;
                    }
                    std::cout << "The repeat was: " << hi << std::endl;
                    throw std::runtime_error ("Repeated non-start halfedge"); // caused by 2 boundary halfedges with the same 'prev'
                }
                // hi emanates from the vertex, so return it.
                rtn.push_back (hi);
                repeat.insert (hi);
                uint32_t pr = this->halfedge[hi].prev;
                if (pr == std::numeric_limits<uint32_t>::max()) {
                    throw std::runtime_error ("halfedge.prev was unset!?!");
                }
                hi = this->halfedge[pr].twin;
                // or hi = this->halfedge[this->halfedge[hi].twin].next; // Clockwise
            } while (hi != a && hi != std::numeric_limits<uint32_t>::max());

            if (hi == std::numeric_limits<uint32_t>::max()) { throw std::runtime_error ("A twin was unset!?!"); }
            return rtn;
        }

        /*!
         * Find all the neighbours of triangle *vertex* index a.
         *
         * Assumes the navmesh is good, and has passed NavMesh::test()
         *
         * \return vector of halfedge indices, including all neighbour triangles AND self (a)
         */
        std::vector<uint32_t> find_neighbours (const uint32_t a) const
        {
            uint32_t hi = a;
            std::vector<uint32_t> rtn = {};
            if (hi > this->halfedge.size()) { return rtn; }
            do {
                // hi emanates from the vertex, so return it.
                rtn.push_back (hi);
                uint32_t pr = this->halfedge[hi].prev;
                hi = this->halfedge[pr].twin;
                // or hi = this->halfedge[this->halfedge[hi].twin].next; // Clockwise
            } while (hi != a);
            return rtn;
        }

        /*!
         * Test the navmesh, to make sure it is perfect
         */
        void test (const bool just_mark_bad = false)
        {
            std::cout << "NavMesh verification test running...\n";
            // 1) Verify that each halfedge is part of a face or hole (boundary) and is not a line?
            for (uint32_t hi = 0; hi < this->halfedge.size(); ++hi) {
                if ((this->halfedge[hi].flags & 0x1) == 0x1) { // 0x1 flag means 'on boundary'
                    if (this->verify_boundary (hi) == false) {
                        if (just_mark_bad == false) {
                            throw std::runtime_error ("Imperfect halfedge mesh (boundary hole)");
                        }
                    }
                } else {
                    if (this->verify_triangle (hi) == false) {
                        if (just_mark_bad == true) {
                            this->halfedge[hi].flags |= 0x2; // 0x2 means 'rogue halfedge'
                        } else {
                            throw std::runtime_error ("Imperfect halfedge mesh (face triangle)");
                        }
                    }

                    std::vector<uint32_t> nb = this->test_neighbours (hi);
                    // Don't expect a very large number of neighbours
                    if (nb.size() > 100) { throw std::runtime_error ("too many neighbours?"); }
                }

                // Make sure twin is set for every halfedge
                if (this->halfedge[hi].twin == std::numeric_limits<uint32_t>::max()) {
                    throw std::runtime_error ("Contains an untwinned halfedge");
                }
            }
        }

        /*!
         * During the construction of the halfedge mesh, after making the neighbour relations from
         * the OpenGL mesh, the last step is to fill in the *boundary* halfedges.
         *
         * Find all halfedges with an unset twin and then start creating the new half edges to fill
         * in.
         */
        void add_boundary_halfedges()
        {
            constexpr uint32_t max = std::numeric_limits<uint32_t>::max();
            constexpr bool debug_bnd = false;
            constexpr bool debug_bnd2 = false;

            const uint32_t sz = this->halfedge.size();
            uint32_t j = 0;
            if constexpr (debug_bnd) { std::cout << "BEFORE adding boundary, halfedge.size() = " << halfedge.size() << std::endl; }
            for (uint32_t i = 0; i < sz; ++i) {
                if (this->halfedge[i].twin == max) {
                    if constexpr (debug_bnd) { std::cout << "STARTING at i = " << i; }
                    // This halfedge does not have a twin, walk the boundary from here
                    const uint32_t j0 = j; // j index at boundary start
                    if constexpr (debug_bnd) { std::cout << ".... with j0 = " << j0 << std::endl; }
                    uint32_t bprev = max;
                    uint32_t cur = i;
                    uint32_t done = 0u;
                    while (!done) {
                        if constexpr (debug_bnd2) { std::cout << "** Search for boundary from cur = " << cur << std::endl; }
                        uint32_t bcand = cur; // bcand starts as an internal halfedge
                        uint32_t bcandi = max;
                        if constexpr (debug_bnd2) { std::cout << "halfedge[" << i << "].twin = " << this->halfedge[i].twin << std::endl; }
                        uint32_t bcand0 = max;
                        do {
                            bcandi = this->halfedge[bcand].prev;
                            bcand = this->halfedge[bcandi].twin; // if max, it's a boundary, else it's internal
                            if constexpr (debug_bnd2) { std::cout << "bcandi (inner): " << bcandi << ", bcand: " << bcand << std::endl; }
                            if (bcand != max && bcand == bcand0) {
                                // We've looped back without finding a boundary halfedge. halfedge[cur] is probably a rogue halfedge/vertex
                                ++done;
                                if constexpr (debug_bnd) { std::cout << "halfedge[cur] is a rogue halfedge/vertex?\n"; }
                            }
                            if (bcand0 == max) { bcand0 = bcand; } // bcand0 tests we we looped back, but not to halfedge[i].twin

                        } while (bcand != max && bcand != this->halfedge[i].twin && !done);
                        // && bcandi != cur <-- This last while() test probably crept in during development with out being necessary

                        if (done) {
                            // The bcand we have right now is NOT a boundary halfedge, nor is it the
                            // twin for cur, so just mark halfedge flags with the 'rogue' flag (0x2)
                            // which can be used by NormalsVisual to show the offending halfedge
                            bool rogue_is_tri = this->verify_triangle (cur, true);
                            std::cout << "Identified a rogue, which " << (rogue_is_tri ? "IS" : "ISN'T")
                                      << " a triangle; navmesh not likely to be usable\n";
                            this->halfedge[cur].flags |= 0x2;
                        } else {
                            // Now we add the new halfedge twin for cur.
                            const uint32_t _bnext = sz + j + 1;
                            const uint32_t newi = halfedge.size();
                            if constexpr (debug_bnd) { std::cout << "Push-back to halfedge[" << newi
                                                                 << "]: " << this->vertex[this->halfedge[cur].vi[1]].p
                                                                 << "," << (this->vertex[this->halfedge[cur].vi[0]].p - this->vertex[this->halfedge[cur].vi[1]].p)
                                                                 << " with prev = " << bprev << " next = " << _bnext
                                                                 << " and twin = " << cur << std::endl; }
                            uint32_t fl = 1;
                            if ((this->vertex[this->halfedge[cur].vi[0]].p - this->vertex[this->halfedge[cur].vi[1]].p).length() < 10.0f * std::numeric_limits<float>::epsilon()) {
                                fl |= 2;
                            }

                            this->halfedge.push_back ({{this->halfedge[cur].vi[1], this->halfedge[cur].vi[0]}, cur, _bnext, bprev, fl});
                            this->halfedge[cur].twin = newi;

                            if (bcandi == i) {
                                // We've come all the way around the boundary loop and we are finished.
                                const uint32_t _first = sz + j0;
                                const uint32_t _last = sz + j;
                                this->halfedge[_first].prev = _last;
                                if constexpr (debug_bnd) {  std::cout << "Update final next for halfedge[" << _last << "] to " <<  _first << std::endl; }
                                if constexpr (debug_bnd) {  std::cout << "Set initial prev for halfedge[" << _first << "] to " <<  _last << std::endl; }
                                this->halfedge[_last].next = _first;
                                ++done;
                            } else {
                                // We've only added one new halfedge to the boundary loop, so carry on...
                                bprev = sz + j;
                                cur = bcandi;
                            }
                            ++j;
                        }
                    }
                    if constexpr (debug_bnd) { std::cout << "Added " << (j - j0) << " halfedges to that boundary\n"; }
                }
            }

            // Lastly - check through for rogues!
            if constexpr (debug_bnd) { std::cout << "Post-search for rogues\n"; }

            for (uint32_t i = 0; i < sz; ++i) {
                if (this->halfedge[i].twin == max) {
                    bool rogue_is_tri = this->verify_triangle (i, true);
                    std::cout << "Identified a rogue, which " << (rogue_is_tri ? "IS" : "ISN'T") << " a triangle.\n";
                    // How to remove? Answer: Preprocess the model in Blender (or similar)
                }
            }
        }

        /*!
         * Determine neighbour relations. That means populating a halfedge data structure. Don't
         * think there's any way around the at-worst O(N^2) computation, so save results into a
         * binary file that can be re-loaded at each startup.
         *
         * The key is the half-edge data structure.
         * See: https://jerryyin.info/geometry-processing-algorithms/half-edge/
         */
        void compute_neighbour_relations()
        {
            constexpr bool debug_nr = false;
            uint32_t sz = this->halfedge.size();
            if constexpr (debug_nr) { std::cout << "Finding twins for " << sz << " halfedge\n"; }

            // Search a 'band' either side of i first, assuming that neighbour faces are likely
            // to have been nearby in the indices array
            const uint32_t band = 3 * 1000;
            uint32_t wider_searches = 0; // Count how many times we make a wider search
            uint64_t twin_meandist = 0; // See how far a search has to search for a twin
            uint32_t twins = 0;

#pragma omp parallel for
            for (uint32_t i = 0; i < sz; ++i) {

                const sm::vec<uint32_t, 2>& vi = this->halfedge[i].vi;

                // halfedge[i].twin may already have been set (as we set two twins at a time)
                if (this->halfedge[i].twin != std::numeric_limits<uint32_t>::max()) { continue; }

                // It's useful to know how long you will have to wait...
                if (i % 20000u == 0u) { std::cout << ((100.0f * i)/sz) << " percent...\n" << std::endl; }

                [[maybe_unused]] uint32_t sb = i >= band ? i - band : 0;
                [[maybe_unused]] uint32_t eb = i + band < sz ? i + band : sz;

                uint32_t wider = 0;
#if 0
                // The Simplest code would be a single loop
                for (uint32_t j = 0; j <  sz; ++j) {
                    if (j == i) { continue; }
                    const sm::vec<uint32_t, 2>& vij = this->halfedge[j].vi;
                    if (vi[0] == vij[1] && vi[1] == vij[0]) { // It's a match.
                        this->halfedge[i].twin = j;
                        this->halfedge[j].twin = i;
                        break;
                    }
                }
#endif
                // But it's worth optimizing:
                // First sb to eb, which we hope is most likely to find a twin
                for (uint32_t j = sb; j < eb; ++j) {
                    if (j == i) { continue; }
                    const sm::vec<uint32_t, 2>& vij = this->halfedge[j].vi;
                    if (vi[0] == vij[1] && vi[1] == vij[0]) { // It's a match.
                        this->halfedge[i].twin = j;
                        this->halfedge[j].twin = i;
                        break;
                    }
                }

                if (this->halfedge[i].twin == std::numeric_limits<uint32_t>::max()) {
                    // Then, if no match, search from 0 to sb
                    if (sb != 0 && !wider) { wider = 1; }
                    for (uint32_t j = 0; j < sb; ++j) {
                        if (j == i) { continue; }
                        const sm::vec<uint32_t, 2>& vij = this->halfedge[j].vi;
                        if (vi[0] == vij[1] && vi[1] == vij[0]) { // It's a match
                            this->halfedge[i].twin = j;
                            this->halfedge[j].twin = i;
                            break;
                        }
                    }
                }

                if (this->halfedge[i].twin == std::numeric_limits<uint32_t>::max()) {
                    // If still no match search from eb to sz
                    if (eb != sz && !wider) { wider = 1; }
                    for (uint32_t j = eb; j < sz; ++j) {
                        if (j == i) { continue; }
                        const sm::vec<uint32_t, 2>& vij = this->halfedge[j].vi;
                        if (vi[0] == vij[1] && vi[1] == vij[0]) { // It's a match
                            this->halfedge[i].twin = j;
                            this->halfedge[j].twin = i;
                            break;
                        }
                    }
                }

                wider_searches += wider;

                if (this->halfedge[i].twin != std::numeric_limits<uint32_t>::max()) {
                    if (wider) {
                        twin_meandist += i > this->halfedge[i].twin ? i - this->halfedge[i].twin : this->halfedge[i].twin - i;
                        ++twins;
                    }
                } // else halfedge[i] is an edge of the mesh
            }
            if constexpr (debug_nr) {
                std::cout << "In " << sz << " halfedge searches, had to widen the search in "
                          << (100.0 * wider_searches) / sz << " percent\n";
                std::cout << "Mean wider twin search distance (in array elements) was "
                          << static_cast<double>(twin_meandist) / twins << "\n";
            }
        }

        /*!
         * A subroutine for find_triangle_crossing
         */
        bool test_tri (std::set<uint32_t>& tested, const uint32_t ontest,
                       const sm::vec<float>& vstart, const sm::vec<float>& vdir,
                       sm::vec<float>& isect_p, uint32_t& isect_ti) const
        {
            if (tested.count (ontest)) { return false; }
            tested.insert (ontest);
            sm::vec<sm::vec<float>, 3> v = this->triangle_vertices (ontest);
            auto [isect, p] = sm::geometry::ray_tri_intersection<float> (v[0], v[1], v[2], vstart, vdir);
            if (isect) {
                isect_p = p;
                isect_ti = ontest;
            }
            return isect;
        }

        /*!
         * Find the location, and the triangle indices at which a ray starting from coord with
         * direction vdir - the 'penetration point' intersects with this NavMesh model.
         *
         * \param model_to_scene Transform that is only passed to find_vertex_normal. May in future be unnecessary.
         *
         * \param ti_ml The most likely triangle, if you know what it probably is, to reduce the
         * search time.
         *
         * \return a tuple containing crossing location, halfedge index (which specifies a triangle)
         */
        std::tuple<sm::vec<float>, uint32_t>
        find_triangle_crossing (const sm::vec<float>& coord_mf, const sm::vec<float>& vdir,
                                const sm::mat<float, 4>& model_to_scene,
                                const uint32_t ti_ml = std::numeric_limits<uint32_t>::max() ) const
        {
            constexpr bool debug_ftc = false;
            constexpr float fmax = std::numeric_limits<float>::max();
            sm::vec<float> vstart = coord_mf - (vdir / 2.0f);

            // Return objects
            sm::vec<float> isect_p = { fmax, fmax, fmax };
            uint32_t isect_ti = std::numeric_limits<uint32_t>::max();

            std::set<uint32_t> tested;

            // Have we been passed a 'most likely triangle' to test first? If so, test it.
            if (ti_ml != std::numeric_limits<uint32_t>::max()) {
                if (test_tri (tested, ti_ml, vstart, vdir, isect_p, isect_ti)) {
                    // we found it in the first triangle!
                    if constexpr (debug_ftc) { std::cout << "Got a first-tri hit!\n"; }
                    return { isect_p, isect_ti };
                }

                // Next, test the neighbours of ti_ml
                std::vector<uint32_t> nbs = this->find_neighbours (ti_ml);
                if constexpr (debug_ftc) { std::cout << "Testing " << nbs.size() << " neighbours of ti_ml for a hit\n"; }
                for (uint32_t nb : nbs) {
                    if (test_tri (tested, nb, vstart, vdir, isect_p, isect_ti)) {
                        if constexpr (debug_ftc) { std::cout << "Got a neighbour hit!\n"; }
                        return { isect_p, isect_ti };
                    }
                }

                // Do neighbours of neighbours...
                for (uint32_t nb : nbs) {
                    std::vector<uint32_t> nbs2 = this->find_neighbours (nb);
                    for (uint32_t nb2 : nbs2) {
                        if (test_tri (tested, nb2, vstart, vdir, isect_p, isect_ti)) {
                            std::cout << "Got a neighbour-neighbour hit!\n";
                            return { isect_p, isect_ti };
                        }
                    }
                }
            }

            // Fall back to testing ALL the triangles...
            if constexpr (debug_ftc) { std::cout << "Oh, no have to test ALL triangles now...\n"; }
            for (auto tri : this->triangles) {
                sm::vec<sm::vec<float>, 3> v = this->triangle_vertices (tri.hi);
                auto [isect, p] = sm::geometry::ray_tri_intersection<float> (v[0], v[1], v[2], vstart, vdir);
                if (isect) {
                    isect_p = p;
                    isect_ti = tri.hi;
                    break;
                }
            }


            if (isect_p[0] == fmax) {
                // Found no triangle intersection; check vertices, in case vdir points perfectly at a vertex.
                if constexpr (debug_ftc) { std::cout << "Finally, check vertices...\n"; }

                constexpr float dist_thresh = 2.0f * std::numeric_limits<float>::epsilon();
                for (auto tri : this->triangles) {
                    sm::vec<sm::vec<float>, 3> v = this->triangle_vertices (tri.hi);
                    for (uint32_t vi = 0; vi < 3; ++vi) {
                        if (sm::geometry::ray_point_intersection ((model_to_scene * v[vi]).less_one_dim(), vstart, vdir, dist_thresh)) {
                            isect_p = v[vi];
                            isect_ti = tri.hi;
                            break;
                        }
                    }
                }
            }

            return { isect_p, isect_ti };
        }

        /*!
         * Find the location, and the triangle indices (by means of a halfedge index) at which a ray
         * between coord (in model frame) and the model centroid cross - the 'penetration point'.
         */
        std::tuple<sm::vec<float>, uint32_t>
        find_triangle_crossing (const sm::vec<float>& coord_mf, const sm::mat<float, 4>& model_to_scene) const
        {
            sm::vec<float> vdir = this->bb.mid() - coord_mf;
            vdir.renormalize();
            return this->find_triangle_crossing (coord_mf, vdir, model_to_scene);
        }

        /*!
         * Find the normal of the vertex specified by halfedge ti
         */
        sm::vec<float> find_vertex_normal (const uint32_t ti, const sm::mat<float, 4>& transform) const
        {
            auto neighbs = this->find_neighbours (ti);
            sm::vec<float> vn = {};
            if (neighbs.size() == 0) { return vn; }
            for (auto nb : neighbs) {
                // Turn nb, a half edge index, into a triangle?
                vn += this->triangle_normal (this->triangle_vertices (nb, transform));
            }
            return (vn / neighbs.size());
        }

        /*!
         * Flags class for partial_movement
         */
        enum class pm_fl : uint32_t
        {
            crossed,        // Means the partial movement crossed an edge
            colinear,       // Means the movement was colinear with an edge
            near_vertex_0,  // The partial movement crossed very close to vertex 0 of the crossed edge
            near_vertex_1   // The partial movement crossed very close to vertex 0 of the crossed edge
        };

        /*!
         * The partial movement that takes us to the crossing point, specified as movement + endpoint
         * (rather than startpoint + movement)
         */
        struct partial_movement
        {
            // The movement vector
            sm::vec<float> mv = {};
            // The end coordinate of the movement
            sm::vec<float> end = {};
            constexpr sm::flags<pm_fl> default_flags()
            {
                sm::flags<pm_fl> _flags;
                _flags.reset();
                _flags.set (pm_fl::crossed, true); // assume crossed in new partial_movement
                return _flags;
            }
            // boolean state
            sm::flags<pm_fl> flags = default_flags();
        };

        /*!
         * Find the part of mv_inplane that gets us to the triangle boundary defined by edge_s and
         * edge_e
         *
         * IS IS ASSUMED that mv_s is in the triangle plane and that a movement of mv_inplane would cross
         * the edge if it were long enough.
         *
         * All vectors and coordinates here are in the same coordinate frame as the triangle
         * vertices. That could be either the model frame OR the scene frame (but always one or the
         * other).
         *
         * \param edge_s Starting coordinate of the edge
         * \param edge_e End coordinate of the edge
         * \param t_norm The triangle normal vector
         * \param mv_s The movement starting point
         * \param mv_inplane The planned movement, starting from hovlocn
         *
         * \return a struct containing the partial movement vector and the end of the movement as a
         * coordinate. If mv_inplane does not cross the edge, then the return object contains the vector
         * mv_inplane itself, and the coordinate that this movement ends at.
         */
        partial_movement find_edge_crossing (const sm::vec<float>& edge_s,
                                             const sm::vec<float>& edge_e,
                                             const sm::vec<float>& t_norm,
                                             const sm::vec<float>& mv_s,
                                             const sm::vec<float>& mv_inplane)
        {
            constexpr bool debug = false;
            partial_movement pm;
            sm::vec<float> edge = edge_e - edge_s;

            sm::vec<float> u_y = edge;
            u_y.renormalize();
            sm::vec<float> u_z = t_norm;
            u_z.renormalize();
            sm::vec<float> u_x = u_y.cross (u_z);
            if constexpr (debug) {
                std::cout << "fec: mv_inplane = " << mv_inplane << std::endl;
                std::cout << "fec: edge = " << edge << std::endl;
                std::cout << "fec: Basis: " << u_x << " " << u_y << " " << u_z << std::endl;
            }

            // Create a matrix to convert from mdl frame movements to the triangle frame of ref.
            sm::mat<float, 4> from_triangle_frame = sm::mat<float, 4>::frombasis (u_x, u_y, u_z);
            sm::mat<float, 4> to_triangle_frame = from_triangle_frame.inverse();

            // Use Edge as our 'y' and the orthogonal as our 'x', then express mv_inplane in terms
            // of these two unit vectors. We also have our 'z' which is the triangle normal.
            sm::vec<float, 4> mv_inplane4d = to_triangle_frame * mv_inplane;
            sm::vec<float, 2> mv_inplane2d = { mv_inplane4d[0], mv_inplane4d[1] };
            sm::vec<float, 4> h_4d = to_triangle_frame * mv_s;
            sm::vec<float, 2> h_2d =  { h_4d[0], h_4d[1] };
            sm::vec<float, 4> edge_4d = to_triangle_frame * edge;
            sm::vec<float, 2> edge_2d =  { edge_4d[0], edge_4d[1] };
            sm::vec<float, 4> edge_s_4d = to_triangle_frame * edge_s;
            sm::vec<float, 2> edge_s_2d =  { edge_s_4d[0], edge_s_4d[1] };

            // Can now apply algo to find crossing point
            if constexpr (debug) {
                std::cout << "fec: intersection test for lines: " << edge_s_2d << " --> " << (edge_2d + edge_s_2d)
                          << " and " << h_2d << " --> " << (h_2d + mv_inplane2d) << "\n";
            }

            std::bitset<2> si = sm::geometry::segments_intersect<float> (edge_s_2d, edge_s_2d + edge_2d, h_2d, h_2d + mv_inplane2d);
            if (si.test(1)) {
                if constexpr (debug) { std::cout << "fec: Colinear with triangle edge!\n"; }
                pm.flags.set (pm_fl::colinear, true);
                // Identify the vertex that we're moving towards. edge_4d is the triangle edge.
                // so: mv_inplane4d.dot (edge_4d) should be positive if edge_e is the vertex
                sm::vec<float> mv_inplane3d = mv_inplane4d.less_one_dim();
                sm::vec<float> edge_e_3d = (to_triangle_frame * edge_e).less_one_dim();
                sm::vec<float> edge_s_3d = edge_s_4d.less_one_dim();

                if constexpr (debug) {
                    std::cout << "mv_inplane: " << mv_inplane3d << ", edge_e: " << edge_e_3d << ", edge_s: " << edge_s_3d << std::endl;
                    std::cout << "mv_inplane.dot (edge_e): " << mv_inplane3d.dot (edge_e_3d) << std::endl;
                    std::cout << "mv_inplane.dot (edge_s): " << mv_inplane3d.dot (edge_s_3d) << std::endl;
                }
                sm::vec<float> to_v = {};
                if (mv_inplane3d.dot (edge_e_3d) > mv_inplane3d.dot (edge_s_3d)) {
                    to_v = edge_e_3d - (h_4d).less_one_dim();
                } else {
                    to_v = edge_s_3d - (h_4d).less_one_dim();
                }

                if (to_v.length() <= mv_inplane3d.length()) {
                    if constexpr (debug) { std::cout << "fec: partial colinear move to vertex\n"; }
                    pm.flags.set (pm_fl::crossed, true);
                    pm.mv = (from_triangle_frame * to_v).less_one_dim(); // need to know if we were to go over a vertex
                    pm.end = (from_triangle_frame * edge_e_3d).less_one_dim();
                } else {
                    if constexpr (debug) { std::cout << "fec: partial colinear along/within edge\n"; }
                    pm.flags.set (pm_fl::crossed, false);
                    // Compute end from mv_inplane4d
                    pm.mv = (from_triangle_frame * mv_inplane4d).less_one_dim();
                    pm.end = (from_triangle_frame * (h_4d + mv_inplane4d)).less_one_dim();
                }

            } else {
                if (si.test(0)) {
                    // Intersects as expected
                    sm::vec<float, 2> cp2d = sm::geometry::crossing_point<float> (edge_s_2d, edge_s_2d + edge_2d, h_2d, h_2d + mv_inplane2d);
                    // Now go from cross point 2d to a point in model coordinates?
                    pm.end = (from_triangle_frame * cp2d.plus_one_dim(edge_s_4d[2])).less_one_dim();
                    if constexpr (debug) { std::cout << "fec: Cross point in mdl frame: " << pm.end << std::endl; }

                    // Check if cross point is close to a vertex
                    sm::vec<float, 2> e1 = cp2d - edge_s_2d;
                    float d2_e1 = e1.length();
                    if (d2_e1 < 100.0f * std::numeric_limits<float>::epsilon()) {
                        if constexpr (debug) {
                            std::cout << "Set near_vertex flag due to end 0 (within " << 100.0f * std::numeric_limits<float>::epsilon() << ")\n";
                        }
                        pm.flags.set (pm_fl::near_vertex_0);
                    }
                    e1 += edge_2d;
                    float d2_e2 = e1.length();
                    if (d2_e2 < 100.0f * std::numeric_limits<float>::epsilon()) {
                        if constexpr (debug) { std::cout << "Set near_vertex flag due to end 1\n"; }
                        pm.flags.set (pm_fl::near_vertex_1);
                    }

                    if constexpr (debug) {
                        std::cout << "fec: Distance to edge end 1: " << d2_e1 << ", and to end 2: " << d2_e2 << std::endl;
                    }

                    pm.mv = pm.end - mv_s;

                } else {
                    // 'No intersection' can occur when: the movement goes over/close to the end of the edge.
                    // Or when: the move starts ON the edge of a triangle and then moves *away* from the tri.
                    if constexpr (debug) {
                        std::cout <<  "fec: No intersection across edge for: "
                                  << (edge_s_2d) << " -- " << (edge_2d + edge_s_2d) << " and "
                                  << h_2d << " -- " << (h_2d + mv_inplane2d) << std::endl;
                    }
                    // Mark that there was no intersection
                    pm.flags.set (pm_fl::crossed, false);
                    pm.mv = sm::vec<float>{};
                    pm.end = mv_s;
                }
            }

            return pm;
        }

        /*!
         * After testing up to all three edges of a triangle, we return information about the crossing
         * location and the indices of the triangle that form the crossed edge in this structure.
         */
        struct crossing_data
        {
            // The crossed halfedge
            uint32_t crossed = std::numeric_limits<uint32_t>::max();
            // A halfedge in the triangle that we cross into. Usually set to this->halfedge[halfedge].twin
            uint32_t into = std::numeric_limits<uint32_t>::max();
            // The crossed edge as a vector
            sm::vec<float> tri_edge = {};
            // The remaining movement after the crossing, with direction rotated about tri_edge. max means unset
            sm::vec<float> mv_rest = { std::numeric_limits<float>::max() };
            // The partial movement. pm.mv is the movement up to the crossing point, pm.end is the crossing point
            partial_movement pm = {};
        };

        /*!
         * Find the location at which a movement from mv_s in the direction mv_inplane crosses one of
         * the edges of the triangle specified by the three vertices in t_verts/t_indices.
         *
         * IT IS ASSUMED that the triangle normal passing through mv_s WILL intersect the
         * triangle (this may include an edge or vertex intersection). (Test beforehand with sm::geometry::ray_tri_intersection)
         *
         * All coordinates are in the frame of the model that contains this triangle.
         *
         * \param t_verts *Ordered* vertices of the triangle. Vertices should be in anti-clockwise
         * order when viewed with the triangle normal vector coming 'out of the page'. These should
         * be the vertices that were generated with the param tri (using function
         * triangle_vertices()).  Could be obtained within this function, but have already been
         * computed, and they may be in a different frame of ref that they have in this->vertex
         *
         * \param tri The halfedge that gives the triangle
         *
         * \param mv_s The start of the planned movement
         *
         * \param mv_inplane The planned movement
         */
        crossing_data compute_crossing_location (const sm::vec<sm::vec<float>, 3>& t_verts,
                                                 const uint32_t tri,
                                                 const sm::vec<float>& mv_s,
                                                 const sm::vec<float>& mv_inplane)
        {
            constexpr bool debug = false;
            crossing_data cd;
            cd.pm.flags.set (pm_fl::crossed, false);

            sm::vec<float> p = mv_s + mv_inplane;
            sm::vec<float> tn = this->triangle_normal (t_verts);

            // do-while with tri
            uint32_t hi = tri;
            uint32_t a = 0;
            sm::vec<bool, 3> inside = { false, false, false };
            do {
                uint32_t b = (a + 1u) % 3u;

                sm::vec<float> edge = t_verts[b] - t_verts[a];
                sm::vec<float> ptoe = p - t_verts[a];

                inside[a] = (tn.dot (edge.cross (ptoe)) >= 0);
                if (!inside[a]) {
                    partial_movement pm = find_edge_crossing (t_verts[a], t_verts[b], tn, mv_s, mv_inplane);
                    if constexpr (debug) {
                        if (pm.flags.test (pm_fl::colinear)) {
                            std::cout << "ccl: fec returned pm.colinear true for t" << a << "t" << b << "\n";
                        }
                    }
                    if (pm.flags.test (pm_fl::crossed) == false && pm.flags.test (pm_fl::colinear) == false) {
                        inside[a] = true;
                        if constexpr (debug) {
                            std::cout << "ccl: No intersection for edge t" << a << "t" << b << " " << t_verts[a] << "," << (t_verts[b] - t_verts[a])
                                      << " and move " << mv_s << "," << mv_inplane << std::endl;
                        }
                    } else {
                        if constexpr (debug) {
                            if (pm.flags.test (pm_fl::colinear)) { std::cout << "ccl: colinear t" << a << "t" << b << "\n"; }
                            if (pm.flags.test (pm_fl::crossed) == false) { std::cout << "ccl: no cross point t" << a << "t" << b << "\n"; }
                            std::cout << "ccl: Intersection for edge t" << a << "t" << b << " " << t_verts[a] << "," << (t_verts[b] - t_verts[a])
                                      << " and move " << mv_s << "," << mv_inplane << std::endl;
                        }
                        cd.pm = pm;
                        cd.tri_edge = edge;
                        cd.crossed = hi;
                    }
                } else {
                    if constexpr (debug) {
                        std::cout << "inside[" << a << "] is true for edge t" << a << "t" << b << " "
                                  << t_verts[a] << "," << (t_verts[b] - t_verts[a]) << "\n";
                    }
                }

                ++a;
                hi = this->halfedge[hi].next;

            } while (hi != tri && a < 3);


            // We've now tested edge crossing for three edges in the triangle.
            if constexpr (debug) {
                if (cd.pm.flags.test (pm_fl::crossed) == true) {
                    std::cout << "ccl: Crossed over" << (inside[0] ? " " : " 0-1")
                              << (inside[1] ? " " : " 2-1") <<  (inside[2] ? " " : " 0-2");
                    if (cd.pm.flags.any_of ({pm_fl::near_vertex_0, pm_fl::near_vertex_1}) == true) { std::cout << " near a vertex"; }
                    std::cout << std::endl;
                    // could test pairs of inside01 etc to detect crossing a vertex
                } else if (cd.pm.flags.test (pm_fl::colinear) == true) {
                    // Movement was colinear. Set Crossed vertex?
                    std::cout << "ccl: movement was colinear!\n";
                    if (cd.pm.flags.test (pm_fl::crossed) == false) {
                        std::cout << "ccl: Colinear along edge" << std::endl;
                    } else {
                        std::cout << "ccl: Colinear to vertex" << std::endl;
                    }
                    // cd.pm.crossed will tell if there's a cross point or not
                } else {
                    // We have NO crossing, which can occur for a variety of reasons
                    std::cout << "ccl: No crossings " << (inside[0] ? " " : "!!0-1")
                              << (inside[1] ? " " : "!!2-1") <<  (inside[2] ? " " : "!!0-2") << std::endl;
                }
            }

            return cd;
        }

        /*!
         * Find the model location, starting from the location of a camera specified in
         * camspace. Cast a ray in the direction \a vdir, starting from the camera location in the
         * model frame \a camloc_mf, and figure out which triangle in the navmesh the ray passes
         * through.
         *
         * \param model_to_scene The model to scene transformation for the parent of the navmesh
         *
         * \param camloc_mf The camera location in the model frame. This gives us the start location
         * for the ray.
         *
         * \param vdir The direction of the ray (its length is also significant).
         *
         * \param ti_ml The most likely triangle, if you know what it probably is, to reduce the
         * search time.
         *
         * \return tuple containing: the hit point in scene coordinates; the index of the triangle
         * we hit (also set into this->ti0).
         */
        std::tuple<sm::vec<float>, uint32_t>
        find_triangle_hit (const sm::mat<float, 4>& model_to_scene,
                           const sm::vec<float>& camloc_mf, const sm::vec<float>& vdir,
                           uint32_t ti_ml = std::numeric_limits<uint32_t>::max())
        {
            this->ti0 = std::numeric_limits<uint32_t>::max();
            sm::vec<float> hit = {};
            // Want to pass 'best tri' to this
            std::tie (hit, this->ti0) = this->find_triangle_crossing (camloc_mf, vdir, model_to_scene, ti_ml);

            if (this->ti0 == std::numeric_limits<uint32_t>::max()) { std::cout << __func__ << ": No hit\n"; }

            sm::vec<float> hp_scene = (model_to_scene * hit).less_one_dim();

            constexpr bool debug = false;
            if constexpr (debug) {
                std::cout << "found hit at " << hit << " (model); " << hp_scene << " (scene) in direction " << vdir << "\n";
                // Check we'll get a hit when we compute_mesh_movement:
                sm::vec<sm::vec<float>, 3> tv_mf = this->triangle_vertices (this->ti0);
                auto tn = this->triangle_normal (tv_mf);
                std::cout << "tn: " << tn << ", length " << tn.length() << std::endl;
                std::cout << "TEST ray_tri_intersection (hit,-tn): " << (hit + (tn / 2.0f)) << "," << -tn << std::endl;
                auto [isect, hov_mf] = sm::geometry::ray_tri_intersection<float> (tv_mf[0], tv_mf[1], tv_mf[2], hit + (tn / 2.0f), -tn);
                if (isect) {
                    std::cout << "ray_tri_intersection confirms we would hit at " << hov_mf << "\n";
                } else {
                    std::cout << "ray_tri_intersection DOES NOT get a hit\n";
                }
            }

            return { hp_scene, this->ti0 };
        }

        /*!
         * Find the model location, starting from the location of a camera specified in
         * camspace. Cast a ray towards the centroid of this navmesh and figure out which triangle
         * in the navmesh the ray passes through.
         *
         * \param camspace The camera transformation matrix that converts camera coordinates into
         * the scene frame. This gives us the start location for the ray.
         *
         * \param model_to_scene The model to scene transformation for the parent of the navmesh
         *
         * \param search_dist_mult a multiplier on the search distance. The length of vdir in this
         * function should cross the landscape model. By default it's the vector from the camera
         * location in the model frame of reference to the middle of the bounding box. If the vector
         * is too long when finding the surface of a convex hull, such as a model of a rock, it is
         * possible to mis-identify the back side of the model. However, for finding a location on a
         * large, flat, one-sided landscape, we want to make vdir long. search_dist_mult is applied
         * to vdir.
         *
         * \return tuple containing: the hit point in scene coordinates and halfedge referring to
         * the triangle we hit.
         */
        std::tuple<sm::vec<float>, uint32_t>
        find_triangle_hit (const sm::mat<float, 4>& camspace, const sm::mat<float, 4>& model_to_scene,
                           const float search_dist_mult = 1.0f)
        {
            sm::mat<float, 4> scene_to_model = model_to_scene.inverse();
            // use camera location in gltf to start from, then find model surface.
            sm::vec<float> camloc_mf = (scene_to_model * camspace * sm::vec<float>{}).less_one_dim();
            sm::vec<float> vdir = this->bb.mid() - camloc_mf;
            vdir *= search_dist_mult;

            return this->find_triangle_hit (model_to_scene, camloc_mf, vdir);
        }

        /*!
         * Position the camera a distance hoverheight above the location hp_scene, with its forward
         * direction _z and its 'x' axis in direction _x. _x, _y and _z are used to form a basis
         * that creates a coordinate transform matrix.
         *
         * \return the transformation matrix that positions the camera
         */
        sm::mat<float, 4> position_camera (const sm::vec<float>& hp_scene, const sm::mat<float, 4>& model_to_scene,
                                           const sm::vec<float>& _x, const sm::vec<float>& _y, const sm::vec<float>& _z,
                                           const float hoverheight)
        {
            // I think this positions correctly now (which is all it has to do). It ignores scaling
            // in model_to_scene. Can be reduced to use fewer mat<>s.
            sm::mat<float, 4> cam_mv_y;
            cam_mv_y.translate (sm::vec<float>{0, hoverheight, 0});

            // The basis _x, _y, _z, where these are vectors in the model frame that define a camera frame
            sm::mat<float, 4> cam_to_model_rotn = sm::mat<float, 4>::frombasis (_x, _y, _z);
            // Get the rotation from scene frame to model
            sm::mat<float, 4> m_to_sc_rotn = model_to_scene.rotation_mat44();
            sm::mat<float, 4> hp_m;
            hp_m.translate (hp_scene);
            sm::mat<float, 4> coord_rotn = hp_m * m_to_sc_rotn * cam_to_model_rotn * cam_mv_y;

            return coord_rotn;
        }

        /*!
         * Using data about the model location for the camera found with find_triangle_hit, return a
         * camera position matrix (scene frame)
         *
         * \return a transform matrix that places a camera frame of reference at hp_scene, oriented
         * with its y-axis in line with the normal of the triangle at the hit point, and with its x
         * and z axes randomly oriented. The frame is set to hover hoverheight 'above' the triangle
         */
        sm::mat<float, 4> position_camera (const sm::vec<float>& hp_scene, const sm::mat<float, 4>& model_to_scene,
                                           const float hoverheight)
        {
            // Let's 'draw' the camera towards the model and then arrange its normal upwards wrt to the normal of the model.
            if (this->ti0 == std::numeric_limits<uint32_t>::max()) {
                std::cout << __func__ << ": No hit/triangle normal\n";
                return sm::mat<float, 4>{};
            }

            // Place the camera on the model, and orient it randomly in the 'model plane'
            // The camera frame always has y up. Choose a random vector in the plane for 'x'
            // and then set z from this random x and the triangle norm (y).
            sm::vec<float> rand_vec;
            rand_vec.randomize();
            sm::vec<sm::vec<float>, 3> tv_sf = this->triangle_vertices (this->ti0, model_to_scene);
            sm::vec<float> tn = this->triangle_normal (tv_sf);
            sm::vec<float> _x = rand_vec.cross (tn);
            _x.renormalize();
            sm::vec<float> _z = _x.cross (tn);

            return this->position_camera (hp_scene, model_to_scene, _x, tn, _z, hoverheight);
        }

        /*!
         * A version of position camera that aligns the camera direction (i.e. where it is looking - its 'forwards')
         * as closely as possible with the passed-in vector
         */
        sm::mat<float, 4> position_camera (const sm::vec<float>& hp_scene, const sm::mat<float, 4>& model_to_scene,
                                           const float hoverheight, const sm::vec<float>& fwds)
        {
            // Let's 'draw' the camera towards the model and then arrange its normal upwards wrt to the normal of the model.
            if (this->ti0 == std::numeric_limits<uint32_t>::max()) {
                std::cout << __func__ << ": No hit/triangle normal\n";
                return sm::mat<float, 4>{};
            }

            // Project fwds onto the plane tn
            sm::vec<sm::vec<float>, 3> tv_sf = this->triangle_vertices (this->ti0, model_to_scene);
            sm::vec<float> tn = this->triangle_normal (tv_sf);
            sm::vec<float> _z = sm::geometry::vector_plane_projection (tn, fwds);
            _z.renormalize();
            sm::vec<float> _x = -_z.cross (tn);
            _x.renormalize();

            return this->position_camera (hp_scene, model_to_scene, _x, tn, _z, hoverheight);
        }

        /*!
         * Return true: intersection found; false: no intersection found. Also returns outputs in
         * the args hov_sf and cam_to_surface.
         *
         * \param tv_sf triangle vertices as coordinates. Input, may be modified?
         *
         * \param tn0 Triangle normal of tv_sf. In/out?
         *
         * \param hov_sf An output. Hit location on triangle
         *
         * \param cam_to_surface An output. pose matrix for hov_sf (is hov_sf in the end just cam_to_surface.translation()?)
         *
         * \param cam_to_scene Transforms camera's ego-frame to scene coordinate frame
         *
         * \param model_to_scene Transforms landscape/model space to scene coordinate frame
         *
         * \param hoverheight Camera height-above-model-surface
         */
        bool find_first_intersection (sm::vec<sm::vec<float>, 3>& tv_sf,
                                      sm::vec<float>& tn0,
                                      sm::vec<float, 3>& hov_sf,
                                      sm::mat<float, 4>& cam_to_surface,
                                      const sm::mat<float, 4>& cam_to_scene,
                                      const sm::mat<float, 4>& model_to_scene,
                                      const float hoverheight)
        {
            constexpr bool debug_move = false;

            // Camera location, scene frame
            const sm::vec<float> camloc_sf = cam_to_scene.translation();

            // Does camloc_sf in dirn tn0 intersect the tv_sf triangle? This returns true if
            // camloc_sf is on the edge of the triangle or on a vertex. Assumes we're above the
            // model and within the length of tn0 of the surface.
            //
            // IF we're on an edge, then this intersection algo may disagree with
            // compute_crossing_location, which currently looks for crossing each of the three
            // boundaries and so expects that the start point is *within* the boundary.
            if constexpr (debug_move) {
                std::cout << "First ray_tri_intersection (raystart,-tn0): " << (camloc_sf + (tn0 / 2.0f)) << "," << -tn0 << std::endl;
            }
            bool isect = false;
            std::tie (isect, hov_sf) = sm::geometry::ray_tri_intersection<float> (tv_sf[0], tv_sf[1], tv_sf[2], camloc_sf + (tn0 / 2.0f), -tn0);

            // Use the detected location, hov_sf to compute the surface location of the camera - its 'hover location'
            cam_to_surface = cam_to_scene;
            cam_to_surface.pretranslate (hov_sf - camloc_sf); // This is now our init pose; the camera is now at the surface

            // Try double precision
            if (isect == false) {
                std::tie (isect, hov_sf) = sm::geometry::ray_tri_intersection<float, double> (tv_sf[0], tv_sf[1], tv_sf[2], camloc_sf + (tn0 / 2.0f), -tn0);
                if constexpr (debug_move) {
                    if (isect == false) {
                        std::cout << "No isect at start with ti0 using float OR double internally" << std::endl;
                    } else {
                        std::cout << "Intersection at start with ti0 using *double* internally" << std::endl;
                    }
                }
            }

            // If that didn't work, try the triangle *vertices*
            if (isect == false) {
                if constexpr (debug_move) { std::cout << "Try the triangle vertices...\n"; }
                uint32_t hi = this->ti0;
                uint32_t i = 0;
                do {
                    // We need to use the *vertex* normal for this test - the average of all the adjacent triangle normals!
                    sm::vec<float> vertex_n = this->find_vertex_normal (hi, model_to_scene);
                    vertex_n.renormalize();
                    if (sm::geometry::ray_point_intersection (tv_sf[i], camloc_sf + (vertex_n / 2.0f), -vertex_n)) {
                        if constexpr (debug_move) {
                            std::cout << "A VERTEX intersection is the start at " << tv_sf[i] << ", compare this with hov_sf = " << hov_sf << "\n";
                            // if start is vertex, need to check movement across all the triangle-neighbours of this vertex
                        }
                        hov_sf = tv_sf[i];
                        isect = true;
                    }
                    ++i;
                    hi = this->halfedge[hi].next;

                } while (hi != this->ti0);
            }

            if (isect == true) {
                if constexpr (debug_move) { std::cout << "First ray_tri_intersected. Start of move is IN triangle ti0\n"; }
            } else {
                if constexpr (debug_move) {
                    std::cout << "No intersection (at start) with triangle ti0, check neighbours (and maybe update ti0)" << std::endl;
                }
                // When very close to the boundary, ray_tri_intersection may fail. This triggers a
                // search for a neighbouring triangle which the camera may instead be hovering over
                // (this can occur when moving along an edge)
                uint32_t hi = this->ti0;
                do {
                    uint32_t twin = this->halfedge[hi].twin;
                    if (twin != std::numeric_limits<uint32_t>::max()) {
                        // Test to see if start location was inside this twin
                        sm::vec<sm::vec<float>, 3> tv_lf = this->triangle_vertices (twin, model_to_scene);
                        if (tv_lf[0][0] == std::numeric_limits<float>::max()) {
                            // This probably means we've attempted to go over a boundary. client code should turn around
                            throw std::runtime_error ("off-edge: twin is not a triangle");
                        }
                        sm::vec<float> _tn = this->triangle_normal (tv_lf);
                        auto [is, h] = sm::geometry::ray_tri_intersection<float> (tv_lf[0], tv_lf[1], tv_lf[2], camloc_sf + (_tn / 2.0f), -_tn);
                        if constexpr (debug_move) { std::cout << "Start of move " << (is ? "IS" : "is NOT") << " in twin " << twin << ", " << tv_lf << std::endl; }
                        if (is) {
                            if constexpr (debug_move) { std::cout << "CORRECT ti0 to the twin " << twin << std::endl; }
                            // We're in this neighbour, so update ti0/tn0 and mark isect true
                            this->ti0 = twin;
                            tv_sf = tv_lf;
                            tn0 = _tn;
                            isect = true;
                            // This requires a number of matrix recomputations:
                            hov_sf = h;
                            cam_to_surface = cam_to_scene;
                            cam_to_surface.pretranslate (hov_sf - camloc_sf); // This is our init pose, placed on the surface
                            break; // out of do-while
                        }
                    }
                    hi = this->halfedge[hi].next;
                } while (hi != this->ti0);

                if (isect == false) {
                    if constexpr (debug_move) { std::cout << "DBG No intersection (at start) with twins" << std::endl; }
                    // Final test to see if we're on boundary?
                    float closest_edge_d = sm::geometry::dist_to_tri_edge (tv_sf[0], tv_sf[1], tv_sf[2], camloc_sf - (tn0 * hoverheight));
                    if constexpr (debug_move) {
                        std::cout << "Closest distance from " << (camloc_sf - (tn0 * hoverheight)) << " to ti0 edge: " << closest_edge_d << std::endl;
                    }
                    constexpr float ced_thresh = std::numeric_limits<float>::epsilon() * 50; // FIX this
                    if (closest_edge_d < ced_thresh) {
                        // make tiny adjustment to camloc_sf so we ARE in the triangle? OR...
                        isect = true; // SAY we are, and proceed? <-- this if it works.
                    } else {
                        // If we still have no intersection, throw an exception
                        throw std::runtime_error ("No intersection (at start) with triangle or neighbours");
                    }
                } else {
                    if constexpr (debug_move) {
                        std::cout << "Found intersection (at start) with twin triangle " << this->ti0 << std::endl;
                    }
                }
            }

            return isect;
        }

        /*!
         * For the two triangles t0 and t1, find if t0 has a twin in t1 and return that halfedge.
         */
        uint32_t test_twin (const uint32_t t0, const uint32_t t1)
        {
            uint32_t rtn = std::numeric_limits<uint32_t>::max();
            uint32_t hi0 = t0;
            do {
                uint32_t hi1 = t1;
                do {
                    if (this->halfedge[hi1].twin == hi0) { rtn = hi0; }
                    hi1 = this->halfedge[hi1].next;
                } while (hi1 != t1 && rtn == std::numeric_limits<uint32_t>::max());
                hi0 = this->halfedge[hi0].next;
            } while (hi0 != t0 && rtn == std::numeric_limits<uint32_t>::max());
            return rtn;
        }

        /*!
         * Find any vertex in t0 that shares a vertex with t1. Return the halfedge in t0 for which
         * that vertex is vi[0]
         */
        uint32_t test_vertex_twin (const uint32_t t0, const uint32_t t1)
        {
            uint32_t rtn = std::numeric_limits<uint32_t>::max();
            uint32_t hi0 = t0;
            do {
                uint32_t hi1 = t1;
                do {
                    if (this->halfedge[hi0].vi[0] == this->halfedge[hi1].vi[0]
                        || this->halfedge[hi0].vi[0] == this->halfedge[hi1].vi[1]) { rtn = hi0; }
                    hi1 = this->halfedge[hi1].next;
                } while (hi1 != t1 && rtn == std::numeric_limits<uint32_t>::max());
                hi0 = this->halfedge[hi0].next;
            } while (hi0 != t0 && rtn == std::numeric_limits<uint32_t>::max());
            return rtn;
        }

        /*!
         * Did the movement pass through the given neighbour triangle, which is probably a
         * neighbour-over-a-vertex?
         *
         * \param nb The halfedge index of the 'to' or neighbour triangle.
         *
         * \param tn_frm The normal of the 'from' triangle.
         *
         * \param mv is the movement, assumed to start in the boundary of the from triangle, and to
         * lie in the plane of the from triangle
         *
         * \param start is the start the movement in the new triangle (and the end in the 'from'
         * triangle). i.e. it's on the border between the two.
         *
         * \param model_to_scene Transform required to obtain triangle vertices in scene frame
         *
         * \return tuple containing true/false 'was a crossing found?'; the rotation axis and the rest
         * of the movement, reoriented to the neighbour
         */
        std::tuple<bool, sm::vec<float>, sm::vec<float>>
        detect_movement_in_neighbour (const uint32_t nb,
                                      const sm::vec<float>& tn_frm,
                                      const sm::vec<float>& mv,
                                      const sm::vec<float>& start,
                                      const sm::mat<float, 4>& model_to_scene)
        {
            constexpr bool debug_move = false;

            bool found = false;

            // Does mv_rest pass through this neighbour?
            sm::vec<sm::vec<float>, 3> tv_nb = this->triangle_vertices (nb, model_to_scene);

            // Have to reorient to each neighbour to test
            auto _tn = this->triangle_normal (tv_nb);
            if constexpr (debug_move) {
                std::cout << __func__ << " called:\n";
                std::cout << "Test candidate movement in nb: " << nb << " " << tv_nb;
                std::cout << "\n norm: " << tv_nb.mean() << "," << (_tn * 0.05f) << "\n";
                std::cout << "start/mv: " << start << "," << mv << "\n";
            }

            sm::vec<float> r_axis = tn_frm.cross (_tn);
            if constexpr (debug_move) { std::cout << "Rotation axis: " << start << "," << r_axis << std::endl; }
            r_axis.renormalize();
            // Compute the reorientation due to the requested movement into this neighbour
            float rotn_angle = tn_frm.angle (_tn, r_axis);
            // If tn0 and _tn are identical, then rotn_angle will be NaN, but in that case we want no rotation
            if (std::isnan (rotn_angle)) { rotn_angle = 0.0f; }
            sm::mat<float, 4> reorient_model; // reorientation transformation in sf between these two triangles
            reorient_model.rotate (r_axis, rotn_angle);
            // The 'new' mv_rest
            sm::vec<float> mv_reoriented = (reorient_model * mv).less_one_dim();
            const float rl = mv_reoriented.length();
            if (std::isnan (rl)) { return {found, r_axis, mv_reoriented}; }
            // Now test points along mv_rest to be in
            if constexpr (debug_move) { std::cout << "candidate mv_reoriented is " << start << "," << mv_reoriented << std::endl; }
            // The far edge will be the next edge
            uint32_t faredge = this->halfedge[nb].next;
            const sm::vec<float> fes = (model_to_scene * this->vertex[this->halfedge[nb].vi[1]].p).less_one_dim();
            const sm::vec<float> fee = (model_to_scene * this->vertex[this->halfedge[faredge].vi[1]].p).less_one_dim();
            if constexpr (debug_move) { std::cout << "Far edge is " << fes << "," << (fee - fes) << std::endl; }
            partial_movement pm = find_edge_crossing (fes, fee, _tn, start, mv_reoriented);
            if (pm.flags.test (pm_fl::crossed) == true) {
                if constexpr (debug_move) { std::cout << "Return 'found'; cross point over faredge (" << faredge << ") of nb " << nb << "\n"; }
                found = true;
            } else {
                if constexpr (debug_move) {
                    std::cout << "NO cross point with faredge (" << faredge << ")  of " << nb << ", did we land in nb " << nb << "?\n";
                }
                // No crossing, did we land in the triangle?
                auto [is, h] = sm::geometry::ray_tri_intersection<float, double> (tv_nb[0], tv_nb[1], tv_nb[2], start + mv_reoriented + (_tn / 2.0f), -_tn);
                if (is) { // then we DID land in this neighbour tri
                    if constexpr (debug_move) { std::cout << "Return found; landed IN nb " << nb << "\n"; }
                    found = true;
                } else {
                    if constexpr (debug_move) { std::cout << "NO ray_tri_intersection in nb " << nb << "\n"; }
                }
            }

            return {found, r_axis, mv_reoriented};
        }

        /*!
         * By searching all over-the-vertex neighbours (i.e. neighbours over all 3 vertices), find a
         * boundary crossing.
         *
         * Sub-calls detect_movement_in_neighbour.
         */
        crossing_data find_nearest_boundary_crossing (const sm::vec<float>& hov_sf,
                                                      const sm::vec<float>& mv_inplane,
                                                      const sm::mat<float, 4>& model_to_scene)
        {
            constexpr bool debug_move = false;
            crossing_data cd;

            // Get the 'from' triangle normal
            sm::vec<sm::vec<float>, 3> tv_frm = this->triangle_vertices (this->ti0, model_to_scene);
            sm::vec<float> tn_frm = this->triangle_normal (tv_frm);
            if constexpr (debug_move) {
                std::cout << "FROM triangle: " << tv_frm << std::endl;
                std::cout << "FROM normal: " << tv_frm.mean() << "," << tn_frm << std::endl;
            }

            // for each vertex in ti0...
            uint32_t _ti = this->ti0;

            std::set<std::set<uint32_t>> tested;
            tested.insert (this->triangle_indices (this->ti0)); // never test ti0

            for (uint32_t i = 0; i < 3; ++i) {

                const sm::vec<float> vtx_loc = this->edge_start (_ti, model_to_scene);
                const sm::vec<float> to_vtx = vtx_loc - hov_sf;
                // to_vtx.cross (mv_inplane) should be very short
                auto cp = to_vtx.cross (mv_inplane);

                if constexpr (debug_move) {
                    std::cout << "i = " << i << ", cp.length(): " << cp.length()
                              << " and to_vtx.dot (mv_inplane) / to_vtx.length() = "
                              << (to_vtx.dot (mv_inplane) / to_vtx.length()) << std::endl;
                }

                // Check to see if vtx_loc is in the direction mv_inplane...
                if (to_vtx.dot (mv_inplane) < 0.0f) {
                    // Then mv_inplane points away from this vtx
                     _ti = this->halfedge[_ti].next;
                     continue;
                }

                // Get neighbours of _ti
                auto nbs = this->find_neighbours (_ti);
                const sm::vec<float> mv_rest = mv_inplane - to_vtx;

                // For each neighbour, test to see if start location was inside a neighbour
                for (auto nb : nbs) {
                    std::set<uint32_t> ind = this->triangle_indices (nb);
                    if (tested.count (ind) > 0) { continue; } // Don't test already-tested
                    tested.insert (ind);
                    auto[found_in, r_axis, _mv_rest] = detect_movement_in_neighbour (nb, tn_frm, mv_rest, vtx_loc, model_to_scene);
                    if (found_in) {
                        cd.crossed = _ti; // the vtx halfedge
                        cd.into = nb;
                        cd.tri_edge = r_axis;
                        cd.mv_rest = _mv_rest;
                        cd.pm.mv = to_vtx;
                        cd.pm.end = vtx_loc;
                        cd.pm.flags.set (pm_fl::crossed);
                        //cd.pm.flags.set (pm_fl::near_vertex_0); // of _ti. If I set this, then it triggers find_triangle_over...

                        break; // also need to break out of outer loop
                    }
                }
                if (cd.into != std::numeric_limits<uint32_t>::max()) { break; }
                _ti = this->halfedge[_ti].next;
            }

            // Did we get a result?
            if (cd.into != std::numeric_limits<uint32_t>::max()) {
                if constexpr (debug_move) { std::cout << "detected a neighbour with detect_movement_in_neighbour()\n"; }
                // cd should have been set up...
            } // else cd.into is max, that means we're off-edge.

            return cd;
        }

        /*!
         * Find which triangle we crossed into over the vertex. Update cd with new
         * tri_edge. crossing_data required here for initial halfedge and also to be populated.
         *
         * The triangle into which we cross is placed into cd.into
         *
         * Sub-calls detect_movement_in_neighbour.
         */
        void find_triangle_over_vertex (crossing_data& cd,
                                        const sm::vec<float>& mv_inplane,
                                        const sm::mat<float, 4>& model_to_scene)
        {
            constexpr bool debug_move = false;

            if (cd.pm.flags.any_of ({pm_fl::near_vertex_0, pm_fl::near_vertex_1}) == false) {
                if constexpr (debug_move) { std::cout << "Crossing cd does not go near a vertex, returning\n"; }
                return;
            }

            if constexpr (debug_move) {
                std::cout << "Finding triangle after crossing halfedge " << cd.crossed << " near vertex "
                          << (cd.pm.flags.test (pm_fl::near_vertex_0) ? "0" : "1") << "\n";
            }

            uint32_t cv = cd.pm.flags.test (pm_fl::near_vertex_0) ? cd.crossed : this->halfedge[cd.crossed].next;
            if constexpr (debug_move) { std::cout << "Vertex is represented by halfedge " << cv << std::endl; }

            std::vector<uint32_t> nbs = find_neighbours (cv);

            // Get the 'from' triangle normal
            sm::vec<sm::vec<float>, 3> tv_frm = this->triangle_vertices (cv, model_to_scene);
            sm::vec<float> tn_frm = this->triangle_normal (tv_frm);
            if constexpr (debug_move) {
                std::cout << "FROM triangle: " << tv_frm << std::endl;
                std::cout << "FROM normal: " << tv_frm.mean() << "," << tn_frm << std::endl;
            }

            const sm::vec<float> mv_rest = mv_inplane - cd.pm.mv;
            const sm::vec<float> end_at_border = cd.pm.end;
            for (auto nb : nbs) {
                if (nb == cv) { continue; } // Don't test crossing into self
                auto[found_in, r_axis, _mv_rest] = detect_movement_in_neighbour (nb, tn_frm, mv_rest, end_at_border, model_to_scene);

                if (found_in) {
                    if constexpr (debug_move) { std::cout << "Found movement into neighbour " << nb << "\n"; }
                    cd.into = nb;
                    cd.crossed = cv;
                    cd.tri_edge = r_axis;
                    cd.mv_rest = _mv_rest;
                    // Don't update cd.pm.mv/cd.pm.end here, they're already set
                    cd.pm.flags.set (pm_fl::crossed);
                    break;
                }
            }
        }

        /*!
         * Move across triangles, until at the end of mv_inplane.
         *
         * This is the main subroutine of compute_mesh_movement.
         *
         * On each loop, we move either to the end of the movement, if it is within the current
         * triangle (ti0) OR we move to the boundary that we cross, and adjust mv_inplane.
         */
        void traverse_triangles (sm::vec<float>& mv_inplane,
                                 sm::vec<sm::vec<float>, 3>& tv_sf,
                                 sm::vec<float>& tn0,
                                 sm::vec<float, 3>& hov_sf,
                                 sm::mat<float, 4>& cam_to_surface,
                                 const sm::mat<float, 4>& model_to_scene)
        {
            constexpr bool debug_move = false;
            // In case we throw off-edge, we need to restore ti0's state
            const uint32_t ti0_save = this->ti0;
            bool done = false;
            // Now loop while our path may traverse one or more triangles
            while (!done) {

                if constexpr (debug_move) {
                    std::cout << "\nWHILE LOOP, TRAVERSING TRIANGLES\n"
                              << "ti0 = (" << this->ti0 << ") = " << tv_sf << "\n"
                              << "mv_inplane: " << hov_sf << "," << mv_inplane << "\n"
                              << "tn0 = " << tn0 << ")\n";
                }

                // zero length mv_inplane should have been tested before calling this function
                if (mv_inplane.length() == 0) { throw std::runtime_error ("Zero length mv_inplane"); }
                if (mv_inplane.has_nan()) { throw std::runtime_error ("mv_inplane contained NaN"); }

                // 1. Apply the fast edge crossing algorithm
                crossing_data cd = this->compute_crossing_location (tv_sf, this->ti0, hov_sf, mv_inplane);

                // If it failed to find a cross point, then we test inside the triangle and neighbours
                // (Also if we moved colinearly along edge past a vertex)
                if ((cd.pm.flags.test (pm_fl::colinear) == true && cd.pm.flags.test (pm_fl::crossed) == true)
                    || (cd.pm.flags.test (pm_fl::colinear) == false && cd.pm.flags.test (pm_fl::crossed) == false)) {
                    // Now test if our movement stays within the triangle
                    sm::vec<float> endmv = (cam_to_surface * sm::vec<float>{}).less_one_dim() + mv_inplane;
                    if constexpr (debug_move) {
                        std::cout << "endmv intersection with ti0 " << ti0 << " test vector " << (endmv + (tn0 / 2.0f)) << "," << -tn0 << " with tri " << tv_sf << std::endl;
                    }
                    auto [isect, isectpoint] = sm::geometry::ray_tri_intersection<float, float> (tv_sf[0], tv_sf[1], tv_sf[2], endmv + (tn0 / 2.0f), -tn0);
                    if constexpr (debug_move) { std::cout << "End lands in ? " << (isect ? "Y" : "N") << std::endl; }
                    if (isect == false) {
                        // Didn't find edge crossing or that the end point is within ti0, so now search neighbours for an end point or boundary crossing.
                        cd = find_nearest_boundary_crossing (hov_sf, mv_inplane, model_to_scene);
                        if (cd.into == std::numeric_limits<uint32_t>::max()) {
                            this->ti0 = ti0_save;
                            throw std::runtime_error ("off-edge: The movement went off the edge of the model over a vertex");
                        }
                    }
                } // else We HAVE a crossing of some sort.

                // crossing_data gives us info about if there is NO cross point in the partial mv crossed flag
                if (cd.pm.flags.test (pm_fl::colinear) == true && cd.pm.flags.test (pm_fl::crossed) == false) {
                    if constexpr (debug_move) { std::cout << "A: Movement stays inside triangle ti0 (colinear within boundary\n"; }
                    cam_to_surface.pretranslate (mv_inplane);
                    done = true;
                } else if (cd.pm.flags.test (pm_fl::crossed) == false) { // move a bit, shorten mv_inplane
                    // mv_inplane moved camera inside triangle.
                    if constexpr (debug_move) { std::cout << "A: Movement stays inside triangle ti0\n"; }
                    cam_to_surface.pretranslate (mv_inplane);
                    done = true;
                } else {

                    if (cd.pm.flags.any_of ({pm_fl::near_vertex_0, pm_fl::near_vertex_1})) {
                        if constexpr (debug_move) {
                            if (cd.pm.flags.test (pm_fl::near_vertex_0)) {
                                std::cout << "B: Crossed near vertex 0 of crossed edge\n";
                            } else if (cd.pm.flags.test (pm_fl::near_vertex_1)) {
                                std::cout << "B: Crossed near vertex 1 of crossed edge\n";
                            }
                        }
                        // The right triangle to reorient onto may not be the twin across the crossed edge
                        // Check all neighbours of the crossed vertex to find out if our path travels through.
                        find_triangle_over_vertex (cd, mv_inplane, model_to_scene); // This could set cd itself

                    } else {
                        // If compute_crossing_location found boundary, then new triangle is the twin of the crossed edge.
                        if (cd.into == std::numeric_limits<uint32_t>::max()) {
                            cd.into = this->halfedge[cd.crossed].twin;
                        } // else: BUT if find_nearest_boundary_crossing found boundary, then new triangle _ti has been set into cd.into

                        if constexpr (debug_move) {
                            std::cout << "B: Crossed a boundary halfedge " << cd.crossed << " which twins/crosses to: " << cd.into << std::endl;
                        }
                    }

                    if (cd.into == std::numeric_limits<uint32_t>::max()) {
                        // We probably went off the edge of our navigation model mesh
                        this->ti0 = ti0_save;
                        throw std::runtime_error ("off-edge: The movement went off the edge of the model");
                    }

                    // Re-orient onto the new triangle
                    sm::vec<sm::vec<float>, 3> newtv_sf = this->triangle_vertices (cd.into, model_to_scene);
                    if (newtv_sf[0][0] == std::numeric_limits<float>::max()) {
                        this->ti0 = ti0_save;
                        throw std::runtime_error ("off-edge: The movement went off the edge of the model");
                        continue;
                    } else {

                        sm::vec<float> _tn = this->triangle_normal (newtv_sf);
                        if constexpr (debug_move) {
                            std::cout << "RE-ORIENT to cd.into: " << cd.into << " " << newtv_sf << " norm: " << newtv_sf.mean() << "," << _tn << "\n";
                        }
                        // Compute the reorientation due to the requested movement.
                        float rotn_angle = tn0.angle (_tn, cd.tri_edge);
                        // If tn0 and _tn are identical, then rotn_angle will be NaN, but in that case we want no rotation
                        if (std::isnan (rotn_angle)) { rotn_angle = 0.0f; }
                        sm::mat<float, 4> reorient_model; // reorientation transformation in sf
                        // No good if cd.tri_edge is (0,0,0)!
                        reorient_model.rotate (cd.tri_edge, rotn_angle);

                        // cd.mv_rest may have been set, or it may need setting
                        if (cd.mv_rest[0] == std::numeric_limits<float>::max()) {
                            if constexpr (debug_move) { std::cout << "cd.mv_rest was unset; setting it from mv_inplane and cd.pm.mv\n"; }
                            cd.mv_rest = (reorient_model * (mv_inplane - cd.pm.mv)).less_one_dim();
                        } else {
                            if constexpr (debug_move) { std::cout << "cd.mv_rest was set up already\n"; }
                        }
                        if constexpr (debug_move) { std::cout << "mv_rest: " << cd.pm.end << "," << cd.mv_rest << std::endl; }

                        const float rl = cd.mv_rest.length();
                        if (std::isnan (rl)) {
                            if constexpr (debug_move) {
                                std::cout << "Got NaN in mv_rest. mv_inplane: " << mv_inplane << ", cd.pm.mv: " << cd.pm.mv << "reorient_model:" << std::endl;
                                std::cout << "reorient_model created from tri_edge " << cd.tri_edge << " and rotn_angle " << rotn_angle << std::endl;
                                std::cout << reorient_model << std::endl;
                            }
                            throw std::runtime_error ("NaN in mv_rest");
                        }
                        reorient_model.pretranslate (hov_sf + cd.pm.mv + cd.mv_rest);
                        reorient_model.translate (-hov_sf); // r_t_to + r_t1 = -(hov_sf + cd.pm.mv) + cd.pm.mv = -hov_sf

                        if (rl <= std::numeric_limits<float>::epsilon()) {
                            // The first movement to edge completed the movement. We actually landed ON the edge.
                            cam_to_surface = reorient_model * cam_to_surface;
                            done = true;
                        } else {
                            // There's additional movement to complete.
                            if constexpr (debug_move) { std::cout << "cd.mv_rest length is " << rl << std::endl; }
                            // Loop To Next. Final movement should be exclusively within a triangle.
                            reorient_model.pretranslate (-cd.mv_rest);
                            cam_to_surface = reorient_model * cam_to_surface;
                            hov_sf = cd.pm.end; // crossing data planned movement end
                            // Also update planned move, which is now shorter and in a new direction
                            tv_sf = newtv_sf;
                            mv_inplane = cd.mv_rest;
                        }

                        this->ti0 = cd.into;
                        tn0 = _tn;
                    }

                } // compute_crossing_location if/else

            } // triangle traversing while loop
        }

        /*!
         * Compute a movement over this navigation mesh.
         *
         * We convert the triangle vertices from the model frame to the scene frame before computing
         * reorientations, so that non-uniform scalings in the model do not fox us.
         *
         * \param mv_camframe A movement vector in the camera's own frame of reference (an
         * ego-motion)
         *
         * \param cam_to_scene The transformation matrix to bring the camera coordinates to the
         * scene frame
         *
         * \param model_to_scene The transformation matrix to convert model coordinates to the scene
         * frame
         *
         * \param hoverheight The height at which we want the camera to 'hover' over the
         * model/landscape surface
         *
         * \return The re-positioned camera transform matrix
         */
        sm::mat<float, 4> compute_mesh_movement (const sm::vec<float>& mv_camframe,
                                                 const sm::mat<float, 4>& cam_to_scene,
                                                 const sm::mat<float, 4>& model_to_scene,
                                                 const float hoverheight)
        {
            constexpr bool debug_move = false;

            // Convert indices to vertices for triangle ti0, converting to the scene frame
            sm::vec<sm::vec<float>, 3> tv_sf = this->triangle_vertices (this->ti0, model_to_scene);
            if (tv_sf[0][0] == std::numeric_limits<float>::max()) { throw std::runtime_error ("ti0 is not a triangle"); }

            // Compute the triangle normal in the scene frame
            sm::vec<float> tn0 = this->triangle_normal (tv_sf);

            if constexpr (debug_move) {
                std::cout << "\n# compute_mesh_movement:\n"
                          << "\nti0: " << this->ti0
                          << "\nti0 (sf): " << tv_sf << "\nnormal " << tv_sf.mean() << "," << tn0
                          << "\nmovement (camframe): " << mv_camframe
                          << "\nInitial camera location: " << cam_to_scene.translation() << "\n\n";
            }

            // Find the intersection point with the triangle ti0:
            sm::vec<float, 3> hov_sf = {};
            sm::mat<float, 4> cam_to_surface;
            bool isect = this->find_first_intersection (tv_sf, tn0, hov_sf, cam_to_surface,
                                                        cam_to_scene, model_to_scene, hoverheight);
            if (!isect) { throw std::runtime_error ("No initial intersection found"); }

            // Find component of movement that is in the current triangle plane (in the scene frame of reference)
            sm::vec<float> mv_sf = (cam_to_scene * mv_camframe).less_one_dim() - cam_to_scene.translation();
            sm::vec<float> mv_orthog = tn0 * (mv_sf.dot (tn0) / (tn0.dot (tn0)));
            sm::vec<float> mv_inplane = mv_sf - mv_orthog; // scene frame, a relative movement
            if (mv_inplane.length() == 0.0f) {
                if constexpr (debug_move) { std::cout << "No movement, so return unchanged camera viewmatrix\n"; }
                return cam_to_scene;
            }

            // Now traverse those triangles! The output of this function is cam_to_surface
            this->traverse_triangles (mv_inplane, tv_sf, tn0, hov_sf, cam_to_surface, model_to_scene);

            // Raise cam_to_surface up by hoverheight and then return
            cam_to_surface.pretranslate (hoverheight * tn0);
            if constexpr (debug_move) {
                std::cout << "looping mv_inplanes completed. Final camloc_sf: " << cam_to_surface.translation() << std::endl;
            }
            return cam_to_surface;

        } // compute_mesh_movement

    }; // struct NavMesh

} // namespace
