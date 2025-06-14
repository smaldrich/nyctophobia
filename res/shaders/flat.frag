#version 330 core

out vec4 color;

in vec3 vNormal;
in vec3 vPos;

uniform vec4 uColor;
uniform vec3 uLightOrigin;

void main()
{
    // vec3 lightDir = normalize(vPos - uLightOrigin);
    // color = uColor + (0.1 * length(lightDir));
    color = uColor;
};
