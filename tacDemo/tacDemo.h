#pragma once

#include "tacLibrary\tacPlatform.h"
#include "tacLibrary\tacMemoryManager.h"
#include "tacLibrary\tacMemoryAllocator.h"
#include "tacGraphics\tacRenderer.h"
#include "tacGraphics\tac4Model.h"
#include "tacGraphics\tacModelLoader.h"
#include "tacPhysics\tacPhysics.h"

#include <vector>

enum class TacGameAssetID
{
  Cube,
  Sphere,
  Tree,
  Arrow,
  Count
};

struct TacModelRaycastInfo
{
  //v3* vertexes;
  //u32 numVertexes;
  std::vector< v3, TacMemoryAllocator< v3 > > vertexes;
  //u32* indexes;
  //u32 numIndexes;
  std::vector< u32, TacMemoryAllocator< u32 > > indexes;


  TacSphere boundingSphere;
};

struct TacGameAssets
{
  TacWorkQueue* highPriorityQueue;
  enum class AsyncTaskStatus
  {
    NotStarted,
    Started,
    PendingCompletion,
    Finished
  };
  struct AsyncModelParams
  {
    AsyncTaskStatus status;
    TacModel* model;
    u32 iTaskArena;
    ModelFormat loadedformat;
  };
  AsyncModelParams params[ ( u32 )TacGameAssetID::Count ];
  TacModel* GetModel( TacGameAssetID gameAssetID );

  TacModelRaycastInfo modelRaycastInfos[ ( u32 )TacGameAssetID::Count ];
  TacModelRaycastInfo* GetModelRaycastInfo( TacGameAssetID gameAssetID );

  TacRenderer* renderer;

  static const u32 numTaskArenas = 2;
  b32 taskArenasUseds[ numTaskArenas ];
  TacMemoryArena taskArenas[ numTaskArenas ];
  u32 GetUnusedTaskArenaIndex()
  {
    u32 iTaskArena;
    for( iTaskArena = 0; iTaskArena < numTaskArenas; ++iTaskArena )
    {
      if( !taskArenasUseds[ iTaskArena ] )
        break;
    }
    return iTaskArena;
  }
  TacVertexFormat* vertexFormats;
  u32 numVertexFormats;

};

struct MessageLoop
{
  static const u32 maxMessages = 10;
  FixedString< 256 > messages[ maxMessages ];
  u32 curMessage;
  u32 numMessages;
  r32 maxMessageTime;
  r32 elapsedMessageTime;
  void Draw();
  void Update( r32 dt );
  void AddMessage( const char* message );
  void PopMessage();
};

enum class TacEntityType
{
  Null,
  Player,
  Ground,
  Tree,
  Unknown,

  Count,
};

struct TacEntity
{
  v3 mPos;
  v3 mScale;
  v3 mRot;
  v3 mColor;

  m4 world;
  m4 worldInverse;
  // true if the position, scale, or rotation have changed
  // and the matrixes need to be updated
  b32 transformDirty;

  TacEntityType mType;
  TacGameAssetID mAssetID;
};

struct TacGameTransientState
{
  TacMemoryArena mTempAllocator;
  TacGameAssets gameAssets;
  TacWorkQueue* highPriorityQueue;
  MessageLoop messages;
};

struct TacCurve
{
  v2 GetPoint( r32 t )
  {
    v2 xy = {};
    switch( numPositions )
    {
      case 0:
      {
        // do nothing
      } break;
      case 1:
      {
        xy.y = positions[ 0 ].y;
      } break;
      case 2:
      {
        xy = Lerp( positions[ 0 ], positions[ 1 ], t );
      } break;
      case 3:
      {
        r32 oneminust = 1.0f - t;
        r32 oneminustt = Square( oneminust );
        r32 tt = Square( t );
        xy =
          oneminustt * positions[ 0 ] +
          2.0f * oneminust * t * positions[ 1 ] +
          tt * positions[ 2 ];
      } break;
      case 4:
      {
        r32 oneminust = 1.0f - t;
        r32 oneminustt = Square( oneminust );
        r32 oneminusttt = oneminust * oneminustt;
        r32 tt = Square( t );
        r32 ttt = t * tt;;
        xy =
          oneminusttt * positions[ 0 ] +
          3.0f * oneminustt * t * positions[ 1 ] +
          3.0f * oneminust * tt * positions[ 2 ] +
          ttt * positions[ 3 ];
      }
      break;
      TacInvalidDefaultCase;
    }
    return xy;
  }
  const static u32 maxpositions = 4;
  v2 positions[ maxpositions ];
  u32 numPositions;
};

struct TacParticle
{
  v3 pos;
  r32 cursize;
  r32 maxsize;
  v3 vel;
  r32 curlife;
  r32 maxlifeinverse;
  v3 color;
};

struct TacEmitter
{
  void Init()
  {
    numAlive = 0;
    numDead = maxparticles;
    spawnrate = 1;
    spawncounter = 0;
    pos = V3( 0.0f, 5.0f, 0.0f );
    accel = 9.8f;
    minlife = 0.5f;
    maxlife = 1.5f;
    shapetype = Shape::Sphere;
    shapedata.sphereradius = 3;
    minsize = 0.5f;
    maxsize = 1.5f;
    colormin = V3( 126 / 255.0f, 192 / 255.0f, 238 / 255.0f );
    colormax = V3( 180 / 255.0f, 82 / 255.0f, 205 / 255.0f );
  }
  u32 SpawnParticles( u32 desiredNum = 1 )
  {
    u32 numToSpawn = Minimum( desiredNum, numDead );
    for( u32 i = 0; i < numToSpawn; ++i )
    {
      TacParticle& p = particles[ numAlive + i ];
      v3 offset;
      switch( shapetype )
      {
        case Shape::Sphere:
        {
          offset = Normalize( V3(
            RandReal( -1.0f, 1.0f ),
            RandReal( -1.0f, 1.0f ),
            RandReal( -1.0f, 1.0f ) ) ) * RandReal( 0, shapedata.sphereradius );
        } break;
        case Shape::Rectangle:
        {
          offset = ComponentwiseMultiply( V3(
            RandReal( -1.0f, 1.0f ),
            RandReal( -1.0f, 1.0f ),
            RandReal( -1.0f, 1.0f ) ), shapedata.recthalfdimensions );
        } break;
        TacInvalidDefaultCase;
      }
      p.pos = pos + offset;
      p.cursize = p.maxsize = RandReal( minsize, maxsize );
      p.vel = V3< r32 >( 0, 0, 0 );
      p.curlife = RandReal( minlife, maxlife );
      p.maxlifeinverse = 1.0f / p.curlife;
      p.color = Lerp( colormin, colormax, RandReal( 0, 1 ) );
    }
    numAlive += numToSpawn;
    numDead -= numToSpawn;
    return numToSpawn;
  }
  void Update( r32 dt );

  static const u32 maxparticles = 100;
  TacParticle particles[ maxparticles ];
  u32 numAlive;
  u32 numDead;

  r32 minsize;
  r32 maxsize;
  r32 minlife;
  r32 maxlife;
  r32 accel;
  r32 spawnrate;
  r32 spawncounter;

  v3 pos;

  TacCurve sizeByLife;
  enum Shape
  {
    Sphere,
    Rectangle,

    Count,
  };
  union
  {
    r32 sphereradius;
    v3 recthalfdimensions;
  } shapedata;
  Shape shapetype;
  v3 colormin;
  v3 colormax;
};

struct TacGameState
{
  TacPhysics mPhysicsTest;

  u32 selectedEntityIndex;
  b32 isEntityCopied;
  TacEntity copiedEntity;

  u32 sphere0EntityIndex;
  u32 sphere1EntityIndex;


  TacGameTransientState* gameTransientState;
  void Init( TacGameInterface& gameInterface );
  void Update( TacGameInterface& gameInterface );
  void Uninit( TacGameInterface& gameInterface );

  TacMemoryManager mMemoryManager;


  time_t shaderModifiedTime;
  time_t gbufferFinalModifiedTime;
  TacShaderHandle shaderHandle;
  TacDepthStateHandle depthStateHandle;
  TacBlendStateHandle blendStateHandle;
  TacVertexFormatHandle vertexFormatHandle;
  TacRasterizerStateHandle rasterizerStateHandle;
  TacCBufferHandle cbufferHandlePerFrame;
  TacCBufferHandle cbufferHandlePerObject;

  TacTextureHandle randomVectorTexture;
  TacTextureHandle gbufferViewSpaceNormal;
  TacTextureHandle gbufferDiffuse;
  TacTextureHandle viewSpacePositionTexture;
  TacDepthBufferHandle gbufferDepthStencil;


  TacSamplerStateHandle linearSampler;
  TacSamplerStateHandle pointSampler;

  TacShaderHandle gbufferFinalShader;
  TacShaderHandle perPixelFrustumVectorsCreator;
  TacCBufferHandle cbufferHandleSSAO;

  TacDepthStateHandle noDepthTesting;
  TacVertexBufferHandle ndcQuadVBO;
  TacIndexBufferHandle ndcQuadIBO;

  TacEntity toAdd;
  u32 TacGameState::AddEntity(
    TacEntityType type,
    TacGameAssetID assetID,
    v3 scale,
    v3 rot,
    v3 translate,
    v3 color );
  std::vector< TacEntity, TacMemoryAllocator< TacEntity > > entities;
  void SaveEntities(
    TacThreadContext* thread,
    FixedString< DEFAULT_ERR_LEN >& errors );
  void SaveEntities(
    TacThreadContext* thread,
    TacFile file,
    FixedString< DEFAULT_ERR_LEN >& errors );
  void LoadEntities(
    TacThreadContext* thread,
    FixedString< DEFAULT_ERR_LEN >& errors );
  void LoadEntities(
    TacThreadContext* thread,
    TacFile file,
    FixedString< DEFAULT_ERR_LEN >& errors );
  FixedString< 128 > entitiesfile;
  u32 playerEntityIndex;
  u32 groundEntityIndex;

  b32 cameraFollowingPlayer;
  v3 playerToCameraOffset;
  v3 cameraVel;

  float cameradamping;
  float cameraangularFreq;


  TacEmitter emitter;
  FixedString< 128 > emitterfile;


  struct Camera
  {
    v3 camPos;
    v3 camDir;
    v3 camWorldUp;
    r32 camNear;
    r32 camFar;
    r32 camFOVYDeg;
  };
  Camera mCamera;
  FixedString< 128 > camerafile;
  void LoadCamera(
    TacThreadContext* thread,
    FixedString< DEFAULT_ERR_LEN >& errors );
  void SaveCamera(
    TacThreadContext* thread,
    FixedString< DEFAULT_ERR_LEN >& errors );
  void LoadCamera(
    TacThreadContext* thread,
    TacFile file,
    FixedString< DEFAULT_ERR_LEN >& errors );
  void SaveCamera(
    TacThreadContext* thread,
    TacFile file,
    FixedString< DEFAULT_ERR_LEN >& errors );

  FixedString< 128 > scenefile;
  void SaveScene(
    TacThreadContext* thread,
    FixedString< DEFAULT_ERR_LEN >& errors );
  void LoadScene(
    TacThreadContext* thread,
    FixedString< DEFAULT_ERR_LEN >& errors );

  v4 clearcolor;
};
