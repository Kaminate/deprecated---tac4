#pragma once
#include "tacLibrary/tacTypes.h"
#include "tacIntrinsics.h"
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>


#define PI32     3.1415926535f
#define TWOPI32  6.2831853071f
#define HALFPI32 1.5707963267f
#define EPSILON 0.0001f

#define DEG2RAD ( PI32 / 180.0f )
#define RAD2DEG ( 180.0f / PI32 )


inline u32 RoundUpToNearestMultiple( u32 val, u32 multiple )
{
  u32 result = ( ( val + multiple - 1 ) / multiple ) * multiple;
  return result;
}

template< typename T>
T Square( T t )
{
  T result = t * t;
  return result;
}

template< typename T >
T Lerp( T r0, T r1, r32 t )
{
  return r0 + ( r1 - r0 ) * t;
}

bool IsAboutEqual( float a, float b );

inline r32 RandReal( r32 r0 = 0.0f, r32 r1 = 1.0f )
{
  r32 t = rand() / ( r32 )RAND_MAX;
  r32 result = Lerp( r0, r1, t );
  return result;
}

inline s32 RandInt( s32 beginInclusive, s32 endExclusive )
{
  return beginInclusive + rand() % ( endExclusive - beginInclusive - 1 );
}

//          z             .(xyz)
//          ^         ___/|
//          |-phi-___/    |
//          | ___/        |
//          |/            |
//          +-------------|----------> y
//         / \___         |   /
//        /      \___     |  /
//       /-theta-    \___ | /
//      /________________\|/
//     /
//    /
//   x
//
// ^ Common math notation
void SphericalCoordinatesTo(
  r32 x,
  r32 y,
  r32 z,
  r32* radius,
  r32* theta,
  r32* phi );
void SphericalCoordinatesFrom(
  r32 radius,
  r32 theta,
  r32 phi,
  r32* x,
  r32* y,
  r32* z );

inline void Spring(
  float &position,
  float &velocity,
  float targetPosition,
  float damping,
  float angularFreq,
  float dt )
{
  const float f = 1.0f + 2.0f * dt * damping * angularFreq;
  const float oo = angularFreq * angularFreq;
  const float hoo = dt * oo;
  const float hhoo = dt * hoo;
  const float detInv = 1.0f / (f + hhoo);
  const float detX = f * position + dt * velocity + hhoo * targetPosition;
  const float detV = velocity + hoo * (targetPosition - position);
  position = detX * detInv;
  velocity = detV * detInv;
};

template< typename T >
void Clamp( T& val, const T& mini, const T& maxi )
{
  if( val < mini ) val = mini;
  else if( val > maxi ) val = maxi;
}

#define VECTOR_ACCESS_OPERATORS \
  T& operator[]( u32 index ) { return e[ index ]; }\
  const T& operator[]( u32 index ) const { return e[ index ]; }

// column vector
template < typename T, int N >
struct TacVector
{
  T e[ N ];
  VECTOR_ACCESS_OPERATORS
};

template< typename T >
struct TacVector< T, 2 >
{
  union
  {
    struct
    {
      T x;
      T y;
    };
    T e[ 2 ];
  };
  VECTOR_ACCESS_OPERATORS
};
typedef TacVector< r32, 2 > v2;
template< typename T >
TacVector< T, 2 > V2( T x, T y )
{
  TacVector< T, 2 > result = { x, y };
  return result;
}

template< typename T >
struct TacVector< T, 3 >
{
  union
  {
    struct
    {
      T x;
      T y;
      T z;
    };
    struct
    {
      TacVector< T, 2 > xy;
      T z;
    };
    T e[ 3 ];
  };

  VECTOR_ACCESS_OPERATORS
};
typedef TacVector< r32, 3 > v3;
typedef TacVector< u32, 3 > v3u;
template< typename T >
TacVector< T, 3 > V3( T x, T y, T z )
{
  TacVector< T, 3 > result = { x, y, z };
  return result;
}

template< typename T >
struct TacVector< T, 4 >
{
  union
  {
    struct
    {
      T x;
      T y;
      T z;
      T w;
    };
    struct
    {
      TacVector< T, 3 > xyz;
      T w;
    };
    T e[ 4 ];
  };
  VECTOR_ACCESS_OPERATORS
};
typedef TacVector< r32, 4 > v4;
typedef TacVector< u32, 4 > v4u;
template< typename T >
TacVector< T, 4 >  V4( T x, T y, T z, T w )
{
  TacVector< T, 4 >  result = { x, y, z, w };
  return result;
}
template< typename T >
TacVector< T, 4 > V4( const TacVector< T, 3 >& xyz, T w )
{
  TacVector< T, 4 > result;
  result.xyz = xyz;
  result.w = w;
  return result;
}

template< typename T, int N >
TacVector< T, N > ComponentwiseMultiply(
    const TacVector< T, N >& lhs,
    const TacVector< T, N >& rhs )
{
  TacVector< T, N > result;
  for( int i = 0; i < N; ++i )
  {
    result[ i ] = lhs[ i ] * rhs[ i ];
  }
  return result;
}

template< typename T, int N >
b32 operator == ( const TacVector< T, N >& lhs, const TacVector< T, N >& rhs )
{
  for( int i = 0; i < N; ++i )
  {
    if( lhs.e[ i ] != rhs.e[ i ] )
    {
      return false;
    }
  }
  return true;
}

template< typename T, int N >
b32 operator != ( const TacVector< T, N >& lhs, const TacVector< T, N >& rhs )
{
  b32 result = !( lhs == rhs );
  return result;
}

template< typename T, int N > TacVector< T, N >&
  operator += ( TacVector< T, N >& lhs, const TacVector< T, N >& rhs )
{
  for( int i = 0; i < N; ++i )
  {
    lhs.e[ i ] += rhs.e[ i ];
  }
  return lhs;
}

template< typename T, int N > TacVector< T, N >&
  operator -= ( TacVector< T, N >& lhs, const TacVector< T, N >& rhs )
{
  for( int i = 0; i < N; ++i )
  {
    lhs.e[ i ] -= rhs.e[ i ];
  }
  return lhs;
}

template< typename T, int N > TacVector< T, N >&
  operator *= ( TacVector< T, N >& lhs, T rhs )
{
  for( int i = 0; i < N; ++i )
  {
    lhs.e[ i ] *= rhs;
  }
  return lhs;
}

template< typename T, int N > TacVector< T, N >&
  operator /= ( TacVector< T, N >& lhs, T rhs )
{
  T invrhs = 1.0f / rhs;
  for( int i = 0; i < N; ++i )
  {
    lhs.e[ i ] *= invrhs;
  }
  return lhs;
}

//   3/25/2016
//   why the fuck does this exist?
//   commenting this shit out
// template< typename T, int N > TacVector< T, N >
// operator / ( T lhs, TacVector< T, N >& rhs )
// {
//   TacVector< T, N > result;
//   for( int i = 0; i < N; ++i )
//   {
//     result.e[ i ] = lhs / rhs.e[ i ];
//   }
//   return result;
// }

template< typename T, int N > TacVector< T, N >
operator / ( const TacVector< T, N >& lhs, T rhs )
{
  TacVector< T, N > result = lhs;
  result /= rhs;
  return result;
}

template< typename T, int N > TacVector< T, N >
operator + ( const TacVector< T, N >& lhs, const TacVector< T, N >& rhs )
{
  TacVector< T, N > result = lhs;
  result += rhs;
  return result;
}

template< typename T, int N > TacVector< T, N >
operator - ( const TacVector< T, N >& lhs, const TacVector< T, N >& rhs )
{
  TacVector< T, N > result = lhs;
  result -= rhs;
  return result;
}

template< typename T, int N > TacVector< T, N >
operator - ( const TacVector< T, N >& lhs )
{
  TacVector< T, N > result = lhs;
  for( int i = 0; i < N; ++i )
  {
    result.e[ i ] *= -1;
  }
  return result;
}

template< typename T, int N > TacVector< T, N >
operator * ( const TacVector< T, N >& lhs, T rhs )
{
  TacVector< T, N > result = lhs;
  result *= rhs;
  return result;
}

template< typename T, int N > TacVector< T, N >
operator * ( r32 lhs, const TacVector< T, N >& rhs )
{
  TacVector< T, N > result = rhs;
  result *= lhs;
  return result;
}

template< typename T, int N > T
  Dot( const TacVector< T, N >& lhs, const TacVector< T, N >& rhs )
{
  T result = 0;

  for( int i = 0; i < N; ++i )
  {
    result += lhs.e[ i ] * rhs.e[ i ];
  }
  return result;
}


template< typename T, int N > T
  Distance( const TacVector< T, N >& lhs, const TacVector< T, N >& rhs )
{
  const TacVector< T, N > lhstorhs = rhs - lhs;
  T result = Length( lhstorhs );
  return result;
}

template< typename T, int N > T
  DistanceSq( const TacVector< T, N >& lhs, const TacVector< T, N >& rhs )
{
  const TacVector< T, N > lhstorhs = rhs - lhs;
  T result = LengthSq( lhstorhs );
  return result;
}

template< typename T, int N > T
  Length( const TacVector< T, N >& lhs )
{
  T result = SquareRoot( LengthSq( lhs ) );
  return result;
}

template< typename T, int N > T
  LengthSq( const TacVector< T, N >& lhs )
{
  T result = Dot( lhs, lhs );
  return result;
}

template< typename T, int N > TacVector< T, N >
Normalize( const TacVector< T, N >& lhs )
{
  TacVector< T, N > result;

  T invlen = 1.0f / Length( lhs );
  for( int i = 0; i < N; ++i )
  {
    result.e[ i ] = lhs.e[ i ] * invlen;
  }
  return result;
}

template< typename T, int N >
bool IsNormalized(  const TacVector< T, N >& lhs )
{
  T len = LengthSq( lhs );
  bool result = IsAboutEqual( len, 1.0f );
  return result;
}

inline v3 Cross( v3 lhs, v3 rhs )
{
  v3 result = {
    lhs.y * rhs.z - rhs.y * lhs.z,
    lhs.z * rhs.x - rhs.z * lhs.x,
    lhs.x * rhs.y - rhs.x * lhs.y };
  return result;
}

inline r32 Angle( v3 a, v3 b )
{
  //a.b = |a||b|costheta
  r32 aob = Dot( a, b );
  r32 lena = Length( a );
  r32 lenb = Length( b );
  r32 costheta = aob / ( lena * lenb );
  r32 theta = Acos( costheta );
  return theta;
}

template< typename T, int N >
inline void Zero( TacVector< T, N >& vec )
{
  for( int i = 0; i < N; ++i )
  {
    vec.e[ i ] = 0;
  }
}

// returns a right handed coordinate system, where
// normalizedDir x normalizedtan1 = normalizedtan2
void GetFrameRH( v3 normalizedDir, v3& normalizedtan1, v3& normalizedtan2 );

// row major matrix
template< typename T, int NUM_ROWS, int NUM_COLUMNS >
struct TacMatrix
{
  T e[ NUM_ROWS * NUM_COLUMNS ];

  T& operator()( u32 row, u32 column ) { return e[ row * NUM_COLUMNS + column ]; }
  const T& operator()( u32 row, u32 column ) const { return e[ row * NUM_COLUMNS + column ]; }
  T& operator[]( u32 index ) { return e[ index ]; }
  const T& operator[]( u32 index ) const { return e[ index ]; }
};

typedef TacMatrix< r32, 2, 2 > m2;
inline m2 M2(
  r32 m00, r32 m01,
  r32 m10, r32 m11 )
{
  m2 result = { 
    m00, m01,
    m10, m11 };
  return result;
}

typedef TacMatrix< r32, 3, 3 > m3;
inline m3 M3(
  r32 m00, r32 m01, r32 m02,
  r32 m10, r32 m11, r32 m12,
  r32 m20, r32 m21, r32 m22 )
{
  m3 result = { 
    m00, m01, m02,
    m10, m11, m12,
    m20, m21, m22 };
  return result;
}

typedef TacMatrix< r32, 3, 4 > m34;
inline m34 M34(
  r32 m00, r32 m01, r32 m02, r32 m03,
  r32 m10, r32 m11, r32 m12, r32 m13,
  r32 m20, r32 m21, r32 m22, r32 m23 )
{
  m34 result = { 
    m00, m01, m02, m03,
    m10, m11, m12, m13,
    m20, m21, m22, m23 };
  return result;
}

typedef TacMatrix< r32, 4, 4 > m4;
inline m4 M4(
  r32 m00, r32 m01, r32 m02, r32 m03,
  r32 m10, r32 m11, r32 m12, r32 m13,
  r32 m20, r32 m21, r32 m22, r32 m23,
  r32 m30, r32 m31, r32 m32, r32 m33 )
{
  m4 result = { 
    m00, m01, m02, m03,
    m10, m11, m12, m13,
    m20, m21, m22, m23,
    m30, m31, m32, m33 };
  return result;
}

template< typename T, int NUM_ROWS, int NUM_MIDDLE, int NUM_COLS >
TacMatrix< T, NUM_ROWS, NUM_COLS > operator* (
  const TacMatrix< T, NUM_ROWS, NUM_MIDDLE >& lhs,
  const TacMatrix< T, NUM_MIDDLE , NUM_COLS >& rhs )
{
  TacMatrix< T, NUM_ROWS, NUM_COLS > result;
  for( int r = 0; r < NUM_ROWS; ++r )
  {
    for( int c = 0; c < NUM_COLS; ++c )
    {
      T value = 0;
      for( int m = 0; m < NUM_MIDDLE; ++m )
      {
        const T& lhsvalue = lhs( r, m );
        const T& rhsvalue = rhs( m, c );
        value += lhsvalue * rhsvalue;
      }
      result( r, c ) = value;
    }
  }
  return result;
}

template< typename T, int N >
TacMatrix< T, N, N >& operator *= (
  TacMatrix< T, N, N >& lhs,
  const TacMatrix< T, N, N >& rhs )
{
  TacMatrix< T, N > result = lhs * rhs;
  lhs = result;
  return lhs;
}

template< typename T, int N >
TacVector< T, N > operator * (
  const TacMatrix< T, N, N >& lhs,
  const TacVector< T, N >& rhs )
{
  TacVector< T, N > result;
  for( s32 r = 0; r < N; ++r )
  {
    T sum = 0;
    for( s32 c = 0; c < N; ++c )
    {
      sum += lhs.e[ r * N + c ] * rhs.e[ c ];
    }
    result.e[ r ] = sum;
  }
  return result;
}

template< typename T, int N >
TacMatrix< T, N, N > operator * (
  const TacMatrix< T, N, N >& lhs,
  T val )
{
  TacMatrix< T, N, N > result;
  for( u32 i = 0; i < N * N; ++i )
  {
    result.e[ i ] = lhs.e[ i ] * val;
  }
  return result;
}


template< typename T, int N >
TacMatrix< T, N, N > operator * (
  T val,
  const TacMatrix< T, N, N >& lhs )
{
  TacMatrix< T, N, N > result;
  for( u32 i = 0; i < N * N; ++i )
  {
    result.e[ i ] = lhs.e[ i ] * val;
  }
  return result;
}

template< typename T, int N >
TacMatrix< T, N, N >& operator *= (
  TacMatrix< T, N, N >& lhs,
  T val )
{
  for( u32 i = 0; i < N * N; ++i )
  {
    lhs.e[ i ] *= val;
  }
  return lhs;
}

template< typename T, int M, int N >
void Identity( TacMatrix< T, M, N >& mat )
{
  for( int r = 0; r < M; ++r )
  {
    for( int c = 0; c < N; ++c )
    {
      mat.e[ r * M + c ] = r == c;
    }
  }
}

template< typename T, int N > TacMatrix< T, N, N >
Transpose( const TacMatrix< T, N, N >& mat )
{
  TacMatrix< T, N, N > result = {};
  for( int r = 0; r < N; ++r )
  {
    for( int c = r + 1; c < N; ++c )
    {
      result( r, c ) = mat( c, r );
      result( c, r ) = mat( r, c );
    }
  }
  for( int i = 0; i < N; ++i )
  {
    result( i, i ) = mat( i, i );
  }
  return result;
}

template< typename T, int N >
TacMatrix< T, N - 1, N - 1 >
  SubMatrix( const TacMatrix< T, N, N >& orig, int ignoreRow, int ignoreCol )
{
  TacMatrix< T, N - 1, N - 1 > result = {};
  int resultIndex = 0;
  for( int r = 0; r < N; ++r )
  {
    if( r == ignoreRow )
      continue;
    for( int c = 0; c < N; ++c )
    {
      if( c == ignoreCol )
        continue;

      result[ resultIndex++ ] = orig( r, c );
    }
  }
  return result;
}

// NOTE:
//  Det( m2 ) = area of paralellogram formed by 2 column vectors
//  Det( m3 ) = area of paralellopiped formed by 3 column vectors
template< typename T, int N >
T Determinant( const TacMatrix< T, N, N >& mat )
{
  T result = 0;
  T sign = 1;
  for( int c = 0; c < N; ++c )
  {
    TacMatrix< T, N - 1, N - 1 > sub = SubMatrix( mat, 0, c );
    result += sign * mat( 0, c ) * Determinant( sub );
    sign *= -1;
  }
  return result;
}

template< typename T >
T Determinant( const TacMatrix< T, 2, 2 >& mat )
{
  T result =
    mat( 0, 0 ) * mat( 1, 1 ) -
    mat( 0, 1 ) * mat( 1, 0 );
  return result;
}

template< typename T >
T Determinant( const TacMatrix< T, 1, 1 >& mat )
{
  T result = mat( 0, 0 );
  return result;
}

template< typename T, int N >
TacMatrix< T, N, N >
  Cofactor( const TacMatrix< T, N, N >& mat )
{
  TacMatrix< T, N, N > result;
  T signouter = 1;
  for( int r = 0; r < N; ++r )
  {
    T signinnder = signouter;
    for( int c = 0; c < N; ++c )
    {
      TacMatrix< T, N - 1, N - 1 > sub = SubMatrix( mat, r, c );
      T det = Determinant( sub );
      result( r, c ) = det * signinnder;
      signinnder *= -1;
    }
    signouter *= -1;
  }
  return result;
}

template< typename T, int N >
TacMatrix< T, N, N >
  Inverse( const TacMatrix< T, N, N >& mat )
{
  T det = Determinant( mat );
  TacMatrix< T, N, N > result = Cofactor( mat );
  result = Transpose( result );
  result *= ( 1.0f / det );
  return result;
}

template< typename T >
TacMatrix< T, 2, 2 >
  Inverse( const TacMatrix< T, 2, 2 >& mat )
{
  TacMatrix< T, 2, 2 > result;
  result( 0, 0 ) = mat( 1, 1 );
  result( 1, 0 ) = mat( 0, 1 );
  result( 0, 1 ) = mat( 1, 0 );
  result( 1, 1 ) = mat( 0, 0 );
  result *= 1.0f / ( mat( 0, 0 ) * mat( 1, 1 ) - mat( 1, 0 ) * mat( 0, 1 ) );
  return result;
}

m3 M3Scale( v3 scale );
m3 M3RotRadX( float rotRad );
m3 M3RotRadY( float rotRad );
m3 M3RotRadZ( float rotRad );
m4 M4Scale( v3 scale );
m4 M4FromM3( const m3& rot );
m4 M4Transform( v3 scale, m3 rot, v3 translate );
m4 M4Transform( v3 scale, v3 rotate, v3 translate );
m4 M4TransformInverse( v3 scale, v3 rotate, v3 translate );
m4 M4Translate( v3 translate );
m4 M4RotRadX( float rotRad );
m4 M4RotRadY( float rotRad );
m4 M4RotRadZ( float rotRad );

m3 M3AngleAxis( r32 angle, v3 axis );
void M3ToEuler( m3 m, v3& euler );

void MatrixUnitTests();

template< typename T, int N >
bool IsAboutEqual(
  const TacVector< T, N >& v0,
  const TacVector< T, N >& v1 )
{
  for( int i = 0; i < N; ++i )
  {
    if( !IsAboutEqual( v0[ i ], v1[ i ] ) )
      return false;
  }
  return true;
}

template< typename T, int NUM_ROWS, int NUM_COLUMNS >
bool IsAboutEqual(
  const TacMatrix< T, NUM_ROWS, NUM_COLUMNS >& v0,
  const TacMatrix< T, NUM_ROWS, NUM_COLUMNS >& v1 )
{
  for( int i = 0; i < NUM_ROWS * NUM_COLUMNS; ++i )
  {
    if( !IsAboutEqual( v0[ i ], v1[ i ] ) )
      return false;
  }
  return true;
}

void PrintMatrix( const m4& m );

struct TacQuaternion
{
  union
  {
    struct  
    {
      r32 i,j,k,w;
    };
    struct  
    {
      r32 x,y,z,w;
    };
    v4 e;
  };
};
m3 QuaternionToMatrix3( const TacQuaternion& q );

// interpolates between two quaterinons given t[0,1]
// returns a quaternion
v4 Slerp( v4 q1, v4 q2, r32 t );

// returns a matrix3x3 rotation matrix from the input quaternion q
m3 QuaternionToMatrix( v4 q );

template< typename T >
struct TacHypersphere
{
  T position;
  r32 radius;
};
typedef TacHypersphere< v3 > TacSphere;
typedef TacHypersphere< v2 > TacCircle;

// naive approach, makes a shitty sphere
TacSphere SphereCentroid(
  v3* points,
  u32 numPoints );

// better approach, makes a better sphere
TacSphere SphereExtents(
  v3* points,
  u32 numPoints );

