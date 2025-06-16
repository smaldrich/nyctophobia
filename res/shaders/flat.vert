#version 330 core

uniform mat4 uVP;
uniform mat4 uModel;

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

out vec3 vPos;
out vec4 vColor;

void main() {
    gl_Position = uVP * uModel * vec4(position, 1);
    vPos = (uModel * vec4(position, 1)).xyz;
    vColor = color;
}
