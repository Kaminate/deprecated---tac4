#version 430

uniform sampler2D depthTexture;
uniform sampler2D normalTexture;
uniform int viewportX;
uniform int viewportY;
uniform int viewportW;
uniform int viewportH;
uniform float cameraNear;
uniform float cameraFar;
uniform mat4 invProj;
uniform mat4 invView;

out float outAO;

// TODO: add #include support for shaders so this can be in a utility file
float ToCameraZ( float depth01, float near, float far )
{
  float A = ( -near - far ) / ( far - near );
  float B = ( -2.0 * near * far ) / ( far - near );
  float zNDC = depth01 * 2.0 - 1.0;
  float zView = -B / ( zNDC + A );

  return zView; // NOTE: range is [ -n, -f ]
}

float HeavisideStep( float f )
{
  return step( 0.0, f );
}

//uniform vec3 points[ 50 ];

vec3 GetViewPos( vec2 uvs )
{
  float depth01 = texture( depthTexture, uvs ).x;
  float vsZ = ToCameraZ( depth01, cameraNear, cameraFar );
  vec4 ndcGeoPos = vec4(
      uvs.x * 2.0 - 1.0,
      uvs.y * 2.0 - 1.0,
      depth01 * 2.0 - 1.0,
      1.0 );
  vec4 clipGeoPos = ndcGeoPos * -vsZ;
  vec4 viewGeoPos = invProj * clipGeoPos;
  return viewGeoPos.xyz;
}
vec3 GetWorldPos( vec3 viewPos )
{
  vec4 worldPos = invView * vec4( viewPos, 1.0 );
  return worldPos.xyz;
}
vec3 GetWorldPos( vec2 uvs )
{
  vec3 viewPos = GetViewPos( uvs ) ;
  return GetWorldPos( viewPos );
}

float alchemyAOxorHash()
{
  uint x = uint( gl_FragCoord.x );
  uint y = uint( gl_FragCoord.y );
  return 30u * ( x ^ y ) + 10u * x * y;
}

void main()
{
  vec2 uvs = vec2(
      ( gl_FragCoord.x - viewportX ) / viewportW,
      ( gl_FragCoord.y - viewportY ) / viewportH );
  vec3 viewGeoPos = GetViewPos( uvs );
  vec3 worldGeoPos = GetWorldPos( viewGeoPos );

  // worldspace normal
  vec3 N = texture( normalTexture, uvs ).xyz * 2.0 - 1.0;

  // outAO = 0.0;
  // outAO += worldGeoPos.z / 5.0; // colors it by dist
  // outAO += N.z / 1000000.0;
  // outAO /= 10000000.0;

  float R = 0.5; // meters
  float pi = 3.14159265358979323;
  float twopi = 6.28318530718;
  float c = 0.1 * R;
  float cSq = c * c;
  float hMultiplierVS = R;
  float hMultiplierCS = hMultiplierVS * ( 1.0 / invProj[ 0 ][ 0 ] );
  float hMultiplierNDC = hMultiplierCS / -viewGeoPos.z;
  float hMultiplierUV = hMultiplierNDC / 2.0;
  float hMultiplier = hMultiplierUV;

  vec3 P = worldGeoPos;

  // random rotation angle at a pixel to reduce banding
  float phi = alchemyAOxorHash();

  // Image in this paper used r = 0.5m
  float s = 0.5; // scale

  // Image in this paper used k = 0.5m
  float k = 4.0; // contrast
  int n = 9;
  float S = 0;

  // bias distance is the ambient-obscurance analog of shadow map bias:
  //  >increase b to reduce self-shadowing
  //  >decrease b if light leaks into corners
  float b = 0.00001;

  // Choose e large enough to prevent underflow
  // and small enough that corners do not become white from light leaks
  // We found that values within two orders of magnitude of
  // e= 0.0001 produced indistinguishable results.
  float e = 0.0001;

  for( int i = 0; i < n; ++i )
  {
    float alpha = ( i + 0.5 ) / n; // [0,1]
    float h = alpha * hMultiplier;

    // i think herron's formula for theta is wrong
    float theta = twopi * alpha * ( 7.0 ) + phi;
    vec2 PiUVs = uvs + h * vec2( cos( theta ), sin( theta ) );

    vec3 viewPosPi = GetViewPos( PiUVs );
    vec3 Pi = GetWorldPos( viewPosPi );

    vec3 wi = Pi - P;

    float numerator = max( 0, dot( N, wi ) + viewPosPi.z * b );
    float denominator = e + dot( wi, wi );

    S += numerator / denominator;
  }
  float A = pow( max( 0.0, 1 - 2.0 * s * S / n ), k );

  // outAO /= 10000.0;
  outAO = A;
}

