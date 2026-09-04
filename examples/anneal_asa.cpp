/*
 * Test Adaptive Simulated Annealing on a 2D objective function, visualizing the
 * progress of the algorithm.
 */

#include <cstdint>
#include <iostream>
#include <string>
#include <memory>
#include <cmath>
#include <vector>
#include <list>

import sm.vvec;
import sm.vec;
import sm.hex;
import sm.hexgrid;
import sm.algo.hexgrid;

import sm.anneal;
import sm.config;

#ifdef VISUALISE
import mplot.visual;
import mplot.visualdatamodel;
import mplot.hexgridvisual;
import mplot.polygonvisual;
import mplot.graphvisual;
#endif

// Choose double or float for the precision used in the Anneal algorithm
typedef double F;

// A global hexgrid for the locations of the objective function
std::unique_ptr<sm::hexgrid<F>> hg;
// And a vvec to be the data
sm::vvec<F> obj_f;

// Set up an objective function. Creates hg and populates obj_f. Note objective has
// discrete values.
void setup_objective();

// Alternative objective function
void setup_objective_boha();

// Return values of the objective function. Params contains coordinates into the
// hexgrid. Values from obj_f are returned.
F objective (const sm::vvec<F>& params);
F objective_boha (const sm::vvec<F>& params);
F objective_hg (const sm::vvec<F>& params);

int main (int argc, char** argv)
{
#ifdef USE_BOHACHEVSKY_FUNCTION
    setup_objective_boha();
#else
    setup_objective();
#endif

    // Here, our search space is 2D
    sm::vvec<F> p = { 0.45, 0.45};
    // These ranges should fall within the hexagonal domain
    sm::vvec<sm::vec<F,2>> p_rng = {{ {-0.3, 0.3}, {-0.3, 0.3} }};

    // Set up the anneal algorithm object
    sm::anneal<F> anneal(p, p_rng);
    // There are defaults hardcoded in anneal, but these work for the cost function here:
    anneal.temperature_ratio_scale = F{1e-2};
    anneal.temperature_anneal_scale = F{200};
    anneal.cost_parameter_scale_ratio = F{3};
    anneal.acc_gen_reanneal_ratio = F{1e-6};
    anneal.delta_param = F{0.01};
    anneal.objective_repeat_precision = F{1e-6};
    anneal.f_x_best_repeat_max = 15;
    anneal.reanneal_after_steps = 100;
    anneal.exit_at_T_f = false; // If true, algo will run faster, but error will likely be non-zero
#ifndef VISUALISE
    anneal.display_temperatures = false;
    anneal.display_reanneal = false;
#endif
    // Optionally, modify ASA parameters from a JSON config specified on the command line.
    if (argc > 1) {
        sm::config conf(argv[1]);
        if (conf.ready) {
            anneal.temperature_ratio_scale = (F)conf.get<double> ("temperature_ratio_scale", 1e-2);
            anneal.temperature_anneal_scale = (F)conf.get<double> ("temperature_anneal_scale", 200.0);
            anneal.cost_parameter_scale_ratio = (F)conf.get<double> ("cost_parameter_scale_ratio", 3.0);
            anneal.acc_gen_reanneal_ratio = (F)conf.get<double> ("acc_gen_reanneal_ratio", 1e-6);
            anneal.delta_param = (F)conf.get<double> ("delta_param", 0.01);
            anneal.objective_repeat_precision = (F)conf.get<double> ("objective_repeat_precision", 1e-6);
            anneal.f_x_best_repeat_max = conf.get<std::uint32_t> ("f_x_best_repeat_max", 15);
            anneal.reanneal_after_steps = conf.get<std::uint32_t> ("reanneal_after_steps", 100);
        } else {
            std::cerr << "Failed to open JSON config in '" << argv[1]
                      << "', continuing with default ASA parameters.\n";
        }
    }
    anneal.init();

#ifdef VISUALISE
    // Set up the visualisation
    mplot::Visual v (1920, 1080, "Adaptive Simulated Annealing Example");
    v.zNear = 0.001;
    v.setSceneTransZ (-3.0f);
    v.lightingEffects (true);

    sm::vec<float, 3> offset = { 0.0, 0.0, 0.0 };
    auto hgv = std::make_unique<mplot::HexGridVisual<F>>(hg.get(), offset);
    hgv->set_parent (v.get_id());
    hgv->setScalarData (&obj_f);
#ifdef USE_BOHACHEVSKY_FUNCTION
    hgv->addLabel ("Objective: See Bohachevsky et al.", { -0.5f, -0.75f, -0.1f });
#else
    hgv->addLabel ("Objective: 2 Gaussians and some noise", { -0.5f, -0.75f, -0.1f });
#endif
    hgv->finalize();
    v.addVisualModel (hgv);

    sm::vec<float, 3> polypos = { static_cast<float>(p[0]), static_cast<float>(p[1]), 0.0f };

    // One object for the 'candidate' position
    std::array<float, 3> col = { 0, 1, 0 };
    auto cand_up = std::make_unique<mplot::PolygonVisual<>>(offset, polypos, sm::vec<float>({1,0,0}), 0.005f, 0.4f, col, 20);
    cand_up->set_parent (v.get_id());
    cand_up->finalize();
    // A second object for the 'best' position
    col = { 1, 0, 0 };
    auto best_up = std::make_unique<mplot::PolygonVisual<>>(offset, polypos, sm::vec<float>({1,0,0}), 0.001f, 0.8f, col, 10);
    best_up->set_parent (v.get_id());
    best_up->finalize();

    // A third object for the currently accepted position
    col = { 1, 0, 0.7f };
    auto curr_up = std::make_unique<mplot::PolygonVisual<>> (offset, polypos, sm::vec<float>({1,0,0}), 0.005f, 0.6f, col, 20);
    curr_up->set_parent (v.get_id());
    curr_up->finalize();

    // Fourth object marks the starting place
    col = { .5f, .5f, .5f };
    polypos[2] = objective(p);
    auto sp = std::make_unique<mplot::PolygonVisual<>> (offset, polypos, sm::vec<float>({1,0,0}), 0.005f, 0.6f, col, 20);
    sp->set_parent (v.get_id());
    sp->finalize();

    auto candp = v.addVisualModel (cand_up);
    auto bestp = v.addVisualModel (best_up);
    auto currp = v.addVisualModel (curr_up);
    v.addVisualModel (sp);

    // Add a graph to track T_i and T_cost
    sm::vec<float> spatOff = {1.2f, -0.5f, 0.0f};
    auto graph1 = std::make_unique<mplot::GraphVisual<F>> (spatOff);
    graph1->set_parent (v.get_id());
    graph1->twodimensional (true);
    graph1->setlimits (0, 1000, -10, 1);
    graph1->policy = mplot::stylepolicy::lines;
    graph1->ylabel = "log(T)";
    graph1->xlabel = "Anneal time";
    graph1->prepdata ("Tparam");
    graph1->prepdata ("Tcost");
    graph1->finalize();
    auto graph1p = v.addVisualModel (graph1);

    spatOff[0] += 1.1f;
    auto graph2 = std::make_unique<mplot::GraphVisual<F>> (spatOff);
    graph2->set_parent (v.get_id());
    graph2->twodimensional (true);
    graph2->setlimits (0, 1000, -1.0f, 1.0f);
    graph2->policy = mplot::stylepolicy::lines;
    graph2->ylabel = "obj value";
    graph2->xlabel = "Anneal time";
    graph2->prepdata ("f_x");
    graph2->prepdata ("f_x_best + .5");
    graph2->prepdata ("f_x_cand");
    graph2->finalize();
    auto graph2p = v.addVisualModel (graph2);

    v.render();
#endif

    // The Optimization:
    //
    // Your job is to loop, calling anneal.step(), until anneal.state tells you to stop...
    while (anneal.state != sm::anneal_state::ready_to_stop) {

        // ...and on each loop, compute the objectives that anneal asks you to:
        if (anneal.state == sm::anneal_state::need_to_compute) {
            // Compute the candidate objective value
            anneal.f_x_cand = objective (anneal.x_cand);

        } else if (anneal.state == sm::anneal_state::need_to_compute_set) {
            // Compute objective values for reannealing
            anneal.f_x_plusdelta = objective (anneal.x_plusdelta);
            // anneal.f_x is already computed. BUT could jump to the x_best on reanneal.

        } else {
            throw std::runtime_error ("Unexpected state for anneal object.");
        }

#ifdef VISUALISE
        // You can update the visualisation within this loop if you like:
        candp->position = { static_cast<float>(anneal.x_cand[0]),
                            static_cast<float>(anneal.x_cand[1]),
                            static_cast<float>(anneal.f_x_cand - F{0.15}) };
        candp->reinit();
        bestp->position = { static_cast<float>(anneal.x_best[0]),
                            static_cast<float>(anneal.x_best[1]),
                            static_cast<float>(anneal.f_x_best - F{0.15}) };
        bestp->reinit();
        currp->position = { static_cast<float>(anneal.x[0]),
                            static_cast<float>(anneal.x[1]),
                            static_cast<float>(anneal.f_x - F{0.15}) };
        currp->reinit();

        // Append to the 2D graph of sums:
        graph1p->append ((float)anneal.steps, std::log(anneal.T_k.mean()), 0);
        graph1p->append ((float)anneal.steps, std::log(anneal.T_cost.mean()), 1);
        graph2p->append ((float)anneal.steps, anneal.f_x-0.2, 0);
        graph2p->append ((float)anneal.steps, anneal.f_x_best, 1);
        graph2p->append ((float)anneal.steps, anneal.f_x_cand+0.2, 2);


        v.waitevents (0.0166);
        v.render();
#endif
        // Finally, you need to ask the algorithm to do its stuff for one step
        anneal.step();
    }

#ifdef VISUALISE
    std::cout << "Last anneal stats: num_improved " << anneal.num_improved << ", num_worse: " << anneal.num_worse
              << ", num_worse_accepted: " << anneal.num_worse_accepted << " (as proportion: "
              << ((double)anneal.num_worse_accepted/(double)anneal.num_worse) << ")\n\n";

    std::cout << "FINISHED in " << anneal.steps << " calls to Anneal::step() (hexgrid has " << hg->num() << " hexes).\n"
              << "Best parameters: " << anneal.x_best << "\n"
              << "Best params obj: " << anneal.f_x_best
              << " vs. " << obj_f.min() << ", the true obj_f.min().\n"
              << "Final error: " <<  anneal.f_x_best - obj_f.min() << "\n";

    std::cout << "(You can close the window with 'Ctrl-q' or take a snapshot with 'Ctrl-s'. 'Ctrl-h' for other help).\n";

    v.keepOpen();
#else
    std::cout << anneal.steps << "," << anneal.f_x_best - obj_f.min() << ","
              << anneal.f_x_best << "," << obj_f.min() << "\n";
#endif

    return 0;
}

// This sets up a noisy 2D objective function with multiple peaks
void setup_objective()
{
    hg = std::make_unique<sm::hexgrid<F>>(F{0.01}, F{1.5}, F{0});
    hg->set_circular_boundary(1);
    obj_f.resize (hg->num());

    // Create 2 Gaussians and sum them as the main features
    sm::vvec<F> obj_f_a(hg->num(), F{0});
    sm::vvec<F> obj_f_b(hg->num(), F{0});

    // Now assign an analytical function to the thing - make it a couple of Gaussians
    F sigma = F{0.045};
    F one_over_sigma_root_2_pi = F{1} / sigma * F{2.506628275};
    F two_sigma_sq = F{2} * sigma * sigma;
    F gauss = F{0};
    F sum = F{0};
    sm::hex<F> chex = *hg->vhexen[200];
    sm::hex<F> chex2 = *hg->vhexen[2000];
    for (auto& k : hg->hexen) {
        // Gaussian profile based on the hex's distance from centre, which is
        // already computed in each hex as hex::r. Don't want this for these. Want dist from some hex/coords
        F r = k.distance_from (chex);
        gauss = (one_over_sigma_root_2_pi * std::exp ( -(r*r) / two_sigma_sq ));
        obj_f_a[k.vi] = gauss;
        sum += gauss;
    }
    for (auto& k : hg->hexen) { obj_f_a[k.vi] *= F{0.01}; }

    sigma = F{0.1};
    one_over_sigma_root_2_pi = F{1} / sigma * F{2.506628275};
    two_sigma_sq = F{2} * sigma * sigma;
    gauss =  F{0};
    sum =  F{0};
    for (auto& k : hg->hexen) {
        F r = k.distance_from (chex2);
        gauss = (one_over_sigma_root_2_pi * std::exp ( -(r*r) / two_sigma_sq ));
        obj_f_b[k.vi] = gauss;
        sum += gauss;
    }
    for (auto& k : hg->hexen) { obj_f_b[k.vi] *= F{0.01}; }

    // Make noise
    sm::vvec<F> noise(hg->num());
    noise.randomize();
    noise *= F{0.2};

    // Then add em up
    obj_f = obj_f_a + obj_f_b + noise;

    // Then smooth...
    // Create a circular hexgrid to contain the Gaussian convolution kernel
    sigma = F{0.005};
    one_over_sigma_root_2_pi = F{1} / sigma * F{2.506628275};
    two_sigma_sq = F{2} * sigma * sigma;
    sm::hexgrid<F> kernel(F{0.01}, F{20}*sigma, 0);
    kernel.set_circular_boundary (F{6}*sigma);
    std::vector<F> kerneldata (kernel.num(), F{0});
    gauss = F{0};
    sum = F{0};
    for (auto& k : kernel.hexen) {
        gauss = (one_over_sigma_root_2_pi * std::exp ( -(k.r*k.r) / two_sigma_sq ));
        kerneldata[k.vi] = gauss;
        sum += gauss;
    }
    for (auto& k : kernel.hexen) { kerneldata[k.vi] /= sum; }

    // A vector for the result
    sm::vvec<F> convolved (hg->num(), F{0});

    // Call the convolution method from hexgrid:
    sm::algo::hexgrid::convolve (*hg, kernel, kerneldata, obj_f, convolved);

    obj_f.swap (convolved);

    // And finally, invert (so we go downhill to the valleys)
    obj_f = -obj_f;
}

// Alternative objective function from Bohachevsky. This *visualises* the function, but
// during the anneal, we'll use the actual function values
void setup_objective_boha()
{
    hg = std::make_unique<sm::hexgrid<F>>(0.01f, 2.5f, 0.0f);
    hg->set_circular_boundary(1.2f);
    obj_f.resize (hg->num());
    F a = F{1}, b = F{2}, c=F{0.3}, d=F{0.4}, alpha=sm::mathconst<F>::three_pi, gamma=sm::mathconst<F>::four_pi;
    for (auto h : hg->hexen) {
        obj_f[h.vi] = a*h.x*h.x + b*h.y*h.y - c * std::cos(alpha*h.x) - d * std::cos (gamma * h.y) + c + d;
    }
}

F objective (const sm::vvec<F>& params)
{
#ifdef USE_BOHACHEVSKY_FUNCTION
    return objective_boha (params);
#else
    return objective_hg (params);
#endif
}

F objective_boha (const sm::vvec<F>& params)
{
    F x = params[0];
    F y = params[1];
    F a = F{1}, b = F{2}, c=F{0.3}, d=F{0.4}, alpha=sm::mathconst<F>::three_pi, gamma=sm::mathconst<F>::four_pi;
    F fn = a*x*x + b*y*y - c * std::cos(alpha*x) - d * std::cos (gamma * y) + c + d;
    return fn;
}

F objective_hg (const sm::vvec<F>& params)
{
    // Find the hex nearest the coordinate defined by params and return its value
    sm::vec<F, 2> coord = { params[0], params[1] };
    std::list<sm::hex<F>>::iterator hn = hg->find_hex_nearest (coord);
    return obj_f[hn->vi];
}
