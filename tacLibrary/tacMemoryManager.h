#pragma once
#include "tacPlatform.h"

struct TacMemoryManager
{
  void* memory;
};

void MemoryManagerInit(
  TacMemoryManager& manager,
  void* memory,
  u32 memorySize );

void* MemoryManagerAllocate(
  TacMemoryManager& manager,
  u32 size );

void MemorymanagerDeallocate(
  TacMemoryManager& manager,
  void* memory );


// MemoryArena is a simple stack allocator
struct TacMemoryArena
{
  size_t size;
  u8* base;
  size_t used;
};
#define PushStruct( arena, mType ) ( mType* )PushSize( arena, sizeof( mType ) )
#define PushArray( arena, count, mType ) ( mType* )PushSize( arena, (count) * sizeof( mType ) )
#define PopStruct( arena, mType ) PopSize( arena, sizeof( mType ) )
#define PopArray( arena, count, mType ) PopSize( arena, (count) * sizeof( mType ) )
void* PushSize( TacMemoryArena* arena, size_t size );
void PopSize( TacMemoryArena* arena, size_t size );

struct TacTemporaryMemory
{
  TacMemoryArena* arena;
  u32 used;
};
TacTemporaryMemory BeginTemporaryMemory( TacMemoryArena* arena );
void EndTemporaryMemory( TacTemporaryMemory temporaryMemory );

TacMemoryManager* GetGlobalMemoryManager();
void SetGlobalMemoryManager( TacMemoryManager* manager );
