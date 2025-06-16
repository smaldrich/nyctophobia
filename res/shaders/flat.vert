#version 330 core

uniform mat4 uVP;
uniform mat4 uModel;

layout(location = 1) in vec3 position;

out vec3 vPos;

void main() {
    gl_Position = uVP * uModel * vec4(position, 1);
    vPos = (uModel * vec4(position, 1)).xyz;
}
