#version 460
#extension GL_ARB_bindless_texture : require

// TILEMAP.FRAG

layout(location = 0) in vec2 fUV;
layout(location = 1) flat in uint mat_idx;

layout(location = 0) out vec4 fragColor;

struct Material {
  sampler2D tex_handle;
  uint color;
};

layout(std430, binding = 20) buffer Materials { Material materials[]; };

vec4 uint_to_rgba(uint val) {
  float r = float((val & 0x000000ff));
  float g = float((val & 0x0000ff00) >> 8);
  float b = float((val & 0x00ff0000) >> 16);
  float a = float((val & 0xff000000) >> 24); // TODO: multipled by 'b'??
  return vec4(r, g, b, a) / 255.0;
};

void main() {
  Material mat = materials[mat_idx];
  vec4 tex_color = texture(mat.tex_handle, fUV);
  vec3 color = uint_to_rgba(mat.color).rgb;
  vec3 blend = tex_color.rgb + color * 0.0;

  fragColor = vec4(blend, 1.0);
}
