#include "tacPhysics.h"

u32 TacPhysics::GetDimensions()
{
  return mNumParticles * 6;
}

void TacPhysics::Update( float dt )
{
  EulerStep( dt );

  // Recalculate manifolds between every box-box collision
  mNumManifolds = 0;
  for( u32 iBox = 0; iBox < mNumBoxes; ++iBox )
  {
    TacPhysicsBox& box0 = mBoxes[ iBox ];
    for( u32 jBox = iBox + 1; jBox < mNumBoxes; ++jBox )
    {
      TacPhysicsBox& box1 = mBoxes[ jBox ];
      CollisionOutput isColliding = IsColliding(
        box0.mWorldSpaceBoxVertexes,
        TacPhysicsBox::sNumBoxVertexes,
        box1.mWorldSpaceBoxVertexes,
        TacPhysicsBox::sNumBoxVertexes );
      if( isColliding.mIsColliding )
      {
        TacAssertIndex( mNumManifolds, sMaxManifolds );
        TacPhysicsManifold& manifold = mManifolds[ mNumManifolds++ ];
        manifold.mBox0Index = iBox;
        manifold.mBox1Index = jBox;
        manifold.mIsColliding = isColliding;
      } 
    }
  }

  // todo: particle/box collision
  //for( u32 i = 0; i < mNumParticles; ++i )
  //{
  //  mIsColliding = IsColliding(
  //    mBox.mWorldSpaceBoxVertexes,
  //    TacPhysicsBox::sNumBoxVertexes,
  //    &mParticles[ i ].mPosition,
  //    1 );
  //}
}

void TacPhysics::AddPartile( const TacPhysicsParticle& particle )
{
  TacAssert( mNumParticles < sMaxParticles );
  mParticles[ mNumParticles++ ] = particle;
}

void TacPhysics::AddBox( const TacPhysicsBox & box )
{
  TacAssertIndex( mNumBoxes, sMaxBoxes );
  TacPhysicsBox& slot = mBoxes[ mNumBoxes++ ];
  slot = box;
  slot.RecalculateVertexes();
}

void TacPhysics::GetState( r32 * destination )
{
  for( u32 i = 0; i < mNumParticles; ++i )
  {
    TacPhysicsParticle& particle = mParticles[ i ];
    for( u32 j = 0; j < 3; ++j )
    {
      *destination++ = particle.mPosition[ j ];
    }
    for( u32 j = 0; j < 3; ++j )
    {
      *destination++ = particle.mVelocity[ j ];
    }
  }
}

void TacPhysics::SetState( r32 * source )
{
  for( u32 i = 0; i < mNumParticles; ++i )
  {
    TacPhysicsParticle& particle = mParticles[ i ];
    for( u32 j = 0; j < 3; ++j )
    {
      particle.mPosition[ j ] = *source++;
    }
    for( u32 j = 0; j < 3; ++j )
    {
      particle.mVelocity[ j ] = *source++;
    }
  }
}

void TacPhysics::ZeroForceAccumulators()
{
  for( u32 i = 0; i < mNumParticles; ++i )
  {
    TacPhysicsParticle& particle = mParticles[ i ];
    Zero( particle.mForceAccumulator );
  }
}

void TacPhysics::AccumulateForces()
{
  v3 zero = {};
  if( mGravity != zero )
  {
    for( u32 i = 0; i < mNumParticles; ++i )
    {
      TacPhysicsParticle& particle = mParticles[ i ];
      particle.mForceAccumulator += mGravity * particle.mMass;
    }
  }
}

void TacPhysics::GetStateDerivates( r32 * destination )
{
  for( u32 i = 0; i < mNumParticles; ++i )
  {
    TacPhysicsParticle& particle = mParticles[ i ];
    float inverseMass = 1.0f / particle.mMass;
    for( u32 j = 0; j < 3; ++j )
    {
      // the derivative of position is velocity
      *destination++ = particle.mVelocity[ j ];
    }
    for( u32 j = 0; j < 3; ++j )
    {
      // the derivative of velocity is acceleration
      // f = ma, so a = f * invmass
      *destination++ = particle.mForceAccumulator[ j ] * inverseMass;
    }
  }
}

void TacPhysics::EulerStep( float dt )
{
  ZeroForceAccumulators();
  AccumulateForces();
  // Euler's Method as a truncation of a Taylor Series
  // x( t0 + h ) = x( t0 ) + h * d1( x )( t0 ) + O( h^2 )
  // d1( x ) = v, velocity is the 1st derivate of position
  u32 dimensions = GetDimensions();
  std::vector< r32 > states( dimensions );
  GetState( &states.front() );
  std::vector< r32 > stateDerivatives( dimensions );
  GetStateDerivates( &stateDerivatives.front() );
  for( u32 i = 0; i < dimensions; ++i )
  {
    states[ i ] += dt * stateDerivatives[ i ];
  }
  SetState( &states.front() );
}

void TacPhysicsBox::RecalculateVertexes()
{
  m4 world = M4Transform( mBoxScale, mBoxRot, mBoxPos );

  // calculate the 8 box vertexes
  v3* worldSpaceBoxVertex = mWorldSpaceBoxVertexes;
  for( u32 x = 0; x < 2; ++x )
  {
    r32 xval = x * 2.0f - 1.0f;
    for( u32 y = 0; y < 2; ++y )
    {
      r32 yval = y * 2.0f - 1.0f;
      for( u32 z = 0; z < 2; ++z )
      {
        worldSpaceBoxVertex->x = xval;
        worldSpaceBoxVertex->y = yval;
        worldSpaceBoxVertex->z = z * 2.0f - 1.0f;
        *worldSpaceBoxVertex = ( world * V4( *worldSpaceBoxVertex, 1.0f ) ).xyz;
        ++worldSpaceBoxVertex;
      }
    }
  }
}
