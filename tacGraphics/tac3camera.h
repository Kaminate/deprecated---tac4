#pragma once
#include "tacLibrary\tacPlatform.h"


struct Viewport
{
  //       +------------------+(screenW, screenH)
  //       |                  |
  //       |                  |
  //   ----+-----+---------+  |
  //   ^   |     |         |  |
  //   |   |     |  .  .   |  |
  //   h   |     |         |  |
  //   |   |     |  \__/   |  |
  //   v   |     |         |  |
  //   ----+-----+---------+  |
  //   ^   |     |         |  |
  //   y   |     |         |  |
  //   v   |     |         |  |
  //   ----+-----+---------+--+
  //  (0,0)|     |         |
  //       |     |         |
  //       |     |         |
  //       |<-x->|<---w--->|

  u32 x;
  u32 y;
  u32 w;
  u32 h;
};

struct Camera
{
  m4 mViewMatrix;
  m4 mInvViewMatrix;
  m4 mPerspectiveProjMatrix;
  m4 mInvPerspectiveProjMatrix;
  r32 mNear;
  r32 mFar;
  r32 mA;
  r32 mB;
  r32 mFieldOfViewYRad;
  r32 mAspectRatio;
  v3 mPosition;
  v3 mWorldSpaceUp;
  v3 mViewDirection;
};
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
  v3 pos );
void GetCamDirections(
  v3 camView, v3 camWorldUp, v3& right, v3& up );
void ComputeViewMatrix(
  m4& mat, v3 camPos, v3 camViewDir, v3 camWorldUp );
void ComputeInverseViewMatrix(
  m4& mat, v3 camPos, v3 camViewDir, v3 camWorldUp );
void ComputePerspectiveProjMatrix(
  r32 a, r32 b, m4& mat, r32 mFieldOfViewYRad, r32 mAspectRatio );
void ComputeInversePerspectiveProjMatrix(
  r32 a, r32 b, m4& mat, r32 mFieldOfViewYRad, r32 mAspectRatio );
void UpdateCameraMatrixes( Camera& cam );


