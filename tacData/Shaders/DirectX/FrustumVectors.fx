cbuffer CBufferPerFrame : register( b0 )
{
  row_major matrix View;
  row_major matrix Projection;
  float4 viewSpaceFrustumCorners[ 4 ];
  float far;
  float near;
  float2 gbufferSize;
}

struct ShaderInput
{
  float3 Position : POSITION;
  float3 Normal : NORMAL;
  float2 UVs : TEXCOORD;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;

  //POSITION semantics are stupid
  float3 mViewSpaceFrustumCorner : TEXCOORD1;
};

VS_OUTPUT VS(
  ShaderInput input )
{
  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = float4(
      input.Position, 1 );
  float cornerIndex = input.Normal.x;
  output.mViewSpaceFrustumCorner =
    viewSpaceFrustumCorners[ cornerIndex ].xyz;
  return output;
}

struct PS_OUTPUT
{
  float3 mViewSpaceFrustumCorner : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mViewSpaceFrustumCorner =
   input.mViewSpaceFrustumCorner;
  return output;
}

