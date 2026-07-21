/*
 * Helper functions for compound-ray/mathplot interoperability
 *
 * Author: Seb James
 * Date: June 2025
 */
#pragma once

// CompoundRay include files (*not* part of mathplot)
#include <MulticamScene.h>
#include <libEyeRenderer.h>
#include <cuda/BufferView.h>
#include <sutil/Matrix.h>

// maths and mathplot includes
#include <sm/vec>
#include <sm/mat>
#include <mplot/Visual.h>
#include <mplot/VerticesVisual.h>

// scene exists at global scope in libEyeRenderer.so
extern MulticamScene* scene;

namespace mplot::compoundray
{
    // Helper to convert sm::mat<float, 4> to Matrix4x4
    sutil::Matrix4x4 mat44_to_Matrix4x4 (const sm::mat<float, 4>& m)
    {
        sutil::Matrix4x4 M = {}; // M is row-major, m is col-major
        M[0]  = m[0];  M[1]  = m[4];  M[2] =  m[8];   M[3]  = m[12];
        M[4]  = m[1];  M[5]  = m[5];  M[6] =  m[9];   M[7]  = m[13];
        M[8]  = m[2];  M[9]  = m[6];  M[10] = m[10];  M[11] = m[14];
        M[12] = m[3];  M[13] = m[7];  M[14] = m[11];  M[15] = m[15];
        return M;
    }

    // Blender has a transformation to convert the native y-up OpenGL/GLTF coordinate system into a
    // z-up coordinate system. To work in Blender, we need a "match blender" mode in which we apply
    // the same transform. This function returns the matrix (as sm::mat<float, 4>) that should be passed to
    // libEyeRenderer's loadGlTFscene
    sm::mat<float, 4> blender_transform_mat()
    {
        return sm::mat<float, 4>::frombasis (sm::vec<float>::ux(), sm::vec<float>::uz(), -sm::vec<float>::uy());
    }

    // Blender has a transformation to convert the native y-up OpenGL/GLTF coordinate system into a
    // z-up coordinate system. To work in Blender, we need a "match blender" mode in which we apply
    // the same transform. This function returns the matrix (as sutil::Matrix4x4) that should be
    // passed to libEyeRenderer's loadGlTFscene
    sutil::Matrix4x4 blender_transform()
    {
        return mplot::compoundray::mat44_to_Matrix4x4 (mplot::compoundray::blender_transform_mat());
    }

    /*!
     * This function finds the meshes in compound-ray's MulticamScene and creates corresponding
     * VisualModels in the mplot::Visual
     */
    template<int glver = mplot::gl::version_4_1>
    void scene_to_visualmodels (MulticamScene* thescene, mplot::Visual<glver>* thevisual, bool make_navmeshes = false)
    {
        static constexpr bool debug_meshload = false;
        std::vector<std::shared_ptr<MulticamScene::MeshGroup>> mymeshes = thescene->getMeshes();
        std::vector<MaterialData::Pbr> mymats = thescene->getMaterials();
        for (unsigned int mi = 0; mi < mymeshes.size(); ++mi) {
            sm::mat<float, 4> tfm;
            for (unsigned int tfi = 0; tfi < 16; ++tfi) { tfm[tfi] = mymeshes[mi]->transform[tfi]; }
            tfm.transpose_inplace(); // Need to transpose tfm after copying data
            if constexpr (debug_meshload) {
                std::cout << "A mesh called " << mymeshes[mi]->name  << " with scene transform\n" << tfm << "\n";
            }

            // We have indices, positions, normals and colours available as cuda::BufferViews.
            sm::vvec<uint32_t> ind = {};
            sm::vvec<sm::vec<float>> posn = {};
            sm::vvec<sm::vec<float>> norm = {};
            sm::vvec<sm::vec<float>> colr = {};
            uint32_t last_n = 0u;
            for (unsigned int ii = 0; ii < mymeshes[mi]->indices.size(); ++ii) {
                cuda::CopiedBufferView<uint32_t> ind_buf (mymeshes[mi]->indices[ii]);
                sm::vvec<uint32_t> these_inds (ind_buf.bv_data.size(), 0u);
                std::swap (these_inds, ind_buf.bv_data);
                these_inds += last_n; // or last index
                ind.append (these_inds);

                cuda::CopiedBufferView<float3> posn_buf (mymeshes[mi]->positions[ii]);
                for (unsigned int j = 0; j < posn_buf.bv_data.size(); ++j) {
                    float3 p = posn_buf[j];
                    posn.push_back (sm::vec<float>{p.x, p.y, p.z});
                }

                cuda::CopiedBufferView<float3> norm_buf (mymeshes[mi]->normals[ii]);
                for (unsigned int j = 0; j < norm_buf.bv_data.size(); ++j) {
                    float3 p = norm_buf[j];
                    norm.push_back (sm::vec<float>{p.x, p.y, p.z});
                }

                if (mymeshes[mi]->host_color_types[ii] == -1) { // No colour vertices, get a material
                    if constexpr (debug_meshload) { std::cout << "No colour vertices, is there material?\n"; }
                    sm::vvec<sm::vec<float>> colr_section (posn_buf.bv_data.size(), sm::vec<float>{1,0,1});
                    if (static_cast<size_t>(mymeshes[mi]->material_idx[ii]) < mymats.size()) {
                        MaterialData::Pbr mypbr = mymats[mymeshes[mi]->material_idx[ii]];
                        // If a texture is present, asign a fixed green colour, else it defaults to white which is impossible to see.
                        sm::vec<float> clr;
                        if (mypbr.base_color.x != 0 || mypbr.metallic_roughness_tex != 0 || mypbr.normal_tex != 0) {
                            clr.set_from (mplot::colour::darkseagreen);
                        } else {
                            clr = { mypbr.base_color.x, mypbr.base_color.y, mypbr.base_color.z };
                        }
                        colr_section.set_from (clr);
                    }
                    colr.append(colr_section);

                } else { // Use colour vertices
                    if constexpr (debug_meshload) { std::cout << "Colour vertices\n"; }
                    if (mymeshes[mi]->host_color_types[ii] == 5126) { // TINYGLTF_COMPONENT_TYPE_FLOAT 5126 = float4
                        if constexpr (debug_meshload) { std::cout << "float3 colour\n"; }
                        cuda::CopiedBufferView<float3> clr_buf (mymeshes[mi]->host_colors_f3[ii]);
                        for (unsigned int j = 0; j < clr_buf.bv_data.size(); ++j) {
                            float3 c = clr_buf[j];
                            colr.push_back (sm::vec<float>{c.x, c.y, c.z});
                        }
                    } else if (mymeshes[mi]->host_color_types[ii] == 5123) { // TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT 5123 = ushort4
                        if constexpr (debug_meshload) { std::cout << "ushort4 colour\n"; }
                        cuda::CopiedBufferView<ushort4> clr_buf (mymeshes[mi]->host_colors_us4[ii]);
                        for (unsigned int j = 0; j < clr_buf.bv_data.size(); ++j) {
                            ushort4 c = clr_buf[j];
                            colr.push_back (sm::vec<float>{c.x / 65535.0f, c.y / 65535.0f, c.z / 65535.0f});
                        }
                    } else if (mymeshes[mi]->host_color_types[ii] == 5121) { // TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE 5121 = uchar4
                        if constexpr (debug_meshload) { std::cout << "uchar4 colour\n"; }
                        cuda::CopiedBufferView<uchar4> clr_buf (mymeshes[mi]->host_colors_uc4[ii]);
                        for (unsigned int j = 0; j < clr_buf.bv_data.size(); ++j) {
                            uchar4 c = clr_buf[j];
                            colr.push_back (sm::vec<float>{c.x / 255.0f, c.y / 255.0f, c.z / 255.0f});
                        }
                    } else { // Revert to a fixed colour
                        if constexpr (debug_meshload) { std::cout << "'else' (fixed colour)\n"; }
                        sm::vvec<sm::vec<float>> colr_section (posn_buf.bv_data.size(), sm::vec<float>{0,1,1});
                        colr.append(colr_section);
                    }

                }
                last_n = posn_buf.bv_data.size();
            }

            // Make a visual model from the harvested indices and positions
            if constexpr (debug_meshload) {
                std::cout << "Make a VerticesVisual with "
                          << ind.size() << " indices, " << posn.size() << " posns, "
                          << norm.size() << " norms, " << colr.size() << " colours" << std::endl;
            }
            auto vertvm = std::make_unique<mplot::VerticesVisual<glver>> (tfm, ind, posn, norm, colr);
            thevisual->bindmodel (vertvm);
            vertvm->name = mymeshes[mi]->name;
            if (make_navmeshes == true) { vertvm->make_navmesh(); }
            vertvm->finalize();
            thevisual->addVisualModel (vertvm);
        }
    }

    // From the camera localspace, we can create a matrix which specifies a camera pose within the world frame
    sm::mat<float, 4> getCameraSpace (MulticamScene* thescene)
    {
        // The camera's localspace is a set of three vectors in world coordinates
        float3 camls_x, camls_y, camls_z;
        thescene->getCamera()->getLocalSpace (camls_x, camls_y, camls_z);
        // Express them as sm::vec<float>:
        sm::vec<float> camls_xv = { camls_x.x, camls_x.y, camls_x.z };
        sm::vec<float> camls_yv = { camls_y.x, camls_y.y, camls_y.z };
        sm::vec<float> camls_zv = { camls_z.x, camls_z.y, camls_z.z };

        // Start with setting the matrix from the localspace basis vectors
        sm::mat<float, 4> camera_space = sm::mat<float, 4>::frombasis (camls_xv, camls_yv, camls_zv);

        // Translate camera_space by the camera position, obtained from compound-ray
        float3 campos = thescene->getCamera()->getPosition();
        sm::vec<float> campos_v = { campos.x, campos.y, campos.z };
        camera_space.pretranslate (campos_v);

        return camera_space;
    }

} // namespace
