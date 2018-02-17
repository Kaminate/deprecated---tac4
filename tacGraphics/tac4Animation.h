#pragma once
#include "tacLibrary/tacPlatform.h"
#include "tacLibrary/tacMath.h"
#include <vector>

struct TacNode
{
  FixedString< 32 > name;
  std::vector< u32 > childIndexes;
  m4 transformation;
};

struct TacKeyFrame
{
  float time;
  v3 translation;
  v4 rotationQuaternion;
};

struct TacTrack
{
  u32 nodeIndex;
  std::vector< TacKeyFrame > keyFrames;
};

struct TacAnimation
{
  // NOTE( N8 ): each track affects a single bone
  std::vector< TacTrack > tracks;
  FixedString< 32 > name;
  float duration;
};

struct TacBone
{
  // NOTE( N8 ): mesh space to bone space
  m4 offset; 
  FixedString< 32 > name;
  u32 nodeIndex;
};
struct TacModelInstance;
struct TacAnimationDriver
{
  virtual ~TacAnimationDriver();
  TacModelInstance* myModelInstance;
  TacAnimationDriver( TacModelInstance* myModelInstance );

  // Set the final matrixes in the modelInstance to
  // send to the gpu, given a weighted array of animations.
  // NOTE( N8 ): animation blending not supported.
  void SetFinalPose(
    const TacAnimation** animations,
    const r32* times,
    const r32* weights,
    u32 num );
  virtual void Update( float dt ) = 0;
  virtual void OnGui(){};
};

