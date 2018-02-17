#include "tacPlatform.h"


void GameOnLoadStub( TacGameInterface& onLoadInterface )
{
  TacUnusedParameter( onLoadInterface );
}

void GameUpdateStub( TacGameInterface& updateInterface )
{
  TacUnusedParameter( updateInterface );
}

void GameOnExitStub( TacGameInterface& updateInterface )
{
  TacUnusedParameter( updateInterface );
}

const char* VA( const char* format, ... )
{
  static char buffer[ 256 ];
  va_list args;
  va_start( args, format );
  int numWritten = vsprintf( buffer, format, args );
  TacAssert( numWritten >= 0 );
  va_end( args );
  return buffer;
}

void PushEntry(
  TacWorkQueue* queue,
  WorkQueueCallback* callback,
  void* data )
{
  u32 newNextEntryToWrite =
    ( queue->nextEntryToWrite + 1 ) % queue->maxEntries;
  TacAssert( newNextEntryToWrite != queue->nextEntryToRead );

  // TODO( N8 ): CAS so any thread can add

  // a
  TacEntryStorage& entry = queue->entries[ queue->nextEntryToWrite ];

  // b
  entry.data = data;
  entry.callback = callback;

  // ensures that a, b, are always written before c, ie:
  // abc
  // bac
  std::atomic_thread_fence( std::memory_order_release );

  // c
  queue->nextEntryToWrite = newNextEntryToWrite;
  ++queue->numToComplete;
}

b32 DoNextEntry( TacWorkQueue* queue, TacThreadContext* thread )
{
  b32 shouldSleep = false;

  u32 originalNextEntryToRead = queue->nextEntryToRead;
  u32 newNextEntryToRead = ( originalNextEntryToRead + 1 ) % queue->maxEntries;
  if( originalNextEntryToRead != queue->nextEntryToWrite )
  {
    if( queue->nextEntryToRead.compare_exchange_strong(
      originalNextEntryToRead,
      newNextEntryToRead,
      std::memory_order_relaxed ) )
    {
      std::atomic_thread_fence( std::memory_order_consume );
      TacEntryStorage& storage = queue->entries[ originalNextEntryToRead ];
      storage.callback( thread, storage.data );
      std::atomic_thread_fence( std::memory_order_release );
      ++queue->numCompleted;
    }
  }
  else
  {
    shouldSleep = true;
  }

  return shouldSleep;
}

void CompleteAllWork( TacWorkQueue* queue, TacThreadContext* thread )
{
  while( queue->numCompleted != queue->numToComplete )
  {
    DoNextEntry( queue, thread );
  }
  queue->numCompleted = 0;
  queue->numToComplete = 0;
}

void ThreadProc(
  TacWorkQueue* queue,
  TacThreadContext* thread,
  b32* running )
{
  while( *running )
  {
    if( DoNextEntry( queue, thread ) )
    {
      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }
  }
}

void TacWorkQueue::Init( u32 numThreads, b32* running )
{
  nextEntryToRead = ( 0 );
  numCompleted = ( 0 );
  numToComplete = ( 0 );
  nextEntryToWrite = ( 0 );

  threads.resize( numThreads );
  threadcontexts.resize( numThreads );

  for( u32 i = 0; i < numThreads; ++i )
  {
    TacThreadContext& threadcontext = threadcontexts[ i ];
    threadcontext.logicalThreadIndex = i;
    std::thread& curThread = threads[ i ];
    curThread = std::thread( ThreadProc, this, &threadcontext, running );
    curThread.detach();
  }
}

time_t PlatformGetFileModifiedTime(
  TacThreadContext* thread,
  const char* filepath,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  time_t time = 0;

  do
  {
    TacFile file = PlatformOpenFile(
      thread,
      filepath,
      OpenFileDisposition::OpenExisting,
      ( FileAccess )0,//FileAccess::Read,
      errors );
    if( errors.size )
      break;
    OnDestruct( PlatformCloseFile( thread, file, errors ); );
    time = PlatformGetFileModifiedTime( thread, file, errors );
    if( errors.size )
      break;

  } while( false );
  return time;
}
