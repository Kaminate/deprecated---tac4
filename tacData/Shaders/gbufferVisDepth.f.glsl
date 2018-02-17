#version 420

in vec2 uvs;
out vec4 outColor;
uniform sampler2D quadTexture;
uniform float n;
uniform float f;

float VerticalLine( vec2 uvs, float x )
{
  float thickness = 0.01f;
  float result = smoothstep( 0, 1, abs( uvs.x - x ) / thickness );
  return result;
}

float ToCameraZ( float depth01, float n, float f )
{
  float A = ( -n - f ) / ( f - n );
  float B = ( -2.0 * n * f ) / ( f - n );
  float zNDC = depth01 * 2.0 - 1.0;
  float zView = -B / ( zNDC + A );

  return zView; // NOTE: range is [ -n, -f ]
}

void main()
{
  float sampled = texture2D(quadTexture, uvs).x;

  outColor = vec4( 0, 0, 0, 1 );

  if( uvs.x < 0.5 )
  {
    // linear
    float camZ = ToCameraZ( sampled, n, f );
    outColor += ( -camZ - n ) / ( f - n );
  }
  else
  {
    // nonlinear
    outColor += vec4( sampled, sampled, sampled, 1.0 );
  }
  outColor *= vec4( VerticalLine( uvs, 0.5f ), 1, 1, 1);
}

