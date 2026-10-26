// This file is included by mplot::Visual. It contains default vertex and fragment
// shaders which get compiled in to your mathplot program.

#pragma once

import mplot.gl.version;

namespace mplot
{
    // The default vertex shader. To study this GLSL, see Visual.vert.glsl, which has
    // some code comments.
    const char* defaultVtxShader_part1a =
    "uniform mat4 m_matrix;\n"
    "uniform mat4 v_matrix;\n"
    "uniform mat4 p_matrix;\n"
    "uniform float alpha;\n"
    "uniform float gamma;\n"
    "uniform int greyscale;\n";
    const char* defaultVtxShader_instance_uniforms =
    "uniform mat4 s_matrix;\n" // scaling matrix
    "uniform int instance_count;\n"
    "uniform int instance_start;\n"
    "uniform int instparam_count;\n";
    const char* defaultVtxShader_part1b =
    "layout(location = 0) in vec4 position;\n"
    "layout(location = 1) in vec4 normalin;\n"
    "layout(location = 2) in vec4 color;\n";

    const char* defaultVtxShader_part2 =
    "out VERTEX\n"
    "{\n"
    "    vec4 normal;\n"
    "    vec4 color;\n"
    "    vec3 fragpos;\n"
    "} vertex;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = (p_matrix * v_matrix * m_matrix * position);\n"
    "    vec4 vgamma = vec4(gamma, gamma, gamma, 1.0);\n"
    "    vec4 ccolor = pow(color, vgamma);\n"
    "    if (greyscale > 0) {\n"
    "        float lum = (ccolor.x + ccolor.y + ccolor.z) * 0.3333;\n"
    "        vertex.color = vec4(lum, lum, lum, ccolor.a * alpha);\n"
    "    } else {\n"
    "        vec4 valpha = vec4(1.0, 1.0, 1.0, alpha);\n"
    "        vertex.color = ccolor * valpha;\n"
    "    }\n"
    "    vertex.fragpos = vec3(m_matrix * position);\n"
    "    vertex.normal = normalin;\n"
    "}\n";

    const char* defaultVtxShader_part2_inst =
    "out VERTEX\n"
    "{\n"
    "    vec4 normal;\n"
    "    vec4 color;\n"
    "    vec3 fragpos;\n"
    "} vertex;\n"
    "void main()\n"
    "{\n"
    "    vec4 vgamma = vec4(gamma, gamma, gamma, 1.0);\n"
    "    vec4 ccolor = pow(color, vgamma);\n"
    "    if (instance_count > 0) {\n"
    "        int ipos_i = instance_start * 3 + gl_InstanceID * 3;\n"
    "        vec4 iposv;\n"
    "        iposv[0] = ipos[ipos_i];\n"
    "        iposv[1] = ipos[ipos_i + 1];\n"
    "        iposv[2] = ipos[ipos_i + 2];\n"
    "        iposv[3] = 0.0;\n"
    "        if (instparam_count > 0) {\n"
    "            int idx = gl_InstanceID % instparam_count;\n"
    "            int ippos_i = instance_start * 5;\n"
    "            float s = iparam[ippos_i + idx * 5 + 4];\n"
    "            vec3 p_three = vec3(position) * s;\n"
    "            vec4 p_scaled = vec4(p_three, 1.0);\n"
    "            gl_Position = (p_matrix * v_matrix * m_matrix * ((s_matrix * p_scaled) + iposv));\n"
    "            ccolor = pow(vec4(iparam[ippos_i + idx * 5], iparam[ippos_i + idx * 5 + 1], iparam[ippos_i + idx * 5 + 2], 1.0), vgamma);\n"
    "            vertex.color = vec4(ccolor.x, ccolor.y, ccolor.z, iparam[ippos_i + idx * 5 + 3]);\n"
    "            vertex.fragpos = vec3(m_matrix * p_scaled);\n"
    "        } else {\n"
    "            gl_Position = (p_matrix * v_matrix * m_matrix * (position + iposv));\n"
    "            if (greyscale > 0) {\n"
    "                float lum = (ccolor.x + ccolor.y + ccolor.z) * 0.3333;\n"
    "                vertex.color = vec4(lum, lum, lum, ccolor.a * alpha);\n"
    "            } else {\n"
    "                vec4 valpha = vec4(1.0, 1.0, 1.0, alpha);\n"
    "                vertex.color = ccolor * valpha;\n"
    "            }\n"
    "            vertex.fragpos = vec3(m_matrix * position);\n"
    "        }\n"
    "    } else {\n"
    "        gl_Position = (p_matrix * v_matrix * m_matrix * position);\n"
    "        if (greyscale > 0) {\n"
    "            float lum = (ccolor.x + ccolor.y + ccolor.z) * 0.3333;\n"
    "            vertex.color = vec4(lum, lum, lum, ccolor.a * alpha);\n"
    "        } else {\n"
    "            vec4 valpha = vec4(1.0, 1.0, 1.0, alpha);\n"
    "            vertex.color = ccolor * valpha;\n"
    "        }\n"
    "        vertex.fragpos = vec3(m_matrix * position);\n"
    "    }\n"
    "    vertex.normal = normalin;\n"
    "}\n";

    std::string getDefaultVtxShader (const int glver)
    {
        std::string shdr;
        shdr += mplot::gl::version::shaderpreamble (glver);
        shdr += defaultVtxShader_part1a;
        if (mplot::gl::version::has_ssbo (glver)) {
            shdr += defaultVtxShader_instance_uniforms;
        }
        shdr += defaultVtxShader_part1b;
        if (mplot::gl::version::has_ssbo (glver)) {
            shdr += "layout (std430, binding = 1) buffer InstPos { float ipos[]; };\n";
            shdr += "layout (std430, binding = 2) buffer InstParam { float iparam[]; };\n";
            shdr += defaultVtxShader_part2_inst;
        } else {
            shdr += defaultVtxShader_part2;
        }
        return shdr;
    }

    // Default fragment shader. To study this GLSL, see Visual.frag.glsl.
    const char* defaultFragShader = "in VERTEX\n"
    "{\n"
    "    vec4 normal;\n"
    "    vec4 color;\n"
    "    vec3 fragpos;\n"
    "} vertex;\n"
    "uniform vec3 light_colour;\n"
    "uniform float ambient_intensity;\n"
    "uniform vec3 diffuse_position;\n"
    "uniform float diffuse_intensity;\n"
    "out vec4 finalcolor;\n"
    "void main()\n"
    "{\n"
    "    vec3 norm = normalize(vec3(vertex.normal));\n"
    "    vec3 light_dirn = normalize(diffuse_position - vertex.fragpos);\n"
    "    float effective_diffuse = max(dot(norm, light_dirn), 0.0);\n"
    "    vec3 diffuse = diffuse_intensity * effective_diffuse * light_colour;\n"
    "    vec3 ambient = ambient_intensity * light_colour;\n"
    "    vec3 result = (ambient+diffuse) * vec3(vertex.color);\n"
    "    finalcolor = vec4(result, vertex.color.w);\n"
    "}\n";

    std::string getDefaultFragShader (const int glver)
    {
        std::string shdr;
        shdr += mplot::gl::version::shaderpreamble (glver);
        shdr += defaultFragShader;
        return shdr;
    }

    // Default text vertex shader. See VisText.vert.glsl
    const char* defaultTextVtxShader = "uniform mat4 m_matrix;\n"
    "uniform mat4 v_matrix;\n"
    "uniform mat4 p_matrix;\n"
    "layout(location = 0) in vec4 position;\n"
    "layout(location = 1) in vec4 vnormal;\n"
    "layout(location = 2) in vec4 vcolor;\n"
    "layout(location = 3) in vec4 texture;\n"
    "out vec2 TexCoords;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = p_matrix * v_matrix * m_matrix * position;\n"
    "    TexCoords = texture.xy;\n"
    "}";

    std::string getDefaultTextVtxShader (const int glver)
    {
        std::string shdr;
        shdr += mplot::gl::version::shaderpreamble (glver);
        shdr += defaultTextVtxShader;
        return shdr;
    }

    // Default text fragment shader. See VisText.frag.glsl
    const char* defaultTextFragShader = "in vec2 TexCoords;\n"
    "out vec4 color;\n"
    "uniform sampler2D text;\n"
    "uniform vec3 textColor;\n"
    "void main()\n"
    "{\n"
    "    color = vec4(textColor, texture(text, TexCoords).r);\n"
    "}\n";

    std::string getDefaultTextFragShader (const int glver)
    {
        std::string shdr;
        shdr += mplot::gl::version::shaderpreamble (glver);
        shdr += defaultTextFragShader;
        return shdr;
    }

} // namespace mplot
