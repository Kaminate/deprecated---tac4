#version 430

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 normal;

layout(location = 0) uniform mat4 world;
layout(location = 3) uniform mat4 invTransWorld;
uniform mat4 view;
uniform mat4 proj;


out vec2 uvs;
out vec3 wsNormal;
out vec3 wsPos;

void main()
{
  vec4 wsPos4 = world * vec4( position, 1.0 );
  wsPos = wsPos4.xyz;
  wsNormal = normalize( ( invTransWorld * vec4( normal, 0.0 ) ).xyz );
  // wsNormal = normal;
  // wsNormal = vec3(0,1,0);

  uvs = uv;
  gl_Position = proj * view * wsPos4;
}
