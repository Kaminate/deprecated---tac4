#version 430

uniform sampler2D depthTexture;
uniform sampler2D normalTexture;
uniform sampler2D aoTexture;

uniform float cameraNear;
uniform float cameraFar;
uniform mat4 invProj;

uniform float gaussWeights9Tap[ 9 ];
in vec2 gaussUVs9Tap[ 9 ];

out float outAO;

// TODO: add #include files
float ToCameraZ( float depth01, float near, float far )
{
  float A = ( -near - far ) / ( far - near );
  float B = ( -2.0 * near * far ) / ( far - near );
  float zNDC = depth01 * 2.0 - 1.0;
  float zView = -B / ( zNDC + A );

  return zView; // NOTE: range is [ -n, -f ]
}
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

void main()
{
  vec2 uvs = gaussUVs9Tap[ 4 ];
  vec3 viewGeoPos = GetViewPos( uvs );
  outAO = 0.0;
  outAO += 0.5;
  float twopi = 6.28318530718;

  vec3 N = texture( normalTexture, uvs ).xyz * 2.0 - 1.0;


  float depth01 = texture( depthTexture, uvs ).r;

  float numeratorSum = 0.0;
  float denomeratorSum = 0.0;
  float variance = 0.01;
  for( int i = 0; i < 9; ++i )
  {
    vec2 uv = gaussUVs9Tap[ i ];
    vec3 Ni = texture( normalTexture, uv ).xyz * 2.0 - 1.0;

    vec3 viewSamplePos = GetViewPos( uv );
    float distSq = pow( viewGeoPos.z - viewSamplePos.z, 2. );

    // float sampleDepth01 = texture( depthTexture, uv ).r;
    // float distSq = pow( depth01 - sampleDepth01, 2. );

    // range kernel: This measures how similar two pixels are in geometry
    // i.e., whether they are on the same surface or across a boundary
    // between two surfaces.
    //  It uses both the normals and the depths (from the eye)
           // to make this judgment.
    float r =
      max( 0.0, dot( Ni, N ) ) *
      ( 1.0 / sqrt( twopi * variance ) ) *
      1.0 / ( exp( distSq / ( 2.0 * variance ) ) );

    // spatial kernel: a Gaussian based on the distance between the
    // two pixels – exactly the same as in the blur from earlier
    // in the semester.
    // This can stored in a pre-calculated table of Gaussian weights.
    float s = gaussWeights9Tap[ i ];

    float i = texture( aoTexture, uv ).r;
    float w = r * s;
    numeratorSum += w * i;
    denomeratorSum += w;
  }

  outAO = numeratorSum / denomeratorSum;
}
