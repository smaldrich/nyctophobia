#version 330 core

out vec4 color;

in vec3 vPos;

uniform vec4 uColor;

void main()
{
    // vec3 lightDir = normalize(vPos - uLightOrigin);
    // color = uColor + (0.1 * length(lightDir));
    color = uColor;
};
