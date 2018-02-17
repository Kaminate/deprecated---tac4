#version 430
layout( location = 0 ) in vec3 position;
layout( location = 2 ) in vec2 uv;

uniform mat4 view;
uniform mat4 proj;
out vec3 cubemapPoint;

void main()
{
  gl_Position = proj * view * vec4( position, 0.0 );
  gl_Position.z = gl_Position.w; 
  cubemapPoint = position;
}
