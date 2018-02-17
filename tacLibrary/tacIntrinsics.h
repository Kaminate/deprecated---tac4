#pragma once
#include "tacLibrary\tacTypes.h"
#include <math.h>
#include <cmath>

#if _MSC_VER
#include <intrin.h>
#pragma intrinsic( _BitScanForward )
#endif


inline s32
  SignOf( s32 val )
{
  s32 result = ( val > 0 ) - ( val < 0 );
  return result;
}

inline r32
  SquareRoot( r32 val )
{
  r32 result = sqrt( val );
  return result;
}

inline r32
  Round( r32 x )
{
  if( x > 0 )
  {
    r32 result = ( r32 )( ( s32 )( x + 0.5f ) );
    return result;
  }
  else
  {
    r32 result = ( r32 )( ( s32 )( x - 0.5f ) );
    return result;
  }
}

inline r32
  AbsoluteValue( r32 val )
{
  r32 result = abs( val );
  return result;
}

inline s32
  RoundReal32ToInt32( r32 val )
{
  s32 result = ( s32 )Round( val );
  return result;
}

inline u32
  RoundReal32ToUInt32( r32 val )
{
  u32 result = ( u32 )Round( val );
  return result;
}

inline s32 
  FloorReal32ToInt32( r32 val )
{
  s32 result = ( s32 )floorf( val );
  return result;
}

inline s32 
  CeilReal32ToInt32( r32 val )
{
  s32 result = ( s32 )ceilf( val );
  return result;
}

inline s32
  TruncateReal32ToInt32( r32 val )
{
  s32 result = ( s32 )val;
  return result;
}

inline s32 Floor( r32 val )
{
  s32 result = ( s32 )floor( val );
  return result;
}
inline r32 Sin( r32 angle )
{
  r32 result = sin( angle );
  return result;
}
inline r32 Asin( r32 angle )
{
  return asin( angle );
}
inline r32 Cos( r32 angle )
{
  return cos( angle );
}
inline r32 Acos( r32 angle )
{
  return acos( angle );
}
inline r32 Tan( r32 angle )
{
  return tan( angle );
}
inline r32 Atan2( r32 y, r32 x )
{
  return atan2( y, x );
}
inline r32 Pow( r32 base, r32 exponent )
{
  return pow( base, exponent );
}

struct BitScanResult
{
  b32 found;
  u32 index;
};

inline BitScanResult FindLeastSignificantSetBit( u32 value )
{
  BitScanResult result = {};

#if _MSC_VER
  result.found = _BitScanForward( 
    ( unsigned long* )&result.index,
    value );
#else
  for( u32 index = 0; index < 32; ++index )
  {
    if( value & ( 1 << index ) )
    {
      result.found = true;
      result.index = index;
      break;
    }
  }
#endif
  return result;
}

inline u32
  RotateLeft( u32 val, s32 amount )
{
#if _MSC_VER
  u32 result = _rotl( val, amount );
#else
  amount &= 31;
  u32 result = ( ( val << amount ) | ( value >> 32 - amount ) );
#endif
  return result;
}
inline u32
  RotateRight( u32 val, s32 amount )
{
#if _MSC_VER
  u32 result = _rotr( val, amount );
#else
  amount &= 31;
  u32 result = ( ( val >> amount ) | ( val << 32 - amount ) );
#endif
  return result;
}


