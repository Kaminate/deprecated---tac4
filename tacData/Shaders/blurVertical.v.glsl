#version 430
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;

out vec2 blurUVs[ 15 ];
uniform float blurAmount;

void main()
{
  gl_Position = vec4( position, 1.0 );

  blurUVs[  0 ] = uv + vec2( 0.0, -0.028 * blurAmount );
  blurUVs[  1 ] = uv + vec2( 0.0, -0.024 * blurAmount );
  blurUVs[  2 ] = uv + vec2( 0.0, -0.020 * blurAmount );
  blurUVs[  3 ] = uv + vec2( 0.0, -0.016 * blurAmount );
  blurUVs[  4 ] = uv + vec2( 0.0, -0.012 * blurAmount );
  blurUVs[  5 ] = uv + vec2( 0.0, -0.008 * blurAmount );
  blurUVs[  6 ] = uv + vec2( 0.0, -0.004 * blurAmount );
  blurUVs[  7 ] = uv + vec2( 0.0,  0.000 * blurAmount );
  blurUVs[  8 ] = uv + vec2( 0.0,  0.004 * blurAmount );
  blurUVs[  9 ] = uv + vec2( 0.0,  0.008 * blurAmount );
  blurUVs[ 10 ] = uv + vec2( 0.0,  0.012 * blurAmount );
  blurUVs[ 11 ] = uv + vec2( 0.0,  0.016 * blurAmount );
  blurUVs[ 12 ] = uv + vec2( 0.0,  0.020 * blurAmount );
  blurUVs[ 13 ] = uv + vec2( 0.0,  0.024 * blurAmount );
  blurUVs[ 14 ] = uv + vec2( 0.0,  0.028 * blurAmount );
}
