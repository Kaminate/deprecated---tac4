cbuffer ConstantBuffer : register( b0 )
{
  row_major matrix World;
  row_major matrix View;
  row_major matrix Projection;
}
cbuffer AnimatedMatrixBuffer
{
  row_major matrix transform[ 100 ];
}

struct VS_OUTPUT
{
  float4 Pos : SV_POSITION;
  float3 worldPos : POSITION;
  float3 worldNor : NORMAL;
  float4 Col : COLOR;
};

VS_OUTPUT VS(
  float3 Pos : POSITION,
  float3 Nor : NORMAL,
  float4 Col : COLOR,
  uint4 boneIndexes : BONEINDEX,
  float4 boneWeights : BONEWEIGHT )
{
  VS_OUTPUT output = ( VS_OUTPUT )0;

  float4 P = ( float4 )0;
  for( int i = 0; i < 4; ++i )
  {
    uint index = boneIndexes[ i ];
    float weight = boneWeights[ i ];
    if( weight > 0 )
    {
      P += mul( transform[ index ], float4( Pos, 1.0 ) ) * weight;
    }
  }

  P = mul( World, P );
  output.worldPos = P.xyz;
  P = mul( View, P );
  P = mul( Projection, P );
  output.Pos = P; 
  output.Col = Col;
  output.worldNor = normalize( mul( World, float4( Nor, 0 ) ).xyz );
  // output.Col = float4( output.worldNor, 1.0 );
  return output;
}


float4 PS( VS_OUTPUT input ) : SV_Target
{
  float3 camPos = float3( 0, 10, 20 );

  float3 worldPxToCam = normalize( camPos - input.worldPos );
  float nol = dot( worldPxToCam, input.worldNor );
  float3 outxyz = ( float3 )0;
  outxyz += 0.5 * input.Col.xyz * nol + 0.5 * input.worldNor;
  return float4( outxyz.x, outxyz.y, outxyz.z, 1.0 );
}


