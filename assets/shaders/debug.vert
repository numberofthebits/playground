#version 460
#extension GL_ARB_bindless_texture : require

layout(location = 0) in vec3 vPos;


void main() {
    gl_Position = Proj * View * data.model_matrix * vec4(vPos, 1.0);    
}
