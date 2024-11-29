#version 460
#extension GL_ARB_bindless_texture : require

// TILEMAP.FRAG

layout (location = 0) in vec3 fCol;
layout (location = 1) in vec2 fUV;
layout (location = 2) flat in uint mat_idx;

layout (location = 0) out vec4 fragColor;

// layout (std430, binding = 19) buffer TextureHandles2D {
//     sampler2D handles[];
// };

struct Material {
    sampler2D tex_handle;
    uint color;
};

layout (std430, binding = 20) buffer Materials {
    Material materials[];
};

/* Is having to flip Y UV coord just a natural consequence of GL and tile maps? */
/* The map coordinate system has Y as downwards. GL has Y has upwards.*/
// vec2 map_uv_to_tiled_tex() {
//     return (vec2(fUV.x, fUV.y) + vec2(fTileIndex.x, 2.0 - fTileIndex.y)) / vec2(10.0, 3.0);
// }

vec4 uint_to_rgba(uint val) {
    float r = float((val & 0x000000ff));
    float g = float((val & 0x0000ff00) >> 8);
    float b = float((val & 0x00ff0000) >> 16);
    float a = float((val & 0xff000000) >> 24) * b;
    return vec4(r, g, b, a) / 255.0;
};

void main() {
    Material mat = materials[mat_idx];
    vec4 tex_color = texture(mat.tex_handle, fUV);
    if (tex_color.a < 0.1) discard;
    vec3 color = uint_to_rgba(mat.color).rgb;
    vec3 blend = tex_color.rgb * color;

    fragColor = vec4(blend, 1.0);
}
