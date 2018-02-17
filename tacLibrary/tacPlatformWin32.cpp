#include "tacPlatform.h"
#include "tacFilesystem.h"
#include "tacPlatformWin32.h"

#include <sstream>
#include <fstream>
#include <ctime>
#include <stdlib.h>


void DisplayError(
  const char* expression,
  int line,
  const char* file,
  const char* fn )
{
  Filepath fileLong;
  fileLong = file;
  Filepath fileShort = FilepathGetFile( fileLong );

  std::stringstream ss;
  ss <<
    "Assert( " <<
    expression <<
    " ) failed in "
    << "file "
    << fileShort.buffer
    << " on line " << line
    << " in function " << fn << "()" << std::endl ;
  OutputDebugStringA( ss.str().c_str() );

  const char* logfile = "AssertLog.txt";
  std::ofstream ofs( logfile );
  if( ofs.is_open() )
  {
    ofs << ss.str();
    ofs.close();
  }

  ss 
    << "Please send " << logfile << " to the dev team" << std::endl;
  MessageBoxA( 0, ss.str().c_str(), "Assert triggered", MB_OK );
}

void WarnOnceAux(
  const char* msg,
  int line,
  const char* file,
  const char* fn )
{
  static FixedString< DEFAULT_ERR_LEN > alreadyWarnedKeys[ 100 ];
  static u32 numAlreadyWarnedKeys = 0;

  FixedString< DEFAULT_ERR_LEN > warningKey;
  warningKey.size = sprintf_s( warningKey.buffer, "%s %i", file, line );

  bool alreadyWarned = false;
  for( u32 iKey = 0; iKey < numAlreadyWarnedKeys; ++iKey )
  {
    if( alreadyWarnedKeys[ iKey ] == warningKey )
    {
      alreadyWarned = true;
      break;
    }
  }

  if( !alreadyWarned )
  {
    if( numAlreadyWarnedKeys < ArraySize( alreadyWarnedKeys ) )
    {
      alreadyWarnedKeys[ numAlreadyWarnedKeys++ ] = warningKey;
      DisplayError( msg, line, file, fn );
    }
    else
    {
      static bool warnedThatTheresTooManyWarnings = false;
      if( !warnedThatTheresTooManyWarnings )
      {
        warnedThatTheresTooManyWarnings = true;
        DisplayError( "Too many warnings, go fix some!", line, file, fn );
      }
    }
  }
}


void AppendWin32Error(
  FixedString< DEFAULT_ERR_LEN >& errors,
  DWORD winErrorValue )
{
  std::string result( "No error" );
  if( winErrorValue )
  {
    LPVOID lpMsgBuf;
    DWORD bufLen = FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      winErrorValue,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      ( LPTSTR ) &lpMsgBuf,
      0, NULL );
    if( bufLen )
    {
      FixedStringAppend( errors, ( LPCSTR )lpMsgBuf, bufLen );
      LocalFree( lpMsgBuf );
    }
  }
}

TacFile PlatformOpenFile(
  TacThreadContext* thread,
  const char* path,
  OpenFileDisposition disposition,
  FileAccess access,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacUnusedParameter( thread );

  DWORD win32disposition = 0;
  switch ( disposition )
  {
  case OpenFileDisposition::CreateAlways:
    win32disposition = CREATE_ALWAYS;
    break;
  case OpenFileDisposition::CreateNew:
    win32disposition = CREATE_NEW;
    break;
  case OpenFileDisposition::OpenAlways:
    win32disposition = OPEN_ALWAYS;
    break;
  case OpenFileDisposition::OpenExisting:
    win32disposition = OPEN_EXISTING;
    break;
  case OpenFileDisposition::TruncateExisting:
    win32disposition = TRUNCATE_EXISTING;
    break;
  default:
    TacAssert( false );
    break;
  }

  DWORD dwaccess = 0;
  if( ( u32 )access & ( u32 )FileAccess::Read ) dwaccess |= GENERIC_READ;
  if( ( u32 )access & ( u32 )FileAccess::Write ) dwaccess |= GENERIC_WRITE;

  void* handle = CreateFile(
    path,
    dwaccess,
    FILE_SHARE_READ,
    0,
    win32disposition,
    0,
    0 );
  if( handle == INVALID_HANDLE_VALUE )
  {
    errors.size = sprintf_s(
      errors.buffer, "Failed to open file %s\n", path );
    AppendWin32Error( errors, GetLastError() );
    handle = nullptr;
  }

  TacFile result;
  result.mHandle = handle;
  return result;
}

void PlatformCloseFile(
  TacThreadContext* thread,
  TacFile& file,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacUnusedParameter( thread );

  if( CloseHandle( file.mHandle ) == 0 )
  {
    FixedStringAppend( errors, "CloseHandle() failed\n" );
    AppendWin32Error( errors, GetLastError() );
  }
  else
  {
    file.mHandle = nullptr;
  }
}

u32 PlatformGetFileSize(
  TacThreadContext* thread,
  const TacFile& file,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacUnusedParameter( thread );

  u32 result = 0;
  LARGE_INTEGER fileSize;
  if( GetFileSizeEx( file.mHandle, &fileSize ) )
  {
    TacAssert( fileSize.QuadPart < UINT32_MAX );
    result = ( u32 )fileSize.QuadPart;
  }
  else
  {
    AppendWin32Error( errors, GetLastError() );
  }

  return result;
}

bool PlatformReadEntireFile(
  TacThreadContext* thread,
  const TacFile& file,
  void* memory,
  u32 size,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacUnusedParameter( thread );

  DWORD bytesRead;
  if( 0 == ReadFile(
    file.mHandle,
    memory,
    size, 
    &bytesRead,
    0 ) )
  {
    AppendWin32Error( errors, GetLastError() );
    return false;
  }

  if( bytesRead == 0 )
    return true;
  else if( bytesRead != size )
  {
    errors.size = sprintf_s(
      errors.buffer,
      "ReadFile read %i bytes instead of %i",
      bytesRead,
      size );
    return false;
  }

  return false;
}

void PlatformWriteEntireFile(
  TacThreadContext* thread,
  const TacFile& file,
  void* memory,
  u32 size,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacUnusedParameter( thread );

  DWORD bytesWritten;
  if( WriteFile( file.mHandle, memory, size, &bytesWritten, 0 ) )
  {
    if( bytesWritten != size )
    {
      errors.size = sprintf_s(
        errors.buffer,
        "WriteFile wrote %i bytes instead of %i",
        bytesWritten,
        size );
    }
  }
  else
  {
    AppendWin32Error( errors, GetLastError() );
  }
}

time_t PlatformGetFileModifiedTime(
  TacThreadContext* thread,
  const TacFile& file,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacUnusedParameter( thread );

  time_t result = -1;
  BY_HANDLE_FILE_INFORMATION fileInfo;
  if( GetFileInformationByHandle( file.mHandle, &fileInfo ) )
  {
    SYSTEMTIME lastWrite;
    if( FileTimeToSystemTime( &fileInfo.ftLastWriteTime, &lastWrite ) )
    {
      tm tempTm;
      tempTm.tm_sec = lastWrite.wSecond;
      tempTm.tm_min = lastWrite.wMinute;
      tempTm.tm_hour = lastWrite.wHour;
      tempTm.tm_mday = lastWrite.wDay;
      tempTm.tm_mon = lastWrite.wMonth - 1;
      tempTm.tm_year = lastWrite.wYear - 1900;
      tempTm.tm_wday; // not needed for mktime
      tempTm.tm_yday; // not needed for mktime
      tempTm.tm_isdst = -1; // i forget what this is

      result = mktime( &tempTm );
      if( result == -1 )
      {
        errors = "Calandar time cannot be represented";
      }
    }
    else
    {
      AppendWin32Error( errors, GetLastError() );
    }
  }
  else
  {
    AppendWin32Error( errors, GetLastError() );
  }

  return result;

}

