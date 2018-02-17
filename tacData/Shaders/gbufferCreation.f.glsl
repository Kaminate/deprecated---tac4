#version 430

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
in vec2 uvs;
in vec3 wsNormal;
in vec3 wsPos;
out vec3 outTextures[ 3 ];

void main()
{
  outTextures[ 0 ] = texture2D( diffuseTexture, uvs ).xyz;
  outTextures[ 1 ] = ( wsNormal + 1.0 ) / 2.0;
  outTextures[ 2 ] = texture2D( specularTexture, uvs ).xyz;

#if 0 // generate normals
  outTextures[ 1 ] /= 1000.0;
  vec3 u = dFdx( wsPos );
  vec3 v = dFdy( wsPos );
  vec3 n = normalize( cross( u, v ) );
  outTextures[ 1 ] += ( n + 1.0 ) / 2.0;
#endif
}
