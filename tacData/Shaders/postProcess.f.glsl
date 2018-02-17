#version 420

in vec2 uvs;
out vec4 outColor;
uniform sampler2D quadTexture;
uniform sampler2D aoTexture;

void main()
{
  // this is probably a good place to do fog, etc

  vec3 texColor = texture( quadTexture, uvs ).xyz;
  float ao = texture( aoTexture, uvs ).r;
  texColor *= ao;

  texColor *= 1;  // hardcoded exposure adjustment

  // filmic tonemap
  vec3 x = max( vec3( 0.0 ), texColor - 0.004 );
  vec3 retColor =
    ( x * ( 6.2 * x + 0.5 ) ) /
    ( x * ( 6.2 * x + 1.7 ) + 0.06 );

  // gamma-correction already included
  outColor = vec4( retColor, 1.0 ); 
}

