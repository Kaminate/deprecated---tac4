#version 430

uniform sampler2D quadTexture;
in vec2 blurUVs[ 15 ];
out vec4 outColor;

// By precomputing all the texutre coordinates in the vertex shader,
// we get a speed boost from texture prefetch
// http://xissburg.com/faster-gaussian-blur-in-glsl/

void main()
{
  outColor = vec4( 0.0 );
  outColor += texture2D( quadTexture, blurUVs[  0 ] ) * 0.0044299121055113265;
  outColor += texture2D( quadTexture, blurUVs[  1 ] ) * 0.00895781211794;
  outColor += texture2D( quadTexture, blurUVs[  2 ] ) * 0.0215963866053;
  outColor += texture2D( quadTexture, blurUVs[  3 ] ) * 0.0443683338718;
  outColor += texture2D( quadTexture, blurUVs[  4 ] ) * 0.0776744219933;
  outColor += texture2D( quadTexture, blurUVs[  5 ] ) * 0.115876621105;
  outColor += texture2D( quadTexture, blurUVs[  6 ] ) * 0.147308056121;
  outColor += texture2D( quadTexture, blurUVs[  7 ] ) * 0.159576912161;
  outColor += texture2D( quadTexture, blurUVs[  8 ] ) * 0.147308056121;
  outColor += texture2D( quadTexture, blurUVs[  9 ] ) * 0.115876621105;
  outColor += texture2D( quadTexture, blurUVs[ 10 ] ) * 0.0776744219933;
  outColor += texture2D( quadTexture, blurUVs[ 11 ] ) * 0.0443683338718;
  outColor += texture2D( quadTexture, blurUVs[ 12 ] ) * 0.0215963866053;
  outColor += texture2D( quadTexture, blurUVs[ 13 ] ) * 0.00895781211794;
  outColor += texture2D( quadTexture, blurUVs[ 14 ] ) * 0.0044299121055113265;
}

