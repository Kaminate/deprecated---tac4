#version 430

layout(location = 0) in vec3 position;

layout(location = 0) uniform mat4 world;
uniform mat4 view;
uniform mat4 proj;

// out vec3 vsPos;

void main()
{
  vec4 wsPos4 = world * vec4( position, 1.0 );
  vec4 vsPos4 = view * wsPos4;
  // vsPos = vsPos4.xyz;
  gl_Position = proj * vsPos4;
}
