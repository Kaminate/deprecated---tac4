#pragma once
#include "tacPlatform.h"

struct TacRay
{
  v3 pos;
  v3 dir;
};

struct TacRaycastResult
{
  b32 collided;
  b32 rayStartedInsideObject;
  r32 dist;
};

struct TacPlane
{
  // plane = ax + by + cz + d = 0 = { a, b, c, d }
  v3 abc;
  r32 d;
};

TacPlane PlaneEquation( const v3& point, const v3& normal );

TacRaycastResult RayPlaneIntersection(
  const TacRay& ray,
  const TacPlane& plane );

TacRaycastResult RaySphereIntersection(
  const TacRay& ray,
  const TacSphere& sphere );

TacRaycastResult RayTriangleIntersection(
  const TacRay& ray,
  const v3& p0,
  const v3& p1,
  const v3& p2 );

struct SphereSphereIntersectResult
{
  // mPosition, mRadius, and mNormal define the circle formed by the
  // intersection of two spheres. These values are only valid when
  // mIsIntersecting is true
  v3 mPosition;
  float mRadius;

  // The normal is in the direction from sphere 0 to sphere 1
  v3 mNormal;

  bool mIsIntersecting;
};

// p0 - position of sphere 0's center
// l0 - length of sphere 0's radius
// p1 - position of sphere 1's center
// l1 - length of sphere 1's radius
// result - contains information about the intersection
void SphereSphereIntersect(
  v3 p0,
  float l0,
  v3 p1,
  float l1,
  SphereSphereIntersectResult* result );
