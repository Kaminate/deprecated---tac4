#pragma once

#include "tacLibrary/tacPlatform.h"
#include "tacRenderer.h"
#include "3rdparty\assimp\Importer.hpp"  // C++ importer interface
#include "3rdparty\assimp\scene.h"       // Output data structure
#include "3rdparty\assimp\postprocess.h" // Post processing flags

//struct TacModel;
//struct TacRenderer;
struct TacMemoryArena;

struct SubModelFormat
{
  void** vertexBuffers;
  u32 numVertexes;

  u32* indexBuffer;
  u32 numIndexes;
};
struct ModelFormat
{
  SubModelFormat* subformats;
  u32 numsubformtas;
  u32* vertexBufferStrides;
  u32 numVertexBuffers;
  TacTextureFormat indexFormat;
  //TacVariableType indexVariableType;
  //u32 indexNumBytes;
};
struct TacModelLoader
{
  void OpenScene(
    uint8_t* memory,
    u32 size,
    const char* ext,
    FixedString< DEFAULT_ERR_LEN >& errors );
  void Release();

  /*
  void LoadModel(
  TacRenderer* renderer,
  TacModel& model,
  TacVertexFormat* vertexFormats,
  u32 numVertexFormats,
  FixedString< DEFAULT_ERR_LEN >& errors );
  */

  ModelFormat AllocateVertexFormat(
    TacMemoryArena* arena,
    TacVertexFormat* vertexFormats,
    u32 numVertexFormats );

  Assimp::Importer mImporter;
  const aiScene* mScene;
};

