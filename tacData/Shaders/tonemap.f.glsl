#version 430

in vec2 uvs;
out vec4 outColor;
layout( location = 0 ) uniform sampler2D quadTexture;

#define TONEMAP_LINEAR 0 // only gamma correction
#define TONEMAP_REINHARD 0
// Haarm-Peter Duikera's curve
// optimized by Jim Hejl and Richard Burgess-Dawson.
#define TONEMAP_FILMIC 1
#define TONEMAP_UNCHARTED2 0
#define TONEMAP_RED 0

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;
float W = 11.2;
vec3 Uncharted2Tonemap( vec3 x )
{
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
#if TONEMAP_LINEAR
  vec3 texColor = texture( quadTexture, uvs ).xyz;
  texColor *= 1.; // hardcoded exposure adjustment
  vec3 retColor = pow( texColor, vec3( 1.0 / 2.2 ) ); // gamma-correction
  outColor = vec4( retColor, 1.0 );
  // if( texColor.r > 1. )
    // outColor = vec4( 1, 0, 0, 1 );

#elif TONEMAP_REINHARD
  vec3 texColor = texture( quadTexture, uvs ).xyz;
  texColor *= 1; // hardcoded exposure adjustment
  texColor = texColor / ( 1.0 + texColor );
  texColor = pow( texColor, vec3( 1.0 / 2.2 ) ); // gamma-correction
  outColor = vec4( texColor, 1.0 );
#elif TONEMAP_RED
  vec3 texColor = texture( quadTexture, uvs ).xyz;
  texColor /= ( texColor + 1.0 );
  outColor = vec4( 1.0, texColor.xy, 1.0 );
#elif TONEMAP_FILMIC
  vec3 texColor = texture( quadTexture, uvs ).xyz;
  texColor *= 1;  // hardcoded exposure adjustment
  vec3 x = max( vec3( 0.0 ), texColor - 0.004 );
  vec3 retColor =
    ( x * ( 6.2 * x + 0.5 ) ) /
    ( x * ( 6.2 * x + 1.7 ) + 0.06 );
  outColor = vec4( retColor, 1.0 ); // gamma-correction already included
#elif TONEMAP_UNCHARTED2
  vec3 texColor = texture( quadTexture, uvs ).xyz;
  texColor *= 1.0; // hardcoded exposure adjustment

  float exposureBias = 2.0;
  vec3 curr = Uncharted2Tonemap( exposureBias * texColor );

  vec3 whiteScale = vec3( 1.0, 1.0, 1.0 ) / Uncharted2Tonemap( vec3( W ) );
  vec3 color = curr * whiteScale;

  vec3 retColor = pow( color, vec3( 1.0 / 2.2 ) ); // gamma-correction
  outColor = vec4( retColor, 1.0 );
#endif
}
