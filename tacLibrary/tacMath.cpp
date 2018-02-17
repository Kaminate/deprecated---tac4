#include "tacMath.h"
#include "tacDefines.h"

bool IsAboutEqual( float a, float b )
{
  return AbsoluteValue( a - b ) < 0.01f;
}

// Common math notation
//            z
//            ^
//            |
//            |       .(xyz)
//            |      /|
//            |-phi-/ |
//            |    /  |
//            |   /   |
//            |  /    |
//            | /     |
//            |/      |
//            +-------|-------+--> y
//           / \      |      /
//          /   \     |     /
//         /     \    |    /
//        /-theta-\   |   /
//       /         \  |  /
//      /           \ | /
//     /             \|/
//    +---------------+
//   /
//  x
//
void SphericalCoordinatesTo(
  r32 x,
  r32 y,
  r32 z,
  r32* radius,
  r32* theta,
  r32* phi )
{
  *radius = SquareRoot( x*x + y*y + z*z );
  *phi = Acos( z / *radius );
  *theta = Atan2( y, x );
}
void SphericalCoordinatesFrom(
  r32 radius,
  r32 theta,
  r32 phi,
  r32* x,
  r32* y,
  r32* z )
{
  r32 sinphi = sin( phi );
  *z = radius * cos( phi );
  *x = radius * cos( theta ) * sinphi;
  *y = radius * sin( theta ) * sinphi;
}

void GetFrameRH( v3 normalizedDir, v3& normalizedtan1, v3& normalizedtan2 )
{
  TacAssert( IsAboutEqual(
    LengthSq( normalizedDir ),
    1.0f ) );

  /////////////////////////////////////////////////////////////////////
  // Erin Catto:                                                     //
  /////////////////////////////////////////////////////////////////////
  // Suppose vector a has all equal components and is a unit vector: //
  // a = (s, s, s)                                                   //
  // Then 3*s*s = 1, s = sqrt(1/3) = 0.57735.                        //
  // This means that at least one component of a unit vector must be //
  // greater or equal to 0.57735.                                    //
  /////////////////////////////////////////////////////////////////////

  if( AbsoluteValue( normalizedDir.x ) >= 0.57735f )
  {
    normalizedtan1 = V3( normalizedDir.y, -normalizedDir.x, 0.0f );
  }
  else
  {
    normalizedtan1 = V3( 0.0f, normalizedDir.z, -normalizedDir.y );
  }

  normalizedtan1 = Normalize( normalizedtan1 );
  normalizedtan2 = Cross( normalizedDir, normalizedtan1 );
}

m4 M4FromM3( const m3& rot )
{
  m4 result;
  for( u32 r = 0; r < 3; ++r )
  {
    for( u32 c = 0; c < 3; ++c )
    {
      result(r,c) = rot(r,c);
    }
  }
  result(0,3) = 0;
  result(1,3) = 0;
  result(2,3) = 0;
  result(3,0) = 0;
  result(3,1) = 0;
  result(3,2) = 0;
  result(3,3) = 1;
  return result;
}

m3 M3RotRadX( float rotRad )
{
  float s = sin( rotRad );
  float c = cos( rotRad );
  return M3(
    1, 0, 0,
    0, c, -s,
    0, s, c);
}
m3 M3RotRadY( float rotRad )
{
  float s = sin( rotRad );
  float c = cos( rotRad );
  return M3(
    c, 0, s,
    0, 1, 0,
    -s, 0, c );
}
m3 M3RotRadZ( float rotRad )
{
  float s = sin( rotRad );
  float c = cos( rotRad );
  return M3(
    c, -s, 0,
    s, c, 0,
    0, 0, 1 );
}
m4 M4Translate( v3 translate )
{
  return M4(
    1, 0, 0, translate.x,
    0, 1, 0, translate.y,
    0, 0, 1, translate.z,
    0, 0, 0, 1 );
}
m4 M4RotRadX( float rotRad )
{
  return M4FromM3( M3RotRadX( rotRad ) );
}
m4 M4RotRadY( float rotRad )
{
  return M4FromM3( M3RotRadY( rotRad ) );
}
m4 M4RotRadZ( float rotRad )
{
  return M4FromM3( M3RotRadZ( rotRad ) );
}
m4 M4Transform( v3 scale, m3 rot, v3 translate )
{
  r32 m00 = scale.x * rot( 0, 0 );
  r32 m01 = scale.y * rot( 0, 1 );
  r32 m02 = scale.z * rot( 0, 2 );

  r32 m10 = scale.x * rot( 1, 0 );
  r32 m11 = scale.y * rot( 1, 1 );
  r32 m12 = scale.z * rot( 1, 2 );

  r32 m20 = scale.x * rot( 2, 0 );
  r32 m21 = scale.y * rot( 2, 1 );
  r32 m22 = scale.z * rot( 2, 2 );

  return M4(
    m00, m01, m02, translate.x,
    m10, m11, m12, translate.y,
    m20, m21, m22, translate.z,
    0, 0, 0, 1 );
}
m4 M4Transform( v3 s, v3 r, v3 t )
{
  if( r == V3( 0.0f, 0.0f, 0.0f ) )
  {
    return M4(
      s.x, 0, 0, t.x,
      0, s.y, 0, t.y,
      0, 0, s.z, t.z,
      0, 0, 0, 1 );
  }
  else
  {
    m3 rot
      = M3RotRadZ( r.z )
      * M3RotRadY( r.y )
      * M3RotRadX( r.x );

    return M4Transform( s, rot, t );
    //return M4(
    //  s.x * rot( 0, 0 ), s.y * rot( 0, 1 ), s.z * rot( 0, 2 ), t.x,
    //  s.x * rot( 1, 0 ), s.y * rot( 1, 1 ), s.z * rot( 1, 2 ), t.y,
    //  s.x * rot( 2, 0 ), s.y * rot( 2, 1 ), s.z * rot( 2, 2 ), t.z,
    //  0, 0, 0, 1 );
  }
}
m4 M4TransformInverse( v3 scale, v3 rotate, v3 translate )
{
  v3 s =
  {
    1.0f / scale.x,
    1.0f / scale.y,
    1.0f / scale.z
  };
  v3 t = -translate;

  if( rotate == V3( 0.0f, 0.0f, 0.0f ) )
  {
    m4 result = M4(
      s.x, 0, 0, s.x * t.x,
      0, s.y, 0, s.y * t.y,
      0, 0, s.z, s.z * t.z,
      0, 0, 0, 1 );
    return result;
  }
  else
  {
    // NOTE( N8 ): this MUST be opposite order of Matrix4::Transform
    // ( Transform goes zyx, so TransformInverse goes xyz )
    m3 r =
      M3RotRadX( -rotate.x ) *
      M3RotRadY( -rotate.y ) *
      M3RotRadZ( -rotate.z );
    float m03 = s.x * ( t.x * r( 0, 0 ) + t.y * r( 0, 1 ) + t.z * r( 0, 2 ) );
    float m13 = s.y * ( t.x * r( 1, 0 ) + t.y * r( 1, 1 ) + t.z * r( 1, 2 ) );
    float m23 = s.z * ( t.x * r( 2, 0 ) + t.y * r( 2, 1 ) + t.z * r( 2, 2 ) );

    m4 result = M4(
      s.x * r( 0, 0 ), s.x * r( 0, 1 ), s.x * r( 0, 2 ), m03,
      s.y * r( 1, 0 ), s.y * r( 1, 1 ), s.y * r( 1, 2 ), m13,
      s.z * r( 2, 0 ), s.z * r( 2, 1 ), s.z * r( 2, 2 ), m23,
      0, 0, 0, 1 );
    return result;
  }
}
m4 M4Scale( v3 scale )
{
  return M4(
    scale.x, 0, 0, 0,
    0, scale.y, 0, 0,
    0, 0, scale.z, 0,
    0, 0, 0, 1 );
}

m3 M3AngleAxis( r32 angleRadians, v3 axisNormalized )
{
  TacAssert( AbsoluteValue( LengthSq( axisNormalized ) - 1.0f ) < 0.001f );

  m3 m;
  r32 c = Cos(angleRadians);
  r32 s = Sin(angleRadians);
  r32 t = 1.0f - c;

  m( 0, 0 ) = c + axisNormalized.x*axisNormalized.x*t;
  m(1,1) = c + axisNormalized.y*axisNormalized.y*t;
  m(2,2) = c + axisNormalized.z*axisNormalized.z*t;

  r32 tmp1 = axisNormalized.x*axisNormalized.y*t;
  r32 tmp2 = axisNormalized.z*s;
  m( 1, 0 ) = tmp1 + tmp2;
  m( 0, 1 ) = tmp1 - tmp2;

  tmp1 = axisNormalized.x*axisNormalized.z*t;
  tmp2 = axisNormalized.y*s;
  m( 2, 0 ) = tmp1 - tmp2;
  m( 0, 2 ) = tmp1 + tmp2;

  tmp1 = axisNormalized.y*axisNormalized.z*t;
  tmp2 = axisNormalized.x*s;
  m(2,1) = tmp1 + tmp2;
  m(1,2) = tmp1 - tmp2;
  return m;
}

void M3ToEuler( m3 m, v3& euler )
{
  float theta, phi, vslash;

  if(
    IsAboutEqual( m( 2, 0 ), 1 ) ||
    IsAboutEqual( m( 2, 0 ), -1 ) )
  {
    theta = -Asin( m( 2, 0 ) );
    r32 costheta = Cos( theta );

    vslash = Atan2(
      m(2,1) / costheta,
      m(2,2) / costheta );

    phi = Atan2(
      m( 1, 0 ) / costheta,
      m( 0, 0 ) / costheta );
  }
  else
  {
    phi = 0; // can be anything
    if( IsAboutEqual( m( 2, 0 ), -1 ) )
    {
      theta = 3.14159f / 2.0f;
      vslash = phi + Atan2( -m( 0, 1 ), m( 0, 2 ) );
    }
    else
    {
      theta = -3.14159f / 2.0f;
      vslash = -phi + Atan2( -m( 0, 1 ), -m( 0, 2 ) );
    }
  }
  euler.x = vslash;
  euler.y = phi;
  euler.z = theta;
}

m3 M3Scale( v3 scale )
{
  m3 result =
  {
    scale.x, 0, 0,
    0, scale.y, 0,
    0, 0, scale.z,
  };
  return result;
}

  void Test (
    const char* origname,
    const m4& orig,
    const m4& test )
  {
    std::stringstream ss;
    ss << "orig " << origname << std::endl;
    OutputDebugString( ss.str().c_str() );
    ss.str("");
    PrintMatrix( orig );

    ss << "test " << origname << std::endl;
    OutputDebugString( ss.str().c_str() );
    ss.str("");
    PrintMatrix( test );

    TacAssert( IsAboutEqual( orig, test ) );
  };

  void Test (
    const char* origname,
    r32 orig,
    r32 test )
  {
    std::stringstream ss;
    ss << "orig " << origname << std::endl;
    ss << orig << std::endl;
    ss << "test " << origname << std::endl;
    ss << test << std::endl;
    OutputDebugString( ss.str().c_str() );
    TacAssert( IsAboutEqual( orig, test ) );
  };

void MatrixUnitTests()
{
  const m4 a = {
    1, 8, 4, 8,
    2, 2, 5, 1,
    1, 6, 8, 0,
    5, 3, 7, 5 };

  const r32 adet = 128.0f;

  const m4 acofactor = {
    -52 , -22 , 23 , 33,
    -344 , -244 , 226 , 174,
    108 , 90 , -65 , -71,
    152 , 84 , -82 , -62
  };

  const m4 acofactortranspose = {
    -52, -344, 108, 152,
    -22, -244, 90, 84,
    23, 226, -65, -82,
    33, 174, -71, -62
  };

  const m4 ainv = acofactortranspose * ( 1.0f / adet );

  const m4 b = {
    3, 3, 9, 0,
    9, 8, 1, 5,
    3, 0, 5, 4,
    6, 2, 1, 7 };

  const m4 ab = {
    135, 83, 45, 112,
    45, 24, 46, 37,
    81, 51, 55, 62,
    93, 49, 88, 78
  };

  const m4 identity = []()
  {
    m4 result;
    Identity( result );
    return result;
  }();

  // wtf the inverse of a 3x4 is another 3x4 o.o mind = blown
  m4 poop = {
    1,2,1,4,
    6,6,7,8,
    9,2,7,6,
    0,0,0,1,
  };
  OutputDebugString("poop\n");
  PrintMatrix( poop );
  OutputDebugString("pee\n");
  m4 pee = Inverse( poop );
  PrintMatrix( pee );


  // tests
  m4 c = a * b;
  Test( "ab", ab, c );

  m4 identityinverse = Inverse( identity );
  Test( "identity inverse", identity, identityinverse );

  m4 test_ainv = Inverse( a );
  Test( "a inverse", ainv, test_ainv );

  r32 test_adet = Determinant( a );
  Test( "a det", adet, test_adet );

  m4 test_acofactor = Cofactor( a );
  Test( "a cofactor", acofactor , test_acofactor );

  m4 test_acofactortranspose = Transpose( test_acofactor );
  Test( "a cofactor transpose", acofactortranspose  , test_acofactortranspose );

  m4 test_ainv2 = test_acofactortranspose * ( 1.0f / test_adet );
  Test( "a inv 2", ainv   , test_ainv2  );
}

void PrintMatrix( const m4& m )
{
  for( u32 r = 0; r < 4; ++r )
  {
    for( u32 c = 0; c < 4 ; ++c )
    {
      std::stringstream ss;
      ss <<
        std::setw( 6 ) <<
        std::fixed << m( r, c ) << " ";
        //std::setprecision( 2 ) <<
        //( m( r, c ) < 0.001 ? 0 : m( r, c ) ) << " ";
      std::cout << ss.str();

      OutputDebugString( ss.str().c_str() );
    }
    std::cout << std::endl;
    OutputDebugString("\n");
  }
}

m3 QuaternionToMatrix3( const TacQuaternion& quat )
{
  r32 xx2 = quat.x * quat.x * 2.0f;
  r32 yy2 = quat.y * quat.y * 2.0f;
  r32 zz2 = quat.z * quat.z * 2.0f;

  r32 xy2 = quat.x * quat.y * 2.0f;
  r32 xz2 = quat.x * quat.z * 2.0f;
  r32 xw2 = quat.x * quat.w * 2.0f;

  r32 yw2 = quat.y * quat.w * 2.0f;
  r32 yz2 = quat.y * quat.z * 2.0f;

  r32 zw2 = quat.z * quat.w * 2.0f;

  m3 result =
  {
    1.0f - ( yy2 + zz2 ),      xy2 + zw2,           xz2 - yw2,
    xy2 - zw2,                 1 - ( xx2 + zz2 ),   yz2 + xw2,
    xz2 + yw2,                 yz2 - xw2,           1 - ( xx2 + yy2 ),
  };
  return result;

}

// interpolates between two quaterinons given t[0,1]
// returns a quaternion
v4 Slerp( v4 q1, v4 q2, r32 t )
{
  // calc cosine theta
  r32 cosom = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

  // adjust signs (if necessary)
  v4 end = q2;
  if( cosom < 0 )
  {
    cosom = -cosom;
    end.x = -end.x;   // Reverse all signs
    end.y = -end.y;
    end.z = -end.z;
    end.w = -end.w;
  }

  // Calculate coefficients
  r32 sclp, sclq;
  if( (1.0f - cosom) > 0.0001f ) // 0.0001 -> some epsillon
  {
    // Standard case (slerp)
    r32 omega, sinom;
    omega = acos( cosom); // extract theta from dot product's cos theta
    sinom = sin( omega);
    sclp  = sin( (1.0f - t) * omega) / sinom;
    sclq  = sin( t * omega) / sinom;
  } else
  {
    // Very close, do linear interp (because it's faster)
    sclp = 1.0f - t;
    sclq = t;
  }

  v4 pOut;
  pOut.x = sclp * q1.x + sclq * end.x;
  pOut.y = sclp * q1.y + sclq * end.y;
  pOut.z = sclp * q1.z + sclq * end.z;
  pOut.w = sclp * q1.w + sclq * end.w;
  return pOut;
}

// returns a matrix3x3 rotation matrix from the input quaternion q
m3 QuaternionToMatrix( v4 q )
{
  m3 result;
  result(0,0) = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
  result(0,1) = 2.0f * (q.x * q.y - q.z * q.w);
  result(0,2) = 2.0f * (q.x * q.z + q.y * q.w);
  result(1,0) = 2.0f * (q.x * q.y + q.z * q.w);
  result(1,1) = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
  result(1,2) = 2.0f * (q.y * q.z - q.x * q.w);
  result(2,0) = 2.0f * (q.x * q.z - q.y * q.w);
  result(2,1) = 2.0f * (q.y * q.z + q.x * q.w);
  result(2,2) = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
  return result;
}

void GetFurthestPoint(
  const v3& point,
  const v3* points,
  unsigned numPoints,
  unsigned& indexFurthest,
  float& distSqFurtest )
{
  TacAssert( numPoints );
  distSqFurtest = DistanceSq( point, points[ indexFurthest = 0 ] );
  for( unsigned indexCur = 1; indexCur < numPoints; ++indexCur )
  {
    float distSqCur = DistanceSq( point, points[ indexCur ] );
    if( distSqCur > distSqFurtest )
    {
      indexFurthest = indexCur;
      distSqFurtest = distSqCur;
    }
  }
}

TacSphere SphereCentroid( v3* points, u32 numPoints )
{
  TacSphere result = {};
  for( u32 iPoint = 0; iPoint < numPoints; ++iPoint )
  {
    v3& point = points[ iPoint ];
    result.position += point;
  }
  result.position /= ( r32 )numPoints;

  u32 farthestIndex;
  r32 farthestDistSq;
  GetFurthestPoint(
    result.position, points, numPoints, farthestIndex, farthestDistSq );
  result.radius = SquareRoot( farthestDistSq );

  return result;
}

TacSphere SphereExtents( v3* points, u32 numPoints )
{
  TacSphere result = {};
  v3 aabbmini;
  v3 aabbmaxi;
  aabbmini = aabbmaxi = points[ 0 ];
  for( u32 iPoint = 1; iPoint < numPoints; ++iPoint )
  {
    v3& point = points[ iPoint ];
    for( u32 iAxis = 0; iAxis < 3; ++iAxis )
    {
      r32& pointVal = point[ iAxis ];
      r32& miniVal = aabbmini[ iAxis ];
      r32& maxiVal = aabbmaxi[ iAxis ];
      if( pointVal < miniVal )
      {
        miniVal = pointVal;
      }
      else if( pointVal > maxiVal )
      {
        maxiVal = pointVal;
      }
    }
  }

  result.position = ( aabbmini + aabbmaxi ) / 2.0f;
  u32 farthestIndex;
  r32 farthestDistSq;
  GetFurthestPoint(
    result.position, points, numPoints, farthestIndex, farthestDistSq );
  result.radius = SquareRoot( farthestDistSq );

  return result;
}

