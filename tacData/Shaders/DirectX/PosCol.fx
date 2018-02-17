cbuffer ConstantBuffer : register( b0 )
{
  row_major matrix World;
  row_major matrix View;
  row_major matrix Projection;
}
bool InBorder( float borderDist, float3 objectSpacePos )
{
  bool zborder = objectSpacePos.z > ( 2.0 - borderDist ) || objectSpacePos.z < borderDist;
  bool xborder = abs( objectSpacePos.x ) > ( 1.0 - borderDist );
  bool yborder = abs( objectSpacePos.y ) > ( 1.0 - borderDist );
  return ( xborder && zborder ) || ( yborder && zborder ) || ( xborder && yborder );
}

struct VS_OUTPUT
{
  float4 Pos : SV_POSITION;
  float4 Col : COLOR;
  float3 objectSpacePos : POSIITON;
};

VS_OUTPUT VS(
  float4 Pos : POSITION,
  float4 Col : COLOR )
{
  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.objectSpacePos = Pos.xyz;
  output.Pos = mul( World, Pos );
  output.Pos = mul( View, output.Pos );
  output.Pos = mul( Projection, output.Pos );
  output.Col = Col;
  return output;
}


float4 PS( VS_OUTPUT input ) : SV_Target
{
  float3 outxyz;
  if( InBorder( 0.05, input.objectSpacePos ) )
  {
    outxyz.x = outxyz.y = outxyz.z = 0.0;
  }
  else if( InBorder( 0.1 , input.objectSpacePos ) )
  {
    outxyz.x = outxyz.y = outxyz.z = 1.;
  }
  else
  {
    outxyz = input.Col.xyz;
  }
  outxyz /= 100.0;
  outxyz += normalize( input.objectSpacePos );
  return float4( outxyz.x, outxyz.y, outxyz.z, 1.0 );
}



