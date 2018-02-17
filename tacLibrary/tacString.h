#pragma once

#include "tacTypes.h"

u32 TacStrLen( const char* str );
u32 TacStrCpy( char* dst, u32 dstLen, const char* src );
u32 TacStrCpy( char* dst, u32 dstLen, const char* src, u32 srcLen );

//return value	indicates
//  <0	the first character that does not match has a lower value in str1 than in str2
//   0  the contents of both strings are equal
//  >0	the first character that does not match has a greater value in str1 than in str2
u32 TacStrCmp( const char* str1, const char* str2 );

#define DEFAULT_ERR_LEN 512

template< u32 N >
struct FixedString
{
  char buffer[ N ]; // includes null-terminator
  u32 size;
  FixedString& operator = ( const char* rhs )
  {
    size = TacStrCpy( buffer, N, rhs );
    return *this;
  }
  b32 operator == ( const FixedString& rhs )
  {
    b32 result = 
      size == rhs.size &&
      TacStrCmp( buffer, rhs.buffer ) == 0;
    return result;
  }
  //b32 operator == ( const char* rhs )
  //{
  //  b32 result = TacStrCmp( buffer, rhs ) == 0;
  //  return result;
  //}
  b32 operator == ( const char* rhs ) const
  {
    b32 result = TacStrCmp( buffer, rhs ) == 0;
    return result;
  }
};

template< u32 N >
void FixedStringClear( FixedString< N >& str )
{
  str.buffer[ 0 ] = '\0';
  str.size = 0;
}
template< u32 N >

FixedString< N > FixedStringCreate( const char* str )
{
  FixedString< N > result;
  result.size = TacStrCpy( result.buffer, N, str );
  return result;
}

template< u32 N >
void FixedStringAppend(
  FixedString< N >& str,
  const char* suffix,
  u32 suffixLen )
{
  u32 copied = TacStrCpy(
    str.buffer + str.size,
    N - str.size,
    suffix,
    suffixLen );
  str.size += copied;
}

template< u32 N >
void FixedStringAppend( FixedString< N >& str, char suffix )
{
  FixedStringAppend( str, &suffix, 1 );
}

template< u32 N, u32 M >
void FixedStringAppend(
  FixedString< N >& str,
  const FixedString< M >& suffix )
{
  FixedStringAppend( str, suffix.buffer, suffix.size );
}

template< u32 N >
void FixedStringAppend(
  FixedString< N >& str,
  const char* suffix )
{
  u32 suffixLen = TacStrLen( suffix );
  FixedStringAppend( str, suffix, suffixLen );
}

// return index into str, or the size of the string if no characters
// in seachCharacters are found
u32 FindFirstOf( const char* str, const char* searchCharacters );
