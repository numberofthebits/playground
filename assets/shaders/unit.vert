#version 460
#extension GL_ARB_bindless_texture : require

// UNIT.VERT

layout(location = 0) in vec3 vPos;
layout(location = 2) in vec2 vUV;

layout(location = 1) out vec2 fUV;
layout (location = 3) flat out uint mat_idx;

struct DrawData {
  mat4 model_matrix;
  uint material_idx;
};

layout (std430, binding = 18) buffer DrawDataBuffer {
  DrawData draw_data[];
};

uniform mat4 View;
uniform mat4 Proj;

void main() {
    DrawData data = draw_data[gl_DrawID];
    fUV = vUV;
    mat_idx = data.material_idx;
    gl_Position = Proj * View * data.model_matrix * vec4(vPos, 1.0);
}
