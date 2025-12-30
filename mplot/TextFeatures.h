#pragma once

#include <array>
#include <mplot/colour.h>
#include <mplot/VisualFont.h>

namespace mplot
{
    struct TextFeatures;
    std::ostream& operator<< (std::ostream&, const TextFeatures&);

    // A way to bundle up font size, colour, etc into a single object. Constructors chosen for max convenience.
    struct TextFeatures
    {
        TextFeatures(){};
        TextFeatures (const float _fontsize,
                      const int _fontres,
                      const bool _centre_horz,
                      const std::array<float, 3> _colour,
                      mplot::VisualFont _font)
            : fontsize(_fontsize), fontres(_fontres), centre_horz(_centre_horz), colour(_colour), font(_font) {}

        TextFeatures (const float _fontsize, const bool _centre_horz = false)
            : fontsize(_fontsize)
        {
            this->centre_horz = _centre_horz;
        }

        TextFeatures (const float _fontsize, const std::array<float, 3> _colour, const bool _centre_horz = false)
            : fontsize(_fontsize), colour(_colour)
        {
            this->centre_horz = _centre_horz;
        }

        TextFeatures (const float _fontsize, const int _fontres,
                      const std::array<float, 3> _colour = mplot::colour::black, const bool _centre_horz = false)
            : fontsize(_fontsize), fontres(_fontres), colour(_colour)
        {
            this->centre_horz = _centre_horz;
        }

        std::string str() const
        {
            std::string s = {};
            switch (this->font) {
            case mplot::VisualFont::DVSans:
                s += "DejaVuSans.ttf (DejaVu sans-serif) ";
                break;
            case mplot::VisualFont::DVSansItalic:
                s += "DejaVuSans-Oblique.ttf (DejaVu sans-serif italic) ";
                break;
            case mplot::VisualFont::DVSansBold:
                s += "DejaVuSans-Bold.ttf (DejaVu sans-serif bold) ";
                break;
            case mplot::VisualFont::DVSansBoldItalic:
                s += "DejaVuSans-BoldOblique.ttf (DejaVu sans-serif bold italic) ";
                break;
            case mplot::VisualFont::Vera:
                s += "Vera.ttf (Vera sans-serif) ";
                break;
            case mplot::VisualFont::VeraItalic:
                s += "VeraIt.ttf (Vera sans-serif italic) ";
                break;
            case mplot::VisualFont::VeraBold:
                s += "VeraBd.ttf (Vera sans-serif bold) ";
                break;
            case mplot::VisualFont::VeraBoldItalic:
                s += "VeraBI.ttf (Vera sans-serif bold italic) ";
                break;
            case mplot::VisualFont::VeraMono:
                s += "VeraMono.ttf (Vera sans-serif monospace) ";
                break;
            case mplot::VisualFont::VeraMonoItalic:
                s += "VeraMoIt.ttf (Vera sans-serif monospace italic) ";
                break;
            case mplot::VisualFont::VeraMonoBold:
                s += "VeraMoBD.ttf (Vera sans-serif monospace bold) ";
                break;
            case mplot::VisualFont::VeraMonoBoldItalic:
                s += "VeraMoBI.ttf (Vera sans-serif monospace bold italic) ";
                break;
            case mplot::VisualFont::VeraSerif:
                s += "VeraSe.ttf (Vera serif) ";
                break;
            case mplot::VisualFont::VeraSerifBold:
                s += "VeraSeBd.ttf (Vera serif bold) ";
                break;
            default:
                s += "Unknown";
                break;
            }

            s += "size " + std::to_string (fontsize);
            s += " res " + std::to_string (fontres);
            s += " colour rgb(" + std::to_string (colour[0]) + "," + std::to_string (colour[0]) + "," + std::to_string (colour[0]) + ")";
            if (this->centre_horz == true) {
                s += " horizontally centred";
            }

            return s;
        }

        //! The size for the font
        float fontsize = 0.1f;
        //! The pixel resolution for the font textures
        int fontres = 24;
        //! If true, then centre the text string horizontally
        bool centre_horz = false;
        //! The font colour
        std::array<float, 3> colour = mplot::colour::black;
        //! The supported font to use when displaying a text string
        mplot::VisualFont font = mplot::VisualFont::DVSans;

        // Maybe also things like rotate, centre_vert, etc
    };

    std::ostream& operator<< (std::ostream& os, const TextFeatures& tf)
    {
        os << tf.str();
        return os;
    }

} // namespace
