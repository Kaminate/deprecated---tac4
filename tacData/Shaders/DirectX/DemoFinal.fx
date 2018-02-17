cbuffer CBufferPerFrame : register( b0 )
{
  row_major matrix View;
  column_major matrix Projection;
  float4 viewSpaceFrustumCorners[ 4 ];
  float far;
  float near;
  float2 gbufferSize;
  float4 clearcolor;
}

struct ShaderInput
{
  float3 Position : POSITION;
  float3 Normal : NORMAL;
  float2 UVs : TEXCOORD;
};

#define pi    3.1415926
#define twopi 6.2831853
#define ssaoKernelSize 8
#define ssaoB 0.0001 // minimum distance for obscurance
#define ssaoE 0.0001 // epsilon, prevents underflow and light leaks
#define ssaoU 0.10 // falloff
cbuffer CBufferSSAO : register( b1 )
{
  float4 ssaoKernel[ ssaoKernelSize ];
  float2 randomVectorsSize;
  float ssaoRadius;
}
Texture2D gbufferColor             : register( t0 );
Texture2D gbufferViewSpaceNormal   : register( t1 );
Texture2D randomVectors            : register( t2 );
Texture2D viewSpacePositionTexture : register( t3 );

sampler linearSampler : register( s0 );
sampler pointSampler  : register( s1 );

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float2 mUVs : TEXCOORD0;
};

VS_OUTPUT VS( ShaderInput input )
{
  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = float4( input.Position, 1 );
  // convert from opengl to directx
  output.mUVs.x = input.UVs.x;
  output.mUVs.y = 1.0 - input.UVs.y;
  return output;
}

float4 PS( VS_OUTPUT input ) : SV_Target
{
  float4 result = float4( 0, 0, 0, 1 );

  float3 viewSpaceNormalPixel = gbufferViewSpaceNormal.Sample(
    pointSampler,
    input.mUVs ).xyz;
  float3 viewSpacePositionPixel
  =  viewSpacePositionTexture.Sample(
    pointSampler,
    input.mUVs ).xyz;

  matrix tbn;
  float3 randomVector = randomVectors.Sample(
    pointSampler,
    input.mUVs *
    gbufferSize / randomVectorsSize ).xyz;
  float3 tangent = normalize( cross(
    randomVector,
    viewSpaceNormalPixel.xyz ) );
  float3 bitangent = cross( viewSpaceNormalPixel, tangent );

  tbn =
  matrix(
    float4( tangent, 0.0 ),
    float4( bitangent, 0.0 ),
    float4( viewSpaceNormalPixel, 0.0 ),
    float4( 0, 0, 0, 1 ) );

  float sum = 0;
  #if 1
  for( int i = 0; i < ssaoKernelSize; ++i )
  {
    float3 viewSpacePositionSample;
    float linearNormalizedDepthSample;
    float2 UVsSample;
    {
      float3 myvec =
      viewSpacePositionPixel +
      mul( ssaoKernel[ i ], tbn ).xyz * ssaoRadius;

      // get the uv of this sample by projection
      float4 clipSpacePositionSample = mul(
        float4( myvec, 1 ),
        Projection );
      float2 NDCSpacePositionSample =
      clipSpacePositionSample.xy /
      clipSpacePositionSample.w;
      UVsSample = ( NDCSpacePositionSample + 1 ) * 0.5;
      UVsSample.y = 1 - UVsSample.y;

      viewSpacePositionSample = viewSpacePositionTexture.Sample(
        // linearSampler,
        pointSampler,
        UVsSample ).xyz;
    }

    float3 v = viewSpacePositionSample - viewSpacePositionPixel;
    sum +=
    max( 0, dot( v, viewSpaceNormalPixel ) ) *
    step( length( v ), ssaoRadius ) /
    max( ssaoU * ssaoU, dot( v, v ) );
  }
  #endif

  float a = 1.0f - ( twopi * ssaoU * sum / ssaoKernelSize );
  float4 color = gbufferColor.Sample(
    // linearSampler,
    pointSampler,
    input.mUVs );


  float3 colorbot = float3(
    172 / 255.0,
    203 / 255.0,
    231 / 255.0 );
  float3 colortop = float3(
    33 / 255.0,
    80 / 255.0,
    152 / 255.0 );

  float3 colorpixel = lerp( colortop, colorbot, input.mUVs.y * 3 );
  if( dot( viewSpacePositionPixel, viewSpacePositionPixel ) < 1 )
  {
    color.xyz = colorpixel;
  }

  result = color * a;
  result = lerp(
    result,
    float4( colorpixel, 1.0 ),
    // float4( 1, 1, 1, 1 ),
    ( -viewSpacePositionPixel.z ) / far );

  return result;
}

