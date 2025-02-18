#version 460

layout(location = 0) in vec3 vPos;

uniform mat4 View;
uniform mat4 Proj;

void main() {
    gl_Position = Proj * View * vec4(vPos, 1.0);
}
