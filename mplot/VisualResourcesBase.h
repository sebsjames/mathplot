/*!
 * \file
 *
 * Declares a VisualResource class to hold the information about Freetype and any other
 * one-per-program resources.
 *
 * \author Seb James
 * \date November 2020
 */

#pragma once

#include <mplot/gl/version.h>
#include <mplot/VisualFont.h>
// FreeType for text rendering
#include <ft2build.h>
#include FT_FREETYPE_H

namespace mplot
{
    // Pointers to mplot::VisualBase are used to index font faces
    template<int>
    class VisualBase;

    //! Singleton resource class for mplot::Visual scenes. (base class, with no GL calls, and no
    //! instance function)
    template <int glver>
    class VisualResourcesBase
    {
    protected:
        VisualResourcesBase() { }
        ~VisualResourcesBase()
        {
            // As with the case for faces, when each mplot::Visual goes out of scope, the FreeType
            // instance gets cleaned up. So at this stage freetypes should also be empy and nothing
            // will happen here either.
            for (auto& ft : this->freetypes) { FT_Done_FreeType (ft.second); }
        }

        //! FreeType library object
        std::map<mplot::VisualBase<glver>*, FT_Library> freetypes;

    public:
        VisualResourcesBase(const VisualResourcesBase<glver>&) = delete;
        VisualResourcesBase& operator=(const VisualResourcesBase<glver> &) = delete;
        VisualResourcesBase(VisualResourcesBase<glver> &&) = delete;
        VisualResourcesBase & operator=(VisualResourcesBase<glver> &&) = delete;

        //! A function to call to simply make sure the singleton instance exists. In derived class
        //! this could be a no-op.
        virtual void create() = 0;

        // Note: freetype_init function is in derived class

        //! When a mplot::Visual goes out of scope, its freetype library instance should be
        //! deinitialized.
        void freetype_deinit (mplot::VisualBase<glver>* _vis)
        {
            // First clear the faces associated with VisualBase<>* _vis
            this->clearVisualFaces (_vis);
            // Second, clean up the FreeType library instance and erase from this->freetypes
            auto freetype = this->freetypes.find (_vis);
            if (freetype != this->freetypes.end()) {
                FT_Done_FreeType (freetype->second);
                this->freetypes.erase (freetype);
            }
        }

        // Note: get/clearVisualFace functions are in derived classes
        virtual void clearVisualFaces (mplot::VisualBase<glver>* _vis) = 0;

        /*!
         * SSBO management
         */
        //! Instanced rendering mode (SSBO access). position data stored in SSBO index 1 (must match GLSL code)
        static constexpr unsigned int instance_index = 1;
        //! colour, scale, rotation stored in SSBO index 2
        static constexpr unsigned int instparam_index = 2;
        //! one 3D vector is 3 floats
        static constexpr unsigned int floats_per_instance = 3;
        //! Instance params are: colour/alpha (4 floats), scale (1 float)
        static constexpr unsigned int floats_per_instparam = 5;

        //! This will control how much GPU RAM is allocated when using instanced rendering
        //! (Hopefully, when I'm finished, the RAM will be allocated only if at least one
        //! VisualModel is marked 'instanced'). Makes a big difference to speed of operation (unless
        //! I can send a portion of a buffer to the GPU).
        static constexpr unsigned int max_instances = 32 * 1024;
        static constexpr unsigned int max_instance_floats = floats_per_instance * max_instances;
        static constexpr unsigned int max_instparam_floats = floats_per_instparam * max_instances;

        // The Current location from which space in the instance SSBOs should be allocated
        unsigned int instance_top = 0;
    };

} // namespace mplot
