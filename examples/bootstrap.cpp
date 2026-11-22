// Test that bootstrapping results match expected results from parameterized statistics. Plot
// distributions and results of bootstrap. Edit examples/bootstrap.json to change the distribution
// parameters.

#include <memory>
#include <iostream>
#include <string>
#include <cmath>
#include <sstream>
#include <random>

import sm.vvec;
import sm.bootstrap;
import sm.random;
import sm.histo;

import mplot.visual;
import mplot.graphvisual;

// Test bootstrap computation of standard error of the mean for a distribution.
int main()
{
    int rtn = -1;

    // First, generate numbers from a normal distribution. Args of rand_normal constructor are mean, sigma.
    double dist1_mean = 5.0;
    double dist1_sigma = 1.0;
    unsigned int dist1_n = 1000;
    sm::rand_normal<double, std::mt19937_64> rnorm (dist1_mean, dist1_sigma);
    sm::vvec<double> dist;
    dist.set_from (rnorm.get(dist1_n));

    // Output some simple stats on the distribution
    std::cout << "The standard deviation is " << dist.std() << std::endl;
    std::cout << "The standard deviation divided by sq root of dist.size() = "
              << (dist.std()/std::sqrt(dist.size())) << std::endl;

    // Invoke the bootstrap to compute the standard error of the mean
    unsigned int num_resamples = 1024;
    double eom = sm::bootstrap::error_of_mean<double> (dist, num_resamples);
    std::cout << "The bootstrapped, standard error of the mean is " << eom << std::endl;

    // Graph the distribution, with labels
    mplot::Visual v(1400, 768, "Bootstrap");
    // You can leave the second 'float' template argument out, as it's the default, but
    // being explicit demonstrates the link to the GraphVisual, which must be
    // GraphVisual<float> to display histo<T, float>
    sm::histo<double, float> h(dist, 100);
    auto gv = std::make_unique<mplot::GraphVisual<float>> (sm::vec<float>({-2,0,0}));
    gv->set_parent (v.get_id());
    gv->setdata (h);
    gv->xlabel = "Value";
    gv->ylabel = "Proportion";
    {
        std::string mlbl = "mean ";
        mlbl += std::to_string(dist.mean());
        gv->addLabel (mlbl, sm::vec<float>({1.1, 0.66, 0}), mplot::TextFeatures(0.05f));
        std::string sdlbl = "SD ";
        sdlbl += std::to_string(dist.std());
        gv->addLabel (sdlbl, sm::vec<float>({1.1, 0.58, 0}), mplot::TextFeatures(0.05f));
        std::string sdlbl2 = "SD/";
        sdlbl2 += mplot::unicode::toUtf8 (mplot::unicode::sqrt);
        sdlbl2 += std::to_string(dist.size()) + std::string (" = ");
        sdlbl2 += std::to_string((dist.std()/std::sqrt(dist.size())));
        gv->addLabel (sdlbl2, sm::vec<float>({1.1, 0.5, 0}), mplot::TextFeatures(0.05f));
        std::string bslbl = "BS stderr: ";
        bslbl += std::to_string(eom);
        gv->addLabel (bslbl, sm::vec<float>({1.1, 0.42, 0}), mplot::TextFeatures(0.05f));
    }

    // Bootstrapped t-test
    double dist2_mean = 5.2;
    double dist2_sigma = 1.0;
    unsigned int dist2_n = 1000;
    sm::rand_normal<double, std::mt19937_64> rnorm2 (dist2_mean, dist2_sigma);
    sm::vvec<double> dist2;
    dist2.set_from (rnorm2.get(dist2_n));
    double eom2 = sm::bootstrap::error_of_mean<double> (dist2, num_resamples);
    std::cout << "Bootstrapped error of mean of dist2 is " << eom2 << std::endl;

    sm::vec<double,2> asl = sm::bootstrap::ttest_equalityofmeans<double> (dist, dist2, num_resamples);
    std::cout << "Achieved significance level: " << asl[0] << " (with minasl: " << asl[1] << ")\n";

    sm::histo<double, float> h2(dist2, 100);
    auto gv2 = std::make_unique<mplot::GraphVisual<float>> (sm::vec<float>({0,0,0}));
    gv2->set_parent (v.get_id());
    gv2->setdata(h); // Add both to second graph
    gv2->setdata (h2);
    gv2->xlabel = "Value";
    gv2->ylabel = "Proportion";
    {
        std::string mlbl = "mean ";
        mlbl += std::to_string(dist2.mean());
        gv2->addLabel (mlbl, sm::vec<float>({1.1, 0.66, 0}), mplot::TextFeatures(0.05f));
        std::string sdlbl = "SD2 ";
        sdlbl += std::to_string(dist2.std());
        gv2->addLabel (sdlbl, sm::vec<float>({1.1, 0.58, 0}), mplot::TextFeatures(0.05f));
        std::string sdlbl2 = "SD2/";
        sdlbl2 += mplot::unicode::toUtf8 (mplot::unicode::sqrt);
        sdlbl2 += std::to_string(dist2.size()) + std::string (" = ");
        sdlbl2 += std::to_string((dist2.std()/std::sqrt(dist.size())));
        gv2->addLabel (sdlbl2, sm::vec<float>({1.1, 0.5, 0}), mplot::TextFeatures(0.05f));
        std::string bslbl = "BS stderr2: ";
        bslbl += std::to_string(eom2);
        gv2->addLabel (bslbl, sm::vec<float>({1.1, 0.42, 0}), mplot::TextFeatures(0.05f));
        gv2->addLabel (std::string("ASL: ") + asl.str(), sm::vec<float>({1.1, 0.34, 0}), mplot::TextFeatures(0.05f));
        // A significance statement label
        double sig_level = 0.01;
        std::stringstream signif;
        if (asl[0] < asl[1]) {
            signif << "Discard the null hypothesis with significance level " << sig_level << ".\n"
                   << "The (low) probability the distributions have the same mean is less than the min. ASL\n"
                   << "The min. possible ASL that can be measured with " << num_resamples << " resamples is " << asl[1];
        } else if (asl[0] < sig_level) {
            signif << "Discard the null hypothesis with signficance level " << sig_level
                   << ".\nThe low probability the distributions have the same mean is " << asl[0];
        } else {
            signif << "Can't discard the null hypothesis that the distributions have the same mean.\n"
                   << "Significance level: " << sig_level << ", ASL: " << asl[0] << std::endl;;
        }
        gv2->addLabel (signif.str(), sm::vec<float>({0.0, -0.27, 0}), mplot::TextFeatures(0.05f));
    }

    // Finalize/add
    gv->finalize();
    gv2->finalize();
    v.addVisualModel (gv);
    v.addVisualModel (gv2);

    v.keepOpen();

    return rtn;
}
