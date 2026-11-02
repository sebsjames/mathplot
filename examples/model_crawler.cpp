/*
 * Move one model (mplot::CoordArrows) around another (mplot::GeodesicVisual), as if it were
 * crawling over it. Demonstrates mplot::NavMesh, which allows you to move over a triangular
 * landscape model, following the exact contour defined by the landscape's mesh.
 */

#include <cstdint>
#include <iostream>
#include <fstream>
#include <cmath>
#include <array>
#include <stdexcept>
#include <string>
#include <memory>

import sm.random;
import sm.vec;
import sm.mat;
import sm.vvec;

import mplot.gl.version;

import mplot.visual;
import mplot.colourmap;
import mplot.coordarrows;
import mplot.instancedscattervisual;
import mplot.geodesicvisual;

constexpr std::int32_t glver = mplot::gl::version_4_3;

int main (int argc, char** argv)
{
    int rtn = -1;

    mplot::Visual<glver> v(1024, 768, "Crawling a surface with NavMesh features");
    v.rotateAboutNearest (true);
    v.backgroundBlack();

    // How big to make the sphere?
    constexpr float radius = 2.0f;
    // How many iterations for the geodesic? Try 2, 3 and 4
    int geo_itrns = 4;
    if (argc > 1) {
        geo_itrns = std::atoi (argv[1]);
        if (geo_itrns > 6) {
            std::cout << "Warning: GeodesicVisual takes a long time to build for iterations > 6!" << std::endl;
        }
    }
    // How high to hover the arrows
    constexpr float hoverheight = 0.05f;
    // Model locations within the scene
    //sm::vec<float, 3> arrows_loc = { 0.01f, radius + 1.5f * hoverheight, 0.2f };
    sm::vec<float, 3> arrows_loc = { 0.0f, radius + 1.5f * hoverheight, 0.0f };
    sm::vec<float, 3> sphere_loc = {};

    // A CoordArrows is our "crawling" agent
    auto ca = std::make_unique<mplot::CoordArrows<glver>> (arrows_loc);
    ca->set_parent (v.get_id());
    ca->finalize();
    [[maybe_unused]] auto cap = v.addVisualModel (ca);

    // Breadcrumb trail
    uint64_t move_counter = 0u;
    uint64_t max_bc = 1000;
    sm::vvec<sm::vec<float, 3>> sv_points = {};
    sm::vvec<float> sv_data = {};
    auto isv = std::make_unique<mplot::InstancedScatterVisual<glver>> (sphere_loc);
    isv->set_parent (v.get_id());
    isv->max_instances = max_bc;
    isv->radiusFixed = 0.01f;
    isv->finalize();
    auto isvp = v.addVisualModel (isv);

    // A sphere, approximated by an icosahedral geodesic, is our landscape
    mplot::ColourMap<float> cm (mplot::ColourMapType::Jet);
    auto cl = cm.convert (0.5f);
    auto gv = std::make_unique<mplot::GeodesicVisual<float, glver>> (sphere_loc, radius);
    gv->set_parent (v.get_id());
    gv->iterations = geo_itrns;
    std::string lbl = "GeodesicVisual with computed NavMesh";
    gv->addLabel (lbl, {0, -(radius + 0.1f), 0}, mplot::TextFeatures (0.06f));
    gv->cm.setType (mplot::ColourMapType::CET_L06);
    gv->colour_bb = cl;
    gv->finalize();
    auto gvp = v.addVisualModel (gv);
    // re-colour sphere with sequential colouring after construction
    gvp->data.linspace (0.0f, 1.0f, gvp->data.size());
    gvp->reinitColours();
    // Make the navmesh for the geodesic, this doesn't occur automatically and has to come after finalize()
    gvp->make_navmesh();

    // We're going to move the coordinate arrows forwards (along its z-axis) on each step
    float move_step = 0.01f;
    sm::vec<float> mv_ca = sm::vec<float>::uz() * move_step;

    // We'll also rotate by a small amount on each step, drawn from a Von Mises distribution
    constexpr float mu = 0.0f;
    constexpr float kappa = 3.0f;
    sm::rand_vonmises<float> rvm (mu, kappa);

    // The viewmatrices have to be passed to mplot::NavMesh::compute_mesh_movement
    sm::mat<float, 4> ca_view = cap->getViewMatrix();
    sm::mat<float, 4> sph_view = gvp->getViewMatrix();

    // Find the triangle that we're initially located above with
    // mplot::NavMesh::find_triangle_hit. This updates internal state in NavMesh. It could be
    // executed automatically in compute_mesh_movement
    auto[hp_scene, ti0] = gvp->navmesh->find_triangle_hit (ca_view, sph_view);
    std::cout << "Find hit finds hit point " << hp_scene << " with ti0 halfedge: " << ti0 << std::endl;

    cap->setHide (true);

    while (!v.readyToFinish()) {

        // Render the scene. Make sure this happens before first call to set_instance_data
        v.render();

        // Wait .018 s and also poll for mouse/keyboard events
        v.waitevents (0.018);

        // Compute a new movement over the landscape mesh (the sphere)
        try {
            // rotate ca_view each time by a little (randomly)
            ca_view.rotate (sm::vec<>::uy(), rvm.get());
            ca_view = gvp->navmesh->compute_mesh_movement (mv_ca, ca_view, sph_view, hoverheight);
        } catch (std::exception& e) {
            std::cout << "Exception navigating mesh at movement count " << move_counter << ": " << e.what() << std::endl;
        }

        // Update the viewmatrix of the coord arrows, setting its position within the scene
        cap->setViewMatrix (ca_view);

        // Compute the new location
        arrows_loc = (ca_view * sm::vec<float>{}).less_one_dim();

        // We're adding and rebuilding the not-very-optimized ScatterVisual, so if move_max is too
        // high, the program will slow down (too many tiny spheres!)
        move_counter++;
        // This should be the right place to update breadcrumbs
        if (sv_points.size() < max_bc) {
            sv_points.push_back (arrows_loc);
            sv_data.push_back (0.0f); // dummy for now
        } else {
            sv_points[move_counter % max_bc] = arrows_loc;
        }
        isvp->set_instance_data (sv_points);
    }

    v.keepOpen();

    return rtn;
}
