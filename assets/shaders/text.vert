#version 460
#extension GL_ARB_bindless_texture : require

// TILEMAP.VERT

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vUV;
layout(location = 2) in vec3 vCol;

layout(location = 0) out vec2 fUV;
layout(location = 1) out vec3 fCol;

uniform mat4 ViewProj;
uniform mat4 Model;

void main() {
  fUV = vUV;
  fCol = vCol;
  gl_Position = ViewProj * Model * vec4(vPos.xyz, 1.0);
}
