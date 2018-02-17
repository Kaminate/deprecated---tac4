#version 430
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;

out vec2 blurUVs[ 15 ];
uniform float blurAmount;

void main()
{
  gl_Position = vec4( position, 1.0 );

  // NOTE( N8 ): I'm pretty sure this should just be 1/textureWidth, with no weights
  blurUVs[  0 ] = uv + vec2( -0.028 * blurAmount, 0.0 );
  blurUVs[  1 ] = uv + vec2( -0.024 * blurAmount, 0.0 );
  blurUVs[  2 ] = uv + vec2( -0.020 * blurAmount, 0.0 );
  blurUVs[  3 ] = uv + vec2( -0.016 * blurAmount, 0.0 );
  blurUVs[  4 ] = uv + vec2( -0.012 * blurAmount, 0.0 );
  blurUVs[  5 ] = uv + vec2( -0.008 * blurAmount, 0.0 );
  blurUVs[  6 ] = uv + vec2( -0.004 * blurAmount, 0.0 );
  blurUVs[  7 ] = uv + vec2(  0.000 * blurAmount, 0.0 );
  blurUVs[  8 ] = uv + vec2(  0.004 * blurAmount, 0.0 );
  blurUVs[  9 ] = uv + vec2(  0.008 * blurAmount, 0.0 );
  blurUVs[ 10 ] = uv + vec2(  0.012 * blurAmount, 0.0 );
  blurUVs[ 11 ] = uv + vec2(  0.016 * blurAmount, 0.0 );
  blurUVs[ 12 ] = uv + vec2(  0.020 * blurAmount, 0.0 );
  blurUVs[ 13 ] = uv + vec2(  0.024 * blurAmount, 0.0 );
  blurUVs[ 14 ] = uv + vec2(  0.028 * blurAmount, 0.0 );
}
