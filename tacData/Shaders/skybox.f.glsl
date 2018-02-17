#version 430
uniform samplerCube cubemap;
in vec3 cubemapPoint;
out vec4 outColor;

void main()
{
  outColor = texture( cubemap, cubemapPoint );
}


