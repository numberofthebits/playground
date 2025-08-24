#version 460
#extension GL_ARB_bindless_texture : require

// TILEMAP.VERT

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vUV;

layout(location = 0) out vec2 fUV;
layout(location = 1) flat out uint fCol;

struct DrawData {
  mat4 view_matrix;
  mat4 model_matrix;
  uint color;
};

layout(std430, binding = 18) buffer DrawDataBuffer { DrawData draw_data[]; };

uniform mat4 Proj;

void main() {
  DrawData draw_data = draw_data[gl_DrawID];
  fUV = vUV;
  fCol = draw_data.color;

  gl_Position =
      Proj * draw_data.view_matrix * draw_data.model_matrix * vec4(vPos, 1.0);
}
