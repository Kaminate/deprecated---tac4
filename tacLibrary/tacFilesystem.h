#pragma once

#include "tacPlatform.h"
#include "tacString.h"

typedef FixedString< 256 > Filepath;

Filepath FilepathAppendFile( const Filepath& folder, const Filepath& filename );
Filepath FilepathGetFolder( const Filepath& path );
Filepath FilepathGetFile( const Filepath& path );
Filepath FilepathCreate( const char* str );

template< u32 N >
void AppendFileInfoAux(
  FixedString< N >& str,
  const char* file,
  int line,
  const char* function )
{
  Filepath myPath;
  myPath = file;
  myPath = FilepathGetFile( myPath );

  FixedString< 256 > suffix;
  suffix.size = sprintf_s(
    suffix.buffer,
    "\nFile: %s\nLine: %i\nFunction: %s\n",
    myPath.buffer,
    line,
    function );
  FixedStringAppend( str, suffix );
}

#define AppendFileInfo( fixedStrRef ) AppendFileInfoAux( fixedStrRef, __FILE__, __LINE__, __FUNCTION__ );
