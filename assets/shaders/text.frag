#version 460
#extension GL_ARB_bindless_texture : require

// TILEMAP.FRAG
layout(bindless_sampler) uniform sampler2D font_atlas;

layout(location = 0) in vec2 fUV;
layout(location = 1) flat in uint fCol;

layout(location = 0) out vec4 fragColor;

vec4 uint_to_rgba(uint val) {
  float r = float((val & 0x000000ff));
  float g = float((val & 0x0000ff00) >> 8);
  float b = float((val & 0x00ff0000) >> 16);
  float a = float((val & 0xff000000) >> 24);

  return vec4(r, g, b, a) / 255.0;
};

void main() {
  vec4 color = uint_to_rgba(fCol);
  vec4 alpha = vec4(texture(font_atlas, fUV).r);
  fragColor = vec4(color.rgb, alpha);
}
