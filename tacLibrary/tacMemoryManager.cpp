#include "tacMemoryManager.h"

// TODO( N8 ): Consider allignment

// MemoryManager::memory
// [ MemoryManagerImpl ]
// [ MemoryBlockHeader ]
// [ data ]
// [ MemoryBlockHeader ]
// [ data ]
// ...
#define MIN_BLOCK_DATA_SIZE 4

struct MemoryBlockHeader
{
  u32 dataSize;

  // null if not in free list
  MemoryBlockHeader* freeNext;
  // null if not in free list
  MemoryBlockHeader* freePrev;

  MemoryBlockHeader* horizPrev;
  MemoryBlockHeader* horizNext;
};

struct MemoryManagerImpl 
{ 
  MemoryBlockHeader dummyFree;
  u32 freeCount;
  u32 allocatedCount;
};

int MemoryManagerJoinable(
  MemoryManagerImpl* impl,
  MemoryBlockHeader* header )
{
  int result = 
    // must be in free list
    header->freeNext != 0 &&

    // must not be a dummy node
    header != &impl->dummyFree;

  return result;
}

void MemoryManagerInit(
  TacMemoryManager& manager,
  void* memory,
  u32 memorySize )
{
  TacAssert( memory );
  TacAssert( 
    sizeof( MemoryManagerImpl ) + 
    sizeof( MemoryBlockHeader ) + 
    MIN_BLOCK_DATA_SIZE 
    <= memorySize );

  manager.memory = memory;



  u8* runningAddress = ( u8* )memory;
  MemoryManagerImpl * impl = ( MemoryManagerImpl* )runningAddress;
  runningAddress += sizeof( MemoryManagerImpl );

  MemoryBlockHeader* blockHeader = ( MemoryBlockHeader* )runningAddress;
  runningAddress += sizeof( MemoryBlockHeader );
  char* begin = ( char* )runningAddress;
  char* end = ( char* )memory + memorySize;
  blockHeader->dataSize = ( u32 )( end - begin );
  blockHeader->horizNext = &( impl->dummyFree );
  blockHeader->horizPrev = &impl->dummyFree;
  blockHeader->freeNext = &impl->dummyFree;
  blockHeader->freePrev = &impl->dummyFree;;

  impl->dummyFree.dataSize = 0;
  impl->dummyFree.freeNext = blockHeader;
  impl->dummyFree.freePrev = blockHeader;
  impl->dummyFree.horizNext = blockHeader;
  impl->dummyFree.horizPrev = blockHeader;
  impl->freeCount = 1;
  impl->allocatedCount = 0;

}

void* MemoryManagerAllocate( TacMemoryManager& manager, u32 size )
{
  void* result = 0;
  size = Maximum( size, MIN_BLOCK_DATA_SIZE );

  MemoryManagerImpl* impl = ( MemoryManagerImpl* )manager.memory;
  for( MemoryBlockHeader* blockHeader = impl->dummyFree.freeNext; 
    blockHeader != &impl->dummyFree;
    blockHeader = blockHeader->freeNext )
  {
    if( blockHeader->dataSize >=
      size +
      MIN_BLOCK_DATA_SIZE +
      sizeof( MemoryBlockHeader ) )
    {
      // split
      MemoryBlockHeader* child = ( MemoryBlockHeader* )
        ( ( u8* )blockHeader + blockHeader->dataSize - size );

      child->dataSize = size;
      blockHeader->dataSize -= 
        child->dataSize +
        sizeof( MemoryBlockHeader );

      // free pointers do not need to be adjusted
      // adjacent pointers do
      child->horizNext = blockHeader->horizNext;
      child->horizNext->horizPrev = child;
      blockHeader->horizNext = child;
      child->horizPrev = blockHeader;
      child->freePrev = 0;
      child->freeNext = 0;

      result =
        ( u8* )child +
        sizeof( MemoryBlockHeader );

      ++impl->allocatedCount;

      break;
    }
    else if( size == blockHeader->dataSize )
    {
      blockHeader->freePrev->freeNext =
        blockHeader->freeNext;
      blockHeader->freeNext->freePrev =
        blockHeader->freePrev;
      blockHeader->freePrev = 0;
      blockHeader->freeNext = 0;

      result =
        ( u8* )blockHeader +
        sizeof( MemoryBlockHeader );
      ++impl->allocatedCount;
      --impl->freeCount;
      break;
    }
  }
  return result;
}

void MemorymanagerDeallocate(
  TacMemoryManager& manager,
  void* memory )
{
  if( !memory )
    return;

  MemoryManagerImpl* impl = ( MemoryManagerImpl* )manager.memory;
  MemoryBlockHeader* header = ( MemoryBlockHeader* )
    ( ( u8* )memory - sizeof( MemoryBlockHeader ) );
  int numJoined = 0;

  if( MemoryManagerJoinable( 
    impl,
    header->horizPrev ) )
  {
    MemoryBlockHeader* node = header->horizPrev;

    node->horizNext = header->horizNext;
    header->horizNext->horizPrev = node;

    node->dataSize += 
      sizeof( MemoryBlockHeader ) +
      header->dataSize;
    ++numJoined;

    // ! 
    header = node; 
  }

  if( MemoryManagerJoinable( 
    impl,
    header->horizNext ) )
  {
    MemoryBlockHeader* node = header->horizNext;

    node->horizNext->horizPrev = header;
    header->horizNext = node->horizNext;

    header->freePrev = node->freePrev;
    node->freePrev->freeNext = header;
    header->freeNext = node->freeNext;
    node->freeNext->freePrev = header;

    header->dataSize +=
      sizeof( MemoryBlockHeader ) +
      node->dataSize;
    ++numJoined;
  }

  switch( numJoined )
  {
  case 0:
    header->freeNext = &impl->dummyFree;
    header->freePrev = impl->dummyFree.freePrev;
    impl->dummyFree.freePrev->freeNext = header;
    impl->dummyFree.freePrev = header;
    ++impl->freeCount;
    break;
  case 1:
    break;
  case 2:
    --impl->freeCount;
    break;
  }

  --impl->allocatedCount;
}

void* PushSize( TacMemoryArena* arena, size_t size )
{
  void* result = nullptr;
  if( arena->used + size <= arena->size  )
  {
    result = arena->base + arena->used;
    arena->used += size;
  }
  return result;
}

void PopSize( TacMemoryArena* arena, size_t size )
{
  TacAssert( arena->used >= size )
  arena->used -= size;
}

TacTemporaryMemory BeginTemporaryMemory( TacMemoryArena* arena )
{
  TacTemporaryMemory result;
  result.arena = arena;
  result.used = arena->used;
  return result;
}

void EndTemporaryMemory( TacTemporaryMemory temporaryMemory )
{
  temporaryMemory.arena->used = temporaryMemory.used;
}

globalVariable TacMemoryManager* gMemoryManager;
TacMemoryManager* GetGlobalMemoryManager()
{
  return gMemoryManager;
}

void SetGlobalMemoryManager( TacMemoryManager* manager )
{
  gMemoryManager = manager;
}
