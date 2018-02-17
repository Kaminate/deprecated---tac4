  #version 430

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform sampler2D specularTexture;

uniform vec3 wsLightCenter;
uniform vec3 lightDiffuse;

uniform int viewportX;
uniform int viewportY;
uniform int viewportW;
uniform int viewportH;

uniform float cameraNear;
uniform float cameraFar;

uniform mat4 invProj;
uniform mat4 invView;

uniform float lightRadius;

uniform vec3 wsCamPos;

uniform mat4 lightView;
uniform mat4 lightProj;
uniform float lightNear;
uniform float lightFar;
uniform sampler2D shadowMap;
uniform float esmCValue;

in vec3 wsNor;
out vec3 outColor;

float ToCameraZ( float depth01, float near, float far )
{
  float A = ( -near - far ) / ( far - near );
  float B = ( -2.0 * near * far ) / ( far - near );
  float zNDC = depth01 * 2.0 - 1.0;
  float zView = -B / ( zNDC + A );

  return zView; // NOTE: range is [ -n, -f ]
}

void main()
{
  vec2 uvs;
  uvs.x = ( gl_FragCoord.x - viewportX ) / viewportW;
  uvs.y = ( gl_FragCoord.y - viewportY ) / viewportH;

  // diffuse/specular is in gamma space, so transform it to linear space
  vec3 diffuse = pow( texture( diffuseTexture, uvs ).xyz, vec3( 2.2 ) );
  vec3 specular = pow( texture( specularTexture, uvs ).xyz, vec3( 2.2 ) );

  float depth01 = texture( depthTexture, uvs ).x;
  float vsZ = ToCameraZ( depth01, cameraNear, cameraFar );

  vec4 ndcGeoPos;
  ndcGeoPos.x = uvs.x * 2.0 - 1.0;
  ndcGeoPos.y = uvs.y * 2.0 - 1.0;
  ndcGeoPos.z = depth01 * 2.0 - 1.0;
  ndcGeoPos.w = 1.0;

  vec4 clipGeoPos = ndcGeoPos * -vsZ;
  vec4 viewGeoPos = invProj * clipGeoPos;
  vec4 worldGeoPos = invView * viewGeoPos;

  vec3 wsGeoToLight = wsLightCenter - worldGeoPos.xyz;
  vec3 wsGeoToCamera = wsCamPos - worldGeoPos.xyz;
  vec3 l = normalize( wsGeoToLight );
  vec3 v = normalize( wsGeoToCamera );
  vec3 n = ( texture( normalTexture, uvs ).xyz * 2.0 ) - 1.0;
  vec3 h = normalize( l + v );


  float distSq = dot( wsGeoToLight, wsGeoToLight );
  float radiusSq = lightRadius * lightRadius;
  float attenuation = clamp(
    1.0 - ( distSq / radiusSq ), 
    0.0,
    1.0 );
  attenuation *= attenuation;

  float ndotl = max( dot( n, l ), 0.0 );
  float ndoth = max( dot( n, h ), 0.0 ); // approx dot( r, v )
  float intensity = 1;
  float shinyness = 64;
  float specNormalize = ( shinyness + 4.0 ) / 8.0;
  specNormalize = 1;
  outColor = diffuse * pow( lightDiffuse, vec3( 2.2 ) ) * intensity * ndotl;
  outColor += pow( ndoth, shinyness ) * specular * specNormalize;
  outColor *= attenuation;

  vec4 pixelLightView = lightView * worldGeoPos;
  vec4 pixelLightClip = lightProj * pixelLightView;
  vec3 pixelLightNDC = pixelLightClip.xyz / pixelLightClip.w;
  vec2 pixelLightUVs = ( pixelLightNDC.xy + 1.0 ) / 2.0;
  float shadowMapValue = texture( shadowMap, pixelLightUVs ).r;

  float pixelDistFromLight01 =
    ( -pixelLightView.z - lightNear ) /
    ( lightFar - lightNear );
  float visibility =
    clamp( exp( -esmCValue * pixelDistFromLight01 ) * shadowMapValue, 0, 1 );
  outColor *= visibility;

  // special light effects
  #if 0
  {
    float lightFxIntensity = 0.2;

    //
    // draw where the light intersects geometry
    //
    float distLightToGeo = distance( wsLightCenter, worldGeoPos.xyz );
    // TODO: remove the if
    if(distLightToGeo > lightRadius - 0.05 &&
      distLightToGeo < lightRadius )
    {
      outColor += lightDiffuse * lightFxIntensity  * ndotl;
    }

    //
    // draw a ring around the edges of the sphere
    //
    vec3 wsCamToGeo = normalize( worldGeoPos.xyz - wsCamPos );
    float val = 1.0f - abs( dot( wsNor, -normalize( wsCamToGeo) ) );
    // TODO: remove the if
    if( val > 0.85 )
    {
      outColor += lightDiffuse * val * lightFxIntensity;
    }
  }
  #endif


  //outColor = pow( outColor, vec3( 1.0 / 2.2 ) );
}
