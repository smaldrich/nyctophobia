#version 330 core

uniform mat4 uVP;
uniform mat4 uModel;

layout(location = 1) in vec3 position;
layout(location = 2) in vec3 normal;

out vec3 vNormal;
out vec3 vPos;

void main() {
    gl_Position = uVP * uModel * vec4(position, 1);
    vNormal = normalize(uModel * vec4(normal, 0)).xyz;
    vPos = (uModel * vec4(position, 1)).xyz;
}
