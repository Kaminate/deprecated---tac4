#include "tacFilesystem.h"
#include <algorithm>

Filepath FilepathAppendFile(
  const Filepath& folder,
  const Filepath& filename )
{
  Filepath concatinated = folder;
  if( concatinated.size )
  {
    if( concatinated.buffer[ concatinated.size - 1 ] != '/' )
    {
      FixedStringAppend( concatinated, '/' );
    }
    FixedStringAppend( concatinated, filename );
  }
  return concatinated;
}

Filepath FilepathGetFolder( const Filepath& path )
{
  Filepath folder = path;
  for( u32 i = folder.size - 1; i != ( u32 )-1; --i )
  {
    if( folder.buffer[ i ] == '/' || folder.buffer[ i ] == '\\')
    {
      folder.buffer[ i ] = '\0';
      folder.size = i;
      break;
    }
  }
  return folder;
}


Filepath FilepathGetFile( const Filepath& path )
{
  Filepath file = path;
  for( u32 i = file.size - 1; i != ( u32 )-1; --i )
  {
    if( file.buffer[ i ] == '/' || file.buffer[ i ] == '\\')
    {
      Filepath ending;
      ending = file.buffer + i + 1;
      file = ending;
      break;
    }
  }
  return file;
}

Filepath FilepathCreate( const char* str )
{
  Filepath result;
  result = str;
  return result;
}
