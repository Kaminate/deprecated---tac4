#pragma once
#include "tacLibrary\tacPlatform.h"
#include "tacGJK.h"

struct TacPhysicsParticle
{
  v3 mPosition;
  v3 mVelocity;
  v3 mForceAccumulator;
  r32 mMass;
};

struct TacPhysicsBox
{
  v3 mBoxPos;
  v3 mBoxScale;
  v3 mBoxRot;
  static const u32 sNumBoxVertexes = 8;
  v3 mWorldSpaceBoxVertexes[ sNumBoxVertexes ];
  void RecalculateVertexes();
};

struct TacPhysicsManifold
{
  // should this be an array?
  u32 mBox0Index;
  u32 mBox1Index;
  CollisionOutput mIsColliding; // always true?
};

struct TacPhysics
{
  static const u32 sMaxBoxes = 10;
  TacPhysicsBox mBoxes[ sMaxBoxes ];
  u32 mNumBoxes;

  // if every single box is overlapping, number of collisions is n choose 2
  static const u32 sMaxManifolds = ( sMaxBoxes * ( sMaxBoxes - 1 ) / 2 ) ;
  TacPhysicsManifold mManifolds[ sMaxManifolds ];
  u32 mNumManifolds;

  v3 mGravity;
  static const u32 sMaxParticles = 10;
  TacPhysicsParticle mParticles[ sMaxParticles ];
  u32 mNumParticles;
  u32 GetDimensions();
  void Update( float dt );
  void AddPartile( const TacPhysicsParticle& particle );
  void AddBox( const TacPhysicsBox& box );
  void GetState( r32* destination );
  void SetState( r32* source );
  void ZeroForceAccumulators();
  void AccumulateForces();
  void GetStateDerivates( r32* destination );
  void EulerStep( float dt );
};

