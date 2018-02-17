#include "tacRaycast.h"
#include "imgui\imgui.h" // temp, for bungie programmer test

TacPlane PlaneEquation( const v3& point, const v3& normal )
{
  // todo: should normal be normalized?
  TacPlane result;
  TacAssert( false ); // is this even called? it seems to be wrong
  // previously, result.abc = point, but shouldn't it be normal?
  result.abc = normal;
  result.d = -Dot( point, normal );
  return result;
}

TacRaycastResult RayPlaneIntersection(
  const TacRay& ray,
  const TacPlane& plane )
{
  r32 denominator = Dot( plane.abc, ray.dir );

  TacRaycastResult result = {};
  result.collided = ( denominator != 0.0f );
  if( result.collided )
  {
    r32 numerator = -plane.d - Dot( plane.abc, ray.pos );
    result.dist = numerator / denominator;
    if( result.dist < 0 )
    {
      result.collided = false;
    }
  }
  return result;
}

TacRaycastResult RaySphereIntersection(
  const TacRay& ray,
  const TacSphere& sphere )
{
  v3 dp = ray.pos - sphere.position;
  TacAssert( IsNormalized( ray.dir ) );
  r32 b = 2.0f * Dot( ray.dir, dp );
  r32 c = Dot( dp, dp ) - sphere.radius * sphere.radius;
  r32 discriminant = b * b - 4.0f * c;

  TacRaycastResult result = {};
  if( discriminant >= 0 )
  {
    r32 sqrtDisciminant = SquareRoot( discriminant );
    r32 t0 = ( -b + sqrtDisciminant ) / 4.0f;
    r32 t1 = ( -b - sqrtDisciminant ) / 4.0f;
    r32 tmin = Minimum( t0, t1 );
    if( tmin > 0 )
    {
      result.collided = true;
      result.dist = tmin;
    }
    else
    {
      result.collided = false;
      r32 tmax = Maximum( t0, t1 );
      if( tmax > 0 )
      {
        result.rayStartedInsideObject = true;
      }
    }
  }
  return result;
}

TacRaycastResult RayTriangleIntersection(
  const TacRay& ray,
  const v3& p0,
  const v3& p1,
  const v3& p2 )
{
  v3 v0 = p0;
  v3 v1 = p1 - p0;
  v3 v2 = p2 - p0;

  v3 b = ray.pos - v0;

  v3 p = Cross( ray.dir, v2 );
  v3 q = Cross( b, v1 );

  r32 denominator = Dot( p, v1 );
  TacRaycastResult result = {};
  result.collided = denominator > 0;
  if( result.collided )
  {
    r32 inverseDenominator = 1.0f / denominator;
    result.dist = Dot( q, v2 ) * inverseDenominator;
    r32 u = Dot( p, b ) * inverseDenominator;
    r32 v = Dot( q, ray.dir ) * inverseDenominator;
    result.collided =
      result.dist > 0 &&
      u > 0 &&
      v > 0 &&
      u + v <= 1;
  }
  return result;
}


void SphereSphereIntersect(
  v3 p0,
  float l0,
  v3 p1,
  float l1,
  SphereSphereIntersectResult* result )
{
  v3 v = p1 - p0;
  float distsq = LengthSq( v );
  if( distsq < EPSILON )
  {
    // The two spheres are concentric, and will never intersect
    // ( unless they are both the same sphere, that as no intersection )
    // This also avoids divison by 0 when normalizing v
    result->mIsIntersecting = false;
    return;
  }

  float d = std::sqrt( distsq );
  v /= d;

  float l0sq = Square( l0 );
  float l1sq = Square( l1 );
  float x = ( l0sq - l1sq + distsq ) / ( 2.0f * d );
  float discriminant = l0sq - Square( x );
  ImGui::DragFloat( "disriminant", &discriminant );

  if( discriminant < 0 )
  {
    // The spheres are too close or too far apart to intersect
    result->mIsIntersecting = false;
    return;
  }

  result->mNormal = v;
  result->mPosition = p0 + v * x;
  result->mRadius = std::sqrt( discriminant );
  result->mIsIntersecting = true;
}

