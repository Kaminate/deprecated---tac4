#include "rail.h"
#include <math.h> // sqrt

void VECScale(
  v3* fromOrTo,
  v3* toOrFrom,
  r32 scale )
{
  toOrFrom->x = fromOrTo->x * scale;
  toOrFrom->y = fromOrTo->y * scale;
  toOrFrom->z = fromOrTo->z * scale;
}
void VECAdd(
  v3* a,
  v3* b,
  v3* ab )
{
  ab->x = a->x + b->x;
  ab->y = a->y + b->y;
  ab->z = a->z + b->z;
}
void VECSubtract(
  v3* a,
  v3* b,
  v3* ab )
{
  ab->x = a->x - b->x;
  ab->y = a->y - b->y;
  ab->z = a->z - b->z;
}
r32 VECSquareDistance(
  v3* positionPrev,
  v3* positionCurr )
{
  v3 prevToCurr;
  VECSubtract( positionCurr, positionPrev, &prevToCurr );
  return LengthSq( prevToCurr );
}

void SwagRailSegmentCompute( 
  SwagRailSegment* railSegmentCurr,
  const SwagRailSegment* railSegmentNext )
{
  r32 distance = 0;
  SwagRailTableEntry* railTableEntry0 =
    &railSegmentCurr->railTable[ 0 ];
  railTableEntry0->ui = 0;
  railTableEntry0->si = 0;

  v3 positionPrev = railSegmentCurr->point;

  // Let index 0 be [0,0]
  // Start setting at index 1
  for( u32 i = 1; i < RAIL_TABLE_SIZE; ++i )
  {
    SwagRailTableEntry* railTableEntry =
      &railSegmentCurr->railTable[ i ];
    railTableEntry->ui = ( r32 )i / ( RAIL_TABLE_SIZE - 1 );

    v3 positionCurr = InterpolateSplineCubicHermite(
      railSegmentCurr->point,
      railSegmentNext->point,
      railSegmentCurr->tangent *
      railSegmentCurr->tangentScale,
      railSegmentNext->tangent *
      railSegmentNext->tangentScale,
      railTableEntry->ui );

    distance += sqrt( VECSquareDistance(
      &positionPrev,
      &positionCurr ) );

    railTableEntry->si = distance;
    positionPrev = positionCurr;
  }
  railSegmentCurr->distance = distance;
}

r32 GetuiFromsi(
  SwagRailSegment* railSegment,
  r32 si )
{
  // look up the distance si in the rail table and get the corr
  // and get the corresponding interpolation parameter ui
  SwagRailTableEntry* entryMax =
    &railSegment->railTable[ RAIL_TABLE_SIZE - 1 ];
  for( u32 i = 0; i < RAIL_TABLE_SIZE; ++i )
  {
    SwagRailTableEntry* railTableEntry = 
      &railSegment->railTable[ i ];

    if( railTableEntry->si > si )
    {
      entryMax = railTableEntry;
      break;
    }
  }

  r32 ui = entryMax->ui;
  if( entryMax != &railSegment->railTable[ 0 ] )
  {
    SwagRailTableEntry* entryMin = entryMax - 1;
    r32 uiMin = entryMin->ui;
    r32 uiMax = entryMax->ui;

    r32 dsi = entryMax->si - entryMin->si;
    if( dsi > 0 )
    {
      r32 t = ( si - entryMin->si ) / dsi;
      ui = uiMin + ( uiMax - uiMin ) * t;
    }
  }
  return ui;
}

void SwagRailPositionUpdate(
  SwagRail* rail,
  SwagRailPosition& railPosition,
  r32 distance )
{
  if( rail && !rail->railSegments.empty() )
  {
    SwagRailSegment* railSegment =
      &rail->railSegments[ railPosition.railSegmentIndex ];

    if( railSegment->distance == 0 )
      return;

    if( railPosition.distanceAlongRailSegment + distance >
      railSegment->distance )
    {
      if(
        !rail->looped &&
        railPosition.railSegmentIndex == rail->railSegments.size() - 2 )
      {
        railPosition.distanceAlongRailSegment = railSegment->distance;
      }
      else
      {
        // wrap forwarsd
        if( ++railPosition.railSegmentIndex == rail->railSegments.size() )
          railPosition.railSegmentIndex = 0;

        distance +=
          railPosition.distanceAlongRailSegment -
          railSegment->distance;
        railPosition.distanceAlongRailSegment = 0;
        SwagRailPositionUpdate( rail, railPosition, distance );
      }
    }
    else if( railPosition.distanceAlongRailSegment + distance < 0 )
    {
      if(
        !rail->looped &&
        railPosition.railSegmentIndex == 0 )
      {
        railPosition.distanceAlongRailSegment = 0;
      }
      else
      {
        // wrap backwards
        railPosition.railSegmentIndex =
          railPosition.railSegmentIndex == 0 ?
          rail->railSegments.size() - 1 :
          railPosition.railSegmentIndex - 1;
        railSegment =
          &rail->railSegments[ railPosition.railSegmentIndex ];
        distance +=
          railPosition.distanceAlongRailSegment +
          railSegment->distance;
        railPosition.distanceAlongRailSegment =
          railSegment->distance;

        SwagRailPositionUpdate( rail, railPosition, distance );
      }
    }
    else
    {
      // no wrap
      railPosition.distanceAlongRailSegment += distance;
    }
  }
}

v3 InterpolateSplineCubicHermite(
  v3 startPoint,
  v3 endPoint,
  v3 startTangent,
  v3 endTangent,
  r32 t )
{
  const float tt = t * t;
  const float ttt = t * tt;
  const float a = 2.0f * ttt - 3.0f * tt + 1.0f;
  const float b = ttt - 2.0f * tt + t;
  const float c = -2.0f * ttt + 3.0f * tt;
  const float d = ttt - tt;
  v3 result = 
    a * startPoint +
    b * startTangent +
    c * endPoint +
    d * endTangent;
  return result;
}

void SwagRail::ComputeTangents()
{
  u32 numPoints = railSegments.size();
  u32 lastPointIndex = numPoints - 1;

  // compute tangent
  for( u32 iPoint = 0; iPoint < numPoints; ++iPoint )
  {
    u32 iPointPrev =
      iPoint > 0 ? iPoint - 1 : lastPointIndex;
    u32 iPointNext =
      iPoint < lastPointIndex ? iPoint + 1 : 0;
    SwagRailSegment* railSegmentPrev =
      &railSegments[ iPointPrev ];
    SwagRailSegment* railSegmentCurr =
      &railSegments[ iPoint ];
    SwagRailSegment* railSegmentNext =
      &railSegments[ iPointNext ];

    if( !looped && iPoint == 0 )
    {
      railSegmentCurr->tangent = Normalize(
        railSegmentNext->point -
        railSegmentCurr->point );
    }
    else if( !looped && iPoint == lastPointIndex )
    {
      railSegmentCurr->tangent = Normalize(
        railSegmentCurr->point -
        railSegmentPrev->point );
    }
    else
    {
      v3 toPrev = Normalize(
        railSegmentPrev->point -
        railSegmentCurr->point );
      v3 toNext = Normalize(
        railSegmentNext->point -
        railSegmentCurr->point );
      v3 tangent =  Normalize(
        toNext -
        toPrev );
      railSegmentCurr->tangent = tangent;
    }
  }
}

void SwagRail::ComputeTables()
{
  u32 numPoints = railSegments.size();
  u32 lastPointIndex = numPoints - 1;
  for( u32 iPoint = 0; iPoint < numPoints; ++iPoint )
  {
    u32 iPointNext =
      iPoint < lastPointIndex ? iPoint + 1 : 0;
    SwagRailSegment* railSegmentCurr =
      &railSegments[ iPoint ];
    SwagRailSegment* railSegmentNext =
      &railSegments[ iPointNext ];
    SwagRailSegmentCompute(
      railSegmentCurr,
      railSegmentNext );
  }

}

v3 SwagRail::GetPosition(
  SwagRailPosition railPosition )
{
  v3 result = { 0, 0, 0 };
  if( !railSegments.empty() )
  {
    SwagRailSegment* railSegmentCurr =
      &railSegments[ railPosition.railSegmentIndex ];
    r32 ui = GetuiFromsi(
      railSegmentCurr,
      railPosition.distanceAlongRailSegment );
    u32 railSegmentIndexNext =
      railPosition.railSegmentIndex + 1;
    if( railSegmentIndexNext == railSegments.size() )
      railSegmentIndexNext = 0;
    SwagRailSegment* railSegmentNext =
      &railSegments[ railSegmentIndexNext ];
    result = InterpolateSplineCubicHermite(
      railSegmentCurr->point,
      railSegmentNext->point,
      railSegmentCurr->tangent *
      railSegmentCurr->tangentScale,
      railSegmentNext->tangent *
      railSegmentNext->tangentScale,
      ui );
  }
  return result;
}

SwagRail::SwagRail()
{
  looped = false;
}


v3 SwagRailRider::GetPositionOnRail()
{
  v3 result = { 0, 0, 0 };
  if( rail )
    result = rail->GetPosition( railPosition );
  return result;
}
