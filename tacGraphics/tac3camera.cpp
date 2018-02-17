#include "tac3camera.h"

void InitCamera(
  Camera& cam,
  r32 n,
  r32 f,
  r32 a,
  r32 b,
  r32 fovyRad,
  r32 aspect,
  v3 worldUp,
  v3 viewDir,
  v3 pos )
{
  cam.mNear = n;
  cam.mFar = f;
  cam.mA = a;
  cam.mB = b;
  cam.mFieldOfViewYRad = fovyRad;
  cam.mAspectRatio = aspect;
  cam.mWorldSpaceUp = worldUp;
  cam.mPosition = pos;
  cam.mViewDirection = viewDir;

  UpdateCameraMatrixes( cam );
}
void GetCamDirections(
  v3 camView, v3 camWorldUp, v3& right, v3& up )
{
  if( AbsoluteValue( Dot( camView, camWorldUp ) ) > 0.99f )
  {
    v3 arbitrayVector = V3( 1.0f, 0.0f, 0.0f );
    if( AbsoluteValue( Dot( camView, arbitrayVector ) ) > 0.99f )
    {
      arbitrayVector = V3( 0.0f, 1.0f, 0.0f );
    }
    right = Cross(camView, arbitrayVector);
  }
  else
  {
    right = Cross( camView,  camWorldUp );
  }

  right = Normalize( right );
  up = Cross( right,camView );
}
void ComputeViewMatrix(
  m4& mat, v3 camPos, v3 camViewDir, v3 camWorldUp )
{
  v3 camR, camU;
  GetCamDirections( camViewDir, camWorldUp, camR, camU );
  v3 negZ = -camViewDir;

  m4 worldToCamRot = {
    camR.x, camR.y, camR.z, 0,
    camU.x, camU.y, camU.z, 0,
    negZ.x, negZ.y, negZ.z, 0,
    0, 0, 0, 1 };

  m4 worldToCamTra = {
    1, 0, 0, -camPos.x,
    0, 1, 0, -camPos.y,
    0, 0, 1, -camPos.z,
    0, 0, 0, 1 };

  mat = worldToCamRot * worldToCamTra;
}
void ComputeInverseViewMatrix(
  m4& mat, v3 camPos, v3 camViewDir, v3 camWorldUp )
{
  v3 camR, camU;
  GetCamDirections( camViewDir, camWorldUp, camR, camU );
  v3 negZ = -camViewDir;

  m4 camToWorRot = {
    camR.x, camU.x, negZ.x, 0,
    camR.y, camU.y, negZ.y, 0,
    camR.z, camU.z, negZ.z, 0,
    0, 0, 0, 1 };

  m4 camToWorTra = {
    1, 0, 0, camPos.x,
    0, 1, 0, camPos.y,
    0, 0, 1, camPos.z,
    0, 0, 0, 1 };

  mat = camToWorTra * camToWorRot;
}
void ComputePerspectiveProjMatrix(
  r32 A,
  r32 B,
  m4& mat,
  r32 mFieldOfViewYRad,
  r32 mAspectRatio )
{

  //                                                        [ x y z ] 
  //                              [x'y'z']             +-----+
  //                                            +-----/      |
  //                                     +-----/             |
  //                              +-----/                    |
  //                       +-----/     |                     x or y
  //                +-----/          x' or y'                |
  //         +-----/   \               |                     |
  //  +-----/     theta |              |                     |
  // /---------------------------------+---------------------+
  //
  // <-------------------------- z -------------------------->


  // Note( N8 ):           
  // P' (PROJ)  PROJ                         P( CAM )
  //           [sX * x / -z] = / w = [sX * x] = [sx 0  0 0][x]
  //           [sY * y / -z] = / w = [sY * y] = [0  sy 0 0][y]
  //           [(az+b) / -z] = / w = [az + b] = [0  0  a b][z]
  //           [1          ] = / w = [-z    ] = [0  0 -1 0][1]
  //
  // sX = d / (pW / 2) = cot(theta) / aspectRatio
  // sY = d / (pH / 2)

  float theta = mFieldOfViewYRad / 2.0f;
  float cotTheta = 1.0f / tan( theta );

  // sX, sY map to -1, 1
  float sX  =  cotTheta / mAspectRatio;
  float sY = cotTheta;


  mat = M4(
    sX, 0, 0, 0,  
    0, sY, 0, 0,  
    0, 0, A, B,   
    0, 0, -1, 0 );
}

void ComputeInversePerspectiveProjMatrix(
  r32 A,
  r32 B,
  m4& mat,
  r32 mFieldOfViewYRad,
  r32 mAspectRatio )
{
  // http://allenchou.net/2014/02/game-math-how-to-eyeball-the-inverse-of-a-matrix/
  float theta = mFieldOfViewYRad / 2.0f;
  float cotTheta = 1.0f / tan(theta);
  float sX  =  cotTheta / mAspectRatio; // maps x to -1, 1
  float sY = cotTheta; // maps y to -1, 1


  mat = M4(
    1.0f/sX, 0, 0, 0,
    0, 1.0f/sY, 0, 0,
    0, 0, 0, -1,
    0, 0, 1.0f/B, A/B);
}

void UpdateCameraMatrixes( Camera& cam )
{
  ComputeViewMatrix(
    cam.mViewMatrix,
    cam.mPosition,
    cam.mViewDirection,
    cam.mWorldSpaceUp );
  ComputeInverseViewMatrix(
    cam.mInvViewMatrix,
    cam.mPosition,
    cam.mViewDirection,
    cam.mWorldSpaceUp );
  ComputePerspectiveProjMatrix(
    cam.mA,
    cam.mB,
    cam.mPerspectiveProjMatrix,
    cam.mFieldOfViewYRad,
    cam.mAspectRatio );
  ComputeInversePerspectiveProjMatrix(
    cam.mA,
    cam.mB,
    cam.mInvPerspectiveProjMatrix,
    cam.mFieldOfViewYRad,
    cam.mAspectRatio );
}

