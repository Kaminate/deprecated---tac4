#pragma once

#include "tacTypes.h"
#include "tacMath.h"
#include <vector>

// Increase for more precision
// Decrease to reduce memory
#define RAIL_TABLE_SIZE 10

struct SwagRailTableEntry
{
  // the interpolation parameter value [ 0, 1 ]
  r32 ui;

  // the total distance along the spline
  // from the rail segment's begin
  // to the position at ui. 
  // [ 0, 1 ] <-- FALSE
  r32 si;
};

struct SwagRailSegment
{
  // the beginning position of the rail segment
  v3 point;

  // ( vector from the previous point to the next point ) / 2
  //v3 halfPrevToNext; 

  v3 tangent;
  float tangentScale;

  // NOTE( N8 )
  //   The distance from this rail segment's point
  //   to the next rail segment's point
  // 
  //   Since this is a spline, this isn't as simple
  //   as the distance between two v3's
  r32 distance;

  // a look up table relating ui to si
  SwagRailTableEntry railTable[ RAIL_TABLE_SIZE ];
};
r32 GetuiFromsi(
  SwagRailSegment* railSegment,
  r32 si );

v3 InterpolateSplineCubicHermite(
  v3 startPoint,
  v3 endPoint,
  v3 startTangent,
  v3 endTangent,
  r32 t );


struct SwagRailPosition
{
  u32 railSegmentIndex;
  r32 distanceAlongRailSegment;
};

struct SwagRail
{
  SwagRail();
  std::vector< SwagRailSegment > railSegments;
  bool looped;
  void ComputeTangents();
  void ComputeTables();
  v3 GetPosition( SwagRailPosition railPosition );
};
void SwagRailPositionUpdate(
  SwagRail* rail,
  SwagRailPosition& railPosition,
  r32 distance );


struct SwagRailRider
{
  SwagRail* rail;
  SwagRailPosition railPosition;
  v3 GetPositionOnRail();
};



