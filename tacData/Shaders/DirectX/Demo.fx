cbuffer CBufferPerFrame : register( b0 )
{
  row_major matrix View;
  column_major matrix Projection;
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

cbuffer CBufferPerObject : register( b1 )
{
  row_major matrix World;
  float3 Color;
}

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float3 mViewSpacePosition : POSITION;
  float3 mViewSpaceNormal : NORMAL;
  float2 mUVs : TEXCOORD;
};

VS_OUTPUT VS(
    ShaderInput input )
{
  float4 worldSpacePosition =
    mul( World, float4( input.Position, 1 ) );
  float4 viewSpacePosition =
    mul( View, worldSpacePosition );
  float4 clipSpacePosition =
    mul( viewSpacePosition, Projection );

  float4 worldSpaceNormal =
    mul( World, float4( input.Normal, 0 ) );
  float4 viewSpaceNormal =
    mul( View, worldSpaceNormal );

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.mViewSpacePosition = viewSpacePosition.xyz;
  output.mViewSpaceNormal = normalize( viewSpaceNormal.xyz );
  output.mUVs = input.UVs;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor             : SV_Target0;
  float3 mViewSpaceNormal   : SV_Target1;
  float3 mViewSpacePosition : SV_Target2;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;
#if 1
  output.mColor = float4( Color, 1.0f );
  output.mViewSpaceNormal = normalize( input.mViewSpaceNormal );
  output.mViewSpacePosition = input.mViewSpacePosition;
  #endif
  return output;
}

