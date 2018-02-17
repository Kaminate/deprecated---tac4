#pragma once
#include "tacLibrary\tacPlatform.h"

struct CollisionOutput
{
  b32 mIsColliding;
  v3 mNoramlizedCollisionNormal;
  r32 mPenetrationDist;
  u32 mTri0[ 3 ];
  u32 mTri1[ 3 ];
  v3 mBarycentric;
};
CollisionOutput IsColliding(
  const v3* verts0,
  u32 numVerts0,
  const v3* verts1,
  u32 numVerts1 );
 