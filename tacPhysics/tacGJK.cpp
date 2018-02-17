#include "tacGJK.h"
#include <map>
#include <vector>
#include <algorithm>

      v3 Project( v3 onto, v3 of )
      {
        r32 l = LengthSq( onto );
        TacAssert( l > 0.001f );
        onto /= SquareRoot( l );
        v3 result = Dot( of, onto ) * onto;
        return result;
      };

struct BarycentricLineInput
{
  r32 point;
  r32 points[ 2 ];
};
v2 BarycentricLine( const BarycentricLineInput& input )
{
  v2 result;
  result.x =
    input.point - input.points[ 1 ] /
    ( input.points[ 0 ] - input.points[ 1 ] );
  result.y = 1.0f - result.x;
  return result;
}

struct BarycentricTriangleInput
{
  v2 point;
  v2 points[ 3 ];
};
v3 BarycentricTriangle( const BarycentricTriangleInput& input )
{
  v3 barycentricCoordinates;
  v2 c0 = input.points[ 0 ] - input.points[ 2 ];
  v2 c1 = input.points[ 1 ] - input.points[ 2 ];
  m2 m =
  {
    c0.x, c1.x,
    c0.y, c1.y,
  };
  m = Inverse( m );
  barycentricCoordinates.xy = m * ( input.point - input.points[ 2 ] );
  barycentricCoordinates.z =
    1.0f -
    barycentricCoordinates.x -
    barycentricCoordinates.y;
  return barycentricCoordinates;
}

struct BarycentricTetrahedronInput
{
  v3 point;
  v3 points[ 4 ];
};
v4 BarycentricTetrahedron( const BarycentricTetrahedronInput& input )
{
  v4 barycentricCoordinates;
  ///////////////
  // Variables //
  ///////////////
  // S: input.point
  // S0: input.points[ 0 ]
  // S1: input.points[ 1 ]
  // S2: input.points[ 2 ]
  // S3: input.points[ 3 ]
  // W0: barycentricCoordinates[ 0 ]
  // W1: barycentricCoordinates[ 1 ]
  // W2: barycentricCoordinates[ 2 ]
  // W3: barycentricCoordinates[ 3 ]
  ///////////
  // Given //
  ///////////
  // S0 * w0 + S1 * w1 + S2 * w2 + S3 * w3 = S
  // w0 + w1 + w2 + w3 = 1
  //////////
  // Math //
  //////////
  // w3 = 1 - w0 - w1 - w2
  // S0 * w0 + S1 * w1 + S2 * w2 + S3 * ( 1 - w0 - w1 - w2 ) = S
  // w0 * ( S0 - S3 ) + w1 * ( S1- S3 ) + w2 * ( S2 - S3 ) + S3 = S
  // [ S0 - S3 | S1 - S3 | S2 - S3 ][ W0 | W1 | W2 ]T = [ S - S3 ]
  v3 c0 = input.points[ 0 ] - input.points[ 3 ];
  v3 c1 = input.points[ 1 ] - input.points[ 3 ];
  v3 c2 = input.points[ 2 ] - input.points[ 3 ];
  m3 m =
  {
    c0.x, c1.x, c2.x,
    c0.y, c1.y, c2.y,
    c0.z, c1.z, c2.z,
  };
  m = Inverse( m );
  barycentricCoordinates.xyz = m * ( input.point - input.points[ 3 ] );
  barycentricCoordinates.w =
    1.0f -
    barycentricCoordinates.x -
    barycentricCoordinates.y -
    barycentricCoordinates.z;
  return barycentricCoordinates;
}

b32 IsCorrectTetrahedronOrientation( v3 a, v3 b, v3 c, v3 d )
{
  // Two possible orientations,
  //   
  //   orientation 1        orientation 2: 
  //   d is in front of     d is behind
  //   ( ab x ac )          ( ab x ac ) 
  //            b                c
  //           /|\              /|\
  //          / | \            / | \
  //         /  |  d   --->   /  |  d
  //        /   | /          /   | /
  //       /    |/          /    |/
  //      a-----c          a-----b
  //
  // Ensure orientation 2
  //
  v3 ab = b - a;
  v3 ac = c - a;
  v3 ad = d - a;
  v3 abc = Cross( ab, ac );
  b32 result = Dot( abc, ad ) < 0;
  return result;
}

typedef s32 EPAIndexType;
const EPAIndexType EPANullIndex = -1;
#define DefineEPAHandle( type ) struct EPA##type##Handle { EPAIndexType mIndex; };
DefineEPAHandle( Point );
DefineEPAHandle( HalfEdge );
DefineEPAHandle( Face );
struct EPAPoint
{
  v3 mPosition;
  u32 mLargestIndex0;
  u32 mLargestIndex1;
  EPAHalfEdgeHandle mHalfEdgeHandle;
  EPAPointHandle mNextInList;
};
struct EPAHalfEdge
{
  EPAPointHandle mFromPointHandle;
  EPAPointHandle mToPointHandle;
  EPAFaceHandle mFaceHandle;
  EPAHalfEdgeHandle mTwinHalfEdgeHandle;
  EPAHalfEdgeHandle mNextInFace;
  EPAHalfEdgeHandle mNextInList;
};
struct EPAFace
{
  EPAPointHandle mP0CCW_TEMPORARY;
  EPAPointHandle mP1CCW_TEMPORARY;
  EPAPointHandle mP2CCW_TEMPORARY;
  EPAHalfEdgeHandle mHalfEdgeHandle;
  v4 mNormalizedPlane;
  EPAFaceHandle mNextInList;
};
struct EPAMesh
{
  typedef u32 EPAKey;
  std::map< EPAKey, EPAHalfEdgeHandle > mHalfEdgeMap;
  std::vector< EPAHalfEdge > mHalfEdges;
  std::vector< EPAPoint > mPoints;
  std::vector< EPAFace > mFaces;
  EPAPoint& GetPointChecked( EPAPointHandle pointHandle )
  {
    TacAssertIndex( pointHandle.mIndex, mPoints.size() );
    EPAPoint& point = mPoints[ pointHandle.mIndex ];
    return point;
  }
  EPAHalfEdge& GetHalfEdgeChecked( EPAHalfEdgeHandle halfEdgeHandle )
  {
    TacAssertIndex( halfEdgeHandle.mIndex, mHalfEdges.size() );
    EPAHalfEdge& halfEdge = mHalfEdges[ halfEdgeHandle.mIndex ];
    return halfEdge;
  }
  EPAFace& GetFaceChecked( EPAFaceHandle faceHandle )
  {
    TacAssertIndex( faceHandle.mIndex, mFaces.size() );
    EPAFace& face = mFaces[ faceHandle.mIndex ];
    return face;
  }
  EPAFaceHandle mAliveFaces;
  EPAFaceHandle mDeadFaces;
  EPAPointHandle mAlivePoints;
  EPAPointHandle mDeadPoints;
  EPAHalfEdgeHandle mAliveHalfEdges;
  EPAHalfEdgeHandle mDeadHalfEdges;
  EPAKey GetKey(
    EPAPointHandle from,
    EPAPointHandle to )
  {
    EPAKey key =
      ( from.mIndex << 16 ) |
      ( to.mIndex << 0 );
    return key;
  }
  EPAHalfEdgeHandle FindHalfEdge( EPAKey key )
  {
    EPAHalfEdgeHandle halfEdgeIndex = { EPANullIndex };
    auto it = mHalfEdgeMap.find( key );
    if( it != mHalfEdgeMap.end() )
    {
      halfEdgeIndex = ( *it ).second;
    }
    return halfEdgeIndex;
  }
  EPAHalfEdgeHandle FindHalfEdge(
    EPAPointHandle from,
    EPAPointHandle to )
  {
    EPAHalfEdgeHandle halfEdgeIndex = FindHalfEdge( GetKey( from, to ) );
    return halfEdgeIndex;
  }
  EPAHalfEdgeHandle AddHalfEdge(
    EPAPointHandle from,
    EPAPointHandle to )
  {
    EPAKey key = GetKey( from, to );
    EPAHalfEdgeHandle halfEdgeHandle = FindHalfEdge( key );
    if( halfEdgeHandle.mIndex == EPANullIndex )
    {
      if( mDeadHalfEdges.mIndex == EPANullIndex )
      {
        halfEdgeHandle.mIndex = mHalfEdges.size();
        mHalfEdges.resize( mHalfEdges.size() + 1 );
      }
      else
      {
        halfEdgeHandle = mDeadHalfEdges;
        EPAHalfEdge& toRevive = GetHalfEdgeChecked( halfEdgeHandle );
        mDeadHalfEdges = toRevive.mNextInList;
      }
      EPAHalfEdge& halfEdge = GetHalfEdgeChecked( halfEdgeHandle );
      halfEdge.mFromPointHandle = from;
      halfEdge.mToPointHandle = to;
      halfEdge.mFaceHandle.mIndex = EPANullIndex;
      halfEdge.mTwinHalfEdgeHandle.mIndex = EPANullIndex;
      halfEdge.mNextInList = mAliveHalfEdges;
      halfEdge.mNextInFace.mIndex = EPANullIndex;
      mAliveHalfEdges = halfEdgeHandle;
      mHalfEdgeMap[ key ] = halfEdgeHandle;
      EPAPoint& point = GetPointChecked( from );
      if( point.mHalfEdgeHandle.mIndex == EPANullIndex )
      {
        point.mHalfEdgeHandle = halfEdgeHandle;
      }
      EPAHalfEdgeHandle twinIndex = FindHalfEdge( to, from );
      if( twinIndex.mIndex != EPANullIndex )
      {
        halfEdge.mTwinHalfEdgeHandle = twinIndex;
        GetHalfEdgeChecked( twinIndex ).mTwinHalfEdgeHandle = halfEdgeHandle;
      }
    }
    return halfEdgeHandle;
  }
  EPAPointHandle AddPoint( v3 position, u32 largestIndex0, u32 largestIndex1 )
  {
    EPAPointHandle result;
    if( mDeadPoints.mIndex == EPANullIndex )
    {
      result.mIndex = mPoints.size();
      mPoints.resize( mPoints.size() + 1 );
    }
    else
    {
      result = mDeadPoints;
      EPAPoint& toRevive = GetPointChecked( result );
      mDeadPoints = toRevive.mNextInList;
    }
    EPAPoint& point = GetPointChecked( result );
    point.mHalfEdgeHandle.mIndex = EPANullIndex;
    point.mPosition = position;
    point.mLargestIndex0 = largestIndex0;
    point.mLargestIndex1 = largestIndex1;
    point.mNextInList = mAlivePoints;
    mAlivePoints = result;
    return result;
  }
  EPAFaceHandle AddFaceCCW(
    EPAPointHandle p0,
    EPAPointHandle p1,
    EPAPointHandle p2 )
  {
    v3 p0Position = GetPointChecked( p0 ).mPosition;
    v3 p1Position = GetPointChecked( p1 ).mPosition;
    v3 p2Position = GetPointChecked( p2 ).mPosition;
    v3 normal = Normalize( Cross(
      p1Position - p0Position,
      p2Position - p0Position ) );
    EPAHalfEdgeHandle halfEdgeHandle01 = AddHalfEdge( p0, p1 );
    EPAHalfEdgeHandle halfEdgeHandle12 = AddHalfEdge( p1, p2 );
    EPAHalfEdgeHandle halfEdgeHandle20 = AddHalfEdge( p2, p0 );
    EPAHalfEdge& halfEdge01 = GetHalfEdgeChecked( halfEdgeHandle01 );
    EPAHalfEdge& halfEdge12 = GetHalfEdgeChecked( halfEdgeHandle12 );
    EPAHalfEdge& halfEdge20 = GetHalfEdgeChecked( halfEdgeHandle20 );
    halfEdge01.mNextInFace = halfEdgeHandle12;
    halfEdge12.mNextInFace = halfEdgeHandle20;
    halfEdge20.mNextInFace = halfEdgeHandle01;
    EPAFaceHandle faceHandle;
    if( mDeadFaces.mIndex == EPANullIndex )
    {
      faceHandle.mIndex = mFaces.size();
      mFaces.resize( mFaces.size() + 1 );
    }
    else
    {
      faceHandle = mDeadFaces;
      EPAFace& toRevive = GetFaceChecked( faceHandle );
      mDeadFaces = toRevive.mNextInList;
    }
    EPAFace& face = GetFaceChecked( faceHandle );
    face.mNormalizedPlane.xyz = normal;
    face.mNormalizedPlane.w = -Dot( normal, p0Position );
    face.mHalfEdgeHandle = halfEdgeHandle01;
    face.mNextInList = mAliveFaces;
    face.mP0CCW_TEMPORARY = p0;
    face.mP1CCW_TEMPORARY = p1;
    face.mP2CCW_TEMPORARY = p2;
    mAliveFaces = faceHandle;
    halfEdge01.mFaceHandle = faceHandle;
    halfEdge12.mFaceHandle = faceHandle;
    halfEdge20.mFaceHandle = faceHandle;
    return faceHandle;
  }
  EPAFaceHandle ClosestFace()
  {
    TacAssert( mAliveFaces.mIndex != EPANullIndex );
    EPAFaceHandle closestFaceHandle = mAliveFaces;
    EPAFace& front = GetFaceChecked( closestFaceHandle );
    r32 closestDistance = -front.mNormalizedPlane.w;
    TacAssert( closestDistance >= 0 );
    EPAFaceHandle faceHandle = front.mNextInList;
    while( faceHandle.mIndex != EPANullIndex )
    {
      EPAFace& face = GetFaceChecked( faceHandle );
      r32 distance = -face.mNormalizedPlane.w;
      TacAssert( distance >= 0 );
      if( distance < closestDistance )
      {
        closestDistance = distance;
        closestFaceHandle = faceHandle;
      }
      faceHandle = face.mNextInList;
    }
    return closestFaceHandle;
  }
  void ChangeHalfEdgeOrDie( EPAPointHandle pointHandle, EPAFaceHandle deadFaceHandle )
  {
    EPAPoint& point = GetPointChecked( pointHandle );
    EPAHalfEdgeHandle halfEdgeHandle = mAliveHalfEdges;
    while( halfEdgeHandle.mIndex != EPANullIndex )
    {
      EPAHalfEdge& halfEdge = GetHalfEdgeChecked( halfEdgeHandle );
      if(
        halfEdge.mFromPointHandle.mIndex == pointHandle.mIndex &&
        halfEdge.mFaceHandle.mIndex != deadFaceHandle.mIndex )
      {
        point.mHalfEdgeHandle = halfEdgeHandle;
        break;
      }
      halfEdgeHandle = halfEdge.mNextInList;
    }

    EPAHalfEdge& halfEdge = GetHalfEdgeChecked( point.mHalfEdgeHandle );
    if( halfEdge.mFaceHandle.mIndex == deadFaceHandle.mIndex )
    {
      // remove from alive list
      EPAPointHandle prevAlivePointHandle;
      prevAlivePointHandle.mIndex = EPANullIndex;
      EPAPointHandle alivePointHandle = mAlivePoints;
      while( alivePointHandle.mIndex != pointHandle.mIndex )
      {
        EPAPoint& curPoint = GetPointChecked( alivePointHandle );
        prevAlivePointHandle = alivePointHandle;
        alivePointHandle = curPoint.mNextInList;
      }
      if( prevAlivePointHandle.mIndex == EPANullIndex )
      {
        mAlivePoints = point.mNextInList;
      }
      else
      {
        GetPointChecked( prevAlivePointHandle ).mNextInList = point.mNextInList;
      }
      // add to dead list
      point.mNextInList = mDeadPoints;
      mDeadPoints = pointHandle;
    }
  }
  void KillHalfEdge( EPAHalfEdgeHandle halfEdgeHandle )
  {
    EPAHalfEdge& halfEdge = GetHalfEdgeChecked( halfEdgeHandle );
    if( halfEdge.mTwinHalfEdgeHandle.mIndex != EPANullIndex )
    {
      EPAHalfEdge& twin = GetHalfEdgeChecked( halfEdge.mTwinHalfEdgeHandle );
      twin.mTwinHalfEdgeHandle.mIndex = EPANullIndex;
    }
    EPAHalfEdgeHandle index = mAliveHalfEdges;
    EPAHalfEdgeHandle prevAliveIndex;
    prevAliveIndex.mIndex = EPANullIndex;
    while( index.mIndex != halfEdgeHandle.mIndex )
    {
      EPAHalfEdge& curHalfEdge = GetHalfEdgeChecked( index );
      prevAliveIndex = index;
      index = curHalfEdge.mNextInList;
    }
    // remove from alive list
    if( prevAliveIndex.mIndex == EPANullIndex )
    {
      mAliveHalfEdges = halfEdge.mNextInList;
    }
    else
    {
      GetHalfEdgeChecked( prevAliveIndex ).mNextInList = halfEdge.mNextInList;
    }
    // add to dead list
    halfEdge.mNextInList = mDeadHalfEdges;
    mDeadHalfEdges = halfEdgeHandle;

    // remove from map
    EPAKey key = GetKey( halfEdge.mFromPointHandle, halfEdge.mToPointHandle );
    auto it = mHalfEdgeMap.find( key );
    TacAssert( it != mHalfEdgeMap.end() );
    mHalfEdgeMap.erase( it );
    //auto it = mHalfEdgeMap.begin();
    //while( it != mHalfEdgeMap.end() )
    //{
    //  if( ( *it ).second.mIndex == halfEdgeHandle.mIndex )
    //  {
    //    break;
    //  }
    //  else
    //  {
    //    ++it;
    //  }
    //}
    //if( it == mHalfEdgeMap.end() )
    //{
    //  TacInvalidCodePath;
    //}
    //else
    //{
    //  mHalfEdgeMap.erase( it );
    //}
  }
  void DebugPrintInfo()
  {
    OutputDebugString( "---INFO-------------------\n" );
    OutputDebugString( "---POINTS---\n" );
    {
      EPAPointHandle pointHandle = mAlivePoints;
      while( pointHandle.mIndex != EPANullIndex )
      {
        EPAPoint& point = GetPointChecked( pointHandle );
        point.mHalfEdgeHandle;
        point.mPosition;
        OutputDebugString( VA(
          "Point %i: HalfEdge %i, Position( %2.2f, %2.2f, %2.2f )\n",
          pointHandle.mIndex,
          point.mHalfEdgeHandle.mIndex,
          point.mPosition.x,
          point.mPosition.y,
          point.mPosition.z ) );
        pointHandle = point.mNextInList;
      }
    }
    OutputDebugString( "---HALF EDGES---\n" );
    {
      EPAHalfEdgeHandle halfEdgeHandle = mAliveHalfEdges;
      while( halfEdgeHandle.mIndex != EPANullIndex )
      {
        EPAHalfEdge& halfEdge = GetHalfEdgeChecked( halfEdgeHandle );

        OutputDebugString( VA(
          "HalfEdge %i, Face %i, ( From %i to %i ), Twin %i, Next in face %i\n",
          halfEdgeHandle.mIndex,
          halfEdge.mFaceHandle.mIndex,
          halfEdge.mFromPointHandle.mIndex,
          halfEdge.mToPointHandle.mIndex,
          halfEdge.mTwinHalfEdgeHandle.mIndex,
          halfEdge.mNextInFace.mIndex ) );
        halfEdgeHandle = halfEdge.mNextInList;
      }
    }
    OutputDebugString( "---FACES---\n" );
    {
      EPAFaceHandle faceHandle = mAliveFaces;
      while( faceHandle.mIndex != EPANullIndex )
      {
        EPAFace& face = GetFaceChecked( faceHandle );
        OutputDebugString( VA(
          "Face %i, points ccw(%i, %i, %i), HalfEdge %i\n",
          faceHandle.mIndex,
          face.mP0CCW_TEMPORARY,
          face.mP1CCW_TEMPORARY,
          face.mP2CCW_TEMPORARY,
          face.mHalfEdgeHandle.mIndex ) );
        faceHandle = face.mNextInList;
      }
    }
  }
  void Update( v3 position, u32 largestIndex0, u32 largestIndex1 )
  {
    DebugPrintInfo();
    OutputDebugString( VA(
      "New Point: ( %2.2f, %2.2f, %2.2f )\n",
      position.x,
      position.y,
      position.z ) );

    // Remove all faces seen by point
    EPAFaceHandle prevAliveIndex = { EPANullIndex };
    EPAFaceHandle index = mAliveFaces;
    while( index.mIndex != EPANullIndex )
    {
      EPAFace& face = GetFaceChecked( index );
      EPAFaceHandle nextIndex = face.mNextInList;
      b32 isFaceSeenByPoint = Dot(
        face.mNormalizedPlane.xyz,
        face.mNormalizedPlane.xyz * face.mNormalizedPlane.w - position ) < 0;
      if( isFaceSeenByPoint )
      {
        // Remove face from alive list
        if( prevAliveIndex.mIndex == EPANullIndex )
        {
          mAliveFaces = face.mNextInList;
        }
        else
        {
          GetFaceChecked( prevAliveIndex ).mNextInList = face.mNextInList;
        }
        // Add face to dead list
        face.mNextInList = mDeadFaces;
        mDeadFaces = index;

        {
          EPAHalfEdgeHandle halfEdgeHandle = face.mHalfEdgeHandle;
          do
          {
            // For each point in the face, if it's halfedge is in the face,
            // change the halfedge if possible, and die otherwise
            EPAHalfEdge& halfEdge = GetHalfEdgeChecked( halfEdgeHandle );
            EPAPoint& to = GetPointChecked( halfEdge.mToPointHandle );
            if( to.mHalfEdgeHandle.mIndex == halfEdge.mNextInFace.mIndex )
            {
              ChangeHalfEdgeOrDie( halfEdge.mToPointHandle, index );
            }
            // Kill all the half edges in this face
            KillHalfEdge( halfEdgeHandle );
            halfEdgeHandle = halfEdge.mNextInFace;
          } while( halfEdgeHandle.mIndex != face.mHalfEdgeHandle.mIndex );
        }
      }
      else
      {
        prevAliveIndex = index;
      }
      index = nextIndex;
    }

    DebugPrintInfo();
    OutputDebugString( VA(
      "New Point: ( %2.2f, %2.2f, %2.2f )\n",
      position.x,
      position.y,
      position.z ) );

    // stich new faces
    EPAPointHandle point = AddPoint( position, largestIndex0, largestIndex1 );

    // find a half edge with no twin
    EPAHalfEdgeHandle twinlessHandle = mAliveHalfEdges;
    while( twinlessHandle.mIndex != EPANullIndex )
    {
      EPAHalfEdge& twinless = GetHalfEdgeChecked( twinlessHandle );
      if( twinless.mTwinHalfEdgeHandle.mIndex == EPANullIndex )
      {
        break;
      }
      else
      {
        twinlessHandle = twinless.mNextInList;
      }
    }
    TacAssert( twinlessHandle.mIndex != EPANullIndex );


    EPAHalfEdge& twinless = GetHalfEdgeChecked( twinlessHandle );
    EPAPointHandle twinlessPointHandle = twinless.mToPointHandle;
    std::vector< EPAPointHandle > edgeLoop;
    edgeLoop.push_back( twinlessPointHandle );
    auto GetNextPoint = [ & ] ( EPAPointHandle previousPointHandle )
    {
      EPAPointHandle newPoint = { EPANullIndex };

      EPAPoint& previouspoint = GetPointChecked( previousPointHandle );
      EPAHalfEdgeHandle halfEdgeHandle = previouspoint.mHalfEdgeHandle;
      do
      {
        const EPAHalfEdge& halfEdge = GetHalfEdgeChecked( halfEdgeHandle );
        if( halfEdge.mTwinHalfEdgeHandle.mIndex == EPANullIndex )
        {
          newPoint = halfEdge.mToPointHandle;
          break;
        }
        else
        {
          const EPAHalfEdge& twin = GetHalfEdgeChecked( halfEdge.mTwinHalfEdgeHandle );
          halfEdgeHandle = twin.mNextInFace;
        }
      } while( halfEdgeHandle.mIndex != previouspoint.mHalfEdgeHandle.mIndex );

      TacAssert( newPoint.mIndex != EPANullIndex );
      return newPoint;
    };
    EPAPointHandle previousPoint = twinlessPointHandle;
    EPAPointHandle newPoint;
    u32 iter = 0;
    while( ( newPoint = GetNextPoint( previousPoint ) ).mIndex != twinlessPointHandle.mIndex )
    {
      edgeLoop.push_back( newPoint );
      previousPoint = newPoint;
      ++iter;
      TacAssert( iter < 1000 );
    }
    for( u32 i = 0; i < edgeLoop.size(); ++i )
    {
      EPAPointHandle p0 = edgeLoop[ i ];
      EPAPointHandle p1 = point;
      EPAPointHandle p2 = edgeLoop[ ( i + 1 ) % edgeLoop.size() ];
      AddFaceCCW( p0, p1, p2 );
    }
  }
  void InitFromTetrahedron(
    v3 a,
    u32 aIndexV0,
    u32 aIndexV1,
    v3 b,
    u32 bIndexV0,
    u32 bIndexV1,
    v3 c,
    u32 cIndexV0,
    u32 cIndexV1,
    v3 d,
    u32 dIndexV0,
    u32 dIndexV1 )
  {
    mAliveFaces.mIndex = EPANullIndex;
    mDeadFaces.mIndex = EPANullIndex;
    mAlivePoints.mIndex = EPANullIndex;
    mDeadPoints.mIndex = EPANullIndex;
    mAliveHalfEdges.mIndex = EPANullIndex;
    mDeadHalfEdges.mIndex = EPANullIndex;
    if( !IsCorrectTetrahedronOrientation( a, b, c, d ) )
    {
      std::swap( b, c );
      std::swap( bIndexV0, cIndexV0 );
      std::swap( bIndexV1, cIndexV1 );
    }
    EPAPointHandle indexA = AddPoint( a, aIndexV0, aIndexV1 );
    EPAPointHandle indexB = AddPoint( b, bIndexV0, bIndexV1 );
    EPAPointHandle indexC = AddPoint( c, cIndexV0, cIndexV1 );
    EPAPointHandle indexD = AddPoint( d, dIndexV0, dIndexV1 );
    AddFaceCCW( indexA, indexB, indexC );
    AddFaceCCW( indexB, indexD, indexC );
    AddFaceCCW( indexA, indexC, indexD );
    AddFaceCCW( indexA, indexD, indexB );
  }
};

struct EPAMesh2
{
  struct EPAPoint2
  {
    v3 mPosition;
    u32 mLargestIndex0;
    u32 mLargestIndex1;
  };
  struct EPATri2
  {
    // ccw
    u32 p0;
    u32 p1;
    u32 p2;
    v3 normalizedNormal;
    r32 absdist;
  };
  struct EPAHalfEdge2
  {
    u32 from;
    u32 to;
  };
  std::vector< EPAPoint2 > mPoints;
  std::vector< EPATri2 > mTriangles;
  u32 AddPoint( v3 pos, u32 index0, u32 index1 )
  {
    u32 index = mPoints.size();
    mPoints.resize( index + 1 );
    EPAPoint2& point = mPoints[ index ];
    point.mPosition = pos;
    point.mLargestIndex0 = index0;
    point.mLargestIndex1 = index1;
    return index;
  }
  u32 AddFaceCCW( u32 p0, u32 p1, u32 p2 )
  {
    u32 index = mTriangles.size();
    mTriangles.resize( index + 1 );
    EPAPoint2& point0 = mPoints[ p0 ];
    EPAPoint2& point1 = mPoints[ p1 ];
    EPAPoint2& point2 = mPoints[ p2 ];
    v3 e1 = point1.mPosition - point0.mPosition;
    v3 e2 = point2.mPosition - point0.mPosition;
    EPATri2& tri = mTriangles[ index ];
    tri.p0 = p0;
    tri.p1 = p1;
    tri.p2 = p2;
    tri.normalizedNormal = Normalize( Cross( e1, e2 ) );
    tri.absdist = AbsoluteValue( Dot( point0.mPosition, tri.normalizedNormal ) );
    return index;
  }
  void InitFromTetrahedron(
    v3 a,
    u32 aIndexV0,
    u32 aIndexV1,
    v3 b,
    u32 bIndexV0,
    u32 bIndexV1,
    v3 c,
    u32 cIndexV0,
    u32 cIndexV1,
    v3 d,
    u32 dIndexV0,
    u32 dIndexV1 )
  {
    if( !IsCorrectTetrahedronOrientation( a, b, c, d ) )
    {
      std::swap( b, c );
      std::swap( bIndexV0, cIndexV0 );
      std::swap( bIndexV1, cIndexV1 );
    }
    mPoints.reserve( 5 );
    u32 indexA = AddPoint( a, aIndexV0, aIndexV1 );
    u32 indexB = AddPoint( b, bIndexV0, bIndexV1 );
    u32 indexC = AddPoint( c, cIndexV0, cIndexV1 );
    u32 indexD = AddPoint( d, dIndexV0, dIndexV1 );
    mTriangles.reserve( 5 );
    AddFaceCCW( indexA, indexB, indexC );
    AddFaceCCW( indexB, indexD, indexC );
    AddFaceCCW( indexA, indexC, indexD );
    AddFaceCCW( indexA, indexD, indexB );
  }
  void Update( v3 pos, u32 index0, u32 index1 )
  {
    u32 iPoint = AddPoint( pos, index0, index1 );
    u32 iTri = 0;
    u32 numTris = mTriangles.size();
    std::map< u32, EPAHalfEdge2 > mHalfEdges;
    while( iTri < numTris )
    {
      EPATri2& tri = mTriangles[ iTri ];
      v3 v = pos - mPoints[ tri.p0 ].mPosition;
      if( Dot( v, tri.normalizedNormal ) > 0 )
      {
        auto UpdateHalfEdge = [ & ] ( u32 from, u32 to )
        {
          auto GetKey = [] ( u32 from, u32 to )
          {
            u32 result = ( from << 16 ) | to;
            return result;
          };
          u32 twinKey = GetKey( to, from );
          auto it = mHalfEdges.find( twinKey );
          if( it == mHalfEdges.end() )
          {
            u32 key = GetKey( from, to );
            EPAHalfEdge2 halfEdge;
            halfEdge.from = from;
            halfEdge.to = to;
            mHalfEdges.insert( std::pair< u32, EPAHalfEdge2 >( key, halfEdge ) );
          }
          else
          {
            mHalfEdges.erase( it );
          }
        };

        UpdateHalfEdge( tri.p0, tri.p1 );
        UpdateHalfEdge( tri.p1, tri.p2 );
        UpdateHalfEdge( tri.p2, tri.p0 );

        // remove the triangle
        tri = mTriangles[ --numTris ];
      }
      else
      {
        ++iTri;
      }
    }
    mTriangles.resize( numTris );
    for( auto& pair : mHalfEdges )
    {
      EPAHalfEdge2& halfEdge = pair.second;
      AddFaceCCW( halfEdge.from, halfEdge.to, iPoint );
    }
  }
  u32 ClosestFace()
  {
    u32 iClosest = 0;
    r32 absDistClosest = mTriangles[ 0 ].absdist;
    for( u32 iTri = 0; iTri < mTriangles.size(); ++iTri )
    {
      EPATri2& tri = mTriangles[ iTri ];
      if( tri.absdist < absDistClosest )
      {
        absDistClosest = tri.absdist;
        iClosest = iTri;
      }
    }
    return iClosest;
  }
};

CollisionOutput IsColliding(
  const v3* verts0,
  u32 numVerts0,
  const v3* verts1,
  u32 numVerts1 )
{
  CollisionOutput result = {};
  
  u32 supportIndexV0[ 4 ] = {};
  u32 supportIndexV1[ 4 ] = {};
  v3 supports[ 4 ] = {};
  u32 numSupports = 0;

  auto Support = [](
    v3 dir,
    const v3* vertexes,
    u32 numVertexes,
    u32& largestIndex )
  {
    TacAssert( numVertexes );
    r32 largestDot = Dot( dir, vertexes[ largestIndex = 0 ] );
    for( u32 i = 1; i < numVertexes; ++i )
    {
      r32 currentDot = Dot( dir, vertexes[ i ] );
      if( currentDot > largestDot )
      {
        largestDot = currentDot;
        largestIndex = i;
      }
    }
  };

  // temp
  static bool once = true;
  if( once )
  {
    once = false;
    OutputDebugString( "GJK test simplex\n" );
    for( int i = 0; i < 8; ++i )
    {
      v3 tempvert = verts0[ i ] - verts1[ 0 ];
      OutputDebugString( VA( "Vertex %i ( %02f, %02f, %02f )\n",
        i,
        tempvert.x,
        tempvert.y,
        tempvert.z ) );
    }
  }

  const r32 collisionEpsilonSq = 0.001f;

  // output
  u32 outVerts0[ 4 ] = {};
  u32 outVerts1[ 4 ] = {};
  u32 outNumVerts = 0;
  r32 outBarycentric[ 4 ] = {};
  v3 outCollisionNormal = {};
  r32 outPenetrationDepth = 0;

  b32 collided = false;
  b32 running = true;
  v3 searchDir = { 1, 0, 0 };
  u32 iter = 0;
  while( running )
  {
    u32 largestIndex0;
    u32 largestIndex1;
    Support( searchDir, verts0, numVerts0, largestIndex0 );
    Support( -searchDir, verts1, numVerts1, largestIndex1 );
    v3 support =
      verts0[ largestIndex0 ] -
      verts1[ largestIndex1 ];
    if( Dot( support, searchDir ) < 0 )
    {
      running = false;
    }
    else
    {
      supports[ numSupports ] = support;
      supportIndexV0[ numSupports ] = largestIndex0;
      supportIndexV1[ numSupports ] = largestIndex1;
      numSupports++;

      const v3 o = {};
      switch( numSupports )
      {
        case 1:
        {
          v3 a = supports[ 0 ];
          v3 ao = o - a;
          searchDir = ao;
          if( LengthSq( ao ) < collisionEpsilonSq )
          {
            collided = true;
            running = false;
          }
        } break;
        case 2:
        {
          v3 b = supports[ 0 ];
          u32 bIndex0 = supportIndexV0[ 0 ];
          u32 bIndex1 = supportIndexV1[ 0 ];
          v3 a = supports[ 1 ];
          u32 aIndex0 = supportIndexV0[ 1 ];
          u32 aIndex1 = supportIndexV1[ 1 ];
          v3 ab = b - a;
          v3 ao = o - a;
          if( Dot( ao, ab ) > 0 )
          {
            v3 bo = o - b;
            v3 ba = -ab;
            if( Dot( bo, ba ) > 0 )
            {
              v3 closestPoint = a + Project( ab, ao );
              searchDir = -closestPoint;
              if( LengthSq( closestPoint ) < collisionEpsilonSq )
              {
                collided = true;
                running = false;
              }
            }
            else
            {
              supports[ 0 ] = b;
              supportIndexV0[ 0 ] = bIndex0;
              supportIndexV1[ 0 ] = bIndex1;
              searchDir = bo;
              numSupports = 1;
              if( LengthSq( bo ) < collisionEpsilonSq )
              {
                collided = true;
                running = false;
              }
            }
          }
          else
          {
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndex0;
            supportIndexV1[ 0 ] = aIndex1;
            searchDir = ao;
            numSupports = 1;
            if( LengthSq( ao ) < collisionEpsilonSq )
            {
              collided = true;
              running = false;
            }
          }
        } break;
        case 3:
        {
          v3 c = supports[ 0 ];
          u32 cIndexV0 = supportIndexV0[ 0 ];
          u32 cIndexV1 = supportIndexV1[ 0 ];

          v3 b = supports[ 1 ];
          u32 bIndexV0 = supportIndexV0[ 1 ];
          u32 bIndexV1 = supportIndexV1[ 1 ];

          v3 a = supports[ 2 ];
          u32 aIndexV0 = supportIndexV0[ 2 ];
          u32 aIndexV1 = supportIndexV1[ 2 ];

          v3 ao = o - a;
          v3 ab = b - a;
          v3 ac = c - a;

          v3 bo = o - b;
          v3 ba = a - b;
          v3 bc = c - b;

          v3 co = o - c;
          v3 ca = a - c;
          v3 cb = b - c;

          v3 n = Cross( ac, ab );
          v3 abOut = Cross( n, ab );
          v3 acOut = Cross( ac, n );
          v3 bcOut = Cross( n, bc );

          // vertex voronoi region
          if(
            Dot( ao, ab ) < 0 &&
            Dot( ao, ac ) < 0 )
          {
            // a
            numSupports = 1;
            searchDir = ao;
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            if( LengthSq( ao ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if(
            Dot( bo, ba ) < 0 &&
            Dot( bo, bc ) < 0 )
          {
            // b
            numSupports = 1;
            searchDir = bo;
            supports[ 0 ] = b;
            supportIndexV0[ 0 ] = bIndexV0;
            supportIndexV1[ 0 ] = bIndexV1;
            if( LengthSq( bo ) < collisionEpsilonSq )
            {
              collided = true;
              running = false;
            }
          }
          else if(
            Dot( co, ca ) < 0 &&
            Dot( co, cb ) < 0 )
          {
            // c
            numSupports = 1;
            searchDir = co;
            supports[ 0 ] = c;
            supportIndexV0[ 0 ] = cIndexV0;
            supportIndexV1[ 0 ] = cIndexV1;
            if( LengthSq( co ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          // edge voronoi region
          else if( Dot( ao, abOut ) > 0 )
          {
            // edge ab
            numSupports = 2;
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            supports[ 1 ] = b;
            supportIndexV0[ 1 ] = bIndexV0;
            supportIndexV1[ 1 ] = bIndexV1;
            v3 closestPoint = a + Project( ab, ao );
            searchDir = o - closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              collided = true;
              running = false;
            }
          }
          else if( Dot( ao, acOut ) > 0 )
          {
            // ac
            numSupports = 2;
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            supports[ 1 ] = c;
            supportIndexV0[ 1 ] = cIndexV0;
            supportIndexV1[ 1 ] = cIndexV1;
            v3 closestPoint = a + Project( ac, ao );
            searchDir = o - closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              collided = true;
              running = false;
            }
          }
          else if( Dot( bo, bcOut ) > 0 )
          {
            // bc
            numSupports = 2;
            supports[ 0 ] = b;
            supportIndexV0[ 0 ] = bIndexV0;
            supportIndexV1[ 0 ] = bIndexV1;
            supports[ 1 ] = c;
            supportIndexV0[ 1 ] = cIndexV0;
            supportIndexV1[ 1 ] = cIndexV1;
            v3 closestPoint = b + Project( bc, bo );
            searchDir = o - closestPoint;
            if( LengthSq( searchDir ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          // triangle voronoi region
          else
          {
            n = Normalize( n );
            v3 closestPoint = Project( n, a );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              collided = true;
              running = false;
            }
          }

        } break;
        case 4:
        {
          v3 d = supports[ 0 ];
          u32 dIndexV0 = supportIndexV0[ 0 ];
          u32 dIndexV1 = supportIndexV1[ 0 ];
          v3 c = supports[ 1 ];
          u32 cIndexV0 = supportIndexV0[ 1 ];
          u32 cIndexV1 = supportIndexV1[ 1 ];
          v3 b = supports[ 2 ];
          u32 bIndexV0 = supportIndexV0[ 2 ];
          u32 bIndexV1 = supportIndexV1[ 2 ];
          v3 a = supports[ 3 ];
          u32 aIndexV0 = supportIndexV0[ 3 ];
          u32 aIndexV1 = supportIndexV1[ 3 ];


          if( !IsCorrectTetrahedronOrientation( a, b, c, d ) )
          {
            std::swap( b, c );
            std::swap( bIndexV0, cIndexV0 );
            std::swap( bIndexV1, cIndexV1 );
          }

          v3 ao = o - a;
          v3 bo = o - b;
          v3 co = o - c;
          v3 d_o = o - d; // ...

          v3 ab = b - a;
          v3 ac = c - a;
          v3 ad = d - a;

          v3 ba = a - b;
          v3 bc = c - b;
          v3 bd = d - b;

          v3 ca = a - c;
          v3 cb = b - c;
          v3 cd = d - c;

          v3 da = a - d;
          v3 db = b - d;
          v3 dc = c - d;

          v3 abc = Cross( ab, ac );
          v3 bcd = Cross( bd, bc );
          v3 acd = Cross( ac, ad );
          v3 abd = Cross( ad, ab );

          if( // vertex a
            Dot( ao, ab ) < 0 &&
            Dot( ao, ac ) < 0 &&
            Dot( ao, ad ) < 0 )
          {
            // a
            searchDir = ao;
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            numSupports = 1;
            if( LengthSq( a ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // vertex b
            Dot( bo, ba ) < 0 &&
            Dot( bo, bc ) < 0 &&
            Dot( bo, bd ) < 0 )
          {
            // b
            searchDir = bo;
            supports[ 0 ] = b;
            supportIndexV0[ 0 ] = bIndexV0;
            supportIndexV1[ 0 ] = bIndexV1;
            numSupports = 1;
            if( LengthSq( b ) < collisionEpsilonSq )
            {
              collided = true;
              running = false;
            }
          }
          else if( // vertex c
            Dot( co, ca ) < 0 &&
            Dot( co, cb ) < 0 &&
            Dot( co, cd ) < 0 )
          {
            // c
            searchDir = co;
            supports[ 0 ] = c;
            supportIndexV0[ 0 ] = cIndexV0;
            supportIndexV1[ 0 ] = cIndexV1;
            numSupports = 1;
            if( LengthSq( c ) < collisionEpsilonSq )
            {
              collided = true;
              running = false;
            }
          }
          else if( // vertex d
            Dot( d_o, da ) < 0 &&
            Dot( d_o, db ) < 0 &&
            Dot( d_o, dc ) < 0 )
          {
            // d
            searchDir = d_o;
            supports[ 0 ] = d;
            supportIndexV0[ 0 ] = dIndexV0;
            supportIndexV1[ 0 ] = dIndexV1;
            numSupports = 1;
            if( LengthSq( d ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // edge ab
            Dot( ao, Cross( ab, abc ) ) > 0 &&
            Dot( ao, Cross( abd, ab ) ) > 0 )
          {
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            supports[ 1 ] = b;
            supportIndexV0[ 1 ] = bIndexV0;
            supportIndexV1[ 1 ] = bIndexV1;
            numSupports = 2;
            v3 closestPoint = a + Project( ab, ao );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              collided = true;
              running = false;
            }
          }
          else if( // edge ac
            Dot( ao, Cross( abc, ac ) ) > 0 &&
            Dot( ao, Cross( ac, acd ) ) > 0 )
          {
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            supports[ 1 ] = c;
            supportIndexV0[ 1 ] = cIndexV0;
            supportIndexV1[ 1 ] = cIndexV1;
            numSupports = 2;
            v3 closestPoint = a + Project( ac, ao );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // edge ad
            Dot( ao, Cross( ad, abd ) ) > 0 &&
            Dot( ao, Cross( acd, ad ) ) > 0 )
          {
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            supports[ 1 ] = d;
            supportIndexV0[ 1 ] = dIndexV0;
            supportIndexV1[ 1 ] = dIndexV1;
            numSupports = 2;
            v3 closestPoint = a + Project( ad, ao );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // edge bc
            Dot( bo, Cross( bc, abc ) ) > 0 &&
            Dot( bo, Cross( bcd, bc ) ) > 0 )
          {
            supports[ 0 ] = b;
            supportIndexV0[ 0 ] = bIndexV0;
            supportIndexV1[ 0 ] = bIndexV1;
            supports[ 1 ] = c;
            supportIndexV0[ 1 ] = cIndexV0;
            supportIndexV1[ 1 ] = cIndexV1;
            numSupports = 2;
            v3 closestPoint = b + Project( bc, bo );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // edge bd
            Dot( bo, Cross( bd, bcd ) ) > 0 &&
            Dot( bo, Cross( abd, bd ) ) > 0 )
          {
            supports[ 0 ] = b;
            supportIndexV0[ 0 ] = bIndexV0;
            supportIndexV1[ 0 ] = bIndexV1;
            supports[ 1 ] = d;
            supportIndexV0[ 1 ] = dIndexV0;
            supportIndexV1[ 1 ] = dIndexV1;
            numSupports = 2;
            v3 closestPoint = b + Project( bd, bo );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // edge cd
            Dot( co, Cross( cd, acd ) ) > 0 &&
            Dot( co, Cross( bcd, cd ) ) > 0 )
          {
            supports[ 0 ] = c;
            supportIndexV0[ 0 ] = cIndexV0;
            supportIndexV1[ 0 ] = cIndexV1;
            supports[ 1 ] = d;
            supportIndexV0[ 1 ] = dIndexV0;
            supportIndexV1[ 1 ] = dIndexV1;
            numSupports = 2;
            v3 closestPoint = c + Project( cd, co );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // face abc
            Dot( ao, abc ) > 0 )
          {
            numSupports = 3;
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            supports[ 1 ] = b;
            supportIndexV0[ 1 ] = bIndexV0;
            supportIndexV1[ 1 ] = bIndexV1;
            supports[ 2 ] = c;
            supportIndexV0[ 2 ] = cIndexV0;
            supportIndexV1[ 2 ] = cIndexV1;
            abc = Normalize( abc );
            v3 closestPoint = Project( abc, a );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // face abd
            Dot( ao, abd ) > 0 )
          {
            numSupports = 3;
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            supports[ 1 ] = b;
            supportIndexV0[ 1 ] = bIndexV0;
            supportIndexV1[ 1 ] = bIndexV1;
            supports[ 2 ] = d;
            supportIndexV0[ 2 ] = dIndexV0;
            supportIndexV1[ 2 ] = dIndexV1;
            v3 closestPoint = Project( Normalize( abd ), a );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // face acd
            Dot( ao, acd ) > 0 )
          {
            numSupports = 3;
            supports[ 0 ] = a;
            supportIndexV0[ 0 ] = aIndexV0;
            supportIndexV1[ 0 ] = aIndexV1;
            supports[ 1 ] = c;
            supportIndexV0[ 1 ] = cIndexV0;
            supportIndexV1[ 1 ] = cIndexV1;
            supports[ 2 ] = d;
            supportIndexV0[ 2 ] = dIndexV0;
            supportIndexV1[ 2 ] = dIndexV1;
            v3 closestPoint = Project( Normalize( acd ), a );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else if( // face bcd
            Dot( bo, bcd ) > 0 )
          {
            numSupports = 3;
            supports[ 0 ] = b;
            supportIndexV0[ 0 ] = bIndexV0;
            supportIndexV1[ 0 ] = bIndexV1;
            supports[ 1 ] = c;
            supportIndexV0[ 1 ] = cIndexV0;
            supportIndexV1[ 1 ] = cIndexV1;
            supports[ 2 ] = d;
            supportIndexV0[ 2 ] = dIndexV0;
            supportIndexV1[ 2 ] = dIndexV1;
            v3 closestPoint = Project( Normalize( bcd ), b );
            searchDir = -closestPoint;
            if( LengthSq( closestPoint ) < collisionEpsilonSq )
            {
              running = false;
              collided = true;
            }
          }
          else // inside the simplex
          {
            running = false;
            collided = true;


            // get epa triangle
            //const EPAFace& closestFace = mesh.mFaces[ closestFaceHandle.mIndex ];
            //EPAHalfEdgeHandle halfEdgeHandle = closestFace.mHalfEdgeHandle;
            //EPAPoint pointsccw[ 3 ];
            //for( u32 i = 0; i < 3; ++i )
            //{
            //  const EPAHalfEdge& halfEdge = mesh.mHalfEdges[ halfEdgeHandle.mIndex ];
            //  const EPAPoint& point = mesh.mPoints[ halfEdge.mToPointHandle.mIndex ];
            //  pointsccw[ i ] = point;
            //  halfEdgeHandle = halfEdge.mNextInFace;
            //}

            //v3 edge01 = pointsccw[ 1 ].mPosition - pointsccw[ 0 ].mPosition;
            //r32 radius01 = Length( edge01 );
            //v3 edge02 = pointsccw[ 2 ].mPosition - pointsccw[ 0 ].mPosition;
            //r32 radius02 = Length( edge02 );
            //r32 angleTriangle = Angle( edge01, edge02 );

            //v3 projectedOrigin = closestFace.mNormalizedPlane.xyz * closestFace.mNormalizedPlane.w;
            //v3 cornerToOrigin = projectedOrigin - pointsccw[ 0 ].mPosition;
            //r32 angleOrigin = Angle( edge01, cornerToOrigin );

            //// make it a 2d problem
            //BarycentricTriangleInput input;
            //input.point = V2( Cos( angleOrigin ), Sin( angleOrigin ) ) * Length( cornerToOrigin );
            //input.points[ 0 ] = V2( 0.0f, 0.0f );
            //input.points[ 1 ] = V2( 0.0f, radius01 );
            //input.points[ 2 ] = V2( Cos( angleTriangle ), Sin( angleTriangle ) ) * radius02;
            //v3 barycentricTriangleOutput = BarycentricTriangle( input );

            //// collision data
            //v3 verts0p0 = verts0[ pointsccw[ 0 ].mLargestIndex0 ];
            //v3 verts0p1 = verts0[ pointsccw[ 1 ].mLargestIndex0 ];
            //v3 verts0p2 = verts0[ pointsccw[ 2 ].mLargestIndex0 ];

            //v3 verts1p0 = verts1[ pointsccw[ 0 ].mLargestIndex1 ];
            //v3 verts1p1 = verts1[ pointsccw[ 1 ].mLargestIndex1 ];
            //v3 verts1p2 = verts1[ pointsccw[ 2 ].mLargestIndex1 ];

            //r32 penetrationDepth = -closestFace.mNormalizedPlane.w;
            //v3 normal = closestFace.mNormalizedPlane.xyz;

          }
        } break;
        TacInvalidDefaultCase;
      }

      if( ++iter > 32 )
      {
        running = false;
      }
    }
  }

  if( collided )
  {
    // fill all 4 supports with a tetrahedron with all verts on the convex hull
    if( numSupports < 4 )
    {
      v3 dirs[] =
      {
        // cardinal
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },

        // negative cardinal
        { -1.0f, 0.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f },

        // xy
        { 1.0f, 1.0f, 0.0f },
        { -1.0f, 1.0f, 0.0f },
        { 1.0f, -1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f },

        // xz
        { 1.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, -1.0f },
        { -1.0f, 0.0f, 1.0f },
        { -1.0f, 0.0f, -1.0f },

        // yz
        { 0.0f, 1.0f, 1.0f },
        { 0.0f, 1.0f, -1.0f },
        { 0.0f, -1.0f, 1.0f },
        { 0.0f, -1.0f, -1.0f },
      };
      u32 numDirs = ArraySize( dirs );

      u32 iDir = 0;
      while( numSupports < 4 )
      {
        TacAssert( iDir < numDirs );
        v3& dir = dirs[ iDir++ ];
        u32 index0;
        Support( dir, verts0, numVerts0, index0 );
        v3 support = verts0[ index0 ];
        u32 index1;
        Support( -dir, verts1, numVerts1, index1 );
        support -= verts1[ index1 ];
        b32 unique = true;
        for( u32 i = 0; i < numSupports; ++i )
        {
          if(
            supportIndexV0[ i ] == index0 &&
            supportIndexV1[ i ] == index1 )
          {
            unique = false;
            break;
          }
        }
        if( unique )
        {
          supports[ numSupports ] = support;
          supportIndexV0[ numSupports ] = index0;
          supportIndexV1[ numSupports ] = index1;
          ++numSupports;
        }
      }
    }

    EPAMesh2 mesh2;
    mesh2.InitFromTetrahedron(
      supports[ 0 ], supportIndexV0[ 0 ], supportIndexV1[ 0 ],
      supports[ 1 ], supportIndexV0[ 1 ], supportIndexV1[ 1 ],
      supports[ 2 ], supportIndexV0[ 2 ], supportIndexV1[ 2 ],
      supports[ 3 ], supportIndexV0[ 3 ], supportIndexV1[ 3 ] );
    b32 epaRunning = true;
    EPAMesh2::EPATri2* closestFace;
    iter = 0;
    do
    {
      closestFace = &mesh2.mTriangles[ mesh2.ClosestFace() ];
      searchDir = closestFace->normalizedNormal;
      u32 index0;
      u32 index1;
      Support( searchDir, verts0, numVerts0, index0 );
      Support( -searchDir, verts1, numVerts1, index1 );
      v3 support = verts0[ index0 ] - verts1[ index1 ];
      v3 v = support - mesh2.mPoints[ closestFace->p0 ].mPosition;
      float distFromSupportToClosestFace = Dot( v, closestFace->normalizedNormal );
      if( distFromSupportToClosestFace < 0.001f )
      {
        // Done - Cannot expand the closest face
        epaRunning = false;
      }
      else
      {
        mesh2.Update( support, index0, index1 );
      }

      if( iter > 100 )
      {
        epaRunning = false;
      }
    } while( epaRunning );

    result.mNoramlizedCollisionNormal = closestFace->normalizedNormal;
    result.mPenetrationDist = closestFace->absdist;
    EPAMesh2::EPAPoint2& p0 = mesh2.mPoints[ closestFace->p0 ];
    result.mTri0[ 0 ] = p0.mLargestIndex0;
    result.mTri1[ 0 ] = p0.mLargestIndex1;
    EPAMesh2::EPAPoint2& p1 = mesh2.mPoints[ closestFace->p1 ];
    result.mTri0[ 1 ] = p1.mLargestIndex0;
    result.mTri1[ 1 ] = p1.mLargestIndex1;
    EPAMesh2::EPAPoint2& p2 = mesh2.mPoints[ closestFace->p2 ];
    result.mTri0[ 2 ] = p2.mLargestIndex0;
    result.mTri1[ 2 ] = p2.mLargestIndex1;

    v3 e1 = p1.mPosition - p0.mPosition;
    v3 e2 = p2.mPosition - p0.mPosition;
    v3 e2Para = Project( e1, e2 );
    v3 e2Perp = e2 - e2Para;
    // closest point on the face to the origin
    v3 closest =
      closestFace->normalizedNormal *
      closestFace->absdist;
    v3 closestPara = Project( e1, closest );
    v3 closestPerp = closest - closestPara;

    BarycentricTriangleInput input;
    input.point = V2( Length( closestPara ), Length( closestPerp ) );
    input.points[ 0 ] = V2( 0.0f, 0.0f );
    input.points[ 1 ] = V2( Length( e1 ), 0.0f );
    input.points[ 2 ] = V2( Length( e2Para ), Length( e2Perp ) );
    result.mBarycentric = BarycentricTriangle( input );
  }

  result.mIsColliding = collided;
  return result;
}

