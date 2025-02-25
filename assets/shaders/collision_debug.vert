#version 460

layout(location = 0) in vec3 vPos;

uniform mat4 View;
uniform mat4 Proj;
uniform mat4 Model;

void main() {
    gl_Position = Proj * View * Model * vec4(vPos, 1.0);
}
