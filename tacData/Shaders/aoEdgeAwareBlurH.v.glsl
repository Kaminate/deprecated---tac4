#version 430
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;

// out vec2 uvs;

out vec2 gaussUVs9Tap[ 9 ];

// same value for depth, normal, and ao texture
uniform float textureWidth; 

void main()
{
  gl_Position = vec4( position, 1.0 );
  // uvs = uv;

  // this should be a 1.0, but 2 gives a bigger blur.
  // and our shit is so grainy
  float uSize = 2.0 / textureWidth;
  gaussUVs9Tap[ 0 ] = uv + vec2( uSize * -4.0, 0.0 );
  gaussUVs9Tap[ 1 ] = uv + vec2( uSize * -3.0, 0.0 );
  gaussUVs9Tap[ 2 ] = uv + vec2( uSize * -2.0, 0.0 );
  gaussUVs9Tap[ 3 ] = uv + vec2( uSize * -1.0, 0.0 );
  gaussUVs9Tap[ 4 ] = uv + vec2( uSize *  0.0, 0.0 );
  gaussUVs9Tap[ 5 ] = uv + vec2( uSize *  1.0, 0.0 );
  gaussUVs9Tap[ 6 ] = uv + vec2( uSize *  2.0, 0.0 );
  gaussUVs9Tap[ 7 ] = uv + vec2( uSize *  3.0, 0.0 );
  gaussUVs9Tap[ 8 ] = uv + vec2( uSize *  4.0, 0.0 );
}
