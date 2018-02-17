#version 430

uniform float shadowMapNear;
uniform float shadowMapFar;

// in vec3 vsPos;
layout( location = 0 ) out float expMap;

// also in gbufferLightingWITHSHADOWS.f.glsl
#define SHADOW_MAP_EXPONENTIAL
uniform float esmCValue;

void main()
{
  // NOTE( N8 ):
  //   clipspace_W = -viewspace_Z
  //   gl_FragCoord.w = 1 / clipspace_W
  //   gl_FragCoord.w = 1 / -viewspace_Z
  //   -1 / gl_FragCoord.w = viewspace_Z <-- a negative number because 
  //                                         view dir is in the 
  //                                         -z direction!

  float distFromCamera = 1.0f / gl_FragCoord.w;
  float dist01 =
    ( distFromCamera - shadowMapNear ) /
    ( shadowMapFar - shadowMapNear );

#ifdef SHADOW_MAP_EXPONENTIAL
  expMap = exp( esmCValue * dist01 );
#else
  expMap = dist01 + (esmCValue / 1000000.0 );
#endif
}
