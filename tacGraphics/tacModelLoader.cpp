#include "tacModelLoader.h"
#include "tacRenderer.h"
#include "tac4Animation.h"
#include "tac4Model.h"
#include "tacLibrary\tacMemoryAllocator.h"

u32 GetTextureFormatSize( TacTextureFormat textureFormat )
{
  switch( textureFormat )
  {
  case TacTextureFormat::R32Float: return 4;
  case TacTextureFormat::RG32Float: return 8;
  case TacTextureFormat::RGB32Float: return 12;
  case TacTextureFormat::RGBA32Float: return 16;
    TacInvalidDefaultCase;
  }
  return 0;
}


void CopyData(
  u32 numVertexes,

  TacVariableType fromVariableType,
  u32 fromNumBytes,
  u32 fromComponentCount,
  void* fromData,
  u32 fromStride,

  TacVariableType toVariableType,
  u32 toNumBytes,
  u32 toComponentCount,
  void* toData,
  u32 toStride )
{
  u32 minComponentCount =
    Minimum( fromComponentCount, toComponentCount );

  uint8_t* fromBytes = ( uint8_t* )fromData;
  uint8_t* toBytes = ( uint8_t* )toData;

  for(
    u32 iVertex = 0;
    iVertex < numVertexes;
  ++iVertex, fromBytes += fromStride, toBytes += toStride )
  {
    for(
      u32 iComponent = 0;
      iComponent < minComponentCount;
    ++iComponent )
    {
      uint8_t* fromComponent = fromBytes + fromNumBytes * iComponent;
      r32 fromR32 = 0;

      switch( fromVariableType )
      {
      case TacVariableType::real:
        {
          r32* from = ( r32* )fromComponent;
          fromR32 = *from;
        } break;
        TacInvalidDefaultCase;
      }

      uint8_t* toComponent = toBytes + toNumBytes * iComponent;
      switch ( toVariableType )
      {
      case TacVariableType::real:
        {
          r32* to = ( r32* )toComponent;
          *to = fromR32;
        } break;
        TacInvalidDefaultCase;
      }
    }
  }
}


m4 AssimpToTacMatrix( aiMatrix4x4 m )
{
  m4 result = 
  { 
    m.a1, m.a2, m.a3, m.a4,
    m.b1, m.b2, m.b3, m.b4,
    m.c1, m.c2, m.c3, m.c4,
    m.d1, m.d2, m.d3, m.d4,
  };
  return result;
}
v3 AssimpToTacVector3( aiVector3D v )
{
  v3 result = { v.x, v.y, v.z };
  return result;
}


void TacModelLoader::OpenScene(
  uint8_t* memory,
  u32 size,
  const char* ext,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  unsigned flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals;
  mScene = mImporter.ReadFileFromMemory( memory, size, flags, ext );

  if( mScene && mScene->mRootNode )
  {
    // success
  }
  else
  {
    FixedStringAppend( errors, "Failed to read file from memory\n" );
    FixedStringAppend( errors, mImporter.GetErrorString() );
    return;
  }
}

void TacModelLoader::Release()
{
  mImporter.FreeScene();
}

ModelFormat TacModelLoader::AllocateVertexFormat(
  TacMemoryArena* arena ,
  TacVertexFormat* vertexFormats,
  u32 numVertexFormats )
{
  ModelFormat format;
  format.numsubformtas = mScene->mNumMeshes;
  format.subformats = ( SubModelFormat* )PushSize(
    arena,
    sizeof( SubModelFormat ) * format.numsubformtas );

  u32* inputSlots =
    ( u32* )PushSize( arena, sizeof( u32 ) * numVertexFormats );
  {
    for( u32 i = 0; i < numVertexFormats; ++i )
    {
      TacVertexFormat& vertexFormat = vertexFormats[ i ];
      inputSlots[ i ] = vertexFormat.mInputSlot;
    }
    std::sort( inputSlots, inputSlots + numVertexFormats );
    u32* it = std::unique( inputSlots, inputSlots + numVertexFormats );
    format.numVertexBuffers = std::distance( inputSlots, it );
  }

  auto GetVertexBufferIndex = [&]( const TacVertexFormat& vertexFormat )
  {
    u32 vertexBufferIndex;
    for( vertexBufferIndex = 0;
      vertexBufferIndex < format.numVertexBuffers;
      ++vertexBufferIndex )
    {
      if( vertexFormat.mInputSlot == inputSlots[ vertexBufferIndex ] )
        break;
    }
    TacAssertIndex( vertexBufferIndex, format.numVertexBuffers );
    return vertexBufferIndex;
  };

  format.vertexBufferStrides =
    ( u32* )PushSize( arena, sizeof( u32 ) * format.numVertexBuffers );
  for(
    u32 iVertexBuffer = 0;
    iVertexBuffer < format.numVertexBuffers;
    ++iVertexBuffer )
  {
    format.vertexBufferStrides[ iVertexBuffer ] = 0;
  }
  {
    for( u32 i = 0; i < numVertexFormats; ++i )
    {
      TacVertexFormat& vertexFormat = vertexFormats[ i ];

      u32 spaceNeeded =
        vertexFormat.mAlignedByteOffset +
        GetTextureFormatSize( vertexFormat.textureFormat );
        //vertexFormat.mNumBytes * vertexFormat.mNumComponents;

      u32 index = GetVertexBufferIndex( vertexFormat );
      u32& stride = format.vertexBufferStrides[ index ];
      stride = Maximum( stride, spaceNeeded );
    }
  }

  for( u32 iMesh = 0; iMesh < mScene->mNumMeshes; ++iMesh )
  {
    aiMesh* mesh = mScene->mMeshes[ iMesh ];
    SubModelFormat& subformat = format.subformats[ iMesh ];

    subformat.numVertexes = mesh->mNumVertices;

    // allocate memory
    {
      subformat.vertexBuffers = ( void** )PushSize(
        arena,
        sizeof( void* ) * format.numVertexBuffers );
      for(
        u32 iVertexBuffer = 0;
        iVertexBuffer < format.numVertexBuffers;
      ++iVertexBuffer )
      {
        u32 vertexBufferSize =
          format.vertexBufferStrides[ iVertexBuffer ] *
          subformat.numVertexes; 

        void* vertexBufferData =
          ( void* )PushSize( arena, vertexBufferSize );
        TacAssert( vertexBufferData );
        memset( vertexBufferData, 0, vertexBufferSize );
        subformat.vertexBuffers[ iVertexBuffer ] = vertexBufferData;
      }
    }

    // copy data
    for(
      u32 iVertexFormat = 0;
      iVertexFormat < numVertexFormats;
    ++iVertexFormat )
    {
      TacVertexFormat& vertexFormat = vertexFormats[ iVertexFormat ];
      u32 vertexBufferIndex = GetVertexBufferIndex( vertexFormat );

      TacVariableType fromMeshVariableType = TacVariableType::real;
      u32 fromMeshNumBytes = 0;
      u32 fromMeshComponentCount = 0;
      void* fromMeshData = 0;
      u32 fromMeshStride = 0;

      u8* toData =
        ( u8* )subformat.vertexBuffers[ vertexBufferIndex ] +
        vertexFormat.mAlignedByteOffset;
      u32 toStride = format.vertexBufferStrides[ vertexBufferIndex ] ;

      switch( vertexFormat.mAttributeType )
      {
      case TacAttributeType::Position:
        {
          TacAssert( mesh->HasPositions() );
          fromMeshVariableType = TacVariableType::real;
          fromMeshNumBytes = 4;
          fromMeshComponentCount = 3;
          fromMeshData = ( void* )mesh->mVertices;
          fromMeshStride = sizeof( aiVector3D );
        } break;
      case TacAttributeType::Normal:
        {
          TacAssert( mesh->HasNormals() );
          fromMeshVariableType = TacVariableType::real;
          fromMeshNumBytes = 4;
          fromMeshComponentCount = 3;
          fromMeshData = ( void* )mesh->mNormals;
          fromMeshStride = sizeof( aiVector3D );
        } break;
      case TacAttributeType::Texcoord:
        {
          const u32 textureCoordIndex = 0;
          if( !mesh->HasTextureCoords( textureCoordIndex  ) )
          {
            continue;
          }
          fromMeshVariableType = TacVariableType::real;
          fromMeshNumBytes = 4;
          fromMeshComponentCount = 3;
          fromMeshData =
            ( void* )mesh->mTextureCoords[ textureCoordIndex ];
          fromMeshStride = sizeof( aiVector3D );
        } break;
        TacInvalidDefaultCase;
      }

      TacVariableType toVariableType = TacVariableType::count;
      u32 toNumBytes = 0;
      u32 toNumComponents = 0;
      switch( vertexFormat.textureFormat )
      {
      case TacTextureFormat::RG32Float:
        {
          toVariableType = TacVariableType::real;
          toNumBytes = 4;
          toNumComponents = 2;
        } break;
      case TacTextureFormat::RGB32Float:
        {
          toVariableType = TacVariableType::real;
          toNumBytes = 4;
          toNumComponents = 3;
        } break;
      case TacTextureFormat::RGBA32Float:
        {
          toVariableType = TacVariableType::real;
          toNumBytes = 4;
          toNumComponents = 4;
        } break;
        TacInvalidDefaultCase;
      }


      CopyData(
        subformat.numVertexes,
        fromMeshVariableType,
        fromMeshNumBytes,
        fromMeshComponentCount,
        fromMeshData,
        fromMeshStride,
        toVariableType,
        toNumBytes,
        toNumComponents,
        toData,
        toStride );
    }


    // index data
    subformat.numIndexes = mesh->mNumFaces * 3;
    subformat.indexBuffer =
      ( u32* )PushSize( arena, sizeof( u32 ) * subformat.numIndexes );
    {
      u32 index = 0;
      for( u32 i = 0; i < mesh->mNumFaces; ++i )
      {
        aiFace& curFace = mesh->mFaces[ i ];
        subformat.indexBuffer[ index++ ] = curFace.mIndices[ 0 ];
        subformat.indexBuffer[ index++ ] = curFace.mIndices[ 1 ];
        subformat.indexBuffer[ index++ ] = curFace.mIndices[ 2 ];
      }
    }

    format.indexFormat = TacTextureFormat::R32Uint;
    //format.indexVariableType = TacVariableType::uint;
    //format.indexNumBytes = 4;
  }

  return format;
}
