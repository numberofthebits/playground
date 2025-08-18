#version 460
#extension GL_ARB_bindless_texture : require

// TILEMAP.VERT

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vUV;
layout(location = 2) in vec3 vCol;

layout(location = 0) out vec2 fUV;
layout(location = 1) out vec3 fCol;

uniform mat4 Proj;
uniform mat4 Scale;

void main() {
  fUV = vUV;
  fCol = vCol;
  vec3 pos = vPos.xyz;
  // pos.x = (vPos.x - 0.5) * 10.0;
  // pos.y = (vPos.y - 0.5) * 10.0;
  gl_Position = Proj * vec4(pos, 1.0);
}
