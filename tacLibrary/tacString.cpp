#include "tacString.h"
#include "tacPlatform.h"

u32 TacStrLen( const char* str )
{
  TacAssert( str );
  const char* end = str;
  while( *end )
  {
    ++end;
  }
  unsigned result = end - str;
  return result;
}

u32 TacStrCpy( char* dst, u32 dstLen, const char* src )
{
  TacAssert( dst );
  TacAssert( src );

  u32 srcLen = TacStrLen( src );
  return TacStrCpy( dst, dstLen, src, srcLen );
}

u32 TacStrCpy( char* dst, u32 dstLen, const char* src, u32 srcLen )
{
  TacAssert( dst );
  TacAssert( src );
  u32 charsToCopy = Minimum( srcLen, dstLen - 1 );
  for( u32 i = 0; i < charsToCopy; ++i )
  {
    dst[ i ] = src[ i ];
  }
  dst[ charsToCopy ] = '\0';
  return charsToCopy;
}

u32 TacStrCmp( const char* str1, const char* str2 )
{
  while( *str1 && (*str1 == *str2 ))
  {
    str1++;
    str2++;
  }
  return *str1 - *str2;
}

u32 FindFirstOf( const char* str, const char* searchCharacters )
{
  u32 i = 0;
  for( const char* c = str; *c; ++c, ++i )
  {
    for( const char* s = searchCharacters; *s; ++s )
    {
      if( *c == *s )
      {
        return i;
      }
    }
  }
  return i;
}
