#pragma once

#define internalFunction static 
#define localPersist static 
#define globalVariable static

#define Minimum( a, b ) ( ( a < b ) ? ( a ) : ( b ) )
#define Maximum( a, b ) ( ( a > b ) ? ( a ) : ( b ) )
#define TacUnusedParameter( param ) ( void ) param

#define STRINGIFY2( x ) #x
#define STRINGIFY( x ) STRINGIFY2( x )
#define COMBINE2( x, y ) x##y
#define COMBINE( x, y ) COMBINE2( x, y )
#define UNIQUE COMBINE( unique, __LINE__ )

template< typename T>
struct OnDestructAux
{
  T mT;
  OnDestructAux( T t ) : mT( t ) {}
  ~OnDestructAux() { mT(); }
};
// dumb shit:
// - can't use preprocessor commands in a OnDestruct
// - can't use __LINE__ in a lambda
#define OnDestruct2( code, lambdaName, dtorName )\
  auto lambdaName = [&](){ code; };\
  OnDestructAux< decltype( lambdaName ) > dtorName( lambdaName );
#define OnDestruct( code )\
  OnDestruct2( code, COMBINE( UNIQUE, lambda ), COMBINE( UNIQUE, dtor ) )

void DisplayError(
  const char* expression,
  int line,
  const char* file,
  const char* fn );
void WarnOnceAux(
  const char* expression,
  int line,
  const char* file,
  const char* fn );

#ifdef TACDEBUG

#define CrashGame() *( int* )0 = 0
#define TacAssert( expression )\
  if( !( expression ) )\
{\
  DisplayError( #expression, __LINE__, __FILE__, __FUNCTION__ );\
  __debugbreak();\
  CrashGame();\
}
#define TacAssertIndex( index, size ) TacAssert( ( u32 )index < size )
#define TacInvalidCodePath\
  DisplayError( "Invalid code path!", __LINE__, __FILE__, __FUNCTION__ );\
  __debugbreak();\
  CrashGame();
#define TacInvalidDefaultCase default: TacInvalidCodePath; break;
#define WarnOnce( msg )\
  WarnOnceAux( msg, __LINE__, __FILE__, __FUNCTION__ );

#else

#define CrashGame()
#define TacAssert( expression )
#define TacAssertIndex( index, size )
#define TacInvalidCodePath
#define TacInvalidDefaultCase
#define WarnOnce( msg )

#endif


#define StaticAssert( expression, mName )\
  typedef char assertion_failed_because_ ## mName[ ( expression ) ? 1 : -1 ];

#define OffsetOf( type, member ) ( ( u32 )( &( ( type* )0 )->member ) )
#define SizeOfMember( type, member ) sizeof( ( ( type* )0 )->member )

//template< typename T, unsigned size >
//  //constexpr <-- not supported?
//    unsigned ArraySize( T( & )[ size ] )
//{
//  return size;
//}
#define ArraySize( array ) ( sizeof( array ) / sizeof( ( array )[ 0 ] ) )
#define Kilobytes( Value ) (          ( Value ) * ( decltype( Value ) )( 1024 ) )
#define Megabytes( Value ) ( Kilobytes( Value ) * ( decltype( Value ) )( 1024 ) )
#define Gigabytes( Value ) ( Megabytes( Value ) * ( decltype( Value ) )( 1024 ) )
#define Terabytes( Value ) ( Gigabytes( Value ) * ( decltype( Value ) )( 1024 ) )

template< typename T>
void Clear( T& t )
{
  T empty = {};
  t = empty;
}
