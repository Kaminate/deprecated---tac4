#pragma once

#include "tacTypes.h"
#include "tacString.h"
#include "tacDefines.h"
#include "tacMath.h"

struct TacWorkQueue;
struct TacThreadContext
{
  u32 logicalThreadIndex;
};

enum class KeyboardKey
{
  Unknown = 0,

  A, B, C, D, E, F, G, H, I, J, K, L, M,
  N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  Num0, Num1, Num2, Num3, Num4,
  Num5, Num6, Num7, Num8, Num9,
  Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
  Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
  F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
  Space, Enter, Backspace, Escape, Control, Alt, Shift,
  UpArrow, DownArrow, LeftArrow, RightArrow,
  Plus, Minus, Period, Comma, DecimalPoint,
  NumpadAdd,
  NumpadSubtract,
  Delete,
  eCount
};

enum class MouseButton
{
  eLeftMouseButton,
  eMiddleMouseButton,
  eRightMouseButton,
  eCount,
};

struct TacGameInput
{
  // mouse position is relative to the top left corner
  // with x increasing to the right, and
  // with y increasing downwards
  u32 mouseX;
  s32 mouseXRel;
  u32 mouseY;
  s32 mouseYRel;
  s32 mouseWheelChange;

  b32 mouseButtonEndedDownPrev[ ( u32 )MouseButton::eCount ];
  b32 mouseButtonEndedDown[ ( u32 )MouseButton::eCount ];

  b32 keyboardKeyEndedDownPrev[ ( u32 )KeyboardKey::eCount ];
  b32 keyboardKeyEndedDown[ ( u32 )KeyboardKey::eCount ];

  r64 elapsedTime; // “Time should be a double of seconds” - John Carmack
  r32 dt; // time deltas can probably be a float
  u32 elapsedFrames;
  u32 windowWidth;
  u32 windowHeight;

  // Keyboard
  b32 KeyboardDown( KeyboardKey key )
  {
    b32 result = keyboardKeyEndedDown[ ( u32 )key ];
    return result;
  }
  b32 KeyboardJustDown( KeyboardKey key )
  {
    b32 result =
      keyboardKeyEndedDown[ ( u32 )key ] &&
      !keyboardKeyEndedDownPrev[ ( u32 )key ];
    return result;
  }
  b32 KeyboardJustUp( KeyboardKey key )
  {
    b32 result =
      !keyboardKeyEndedDown[ ( u32 )key ] &&
      keyboardKeyEndedDownPrev[ ( u32 )key ];
    return result;
  }

  // Mouse
  b32 MouseDown( MouseButton button )
  {
    b32 result = mouseButtonEndedDown[ ( u32 )button ];
    return result;
  }
  b32 MouseJustDown( MouseButton button )
  {
    b32 result =
      mouseButtonEndedDown[ ( u32 )button ] &&
      !mouseButtonEndedDownPrev[ ( u32 )button ];
    return result;
  }
  b32 MouseJustUp( MouseButton button )
  {
    b32 result =
      !mouseButtonEndedDown[ ( u32 )button ] &&
      mouseButtonEndedDownPrev[ ( u32 )button ];
    return result;
  }
};


enum class FileAccess
{
  Read = 0x1,
  Write = 0x2,
};

enum class OpenFileDisposition
{
  CreateAlways,
  CreateNew,
  OpenAlways,
  OpenExisting,
  TruncateExisting,
  Count
};

struct TacFile
{
  void* mHandle;
};

TacFile PlatformOpenFile(
  TacThreadContext* thread,
  const char* path,
  OpenFileDisposition disposition,
  FileAccess access,
  FixedString< DEFAULT_ERR_LEN >& errors );

void PlatformCloseFile(
  TacThreadContext* thread,
  TacFile& file,
  FixedString< DEFAULT_ERR_LEN >& errors );

u32 PlatformGetFileSize(
  TacThreadContext* thread,
  const TacFile& file,
  FixedString< DEFAULT_ERR_LEN >& errors );

// returns true if the end of the file was reached, else returns false
bool PlatformReadEntireFile(
  TacThreadContext* thread,
  const TacFile& file,
  void* memory,
  u32 size,
  FixedString< DEFAULT_ERR_LEN >& errors );

void PlatformWriteEntireFile(
  TacThreadContext* thread,
  const TacFile& file,
  void* memory,
  u32 size,
  FixedString< DEFAULT_ERR_LEN >& errors );

time_t PlatformGetFileModifiedTime(
  TacThreadContext* thread,
  const TacFile& file,
  FixedString< DEFAULT_ERR_LEN >& errors );

time_t PlatformGetFileModifiedTime(
  TacThreadContext* thread,
  const char* filepath,
  FixedString< DEFAULT_ERR_LEN >& errors );

// Work Queue -------------------------------------------------------------

typedef void WorkQueueCallback(
  TacThreadContext* thread,
  void* data );
#include <atomic> // atomic_thead_fense
#include <vector>
#include <thread>
struct TacEntryStorage
{
  WorkQueueCallback* callback;
  void* data;
};

struct TacWorkQueue
{
  void Init( u32 numThreads, b32* running );
  std::vector< std::thread > threads;
  std::vector< TacThreadContext > threadcontexts;
  const static u32 maxEntries = 100;
  TacEntryStorage entries[ maxEntries ];
  std::atomic< u32 > nextEntryToRead;
  std::atomic< u32 > numCompleted;
  std::atomic< u32 > numToComplete;
  std::atomic< u32 > nextEntryToWrite;
};
void PushEntry(
  TacWorkQueue* queue,
  WorkQueueCallback* callback,
  void* data );

b32 DoNextEntry( TacWorkQueue* queue, TacThreadContext* thread );

void CompleteAllWork( TacWorkQueue* queue, TacThreadContext* thread );



struct TacGameMemory
{
  b32 initialized;
  TacWorkQueue* highPriorityQueue;

  u32 permanentStorageSize;
  void* permanentStorage; // required to be cleard to zero on startup

  u32 transientStorageSize;
  void* transientStorage; // required to be cleard to zero on startup
};

struct TacRenderer;
struct TacGameInterface
{
  TacThreadContext* thread;
  TacGameMemory* gameMemory;
  TacGameInput* gameInput;
  void* imguiInternalState;
  TacRenderer* renderer;
  u32 windowWidth;
  u32 windowHeight;
  b32 running;
  FixedString< DEFAULT_ERR_LEN > gameUnrecoverableErrors;
};

extern "C" void GameOnLoadStub( TacGameInterface& gameInterface );
#define TAC_GAME_LOAD_FUNCTION_NAME GameOnLoad

extern "C" void GameOnExitStub( TacGameInterface& gameInterface );
#define TAC_GAME_EXIT_FUNCTION_NAME GameOnExit

extern "C" void GameUpdateStub( TacGameInterface& gameInterface );
#define TAC_GAME_UPDATE_FUNCTION_NAME GameUpdateAndRender

struct Win32GameCode
{
  time_t lastWrite;
  void* dll;
  decltype( &GameUpdateStub ) onUpdate;
  decltype( &GameOnLoadStub ) onLoad;
  decltype( &GameOnExitStub ) onExit;
};


// TODO: make this thready
const char* VA( const char* format, ... );


struct TacWin32State
{
  TacGameMemory memory;
  TacFile recordingHandle;
  const char* filename;
  int iRecord;
  int iPlayback;
};
