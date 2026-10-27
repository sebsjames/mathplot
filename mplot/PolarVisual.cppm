/*
 * A visual for polar plots (rho, theta)
 */
module;

#include <cstdint>
#include <array>
#include <deque>
#include <string>
#include <cmath>
#include <utility>
#include <stdexcept>

export module mplot.polarvisual;

import sm.mathconst;
import sm.vec;

import mplot.unicode;
import mplot.visualdatamodel;
import mplot.graphvisual;

export namespace mplot
{
    template <typename F, std::int32_t glver = mplot::gl::version_4_1>
    struct PolarVisual : public VisualDataModel<F, glver>
    {
        using mc = sm::mathconst<F>;

        //! Constructor
        //! \param _offset The offset within mplot::Visual space to place this model
        PolarVisual (const sm::vec<float> _offset)
        {
            this->viewmatrix.translate (_offset);
            // Set default text features
            this->tf.fontsize = 0.05f;
            this->tf.fontres = 48;
            this->tf.colour = this->framecolour;
            // Like graphs, Polar visuals don't rotate by default. If you want yours to, set this false in your client code.
            this->twodimensional (true);

            this->zScale.identity_scaling();
            this->colourScale.do_autoscale = true;
            this->colourScale2.do_autoscale = true;
            this->colourScale3.do_autoscale = true;
        }

        void setTextColour (const std::array<float, 3>& c) { this->tf.colour = c; }
        void setFrameColour (const std::array<float, 3>& c) { this->framecolour = c; }
        void setFrameTextColour (const std::array<float, 3>& c) { this->framecolour = this->tf.colour = c; }

        void initializeVertices()
        {
            this->determine_datasize();
            if (this->datasize == 0) { return; }

            this->setupScaling();

            // Auto-set autolabelgap
            auto em = this->makeVisualTextModel (this->tf);
            mplot::TextGeometry em_geom = em->getTextGeometry (std::string("m"));
            this->autolabelgap = em_geom.width() / 2.0f;
            this->drawFrame();
            this->drawTickLabels();
            this->fillFrameWithColour();
        }

        //! Draw a circular frame around the polar plot
        void drawFrame()
        {
            // Draw an approximation of a circle. Passing -uy as the inplane vector ensures
            // flatcircle line aligns with colour drawn by fillFrameWithColour()
            this->computeFlatCircleLine (sm::vec<float>{0,0,this->z}, sm::vec<>::uz(), -sm::vec<>::uy(), this->radius + this->framelinewidth/2.0f,
                                         this->framelinewidth, this->framecolour, this->numsegs);
        }

        //! Draw the tick labels (the numbers or whatever text the client code has given us)
        void drawTickLabels()
        {
            // Reset these members
            this->ticklabelheight = 0.0f;
            this->ticklabelwidth = 0.0f;

            if (this->label_angles.empty()) {
                this->label_angles.resize (this->labels.size());
                for (std::uint32_t i = 0; i < this->labels.size(); ++i) {
                    this->label_angles[i] = i * (mc::two_pi / this->labels.size());
                    // Rescale any that exceed 2pi:
                    this->label_angles[i] = this->label_angles[i] < F{0} ? this->label_angles[i] + mc::two_pi : this->label_angles[i];
                    this->label_angles[i] = this->label_angles[i] > mc::two_pi ? this->label_angles[i] - mc::two_pi : this->label_angles[i];
                }
            }

            for (std::uint32_t i = 0; i < this->label_angles.size(); ++i) {
                std::string s = this->labels[i];
                auto lbl = this->makeVisualTextModel (this->tf);
                mplot::TextGeometry geom = lbl->getTextGeometry (s);
                this->ticklabelheight = geom.height() > this->ticklabelheight ? geom.height() : this->ticklabelheight;
                this->ticklabelwidth = geom.width() > this->ticklabelwidth ? geom.width() : this->ticklabelwidth;
                // Dep. on angle, the additional gap for the text will need to be based on different aspects of the text geometry
                float geom_gap = std::abs(std::cos(label_angles[i]) * geom.half_width()) + std::abs(std::sin(label_angles[i]) * geom.half_height());
                float lbl_r = this->radius + this->framelinewidth + this->ticklabelgap + this->autolabelgap + geom_gap;
                sm::vec<float> lblpos = {
                    lbl_r * std::cos (static_cast<float>(label_angles[i])) - geom.half_width(),
                    lbl_r * std::sin (static_cast<float>(label_angles[i])) - geom.half_height(),
                    this->z
                };
                lbl->setupText (s, lblpos + this->viewmatrix.translation(), this->tf.colour);
                this->texts.push_back (std::move(lbl));
            }
        }

        // Draw the Polar plot colour
        void fillFrameWithColour()
        {
            // Check numrings * numsegs is same size as scalar or vector data
            if (this->numrings * this->numsegs != this->datasize) {
                throw std::runtime_error ("Uh oh");
            }

            // Note: Going from out to in, rather than in to out
            for (std::int32_t ring = this->numrings - 1; ring > 0; ring--) {

                float r_out = this->radius * static_cast<float>(ring)/(this->numrings-1);
                float r_in = this->radius * static_cast<float>(ring-1)/(this->numrings-1);

                for (std::int32_t j = 0; j < static_cast<std::int32_t>(this->numsegs); j++) {

                    std::array<float, 3> clr = this->setColour ((ring - 1) * this->numsegs + j);

                    float t = j * sm::mathconst<float>::two_pi/static_cast<float>(this->numsegs);

                    float outer_z = this->dcopy[ring * this->numsegs + j];
                    float inner_z = this->dcopy[(ring - 1) * this->numsegs + j];

                    sm::vec<float> c_in = sm::vec<>::uy() * sin(t) * r_in + sm::vec<>::ux() * cos(t) * r_in;
                    c_in[2] = inner_z;

                    this->vertex_push (c_in, this->vertexPositions);
                    this->vertex_push (sm::vec<>::uz(), this->vertexNormals);
                    this->vertex_push_colour (clr, this->vertexColors);

                    sm::vec<float> c_out = sm::vec<>::uy() * sin(t) * r_out + sm::vec<>::ux() * cos(t) * r_out;
                    c_out[2] = outer_z;

                    this->vertex_push (c_out, this->vertexPositions);
                    this->vertex_push (sm::vec<>::uz(), this->vertexNormals);
                    this->vertex_push_colour (clr, this->vertexColors);
                }
                // Added 2*segments vertices to vertexPositions
                for (std::int32_t j = 0; j < static_cast<std::int32_t>(this->numsegs); j++) {
                    std::int32_t jn = (numsegs + ((j + 1) % numsegs)) % numsegs;

                    this->indices.push_back (this->idx + (2 * j));
                    this->indices.push_back (this->idx + (2 * jn));
                    this->indices.push_back (this->idx + (2 * jn + 1));

                    this->indices.push_back (this->idx + (2 * j));
                    this->indices.push_back (this->idx + (2 * jn + 1));
                    this->indices.push_back (this->idx + (2 * j + 1));
                }
                this->idx += 2 * this->numsegs; // nverts
            }
        }

        //! The radius of the HSVwheel
        float radius = 1.0f;
        //! Position in z in model space. Default is just 0.
        float z = 0.0f;
        //! colour for the axis box/lines. Text colour is in TextFeatures tf.colour
        std::array<float, 3> framecolour = mplot::colour::black;
        //! The line width of the frame
        float framelinewidth = 0.01f;
        //! The label strings  that should be displayed. Order the elements anti-clockwise, starting from the 'north' element.
        std::deque<std::string> labels = {
            "0",
            mplot::unicode::toUtf8 (mplot::unicode::pi) + std::string("/2"),
            mplot::unicode::toUtf8 (mplot::unicode::pi),
            std::string ("3") + mplot::unicode::toUtf8 (mplot::unicode::pi) + std::string("/2")
        };
        //! The positions, as angles for the labels. If empty, these will be auto-computed
        std::deque<F> label_angles = { /*mc::pi_over_2, mc::pi, mc::three_pi_over_2, 0.0f*/ };
        // Stores all the text features for this ColourBar (font, colour, font size, font res)
        mplot::TextFeatures tf;
        //! Additional gap to x axis tick labels for user to set
        float ticklabelgap = 0.0f;
        //! The number of segments to make in each ring of the colourmap fill. Depends on your data.
        std::uint32_t numsegs = 128;
        //! How many rings of colour? Depends on your data. This is really 'the number of rings
        //! you'll see + 1'. It's related to the number of rings of data you have.
        std::uint32_t numrings = 64;
    protected:
        float autolabelgap = 0.0f;
        //! tick label height
        float ticklabelheight = 0.0f;
        //! tick label width
        float ticklabelwidth = 0.0f;
    };

} // namespace
