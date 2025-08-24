#version 460
#extension GL_ARB_bindless_texture : require

// TILEMAP.VERT

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vUV;

layout(location = 0) out vec2 fUV;
layout(location = 1) flat out uint mat_idx;

struct DrawData {
  mat4 model_matrix;
  vec2 tex_offset;
  vec2 tex_scale;
  uint material_idx;
};

layout(std430, binding = 18) buffer DrawDataBuffer { DrawData draw_data[]; };

uniform mat4 ViewProj;

void main() {
  DrawData data = draw_data[gl_DrawID];
  fUV = data.tex_scale * vUV + data.tex_offset;
  mat_idx = data.material_idx;

  gl_Position = ViewProj * data.model_matrix * vec4(vPos, 1.0);
}
