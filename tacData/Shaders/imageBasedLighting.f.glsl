#version 430

// uniform sampler2D diffuseTexture;
// uniform sampler2D specularTexture;

// uniform samplerCube skybox;
uniform samplerCube radianceCubemapSampler;
uniform samplerCube irradianceCubemapSampler;
uniform float roughness;

// TODO( N8 ): name this something else..
// maybe make it obvious that its a varying attribute
in vec2 uvs;
in vec3 wsNormal;
in vec3 wsPos;
out vec3 outColor;

const float pi = 3.1415926535897932;
const float twopi = 6.28318530718;
const float oneOverPi = 0.31830988618;
const float piOver2 = 1.57079632679;


// > from naty
// The Fresnel reflectance is the fraction of incoming light that
// is reflected (as opposed to refracted) from an optically flat
// surface of a given substance. It varies based on the light direction
// and the surface (in this case microfacet) normal. Fresnel reflectance
// tells us how much of the light hitting the relevant microfacets
// (the ones facing in the half-angle direction) is reflected.
//
// todo: remove unused arguments
float F( float NoL, float NoV, float NoH, float VoH, float LoH )
{
  float n1 = 1.0003; // air

  float n2 =
    1.486; // marble
    // 3.; // diamond
    // .166; // gold
    // 2 = 2.33; // platinum

  // how much light is reflected when N == incident ray
  float r0 = ( n1 - n2 ) / ( n1 + n2 );
  r0 *= r0;

  // nol will probably work but not as micro-y
  // float r = r0 + ( 1.0 - r0 ) * pow( 1 - NoH, 5 ); // ???
  // float r = r0 + ( 1.0 - r0 ) * pow( 1 - NoV, 5 ); // macro 
  // float r = r0 + ( 1.0 - r0 ) * pow( 1 - VoH, 5 ); // micro

  // herron and naty say this one
  float r = r0 + ( 1.0 - r0 ) * pow( 1 - LoH, 5 ); 

  return r;
}

// The next part of the microfacet BRDF we will discuss is the
// microfacet normal distribution function, or NDF. The NDF gives the
// concentration of microfacet normals pointing in a given direction
// (in this case, the half-angle direction), relative to surface area. The
// NDF determines the size and shape of the highlight.
float D(
    float NoL,
    float NoV,
    float NoH,
    float VoH,
    float roughness )
{
#define D_p 1

#if D_p
  // first function on page 69 of physics of math and shading  by
  // naty hoffman 2014 siggraph
  float shinyness = ( 1 - sqrt( roughness ) ) * 50.0;
  return ( ( shinyness + 2.0 ) / twopi ) * pow( NoH, shinyness );
#endif
}

// todo: ask trent what his g looks like
//
// The geometry or shadowing-masking function gives the chance
// that a microfacet with a given orientation (again, the half-angle
// direction is the relevant one) is lit and visible
// (in other words, not shadowed and/or masked) from the given light and
// view directions.
float G(
    float NoL,
    float NoV,
    float NoH,
    float VoH,
    float roughness )
{
#define G_COOKTORRANCE 1

#if G_COOKTORRANCE
  // http://renderman.pixar.com/view/cook-torrance-shader
  // this is also called Gct in naty hoffman's siggraph 2014
  //   physics of math and shading
  float G = min(1.0, min(
        ( 2.0 * NoH * NoV / VoH ),
        ( 2.0 * NoH * NoL / VoH ) ) );
  return G;
#endif

}

float rand(vec2 co)
{
  return fract(sin(dot(co.xy,vec2(12.9898,78.233))) * 43758.5453);
}

mat3 GenerateYFrame( vec3 y )
{
  vec3 perp = vec3( 0, 0, 1 );
  if( abs( y.x ) < 0.999 )
  {
    perp = vec3( 1, 0, 0 );
  }
  vec3 x = normalize( cross( y, perp ) );
  vec3 z = cross( x, y );
  mat3 transform = mat3( x, y, z );
  return transform;
}

// n - normal
// v - view
vec3 Specular(
    vec3 n,
    vec3 v,
    in float roughness,
    out float kd )
{
  float NoV = max( 0.0, dot( n, v ) );

  kd = 0;
  float runningFresnel = 0;

  vec3 reflectionVector = reflect( -v, n );
  mat3 transform = GenerateYFrame( reflectionVector );
  //mat3 transform = GenerateYFrame( n );

  vec3 specularTerm = vec3( 0, 0, 0 );
  float fresnelSum = 0;

  int sampleCount = 40;
  for( int i = 0; i < sampleCount; ++i )
  {
    float xi0 = rand( vec2( wsPos.x + i, wsPos.y + i ) );
    float xi1 = rand( vec2( wsPos.z + i, wsPos.x + i ) );

    float p = xi0 * twopi;
    float t = acos( sqrt( 1 - xi1 ) );
    vec3 localDir = vec3(
        sin( p ) * sin( t ),
        cos( t ),
        cos( p ) * sin( t ) );

    // vec3 l = ( localDir + 1.0 ) / 2.0;
    // vec3 l = ( n + 1.0 ) / 2.0;
    vec3 l = transform * localDir;
    // vec3 l = n;
    // vec3 l = normalize( n + v );
    // vec3 l = localDir * 2.0;
    // specularTerm += rand( vec2(
    // wsPos.x * i,
    // wsPos.y * i ) );
    // vec3 l = normalize( vec3(
    // 1, 1, 0

    // rand( vec2( wsPos.x, wsPos.y + i ) ),
    // rand( vec2( wsPos.x, 0 ) ),
    // rand( vec2( wsPos.y + i, wsPos.z ) )

    // ) * 2.0 - 1.0 );

    // specularTerm += l;
    // float l = rand( vec2( wsPos.x, wsPos.y + i ) );

    vec3 h = normalize( l + v );  // NoH approx RoV
    float NoL = max( 0.0, dot( n, l ) );
    float NoH = max( 0.0, dot( n, h ) );
    float VoH = max( 0.0, dot( v, h ) );
    float LoH = max( 0.0, dot( l, h ) );

    // specularTerm += g( nol, nov, noh, voh, roughness );
    // specularTerm += D( NoL, NoV, NoH, VoH, roughness );
    // specularTerm += F( NoL, NoV, NoH, VoH, LoH );

    float g = G( NoL, NoV, NoH, VoH, roughness );
    float d = D( NoL, NoV, NoH, VoH, roughness );
    float f = F( NoL, NoV, NoH, VoH, LoH );

    runningFresnel += f;

    // added 0.01 to deal with division by zero
    float fCookTorrance = ( g * d * f ) / ( 4. * NoL * NoV + 0.01 );
    // float fCookTorrance = ( 1.* d * f ) / ( 4. * NoL * NoV + 0.01 );
    // float fCookTorrance = ( g * 1.* f ) / ( 4. * NoL * NoV + 0.01 );
    // float fCookTorrance = ( g * d * 1.) / ( 4. * NoL * NoV + 0.01 );
    specularTerm += fCookTorrance * texture( radianceCubemapSampler, l ).xyz * NoL;

    // specularTerm += g;
    // specularTerm += f;
    // specularTerm += d;

    // specularTerm += (l + 1.) / 2.;
    // specularTerm += l;

    // specularTerm += ( n + 1. ) / 2.;
    // specularTerm += n;

    // specularTerm += ( reflectionVector + 1. ) / 2.;
    // specularTerm += reflectionVector;
  }

  runningFresnel /= sampleCount;
  kd = 1.0 - runningFresnel;
  return specularTerm / float( sampleCount );
}

bool isIsh( vec3 v, float ish )
{
  float epsilon = 0.01;
  return
    abs( v.x - ish ) < epsilon &&
    abs( v.y - ish ) < epsilon &&
    abs( v.z - ish ) < epsilon;
}

// transform pixel postion to worldspace
uniform int viewportX;
uniform int viewportY;
uniform int viewportW;
uniform int viewportH;
uniform float cameraNear;
uniform float cameraFar;
uniform mat4 invProj;
uniform mat4 invView;
uniform vec3 wsCamPos;

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
  vec3 n = normalize( wsNormal );
  {
    outColor = vec3( 0, 0, 0 );
    outColor += texture( irradianceCubemapSampler, n ).xyz;
    outColor += texture( radianceCubemapSampler, n ).xyz;
    outColor += roughness / 100000.0;
    outColor += wsCamPos.x / 100000.0;
    outColor += viewportX / 100000.0;
    outColor += viewportY / 100000.0;
    outColor += viewportW / 100000.0;
    outColor += viewportH / 100000.0;
    outColor += cameraNear / 100000.0;
    outColor += cameraFar / 100000.0;
    outColor += invView[0][0] / 100000.0;
    outColor += invProj[0][0] / 100000.0;
    outColor /= 10000000.0;
  }

  float depth01 = gl_FragCoord.z;
  float vsZ = ToCameraZ( depth01, cameraNear, cameraFar );
  vec4 ndcGeoPos;
  ndcGeoPos.x = ( ( gl_FragCoord.x - viewportX ) / viewportW ) * 2.0 - 1.0;
  ndcGeoPos.y = ( ( gl_FragCoord.y - viewportY ) / viewportH ) * 2.0 - 1.0;
  ndcGeoPos.z = depth01 * 2.0 - 1.0;
  ndcGeoPos.w = 1.0;
  vec4 clipGeoPos = ndcGeoPos * -vsZ;
  vec4 viewGeoPos = invProj * clipGeoPos;
  vec4 worldGeoPos = invView * viewGeoPos;

  vec3 v = normalize( wsCamPos - worldGeoPos.xyz );

  float kd = 0.0;
  vec3 spec = Specular( n, v, roughness, kd );
  vec3 diff = kd * texture( irradianceCubemapSampler, n ).xyz / pi;

  outColor += spec;
  outColor += diff;
  // outColor += texture( irradianceCubemapSampler, n ).xyz;
  // outColor += texture( radianceCubemapSampler, n ).xyz;
}

