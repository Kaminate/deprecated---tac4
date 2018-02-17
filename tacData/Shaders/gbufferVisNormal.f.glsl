#version 420

in vec2 uvs;
out vec4 outColor;
uniform sampler2D quadTexture;

void main()
{
  vec3 sampledColor = texture2D(quadTexture, uvs).xyz;
  outColor = vec4(sampledColor, 1);
}

