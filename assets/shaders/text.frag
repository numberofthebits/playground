#version 460
#extension GL_ARB_bindless_texture : require

// TILEMAP.FRAG
layout(bindless_sampler) uniform sampler2D font_atlas;

layout(location = 0) in vec2 fUV;
layout(location = 1) in vec3 fCol;

layout(location = 0) out vec4 fragColor;

void main() {

  float luminance = texture(font_atlas, fUV).r;
  fragColor = vec4(fCol * luminance, 1.0);
  //  fragColor = vec4(fUV, 0.0, 1.0);
}
