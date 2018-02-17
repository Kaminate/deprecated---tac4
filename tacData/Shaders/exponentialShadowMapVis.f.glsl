#version 430

uniform float esmCValue;
uniform sampler2D shadowMap;

in vec2 uvs;
out vec4 outColor;

void main()
{
  float shadowMapValue = texture2D( shadowMap, uvs ).r;

  float depth01 = log( shadowMapValue ) / esmCValue;
  outColor = vec4( depth01, 0, 0, 1.0 );

  // NOTE( N8 ): 
  //   because this is linear, 
  //   if nothing shows up, or 
  //   if it's all red, 
  //   make sure the far plane is short( ie: 20 )
  //   instead of far( ie: 1000 )
}
