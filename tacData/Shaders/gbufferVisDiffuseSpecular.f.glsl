#version 430

in vec2 uvs;
out vec4 outColor;
layout( location = 0 ) uniform sampler2D quadTexture;

void main()
{
  outColor = vec4( texture2D( quadTexture, uvs ).xyz, 1.0 );
}
