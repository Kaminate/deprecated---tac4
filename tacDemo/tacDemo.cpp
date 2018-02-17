#include "tacDemo.h"
#include "tacLibrary\imgui\imgui.h"
#include "tacLibrary\tacRaycast.h"
#include "tacGraphics\tac3camera.h"
#include "tacGraphics\tacRenderGroup.h"

const char* demofinalfilepath = "tacData/Shaders/Directx/DemoFinal.fx";
const char* demofxfilepath = "tacData/Shaders/Directx/Demo.fx";

struct FrustumCorners
{
  v4 viewSpaceFrustumCorners[ 4 ];
};

FrustumCorners GetFrustumCorners( r32 camfar, r32 fovydeg, r32 aspect )
{
  FrustumCorners result = {};
  r32 farPlaneHalfHeight =
    camfar * tan( DEG2RAD * fovydeg * 0.5f );
  r32 farPlaneHalfWidth = aspect * farPlaneHalfHeight;
  r32 xSign[ 4 ] = { -1, 1, 1, -1 };
  r32 ySign[ 4 ] = { -1, -1, 1, 1 };

  for( u32 i = 0; i < 4; ++i )
  {
    v4& corner = result.viewSpaceFrustumCorners[ i ];
    corner.x = xSign[ i ] * farPlaneHalfWidth;
    corner.y = ySign[ i ] * farPlaneHalfHeight;
    corner.z = -camfar;
  }
  return result;
}

const u32 numRows = 4;
const u32 numCols = 4;
const char* assetIDstrings[ ( u32 )TacGameAssetID::Count ] =
{
  "Cube",
  "Sphere",
  "Tree",
  "Arrow",
};

struct CBufferPerFrame
{
  static const char* GetName() { return "CBufferPerFrame"; }
  static const char* GetNameView() { return "View"; };
  static const char* GetNameProj() { return "Projection"; };
  static const char* GetNameViewSpaceFrustumCorners() { return "viewSpaceFrustumCorners"; };
  static const char* GetNameFar() { return "far"; };
  static const char* GetNameNear() { return "near"; };
  static const char* GetNameGBufferSize() { return "gbufferSize"; };
  static const char* GetNameClearColor() { return "clearcolor"; };
};

struct CBufferSSAO
{
  static const char* GetName(){ return "CBufferSSAO"; }
  static const char* GetNameSSAOKernel(){ return "ssaoKernel"; }
  static const char* GetNameRandomVectorsSize(){ return "randomVectorsSize"; }
  static const char* GetNameSSAORadius(){ return "ssaoRadius"; }
  static const int GetArrayCountSSAOKernel = 8;
};

struct DemoFinalShader
{
  // cbuffers
  static u32 GetRegisterCBufferPerFrame() { return 0; };
  static u32 GetRegisterCBufferSSAO() { return 1; };

  // textures
  static const char* GetNameGBufferColor() { return "gbufferColor"; }
  static u32 GetRegisterGBufferColor() { return 0; }

  static const char* GetNameGBufferViewSpaceNormal() { return "gbufferViewSpaceNormal"; }
  static u32 GetRegisterGBufferViewSpaceNormal() { return 1; }

  static const char* GetNameRandomVectors() { return "randomVectors"; }
  static u32 GetRegisterRandomVectors() { return 2; }

  static const char* GetNameviewSpacePositionTexture() { return "viewSpacePositionTexture"; }
  static u32 GetRegisterviewSpacePositionTexture() { return 3; }

  // samplers
  static const char* GetNameLinearSampler() { return "linearSampler"; }
  static u32 GetRegisterLinearSampler() { return 0; }

  static const char* GetNamePointSampler() { return "pointSampler"; }
  static u32 GetRegisterPointSampler() { return 1; }

};

struct Constants
{
  Constants( const char* name ) :offset( 0 ), mname( name ){}
  void Add( const char* name, u32 size )
  {
    // todo: auto add padding
    TacConstant constant;
    constant.name = name;
    constant.offset = offset;
    offset += ( constant.size = size );
    constantVector.push_back( constant );

  }
  TacCBufferHandle Create(
    TacRenderer* renderer,
    FixedString< DEFAULT_ERR_LEN >& errors )
  {
    TacCBufferHandle result = renderer->LoadCbuffer(
      mname,
      constantVector.data(),
      constantVector.size(),
      errors );
    return result;
  }

  u32 offset;
  const char* mname;
  std::vector< TacConstant > constantVector;
};

v3 GetRandomTreeColor()
{
  auto ToColor = [] ( const char* str )
  {
    v3 result;
    char buffer[ 3 ] = {};
    buffer[ 0 ] = str[ 1 ];
    buffer[ 1 ] = str[ 2 ];
    result.x = ( r32 )strtol( buffer, 0, 16 );
    buffer[ 0 ] = str[ 3 ];
    buffer[ 1 ] = str[ 4 ];
    result.y = ( r32 )strtol( buffer, 0, 16 );
    buffer[ 0 ] = str[ 5 ];
    buffer[ 1 ] = str[ 6 ];
    result.z = ( r32 )strtol( buffer, 0, 16 );
    result /= 255.0f;
    return result;
  };
  const u32 numcolors = 7;
  v3 colors[ numcolors ] = {};
  u32 icolor = 0;
  colors[ icolor++ ] = ToColor( "#A37F16" );
  colors[ icolor++ ] = ToColor( "#AF7206" );
  colors[ icolor++ ] = ToColor( "#AF5206" );
  colors[ icolor++ ] = ToColor( "#AB910B" );
  colors[ icolor++ ] = ToColor( "#988922" );
  colors[ icolor++ ] = ToColor( "#847A33" );
  colors[ icolor++ ] = ToColor( "#88995D" );
  r32 probabilities[ numcolors ] =
  { 0.07f, 0.08f, 0.10f, 0.30f, 0.30f, 0.10f, .05f };
  r32 cumulativeProbabilities[ numcolors ];
  cumulativeProbabilities[ 0 ] = probabilities[ 0 ];
  for( u32 iProbability = 1; iProbability < numcolors; ++iProbability )
  {
    cumulativeProbabilities[ iProbability ] =
      cumulativeProbabilities[ iProbability - 1 ] +
      probabilities[ iProbability ];
  }
  r32 probability = RandReal();
  u32 colorIndex = 0;
  for( u32 iColor = 0; iColor < numcolors; ++iColor )
  {
    if( probability < cumulativeProbabilities[ iColor ] )
    {
      colorIndex = iColor;
      break;
    }
  }
  v3 color = colors[ colorIndex ];
  return color;
}
// ------------------------------------------------------------------------

extern "C" __declspec( dllexport )
void GameOnLoad(
  TacGameInterface& gameInterface )
{
  ImGui::SetInternalState( gameInterface.imguiInternalState );

  if( !gameInterface.gameMemory->initialized )
  {
    TacGameState* state =
      ( TacGameState* )gameInterface.gameMemory->permanentStorage;
    TacGameTransientState* statetransient =
      ( TacGameTransientState* )gameInterface.gameMemory->transientStorage;
    gameInterface.gameMemory->initialized = true;

    if( sizeof( TacGameState ) >
      gameInterface.gameMemory->permanentStorageSize )
    {
      gameInterface.gameUnrecoverableErrors.size = sprintf_s(
        gameInterface.gameUnrecoverableErrors.buffer,
        "Not enough memory( size: %i ) to store GameState( size: %i )",
        gameInterface.gameMemory->permanentStorageSize,
        sizeof( TacGameState ) );
      return;
    }

    new( state )TacGameState();

    {
      void* memory =
        ( u8* )gameInterface.gameMemory->permanentStorage +
        sizeof( TacGameState );
      u32 memorySize =
        gameInterface.gameMemory->permanentStorageSize -
        sizeof( TacGameState );
      MemoryManagerInit(
        state->mMemoryManager,
        memory,
        memorySize );
      SetGlobalMemoryManager( &state->mMemoryManager );
    }

    if( sizeof( TacGameTransientState ) >
      gameInterface.gameMemory->transientStorageSize )
    {
      gameInterface.gameUnrecoverableErrors.size = sprintf_s(
        gameInterface.gameUnrecoverableErrors.buffer,
        "Not enough memory( size: %i ) to store "
        "GameTransientState( size: %i )",
        gameInterface.gameMemory->transientStorageSize,
        sizeof( TacGameTransientState ) );
      return;
    }

    // init transient memory allocator
    {
      statetransient->mTempAllocator.base =
        ( uint8_t* )gameInterface.gameMemory->transientStorage +
        sizeof( TacGameTransientState );
      statetransient->mTempAllocator.size =
        gameInterface.gameMemory->transientStorageSize -
        sizeof( TacGameTransientState );
      statetransient->mTempAllocator.used = 0;


    }

    statetransient->highPriorityQueue =
      gameInterface.gameMemory->highPriorityQueue;


    state->gameTransientState = statetransient;
    state->Init( gameInterface );
  }

}

extern "C" __declspec( dllexport )
void GameUpdateAndRender(
  TacGameInterface& gameInterface )
{
  TacGameState* state = ( TacGameState* )gameInterface.gameMemory->permanentStorage;
  state->Update( gameInterface );
}

extern "C" __declspec( dllexport )
void GameOnExit(
  TacGameInterface& gameInterface )
{
  TacGameState* state = ( TacGameState* )gameInterface.gameMemory->permanentStorage;
  state->Uninit( gameInterface );
}

// ------------------------------------------------------------------------

struct DemoVertex
{
  v3 pos;
  v3 nor;
  v2 uv;
  static const u32 numVertexFormats = 3;
  static void Fill( TacVertexFormat* vertexFormats )
  {
    {
      TacVertexFormat& vertexFormat = vertexFormats[ 0 ];
      vertexFormat.mAlignedByteOffset = OffsetOf( DemoVertex, pos );
      vertexFormat.mAttributeType = TacAttributeType::Position;
      vertexFormat.mInputSlot = 0;
      vertexFormat.textureFormat = TacTextureFormat::RGB32Float;
      //vertexFormat.mNumBytes = 4;
      //vertexFormat.mNumComponents = 3;
      //vertexFormat.mVariableType = TacVariableType::real;
    }

    {
      TacVertexFormat& vertexFormat = vertexFormats[ 1 ];
      vertexFormat.mAlignedByteOffset = OffsetOf( DemoVertex, nor );
      vertexFormat.mAttributeType = TacAttributeType::Normal;
      vertexFormat.mInputSlot = 0;
      vertexFormat.textureFormat = TacTextureFormat::RGB32Float;
      //vertexFormat.mNumBytes = 4;
      //vertexFormat.mNumComponents = 3;
      //vertexFormat.mVariableType = TacVariableType::real;
    }

    {
      TacVertexFormat& vertexFormat = vertexFormats[ 2 ];
      vertexFormat.mAlignedByteOffset = OffsetOf( DemoVertex, uv );
      vertexFormat.mAttributeType = TacAttributeType::Texcoord;
      vertexFormat.mInputSlot = 0;
      vertexFormat.textureFormat = TacTextureFormat::RG32Float;
      //vertexFormat.mNumBytes = 4;
      //vertexFormat.mNumComponents = 2;
      //vertexFormat.mVariableType = TacVariableType::real;
    }
  }
};

void TacGameState::Init( TacGameInterface& gameInterface )
{
  clearcolor = V4< r32 >(
    126 / 255.0f,
    192 / 255.0f,
    238 / 255.0f,
    255 / 255.0f );
  gameInterface.renderer->DebugBegin( "Game init" );
  OnDestruct( gameInterface.renderer->DebugEnd(); );


  // load shader
  {
    const char* shaderPaths[ ( u32 )TacShaderType::Count ] = {};
    const char* entryPoints[ ( u32 )TacShaderType::Count ] = {};
    const char* shaderModels[ ( u32 )TacShaderType::Count ] = {};

    u32 shaderTypeIndex = ( u32 )TacShaderType::Vertex;
    shaderPaths[ shaderTypeIndex ] = demofxfilepath;
    entryPoints[ shaderTypeIndex ] = "VS";
    shaderModels[ shaderTypeIndex ] = "vs_4_0";

    shaderTypeIndex = ( u32 )TacShaderType::Fragment;
    shaderPaths[ shaderTypeIndex ] = demofxfilepath;
    entryPoints[ shaderTypeIndex ] = "PS";
    shaderModels[ shaderTypeIndex ] = "ps_4_0";

    shaderHandle = gameInterface.renderer->LoadShader(
      shaderPaths,
      entryPoints,
      shaderModels,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    gameInterface.renderer->SetName(
      shaderHandle,
      STRINGIFY( shaderHandle ) );

    FixedString< DEFAULT_ERR_LEN > errors = {};
    time_t time = PlatformGetFileModifiedTime( gameInterface.thread, demofxfilepath, errors );
    if( errors.size )
    {
      gameTransientState->messages.AddMessage( errors.buffer );
    }
    else
    {
      shaderModifiedTime = time;
    }
  }

  // init the temporary state
  {
    gameTransientState->messages.maxMessageTime = 2;

    gameTransientState->mTempAllocator;
    gameTransientState->gameAssets.highPriorityQueue =
      gameTransientState->highPriorityQueue;
    gameTransientState->gameAssets.renderer = gameInterface.renderer;
    for(
      u32 i = 0;
      i < gameTransientState->gameAssets.numTaskArenas;
      ++i )
    {
      TacMemoryArena& arena =
        gameTransientState->gameAssets.taskArenas[ i ];
      arena.base = ( u8* )PushSize(
        &gameTransientState->mTempAllocator,
        arena.size = Kilobytes( 1024 ) );
    }

    // load vertex format
    {
      gameTransientState->gameAssets.vertexFormats = ( TacVertexFormat* )
        PushSize( &gameTransientState->mTempAllocator,
          sizeof( TacVertexFormat ) * DemoVertex::numVertexFormats );
      gameTransientState->gameAssets.numVertexFormats =
        DemoVertex::numVertexFormats;
      DemoVertex::Fill( gameTransientState->gameAssets.vertexFormats );
      vertexFormatHandle = gameInterface.renderer->AddVertexFormat(
        gameTransientState->gameAssets.vertexFormats,
        gameTransientState->gameAssets.numVertexFormats,
        shaderHandle,
        gameInterface.gameUnrecoverableErrors );

      gameInterface.renderer->SetName(
        vertexFormatHandle,
        STRINGIFY( vertexFormatHandle ) );
    }

  }


  scenefile = "tacData/.tacBinary";

  FixedString< DEFAULT_ERR_LEN > sceneErrors = {};
  LoadScene( gameInterface.thread, sceneErrors );
  if( sceneErrors.size )
  {
    gameTransientState->messages.AddMessage( sceneErrors.buffer );
  }

  playerEntityIndex = groundEntityIndex = 0;
  for( u32 iEntity = 0; iEntity < entities.size(); ++iEntity )
  {
    TacEntity& entity = entities[ iEntity ];
    if( entity.mType == TacEntityType::Player )
    {
      playerEntityIndex = iEntity;
    }
    if( entity.mType == TacEntityType::Ground )
    {
      groundEntityIndex = iEntity;
    }
  }

  if( playerEntityIndex )
  {
    playerToCameraOffset =
      mCamera.camPos -
      entities[ playerEntityIndex ].mPos;
  }

  cameradamping = 1;
  cameraangularFreq = 3.88f;
  cameraFollowingPlayer = true;
  selectedEntityIndex = playerEntityIndex;

  // load depth state
  {
    depthStateHandle = gameInterface.renderer->AddDepthState(
      true,
      true,
      TacDepthFunc::Less,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    gameInterface.renderer->SetName(
      depthStateHandle,
      STRINGIFY( depthStateHandle ) );
  }

  // load blend state
  {
    blendStateHandle = gameInterface.renderer->AddBlendState(
      TacBlendConstants::One,
      TacBlendConstants::Zero,
      TacBlendMode::Add,
      TacBlendConstants::One,
      TacBlendConstants::Zero,
      TacBlendMode::Add,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      blendStateHandle,
      STRINGIFY( blendStateHandle ) );
  }

  // load rasterizer state
  {
    rasterizerStateHandle = gameInterface.renderer->AddRasterizerState(
      TacFillMode::Solid,
      TacCullMode::Back,
      true,
      false,
      false,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      rasterizerStateHandle,
      STRINGIFY( rasterizerStateHandle ) );
  }

  // constant buffer per frame
  {
    Constants constants( CBufferPerFrame::GetName() );
    constants.Add(
      CBufferPerFrame::GetNameView(),
      sizeof( m4 ) );
    constants.Add(
      CBufferPerFrame::GetNameProj(),
      sizeof( m4 ) );
    constants.Add(
      CBufferPerFrame::GetNameViewSpaceFrustumCorners(),
      sizeof( v4 ) * 4 );
    constants.Add(
      CBufferPerFrame::GetNameFar(),
      sizeof( r32 ) );
    constants.Add(
      CBufferPerFrame::GetNameNear(),
      sizeof( r32 ) );
    constants.Add(
      CBufferPerFrame::GetNameGBufferSize(),
      sizeof( v2 ) );
    constants.Add(
      CBufferPerFrame::GetNameClearColor(),
      sizeof( v4 ) );
    cbufferHandlePerFrame = constants.Create(
      gameInterface.renderer,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      cbufferHandlePerFrame,
      STRINGIFY( cbufferHandlePerFrame ) );

    gameInterface.renderer->AddCbuffer(
      shaderHandle,
      cbufferHandlePerFrame,
      0,
      TacShaderType::Vertex,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    gameInterface.renderer->AddCbuffer(
      shaderHandle,
      cbufferHandlePerFrame,
      0,
      TacShaderType::Fragment,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
  }

  // constant buffer per object
  {
    Constants constants( "CBufferPerObject" );
    constants.Add( "World", sizeof( m4 ) );
    constants.Add( "Color", sizeof( v3 ) );
    cbufferHandlePerObject = constants.Create(
      gameInterface.renderer,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      cbufferHandlePerObject,
      STRINGIFY( cbufferHandlePerObject ) );

    gameInterface.renderer->AddCbuffer(
      shaderHandle,
      cbufferHandlePerObject,
      1,
      TacShaderType::Vertex,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    gameInterface.renderer->AddCbuffer(
      shaderHandle,
      cbufferHandlePerObject,
      1,
      TacShaderType::Fragment,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
  }

  // ndc quad
  {
    // vbo
    {
      const u32 numQuadVertexes = 4;
      DemoVertex vertexData[ numQuadVertexes ] = {};
      {
        r32 xSign[ 4 ] = { -1, 1, 1, -1 };
        r32 ySign[ 4 ] = { -1, -1, 1, 1 };
        for( u32 i = 0; i < 4; ++i )
        {
          DemoVertex& vertex = vertexData[ i ];
          vertex.pos = V3( xSign[ i ], ySign[ i ], 0.0f );
          vertex.uv = ( vertex.pos.xy + V2( 1.0f, 1.0f ) ) * 0.5f;
          vertex.nor.x = ( r32 )i;
        }
      }

      ndcQuadVBO = gameInterface.renderer->AddVertexBuffer(
        TacBufferAccess::Static,
        vertexData,
        numQuadVertexes,
        sizeof( DemoVertex ),
        gameInterface.gameUnrecoverableErrors );
      if( gameInterface.gameUnrecoverableErrors.size )
        return;
      gameInterface.renderer->SetName(
        ndcQuadVBO,
        STRINGIFY( ndcQuadVBO ) );
    }

    // ibo
    {
      const u32 numIndexes = 6;
      u32 indexData[ numIndexes ] =
      {
        0, 1, 2,
        0, 2, 3,
      };
      ndcQuadIBO = gameInterface.renderer->AddIndexBuffer(
        TacBufferAccess::Static,
        indexData,
        numIndexes,
        TacTextureFormat::R32Uint,
        sizeof( indexData ),
        //TacVariableType::uint,
        //4,
        gameInterface.gameUnrecoverableErrors );
      if( gameInterface.gameUnrecoverableErrors.size )
        return;
      gameInterface.renderer->SetName(
        ndcQuadIBO,
        STRINGIFY( ndcQuadIBO ) );
    }
  }

  // no depth testing
  {
    noDepthTesting = gameInterface.renderer->AddDepthState(
      false,
      false,
      TacDepthFunc::Less,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      noDepthTesting,
      STRINGIFY( noDepthTesting ) );
  }

  // create the viewspace position texture
  {
    TacImage image = {};
    image.mWidth = gameInterface.windowWidth;
    image.mHeight = gameInterface.windowHeight;

    // better precision, doesnt have banding
    image.mTextureFormat = TacTextureFormat::RGBA32Float;
    //image.mTextureFormat = TacTextureFormat::RGBA16Float;
    image.textureDimension = 2;
    viewSpacePositionTexture = gameInterface.renderer->AddTextureResource(
      image,
      TacTextureUsage::Default,
      TacBinding::ShaderResource | TacBinding::RenderTarget,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      viewSpacePositionTexture,
      STRINGIFY( viewSpacePositionTexture ) );
  }

  // deferred?
  TacImage myimage = {};
  myimage.mWidth = gameInterface.windowWidth;
  myimage.mHeight = gameInterface.windowHeight;
  myimage.textureDimension = 2;
  myimage.mData = nullptr;
  myimage.mTextureFormat = TacTextureFormat::RGBA8Snorm;
  //myimage.mTextureFormat = TacTextureFormat::RGBA32Float;
  gbufferViewSpaceNormal = gameInterface.renderer->AddTextureResource(
    myimage,
    TacTextureUsage::Default,
    TacBinding::RenderTarget | TacBinding::ShaderResource,
    gameInterface.gameUnrecoverableErrors );
  if( gameInterface.gameUnrecoverableErrors.size )
    return;
  gameInterface.renderer->SetName(
    gbufferViewSpaceNormal,
    STRINGIFY( gbufferViewSpaceNormal ) );

  myimage.mTextureFormat = TacTextureFormat::RGBA8Unorm;
  //myimage.mTextureFormat = TacTextureFormat::RGBA32Float;
  gbufferDiffuse = gameInterface.renderer->AddTextureResource(
    myimage,
    TacTextureUsage::Default,
    TacBinding::RenderTarget | TacBinding::ShaderResource,
    gameInterface.gameUnrecoverableErrors );
  if( gameInterface.gameUnrecoverableErrors.size )
    return;
  gameInterface.renderer->SetName(
    gbufferDiffuse,
    STRINGIFY( gbufferDiffuse ) );

  //myimage.mTextureFormat = TacTextureFormat::R32Float;
  ////myimage.mTextureFormat = TacTextureFormat::RGBA32Float;
  //gbufferLinearDepth = gameInterface.renderer->AddTextureResource(
  //  myimage,
  //  TacTextureUsage::Default,
  //  TacBinding::RenderTarget | TacBinding::ShaderResource,
  //  gameInterface.gameUnrecoverableErrors );
  //if( gameInterface.gameUnrecoverableErrors.size )
  //  return;
  //gameInterface.renderer->SetName(
  //  gbufferLinearDepth,
  //  STRINGIFY( gbufferLinearDepth ) );

  gbufferDepthStencil = gameInterface.renderer->AddDepthBuffer(
    gameInterface.windowWidth,
    gameInterface.windowHeight,
    TacTextureFormat::D24S8,
    //24,
    //8,
    gameInterface.gameUnrecoverableErrors );
  if( gameInterface.gameUnrecoverableErrors.size )
    return;
  gameInterface.renderer->SetName(
    gbufferDepthStencil,
    STRINGIFY( gbufferDepthStencil ) );

  // gbuffer shader + texture + sampler
  {
    // load the shader
    {
      const char* shaderPaths[ ( u32 )TacShaderType::Count ] = {};
      const char* entryPoints[ ( u32 )TacShaderType::Count ] = {};
      const char* shaderModels[ ( u32 )TacShaderType::Count ] = {};


      u32 shaderTypeIndex = ( u32 )TacShaderType::Vertex;
      shaderPaths[ shaderTypeIndex ] = demofinalfilepath;
      entryPoints[ shaderTypeIndex ] = "VS";
      shaderModels[ shaderTypeIndex ] = "vs_4_0";

      shaderTypeIndex = ( u32 )TacShaderType::Fragment;
      shaderPaths[ shaderTypeIndex ] = demofinalfilepath;
      entryPoints[ shaderTypeIndex ] = "PS";
      shaderModels[ shaderTypeIndex ] = "ps_4_0";

      gbufferFinalShader = gameInterface.renderer->LoadShader(
        shaderPaths,
        entryPoints,
        shaderModels,
        gameInterface.gameUnrecoverableErrors );
      if( gameInterface.gameUnrecoverableErrors.size )
        return;
      gameInterface.renderer->SetName(
        gbufferFinalShader,
        STRINGIFY( gbufferFinalShader ) );

      FixedString< DEFAULT_ERR_LEN > errors = {};
      time_t time = PlatformGetFileModifiedTime( gameInterface.thread, demofinalfilepath, errors );
      if( errors.size )
      {
        gameTransientState->messages.AddMessage( errors.buffer );
      }
      else
      {
        gbufferFinalModifiedTime = time;
      }

    }

    linearSampler = gameInterface.renderer->AddSamplerState(
      TacAddressMode::Clamp,
      TacAddressMode::Clamp,
      TacAddressMode::Clamp,
      TacComparison::Never,
      TacFilter::Linear,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      linearSampler,
      STRINGIFY( linearSampler ) );
    gameInterface.renderer->AddSampler(
      DemoFinalShader::GetNameLinearSampler(),
      gbufferFinalShader,
      TacShaderType::Fragment,
      DemoFinalShader::GetRegisterLinearSampler(),
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    pointSampler = gameInterface.renderer->AddSamplerState(
      TacAddressMode::Wrap,
      TacAddressMode::Wrap,
      TacAddressMode::Wrap,
      TacComparison::Never,
      TacFilter::Point,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      pointSampler,
      STRINGIFY( pointSampler ) );

    gameInterface.renderer->AddSampler(
      DemoFinalShader::GetNamePointSampler(),
      gbufferFinalShader,
      TacShaderType::Fragment,
      DemoFinalShader::GetRegisterPointSampler(),
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;


    gameInterface.renderer->AddTexture(
      DemoFinalShader::GetNameGBufferColor(),
      gbufferFinalShader,
      TacShaderType::Fragment,
      DemoFinalShader::GetRegisterGBufferColor(),
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    gameInterface.renderer->AddTexture(
      DemoFinalShader::GetNameGBufferViewSpaceNormal(),
      gbufferFinalShader,
      TacShaderType::Fragment,
      DemoFinalShader::GetRegisterGBufferViewSpaceNormal(),
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    //gameInterface.renderer->AddTexture(
    //  DemoFinalShader::GetNameGBufferDepth(),
    //  gbufferFinalShader,
    //  TacShaderType::Fragment,
    //  DemoFinalShader::GetRegisterGBufferDepth(),
    //  gameInterface.gameUnrecoverableErrors );
    //if( gameInterface.gameUnrecoverableErrors.size )
    //  return;

    //gameInterface.renderer->AddTexture(
    //  DemoFinalShader::GetNameViewSpaceFrustumCornerTexture(),
    //  gbufferFinalShader,
    //  TacShaderType::Fragment,
    //  DemoFinalShader::GetRegisterViewSpaceFrustumCornerTexture(),
    //  gameInterface.gameUnrecoverableErrors );
    //if( gameInterface.gameUnrecoverableErrors.size )
    //  return;

    gameInterface.renderer->AddTexture(
      DemoFinalShader::GetNameviewSpacePositionTexture(),
      gbufferFinalShader,
      TacShaderType::Fragment,
      DemoFinalShader::GetRegisterviewSpacePositionTexture(),
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    gameInterface.renderer->AddCbuffer(
      gbufferFinalShader,
      cbufferHandlePerFrame,
      DemoFinalShader::GetRegisterCBufferPerFrame(),
      TacShaderType::Vertex,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    gameInterface.renderer->AddCbuffer(
      gbufferFinalShader,
      cbufferHandlePerFrame,
      DemoFinalShader::GetRegisterCBufferPerFrame(),
      TacShaderType::Fragment,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    srand( 0 );
    std::vector< v4 > randomVectors;
    randomVectors.resize( numRows * numCols );
    for( u32 i = 0; i < numRows * numCols; ++i )
    {
      v4 v =
      {
        RandReal( -1, 1 ),
        RandReal( -1, 1 ),
        RandReal( -1, 1 ),
        0
      };
      randomVectors[ i ] = Normalize( v );
    }
    TacImage randomVectorImage = {};
    randomVectorImage.mData = ( u8* )randomVectors.data();
    randomVectorImage.mWidth = numRows;
    randomVectorImage.mHeight = numCols;
    randomVectorImage.mPitch = sizeof( v4 ) * numCols;
    randomVectorImage.mTextureFormat = TacTextureFormat::RGBA32Float;
    randomVectorImage.textureDimension = 2;
    randomVectorTexture = gameInterface.renderer->AddTextureResource(
      randomVectorImage,
      TacTextureUsage::Immutable,
      TacBinding::ShaderResource,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      randomVectorTexture,
      STRINGIFY( randomVectorTexture ) );

    gameInterface.renderer->AddTexture(
      DemoFinalShader::GetNameRandomVectors(),
      gbufferFinalShader,
      TacShaderType::Fragment,
      DemoFinalShader::GetRegisterRandomVectors(),
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;

    Constants constants( CBufferSSAO::GetName() );
    constants.Add(
      CBufferSSAO::GetNameSSAOKernel(),
      sizeof( v4 ) * CBufferSSAO::GetArrayCountSSAOKernel );
    constants.Add(
      CBufferSSAO::GetNameRandomVectorsSize(),
      sizeof( v2 ) );
    constants.Add(
      CBufferSSAO::GetNameSSAORadius(),
      sizeof( r32 ) );
    cbufferHandleSSAO = constants.Create(
      gameInterface.renderer,
      gameInterface.gameUnrecoverableErrors );
    if( gameInterface.gameUnrecoverableErrors.size )
      return;
    gameInterface.renderer->SetName(
      cbufferHandleSSAO,
      STRINGIFY( cbufferHandleSSAO ) );

    gameInterface.renderer->AddCbuffer(
      gbufferFinalShader,
      cbufferHandleSSAO,
      DemoFinalShader::GetRegisterCBufferSSAO(),
      TacShaderType::Fragment,
      gameInterface.gameUnrecoverableErrors );

  }


  toAdd.mScale = V3( 1.0f, 1.0f, 1.0f );
  toAdd.mColor = V3( 1.0f, 1.0f, 1.0f );
  emitter.Init();
}

void DisplayEntity( TacEntity& entity )
{
  int currentEntityType = ( int )entity.mType;
  const char* entityTypeStrings[ ( u32 )TacEntityType::Count ] =
  {
    "Null",
    "Player",
    "Ground",
    "Unknown",
    "Tree",
  };
  if( ImGui::Combo( "Entity type", &currentEntityType, entityTypeStrings, ( u32 )TacEntityType::Count ) )
  {
    entity.mType = ( TacEntityType )currentEntityType;
  }

  int currentAssetID = ( int )entity.mAssetID;
  if( ImGui::Combo( "Asset id", &currentAssetID, assetIDstrings, ( u32 )TacGameAssetID::Count ) )
  {
    entity.mAssetID = ( TacGameAssetID )currentAssetID;
  }

  {
    if( ImGui::DragFloat3( "scale", &entity.mScale.x, 0.1f ) )
    {
      entity.transformDirty = true;
    }

    v3 rotDeg = entity.mRot * RAD2DEG;
    if( ImGui::DragFloat3( "rotate", &rotDeg.x, 0.1f ) )
    {
      entity.mRot = rotDeg * DEG2RAD;
      entity.transformDirty = true;
    }

    if( ImGui::DragFloat3( "translate", &entity.mPos.x, 0.1f ) )
    {
      entity.transformDirty = true;
    }

    ImGui::ColorEdit3( "color", &entity.mColor.x );
  }
}

TacRaycastResult RaycastEntityTris(
  TacEntity& entity,
  TacRay ray,
  TacGameAssets& gameAssets )
{
  TacRaycastResult raycastResultClosestTri = {};
  raycastResultClosestTri.dist = R32MAX;
  TacModelRaycastInfo* modelRaycastInfo =
    gameAssets.GetModelRaycastInfo( entity.mAssetID );
  if( !modelRaycastInfo )
    return raycastResultClosestTri;
  ray.pos = ( entity.worldInverse * V4( ray.pos, 1.0f ) ).xyz;
  ray.dir = ( entity.worldInverse * V4( ray.dir, 0.0f ) ).xyz;
  u32 numtris = modelRaycastInfo->indexes.size() / 3;
  u32 iicorner0 = 0;
  u32 iicorner1 = 1;
  u32 iicorner2 = 2;
  for(
    u32 iTri = 0;
    iTri < numtris;
    ++iTri, iicorner0 += 3, iicorner1 += 3, iicorner2 += 3 )
  {
    u32 index0 = modelRaycastInfo->indexes[ iicorner0 ];
    u32 index1 = modelRaycastInfo->indexes[ iicorner1 ];
    u32 index2 = modelRaycastInfo->indexes[ iicorner2 ];
    v3& p0 = modelRaycastInfo->vertexes[ index0 ];
    v3& p1 = modelRaycastInfo->vertexes[ index1 ];
    v3& p2 = modelRaycastInfo->vertexes[ index2 ];
    TacRaycastResult raycastResult =
      RayTriangleIntersection( ray, p0, p1, p2 );
    if( raycastResult.collided &&
      raycastResult.dist < raycastResultClosestTri.dist )
    {
      raycastResultClosestTri = raycastResult;
      TacAssert( raycastResultClosestTri.collided );
    }
  }
  if( raycastResultClosestTri.collided )
  {
    // transform the distance from modelspace to worldspace
    v3 worldspaceP0 = ( entity.world * V4( ray.pos, 1.0f ) ).xyz;
    v3 modelspaceP1 = ray.pos + ray.dir * raycastResultClosestTri.dist;
    v3 worldspaceP1 = ( entity.world * V4( modelspaceP1, 1.0f ) ).xyz;
    raycastResultClosestTri.dist = Distance( worldspaceP0, worldspaceP1 );
  }
  return raycastResultClosestTri;
}

void TacGameState::Update( TacGameInterface& gameInterface )
{
  const r32 aspect =
    ( r32 )gameInterface.windowWidth /
    ( r32 )gameInterface.windowHeight;
  const r32 camFOVYRad = mCamera.camFOVYDeg * DEG2RAD;
  TacDepthBufferHandle backbufferDepth =
    gameInterface.renderer->GetBackbufferDepth();
  TacTextureHandle backbufferColor =
    gameInterface.renderer->GetBackbufferColor();



  r32 perspectiveProjectionA;
  r32 perspectiveProjectionB;
  gameInterface.renderer->GetPerspectiveProjectionAB(
    mCamera.camFar,
    mCamera.camNear,
    perspectiveProjectionA,
    perspectiveProjectionB );
  m4 proj;
  ComputePerspectiveProjMatrix(
    perspectiveProjectionA,
    perspectiveProjectionB,
    proj,
    camFOVYRad,
    aspect );

  emitter.Update( gameInterface.gameInput->dt );
  TacModel* spheremodel = gameTransientState->gameAssets.GetModel(
    TacGameAssetID::Sphere );
  TacModel* cubemodel = gameTransientState->gameAssets.GetModel(
    TacGameAssetID::Cube );

  if( gameInterface.gameInput->KeyboardDown( KeyboardKey::Escape ) )
  {
    gameInterface.running = false;
    return;
  }

  if( ImGui::CollapsingHeader( "Scene" ) )
  {
    ImGui::InputText(
      "Scene filepath",
      scenefile.buffer,
      ArraySize( scenefile.buffer ) );

    if( ImGui::Button( "Save Scene" ) )
    {
      FixedString< DEFAULT_ERR_LEN > sceneErrors = {};
      SaveScene( gameInterface.thread, sceneErrors );
      if( sceneErrors.size )
      {
        gameTransientState->messages.AddMessage( sceneErrors.buffer );
      }
    }

    if( ImGui::Button( "Load Scene" ) )
    {
      FixedString< DEFAULT_ERR_LEN > sceneErrors = {};
      LoadScene( gameInterface.thread, sceneErrors );
      if( sceneErrors.size )
      {
        gameTransientState->messages.AddMessage( sceneErrors.buffer );
      }
    }
  }
  if( ImGui::CollapsingHeader( "Camera" ) )
  {
    ImGui::Indent();
    ImGui::DragFloat3( "campos", &mCamera.camPos.x, 0.1f );
    if( ImGui::DragFloat3( "camdir", &mCamera.camDir.x, 0.01f ) )
    {
      mCamera.camDir = Normalize( mCamera.camDir );
    }

    ImGui::DragFloat(
      "camNear", &mCamera.camNear, 0.1f, 0.01f, mCamera.camFar );
    ImGui::DragFloat(
      "camFar", &mCamera.camFar, 1.0f, mCamera.camNear, 999999.0f );
    ImGui::DragFloat( "camFOVYDeg", &mCamera.camFOVYDeg, 0.1f );

    ImGui::InputText(
      "Camera filepath",
      camerafile.buffer,
      ArraySize( camerafile.buffer ) );
    if( ImGui::Button( "Load Camera" ) )
    {

      FixedString< DEFAULT_ERR_LEN > loadErrors = {};
      LoadCamera(
        gameInterface.thread,
        loadErrors );
      if( loadErrors.size )
      {
        gameTransientState->messages.AddMessage( loadErrors.buffer );
      }
    }
    if( ImGui::Button( "Save Camera" ) )
    {
      FixedString< DEFAULT_ERR_LEN > loadErrors = {};
      SaveCamera(
        gameInterface.thread,
        loadErrors );
      if( loadErrors.size )
      {
        gameTransientState->messages.AddMessage( loadErrors.buffer );
      }
    }
    ImGui::Unindent();
  }
  if( ImGui::CollapsingHeader( "Add Entity" ) )
  {
    ImGui::Indent();
    ImGui::PushID( entities.size() );
    DisplayEntity( toAdd );
    if( ImGui::Button( "Create" ) )
    {
      AddEntity(
        toAdd.mType,
        toAdd.mAssetID,
        toAdd.mScale,
        toAdd.mRot,
        toAdd.mPos,
        toAdd.mColor );
    }
    ImGui::PopID();
    ImGui::Unindent();
  }
  ImGui::InputText(
    "Entities filepath",
    entitiesfile.buffer,
    ArraySize( entitiesfile.buffer ) );
  if( ImGui::Button( "Load Entities" ) )
  {
    FixedString< DEFAULT_ERR_LEN > loadErrors = {};
    LoadEntities(
      gameInterface.thread,
      loadErrors );
    if( loadErrors.size )
    {
      gameTransientState->messages.AddMessage( loadErrors.buffer );
    }
  }
  if( ImGui::Button( "Save Entities" ) )
  {
    FixedString< DEFAULT_ERR_LEN > loadErrors = {};
    SaveEntities(
      gameInterface.thread,
      loadErrors );
    if( loadErrors.size )
    {
      gameTransientState->messages.AddMessage( loadErrors.buffer );
    }
  }
  if( ImGui::CollapsingHeader( "Entities" ) )
  {
    ImGui::Indent();
    bool drawSeparater = false;
    for( u32 iEntity = 0; iEntity < entities.size(); ++iEntity )
    {
      if( drawSeparater )
      {
        ImGui::Separator();
      }
      ImGui::PushID( iEntity );
      DisplayEntity( entities[ iEntity ] );
      ImGui::PopID();
      drawSeparater = true;
    }
    ImGui::Unindent();
  }

  static r32 reloadt = 0;
  reloadt += gameInterface.gameInput->dt;
  if( reloadt > 1 )
  {
    reloadt = 0;

    auto DoReloadStuff = [ & ] ( const char* shaderFilepath, time_t& lastModifiedTime, TacShaderHandle shader )
    {
      FixedString< DEFAULT_ERR_LEN > errors = {};
      time_t time = PlatformGetFileModifiedTime( gameInterface.thread, shaderFilepath, errors );
      if( errors.size )
      {
        gameTransientState->messages.AddMessage( errors.buffer );
      }
      else if( lastModifiedTime != time )
      {
        lastModifiedTime = time;
        gameInterface.renderer->ReloadShader( shader );
        gameTransientState->messages.AddMessage( VA( "Shader %s reloaded", shaderFilepath ) );
      }
    };
    DoReloadStuff( demofxfilepath, shaderModifiedTime, shaderHandle );
    DoReloadStuff( demofinalfilepath, gbufferFinalModifiedTime, gbufferFinalShader );
  }

  ImGui::ShowTestWindow();
  ImGui::Text( "MouseX: %f", ( r32 )gameInterface.gameInput->mouseX );
  ImGui::Text( "MouseY: %f", ( r32 )gameInterface.gameInput->mouseY );

  // Update entity transforms
  for( u32 iEntity = 0; iEntity < entities.size(); ++iEntity )
  {
    TacEntity& entity = entities[ iEntity ];
    if( entity.transformDirty )
    {
      entity.transformDirty = false;
      entity.world = M4Transform( entity.mScale, entity.mRot, entity.mPos );
      entity.worldInverse = M4TransformInverse( entity.mScale, entity.mRot, entity.mPos );
    }
  }

  if( playerEntityIndex && groundEntityIndex )
  {
    TacEntity& player = entities[ playerEntityIndex ];
    TacEntity& ground = entities[ groundEntityIndex ];
    player.mPos.y =
      ground.mPos.y +
      ground.mScale.y +
      player.mScale.y;
  }

  TacRay ray;
  {
    v2 ndcPixel;
    ndcPixel.x =
      ( r32 )gameInterface.gameInput->mouseX /
      ( r32 )gameInterface.windowWidth;
    ndcPixel.y =
      ( r32 )gameInterface.gameInput->mouseY /
      ( r32 )gameInterface.windowHeight;
    ndcPixel.y = 1.0f - ndcPixel.y;
    ndcPixel *= 2.0f;
    ndcPixel -= V2( 1.0f, 1.0f );

    float wsHalfProjH = mCamera.camNear * Tan( camFOVYRad / 2.0f );
    float wsHalfProjW = wsHalfProjH * aspect;

    v3 camright;
    v3 camup;
    GetCamDirections( mCamera.camDir, mCamera.camWorldUp, camright, camup );

    v3 wsCenterPixel = mCamera.camPos + mCamera.camDir * mCamera.camNear;
    v3 wsCurrentPixel =
      wsCenterPixel +
      camright * wsHalfProjW * ndcPixel.x +
      camup * wsHalfProjH * ndcPixel.y;

    ray.pos = mCamera.camPos;
    ray.dir = Normalize( wsCurrentPixel - mCamera.camPos );
  }

  FrustumCorners viewSpaceFrustumCorners = GetFrustumCorners(
    mCamera.camFar,
    mCamera.camFOVYDeg,
    aspect );

  TacRaycastResult groundClosestTri = RaycastEntityTris(
    entities[ groundEntityIndex ],
    ray,
    gameTransientState->gameAssets );

  // render
  TacRaycastResult raycastResultClosestTri;
  raycastResultClosestTri.collided = false;
  raycastResultClosestTri.dist = R32MAX;
  u32 closestEntityIndex = 0;
  for( u32 iEntity = 0; iEntity < entities.size(); ++iEntity )
  {
    TacEntity& entity = entities[ iEntity ];
    if( entity.mType == TacEntityType::Null )
      continue;

    TacModelRaycastInfo* modelRaycastInfo =
      gameTransientState->gameAssets.GetModelRaycastInfo( entity.mAssetID );
    if( !modelRaycastInfo )
      continue;

    r32 scale = Maximum( Maximum(
      entity.mScale.x,
      entity.mScale.y ),
      entity.mScale.z );

    TacSphere worldspaceBoundingSphere;
    worldspaceBoundingSphere.position = ( entity.world *
      V4( modelRaycastInfo->boundingSphere.position, 1.0f ) ).xyz;
    worldspaceBoundingSphere.radius = modelRaycastInfo->boundingSphere.radius * scale;

    TacRaycastResult raycastBoundingSphereResult =
      RaySphereIntersection( ray, worldspaceBoundingSphere );
    if( raycastBoundingSphereResult.collided ||
      raycastBoundingSphereResult.rayStartedInsideObject )
    {
      TacRaycastResult triResult = RaycastEntityTris(
        entity,
        ray,
        gameTransientState->gameAssets );
      if( triResult.collided &&
        triResult.dist < raycastResultClosestTri.dist )
      {
        raycastResultClosestTri = triResult;
        closestEntityIndex = iEntity;
      }
    }
  }

  if( raycastResultClosestTri.collided )
  {
    TacEntity& closestEntity = entities[ closestEntityIndex ];

    if( gameInterface.gameInput->MouseDown( MouseButton::eRightMouseButton ) )
    {
      if( closestEntity.mType == TacEntityType::Ground )
      {
        if( selectedEntityIndex )
        {
          TacEntity& selectedEntity = entities[ selectedEntityIndex ];
          v3 raycastPosition =
            ray.pos +
            ray.dir * raycastResultClosestTri.dist;
          selectedEntity.mPos.x = Lerp(
            selectedEntity.mPos.x, raycastPosition.x, 0.1f );
          selectedEntity.mPos.z = Lerp(
            selectedEntity.mPos.z, raycastPosition.z, 0.1f );
          selectedEntity.transformDirty = true;
        }
      }
      else if( gameInterface.gameInput->MouseJustDown( MouseButton::eRightMouseButton ) )
      {
        selectedEntityIndex = closestEntityIndex;
      }
    }
  }

  TacTemporaryMemory tempRenderMemory =
    BeginTemporaryMemory( &gameTransientState->mTempAllocator );
  TacRenderGroup renderGroup = {};
  renderGroup.mPushBufferMemory =
    ( u8* )PushSize( &gameTransientState->mTempAllocator,
      renderGroup.mPushBufferMemorySizeMax = Megabytes( 1 ) );
  if( !renderGroup.mPushBufferMemory )
  {
    gameInterface.gameUnrecoverableErrors =
      "Could not allocate temporary push buffer memory";
    return;
  }

  // rendering ------------------------------------------------------------
  ImGui::ColorEdit4( "clear color", &clearcolor.x, false );

  auto DrawRectangleWireframe = [ & ](
    v3 recthalfdimensions,
    v3 rot,
    v3 translate,
    r32 thickness = 0.2f )
  {
    v3 zero = { 0, 0, 0 };
    v3 one = { 1, 1, 1 };
    for( u32 iAxis = 0; iAxis < 3; ++iAxis )
    {
      u32 iX = ( iAxis + 0 ) % 3;
      u32 iY = ( iAxis + 1 ) % 3;
      u32 iZ = ( iAxis + 2 ) % 3;

      v3 scale;
      scale[ iX ] = recthalfdimensions[ iX ];
      scale[ iY ] = thickness;
      scale[ iZ ] = thickness;

      v3 offset;
      offset[ iX ] = 0;
      offset[ iY ] = recthalfdimensions[ iY ];
      offset[ iZ ] = recthalfdimensions[ iZ ];

      for( int i = 0; i < 2; ++i )
      {
        offset[ iY ] *= -1;
        for( int j = 0; j < 2; ++j )
        {
          offset[ iZ ] *= -1;
          m4 world =
            M4Transform( one, rot, translate ) *
            M4Transform( scale, zero, offset );
          renderGroup.PushUniform( "World", &world, sizeof( m4 ) );
          renderGroup.PushModel( cubemodel );
        }
      }
    }
  };


  renderGroup.PushClearColor(
    viewSpacePositionTexture, V4< r32 >( 0, 0, 0, 0 ) );
  renderGroup.PushClearColor(
    gbufferViewSpaceNormal, V4< r32 >( 0, 0, 0, 0 ) );
  renderGroup.PushClearColor(
    gbufferDiffuse, clearcolor );
  renderGroup.PushClearDepthStencil(
    gbufferDepthStencil, true, 1.0f, false, 0 );
  renderGroup.PushClearDepthStencil(
    backbufferDepth, true, 1.0f, false, 0 );

  {
    const u32 numRenderTargets = 3;
    TacTextureHandle renderTargets[ numRenderTargets ] =
    {
      gbufferDiffuse,
      gbufferViewSpaceNormal,
      viewSpacePositionTexture
    };
    renderGroup.PushRenderTargets( renderTargets, numRenderTargets, gbufferDepthStencil );
  }

  renderGroup.PushBlendState( blendStateHandle );
  renderGroup.PushDepthState( depthStateHandle );
  renderGroup.PushViewport(
    0,
    0,
    ( r32 )gameInterface.windowWidth,
    ( r32 )gameInterface.windowHeight );
  renderGroup.PushShader( shaderHandle );
  renderGroup.PushPrimitive( TacPrimitive::TriangleList );
  renderGroup.PushVertexFormat( vertexFormatHandle );
  renderGroup.PushRasterizerState( rasterizerStateHandle );
  m4 view;
  ComputeViewMatrix(
    view, mCamera.camPos, mCamera.camDir, mCamera.camWorldUp );


  renderGroup.PushUniform(
    CBufferPerFrame::GetNameView(),
    ( void* )&view,
    sizeof( m4 ) );
  renderGroup.PushUniform(
    CBufferPerFrame::GetNameProj(),
    ( void* )&proj,
    sizeof( m4 ) );
  renderGroup.PushUniform(
    CBufferPerFrame::GetNameViewSpaceFrustumCorners(),
    ( void* )&viewSpaceFrustumCorners,
    sizeof( v4 ) * 4 );
  renderGroup.PushUniform(
    CBufferPerFrame::GetNameFar(),
    ( void* )&mCamera.camFar,
    sizeof( r32 ) );
  renderGroup.PushUniform(
    CBufferPerFrame::GetNameNear(),
    ( void* )&mCamera.camNear,
    sizeof( r32 ) );
  v2 gbufferSize = {
    ( r32 )gameInterface.windowWidth,
    ( r32 )gameInterface.windowHeight };
  renderGroup.PushUniform(
    CBufferPerFrame::GetNameGBufferSize(),
    ( void* )&gbufferSize,
    sizeof( v2 ) );
  renderGroup.PushUniform(
    CBufferPerFrame::GetNameClearColor(),
    ( void* )&clearcolor,
    sizeof( v4 ) );

  // add draw calls
  {
    // add entity draw calls
    for( TacEntity& entity : entities )
    {
      if( entity.mType == TacEntityType::Null )
        continue;

      TacModel* model =
        gameTransientState->gameAssets.GetModel( entity.mAssetID );
      if( !model )
        continue;

      renderGroup.PushUniform( "World", &entity.world, sizeof( m4 ) );
      renderGroup.PushUniform( "Color", &entity.mColor, sizeof( v3 ) );
      renderGroup.PushModel( model );
    }

    // draw the raycast point
    if( raycastResultClosestTri.collided )
    {
      v3 raycastPosition =
        ray.pos +
        ray.dir * raycastResultClosestTri.dist;

      m4 world = M4Transform(
        V3( 1.0f, 1.0f, 1.0f ) * 0.5f,
        V3( 0.0f, 0.0f, 0.0f ),
        raycastPosition );

      TacEntity& closestEntity = entities[ closestEntityIndex ];
      v3 color = closestEntity.mColor / 4.0f;
      renderGroup.PushUniform( "World", &world, sizeof( m4 ) );
      renderGroup.PushUniform( "Color", &color, sizeof( v3 ) );
      renderGroup.PushModel( spheremodel );
    }

    // draw particles
    v3 zero = {};

    for( u32 iParticle = 0; iParticle < emitter.numAlive; ++iParticle )
    {
      TacParticle& particle = emitter.particles[ iParticle ];

      m4 world = M4Transform(
        V3( 1.0f, 1.0f, 1.0f ) * particle.cursize,
        zero,
        particle.pos );
      renderGroup.PushUniform( "World", &world, sizeof( m4 ) );
      renderGroup.PushUniform( "Color", &particle.color, sizeof( v3 ) );
      renderGroup.PushModel( spheremodel );
    }

    r32 imguidragspeed = 0.01f;
    ImGui::Begin( "Emitter" );
    ImGui::DragFloat( "gravity", &emitter.accel, imguidragspeed );
    ImGui::DragFloat( "min life", &emitter.minlife, imguidragspeed );
    ImGui::DragFloat( "max life", &emitter.maxlife, imguidragspeed );
    ImGui::DragFloat( "min scale", &emitter.minsize, imguidragspeed, 0, emitter.maxsize );
    ImGui::DragFloat(
      "max scale",
      &emitter.maxsize,
      imguidragspeed,
      emitter.minsize,
      100 );
    ImGui::ColorEdit3( "particle color 1", &emitter.colormin.x );
    ImGui::ColorEdit3( "particle color 2", &emitter.colormax.x );
    if( ImGui::CollapsingHeader( "shape" ) )
    {
      ImGui::Indent();
      v3 shapecolor = V3< r32 >( 1, 1, 1 ) * 0.5f;
      renderGroup.PushUniform( "Color", &shapecolor, sizeof( v3 ) );
      int currentshapeType = ( int )emitter.shapetype;
      const char* shapeTypeStrings[ ( u32 )TacEmitter::Shape::Count ] =
      {
        "Sphere",
        "Rectangle",
      };
      if( ImGui::Combo( "spawn shape", &currentshapeType, shapeTypeStrings, ( u32 )TacEmitter::Shape::Count ) )
      {
        emitter.shapetype = ( TacEmitter::Shape )currentshapeType;
      }
      switch( emitter.shapetype )
      {
        case TacEmitter::Shape::Sphere:
        {
          ImGui::DragFloat(
            "spawn radius",
            &emitter.shapedata.sphereradius,
            imguidragspeed );
        } break;
        case TacEmitter::Shape::Rectangle:
        {
          ImGui::DragFloat3(
            "rectangle halfdim",
            &emitter.shapedata.recthalfdimensions.x,
            imguidragspeed );
        } break;
        TacInvalidDefaultCase;
      }

      r32 emitterThickness = 0.1f;
      switch( emitter.shapetype )
      {
        case TacEmitter::Shape::Sphere:
        {
          for( u32 iAxis = 0; iAxis < 3; ++iAxis )
          {
            v3 scale = V3( 1.0f, 1.0f, 1.0f ) * emitterThickness;
            scale[ iAxis ] = emitter.shapedata.sphereradius;
            m4 emitterworld = M4Transform(
              scale,
              zero,
              emitter.pos );
            renderGroup.PushUniform( "World", &emitterworld, sizeof( m4 ) );
            renderGroup.PushModel( cubemodel );
          }
        } break;
        case TacEmitter::Shape::Rectangle:
        {
          DrawRectangleWireframe(
            emitter.shapedata.recthalfdimensions,
            V3< r32 >( 0, 0, 0 ),
            emitter.pos,
            emitterThickness );
        } break;
        TacInvalidDefaultCase;
      }
      ImGui::Unindent();
    }
    ImGui::DragFloat( "spawn rate", &emitter.spawnrate, imguidragspeed );
    ImGui::DragFloat3( "spawn pos", &emitter.pos.x, imguidragspeed );
    ImGui::InputText( "Emitter file", emitterfile.buffer, ArraySize( emitterfile.buffer ) );

    auto LoadEmitter = [ & ] (){
      FixedString< DEFAULT_ERR_LEN > loadErrors = {};
      TacFile file = PlatformOpenFile(
        gameInterface.thread,
        emitterfile.buffer,
        OpenFileDisposition::OpenExisting,
        FileAccess::Read,
        loadErrors );
      OnDestruct(
        PlatformCloseFile( gameInterface.thread, file, loadErrors );
      if( loadErrors.size )
      {
        gameTransientState->messages.AddMessage( loadErrors.buffer );
      }
      );
      if( loadErrors.size )
        return;
      PlatformReadEntireFile(
        gameInterface.thread,
        file,
        &emitter,
        sizeof( TacEmitter ),
        loadErrors );
      if( loadErrors.size )
        return;
    };
    static bool loademitteronce = true;
    if( loademitteronce )
    {
      emitterfile = "tacData/emitter.tacBinary";
      LoadEmitter();
      loademitteronce = false;
    }
    if( ImGui::Button( "Load Emitter" ) )
    {
      LoadEmitter();
    }

    if( ImGui::Button( "Save Emitter" ) )
    {
      [ & ](){
        FixedString< DEFAULT_ERR_LEN > loadErrors = {};
        TacFile file = PlatformOpenFile(
          gameInterface.thread,
          emitterfile.buffer,
          OpenFileDisposition::CreateAlways,
          FileAccess::Write,
          loadErrors );
        OnDestruct(
          PlatformCloseFile( gameInterface.thread, file, loadErrors );
        if( loadErrors.size )
        {
          gameTransientState->messages.AddMessage( loadErrors.buffer );
        }
        );
        if( loadErrors.size )
          return;
        PlatformWriteEntireFile(
          gameInterface.thread,
          file,
          &emitter,
          sizeof( TacEmitter ),
          loadErrors );
        if( loadErrors.size )
          return;
      }( );
    }
    ImGui::End();

    ImGui::SetNextWindowSize( ImVec2( 300, 350 ), ImGuiSetCond_FirstUseEver );
    if( ImGui::Begin( "Interpolation" ) )
    {
      static r32 radius = 10;
      ImGui::DragFloat( "radius", &radius, 0.01f );
      ImGui::DragInt(
        "numPositions",
        ( int* )&emitter.sizeByLife.numPositions,
        0.01f,
        0,
        emitter.sizeByLife.maxpositions );
      for( u32 iPosition = 0; iPosition < emitter.sizeByLife.numPositions; ++iPosition )
      {
        ImGui::PushID( iPosition );
        ImGui::DragFloat2( "position", &emitter.sizeByLife.positions[ iPosition ].x, 0.01f );
        ImGui::PopID();
      }
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      // ImDrawList API uses screen coordinates!
      ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
      // Resize canvas to what's available
      ImVec2 canvas_size = ImGui::GetContentRegionAvail();
      if( canvas_size.x < 50.0f ) canvas_size.x = 50.0f;
      if( canvas_size.y < 50.0f ) canvas_size.y = 50.0f;
      draw_list->AddRectFilledMultiColor(
        canvas_pos,
        ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y ),
        ImColor( 0, 0, 0 ),
        ImColor( 255, 0, 0 ),
        ImColor( 255, 255, 0 ),
        ImColor( 0, 255, 0 ) );
      draw_list->AddRect(
        canvas_pos,
        ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y ),
        ImColor( 255, 255, 255 ) );
      ImGui::InvisibleButton( "canvas", canvas_size );
      // clip lines within the canvas (if we resize it, etc.)
      draw_list->PushClipRect( ImVec4(
        canvas_pos.x,
        canvas_pos.y,
        canvas_pos.x + canvas_size.x,
        canvas_pos.y + canvas_size.y ) );

      auto ToCanvasPosition = [ & ] ( v2 xy )
      {
        ImVec2 result;
        result.x = canvas_pos.x + canvas_size.x * xy.x;
        result.y = canvas_pos.y + canvas_size.y * ( 1.0f - xy.y );
        return result;
      };

      ImU32 curveColor = 0xFFFF0000;
      ImU32 shellColor = 0xFFFF00FF;

      // draw control points
      ImVec2 xyold = ToCanvasPosition( emitter.sizeByLife.positions[ 0 ] );
      for( u32 iPosition = 0; iPosition < emitter.sizeByLife.numPositions; ++iPosition )
      {
        ImVec2 xy = ToCanvasPosition( emitter.sizeByLife.positions[ iPosition ] );
        draw_list->AddCircleFilled(
          xy,
          radius,
          curveColor );

        // draw shells
        if( iPosition > 0 && emitter.sizeByLife.numPositions > 2 )
        {
          draw_list->AddLine( xyold, xy, shellColor );
          xyold = xy;
        }
      }

      // draw bezier curve
      u32 iters = 30;
      xyold = ToCanvasPosition( emitter.sizeByLife.GetPoint( 0 ) );
      for( u32 iter = 1; iter < iters; ++iter )
      {
        r32 t = ( r32 )( iter + 1 ) / ( r32 )iters;
        ImVec2 xy = ToCanvasPosition( emitter.sizeByLife.GetPoint( t ) );
        draw_list->AddLine( xyold, xy, curveColor );
        xyold = xy;
      }
      draw_list->PopClipRect();
    }
    ImGui::End();
  }


  // physics test
  {
    ImGui::Begin( "Physics Test" );
    const float imguispeed = 0.05f;

    ImGui::DragFloat3( "Gravity", &mPhysicsTest.mGravity.x, imguispeed );
    ImGui::LabelText( "Max Particles", "%i", mPhysicsTest.sMaxParticles );
    ImGui::LabelText( "Num Particles", "%i", mPhysicsTest.mNumParticles );

    static v3 physicsBoxColor = V3< r32 >( 1, 1, 1 ) * 0.2f;
    ImGui::ColorEdit3( "Box Color", &physicsBoxColor.x );
    static v3 physicsParticleColor = V3< r32 >( 0.6f, 0, 0 );
    ImGui::ColorEdit3( "Particle Color", &physicsParticleColor.x );

    static float particleSize = 0.1f;
    ImGui::DragFloat( "Particle Size", &particleSize, imguispeed );
    v3 scale = V3< r32 >( 1, 1, 1 ) * particleSize;
    v3 zero = V3< r32 >( 0, 0, 0 );

    renderGroup.PushUniform( "Color", &physicsBoxColor, sizeof( v3 ) );
    static float boxThickness = 0.1f;
    ImGui::DragFloat( "Box thickness", &boxThickness, imguispeed );
    static float vertexRadius = 0.3f;
    ImGui::DragFloat( "Vertex Radius", &vertexRadius, imguispeed );
    v3 boxVertexScale = V3< r32 >( 1, 1, 1 ) * vertexRadius;


    static TacPhysicsParticle defaultparticle = [] (){
      TacPhysicsParticle result = {};
      result.mMass = 1;
      result.mPosition = V3( 0.0f, 0.0f, 0.0f );
      return result;
    }( );
    static TacPhysicsBox defaultBox = [] (){
      TacPhysicsBox result = {};
      result.mBoxPos = V3< r32 >( -.7f, -.7f, -.7f );
      result.mBoxScale = V3< r32 >( 1, 1, 1 );
      return result;
    }( );
    static bool once = true;
    if( once )
    {
      once = false;
      // box
      mPhysicsTest.AddBox( defaultBox );
      // particles
      mPhysicsTest.AddPartile( defaultparticle );
      // gravity
      mPhysicsTest.mGravity.y;//= -5.0f;
    }

    // draw each box
    for( u32 iBox = 0; iBox < mPhysicsTest.mNumBoxes; ++iBox )
    {
      TacPhysicsBox& box = mPhysicsTest.mBoxes[ iBox ];
      DrawRectangleWireframe(
        box.mBoxScale,
        box.mBoxRot,
        box.mBoxPos,
        boxThickness );
      // draw each vertex as a sphere
      for( u32 i = 0; i < 8; ++i )
      {
        v3& currentVertex = box.mWorldSpaceBoxVertexes[ i ];
        m4 world = M4Transform(
          boxVertexScale,
          zero,
          currentVertex );
        renderGroup.PushUniform( "World", &world, sizeof( m4 ) );
        renderGroup.PushModel( spheremodel );
      }
    }

    auto EditBox = [ & ](
      TacPhysicsBox& box,
      u32 iBox )
    {
      ImGui::PushID( iBox );
      bool recalculateBox = false;
      recalculateBox |= ImGui::DragFloat3(
        "Box pos", &box.mBoxPos.x, imguispeed );
      recalculateBox |= ImGui::DragFloat3(
        "Box rot", &box.mBoxRot.x, imguispeed );
      recalculateBox |= ImGui::DragFloat3(
        "Box scale", &box.mBoxScale.x, imguispeed );
      if( recalculateBox )
      {
        box.RecalculateVertexes();
      }
      ImGui::PopID();
    };

    // add box button
    if( mPhysicsTest.mNumBoxes < mPhysicsTest.sMaxBoxes &&
      ImGui::CollapsingHeader( "Spawn Box" ) )
    {
      EditBox( defaultBox, mPhysicsTest.mNumBoxes );
      if( ImGui::Button( "Create box with parameters" ) )
      {
        mPhysicsTest.AddBox( defaultBox );
      }
    }


    // Box widget
    if( mPhysicsTest.mNumBoxes && ImGui::CollapsingHeader( "Boxes" ) )
    {
      for( u32 iBox = 0; iBox < mPhysicsTest.mNumBoxes; ++iBox )
      {
        TacPhysicsBox& box = mPhysicsTest.mBoxes[ iBox ];
        EditBox( box, iBox );
        ImGui::Separator();
      }
    }

    auto EditParticle = [ & ](
      TacPhysicsParticle& particle,
      u32 iphysicsparticle )
    {
      ImGui::PushID( iphysicsparticle );
      ImGui::DragFloat3( "Position", &particle.mPosition.x, imguispeed );
      ImGui::DragFloat3( "Velocity", &particle.mVelocity.x, imguispeed );
      ImGui::DragFloat3( "Force", &particle.mForceAccumulator.x, imguispeed );
      ImGui::DragFloat( "Mass", &particle.mMass, imguispeed, 0.01f, 1000.0f );
      ImGui::PopID();
    };

    if( mPhysicsTest.mNumParticles < mPhysicsTest.sMaxParticles &&
      ImGui::CollapsingHeader( "Spawn Particle" ) )
    {
      EditParticle( defaultparticle, mPhysicsTest.mNumParticles );
      if( ImGui::Button( "Create particle with parametesr" ) )
      {
        mPhysicsTest.AddPartile( defaultparticle );
      }
    }

    // Particle widget
    if( mPhysicsTest.mNumParticles &&
      ImGui::CollapsingHeader( "Particles" ) )
    {
      for(
        u32 iphysicsparticle = 0;
        iphysicsparticle < mPhysicsTest.mNumParticles;
        ++iphysicsparticle )
      {
        TacPhysicsParticle& particle =
          mPhysicsTest.mParticles[ iphysicsparticle ];
        EditParticle( particle, iphysicsparticle );
        ImGui::Separator();
      }
    }

    // physics step
    mPhysicsTest.Update( gameInterface.gameInput->dt );

    // display manifolds
    for( u32 iManifold = 0; iManifold < mPhysicsTest.mNumManifolds; ++iManifold )
    {
      TacPhysicsManifold& manifold = mPhysicsTest.mManifolds[ iManifold ];
      bool b = manifold.mIsColliding.mIsColliding != 0;
      ImGui::Checkbox( "Is Colliding", &b );

      auto DoShitttt = [&]( TacPhysicsBox& box, u32* tris )
      {
        v3 verts[ 3 ];
        for( u32 tri = 0; tri < 3; ++tri )
        {
          verts[ tri ] = box.mWorldSpaceBoxVertexes[ tris[ tri ] ];
        }

        v3 colors[ 3 ] =
        {
          { 1, 0, 0 },
          { 0, 1, 0 },
          { 0, 0, 1 },
        };
        for( u32 tri = 0; tri < 3; ++tri )
        {
          m4 world = M4Transform( boxVertexScale * 1.1f, zero, verts[ tri ] );
          v3 color = colors[ tri ];
          renderGroup.PushUniform( "Color", &color, sizeof( v3 ) );
          renderGroup.PushUniform( "World", &world, sizeof( m4 ) );
          renderGroup.PushModel( spheremodel );
        }
        v3 closestPoint =
          verts[ 0 ] * manifold.mIsColliding.mBarycentric[ 0 ] +
          verts[ 1 ] * manifold.mIsColliding.mBarycentric[ 1 ] +
          verts[ 2 ] * manifold.mIsColliding.mBarycentric[ 2 ];
        m4 world = M4Transform( boxVertexScale * 1.1f, zero, closestPoint );
        v3 color = V3< r32 >( 0.5f, 0.5f, 0.5f );
        renderGroup.PushUniform( "Color", &color, sizeof( v3 ) );
        renderGroup.PushUniform( "World", &world, sizeof( m4 ) );
        renderGroup.PushModel( spheremodel );
      };

      DoShitttt( mPhysicsTest.mBoxes[ manifold.mBox0Index ], manifold.mIsColliding.mTri0 );
      DoShitttt( mPhysicsTest.mBoxes[ manifold.mBox1Index ], manifold.mIsColliding.mTri1 );

      ImGui::LabelText( "Penetration Dist", "%f", manifold.mIsColliding.mPenetrationDist );
      ImGui::LabelText(
        "Penetration Normal",
        "( %.2f, %.2f, %.2f )",
        manifold.mIsColliding.mNoramlizedCollisionNormal.x,
        manifold.mIsColliding.mNoramlizedCollisionNormal.y,
        manifold.mIsColliding.mNoramlizedCollisionNormal.z );
    }

    // draw each particle
    renderGroup.PushUniform( "Color", &physicsParticleColor, sizeof( v3 ) );
    for(
      u32 iphysicsparticle = 0;
      iphysicsparticle < mPhysicsTest.mNumParticles;
      ++iphysicsparticle )
    {
      TacPhysicsParticle& particle =
        mPhysicsTest.mParticles[ iphysicsparticle ];
      m4 world = M4Transform( scale, zero, particle.mPosition );
      renderGroup.PushUniform( "World", &world, sizeof( m4 ) );
      renderGroup.PushModel( spheremodel );
    }
    ImGui::End();
  }

  gameInterface.renderer->DebugBegin( "SSAO" );
  OnDestruct( gameInterface.renderer->DebugEnd(); );

  // set rendertarget to backbuffer
  {
    const u32 numRenderTargets = 1;
    TacTextureHandle renderTargets[ numRenderTargets ] =
    {
      backbufferColor,
    };
    renderGroup.PushRenderTargets(
      renderTargets,
      numRenderTargets,
      backbufferDepth );
  }
  renderGroup.PushDepthState( noDepthTesting, 0 );
  renderGroup.PushShader( gbufferFinalShader );
  renderGroup.PushSamplerState(
    DemoFinalShader::GetNameLinearSampler(),
    linearSampler );
  renderGroup.PushSamplerState(
    DemoFinalShader::GetNamePointSampler(),
    pointSampler );
  renderGroup.PushVertexBuffers( &ndcQuadVBO, 1 );
  renderGroup.PushIndexBuffer( ndcQuadIBO );
  renderGroup.PushTexture(
    DemoFinalShader::GetNameGBufferColor(),
    gbufferDiffuse );
  renderGroup.PushTexture(
    DemoFinalShader::GetNameGBufferViewSpaceNormal(),
    gbufferViewSpaceNormal );
  renderGroup.PushTexture(
    DemoFinalShader::GetNameRandomVectors(),
    randomVectorTexture );
  renderGroup.PushTexture(
    DemoFinalShader::GetNameviewSpacePositionTexture(),
    viewSpacePositionTexture );

  static v4 ssaoKernel[ CBufferSSAO::GetArrayCountSSAOKernel ];
  static bool once = true;
  if( once )
  {
    once = false;
    for( u32 i = 0; i < CBufferSSAO::GetArrayCountSSAOKernel; ++i )
    {
      v4& offset = ssaoKernel[ i ];
      offset = V4(
        RandReal( -1.0f, 1.0f ),
        RandReal( -1.0f, 1.0f ),
        RandReal( 0.0f, 1.0f ),
        0.0f );
      offset = Normalize( offset );
      r32 scale =
        ( r32 )( i + 1 ) /
        ( r32 )CBufferSSAO::GetArrayCountSSAOKernel;
      offset *= scale * scale;
    }
  }
  renderGroup.PushUniform(
    CBufferSSAO::GetNameSSAOKernel(),
    ssaoKernel,
    sizeof( v4 ) * CBufferSSAO::GetArrayCountSSAOKernel );

  v2 randomVectorsSize =
  {
    ( r32 )numRows,
    ( r32 )numCols,
  };

  renderGroup.PushUniform(
    CBufferSSAO::GetNameRandomVectorsSize(),
    ( void* )&randomVectorsSize,
    sizeof( v2 ) );

  static r32 ssaoRadius = 1.0f;
  ImGui::DragFloat( "ssaoRadius", &ssaoRadius, 0.01f );
  renderGroup.PushUniform(
    CBufferSSAO::GetNameSSAORadius(),
    ( void* )&ssaoRadius,
    sizeof( r32 ) );
  renderGroup.PushApply();
  renderGroup.PushDraw();

  // unbind the framebuffers from input
  TacTextureHandle none = {};
  renderGroup.PushTexture(
    DemoFinalShader::GetNameGBufferColor(),
    none );
  renderGroup.PushTexture(
    DemoFinalShader::GetNameGBufferViewSpaceNormal(),
    none );
  renderGroup.PushTexture(
    DemoFinalShader::GetNameviewSpacePositionTexture(),
    none );
  renderGroup.PushApply();

  if( selectedEntityIndex )
  {
    {
      ImGui::Begin( "Selected" );
      if(
        ImGui::Button( "Delete Entity" ) ||
        gameInterface.gameInput->KeyboardJustDown( KeyboardKey::Backspace ) ||
        gameInterface.gameInput->KeyboardJustDown( KeyboardKey::Delete ) )
      {
        entities[ selectedEntityIndex ] = entities.back();
        entities.pop_back();
        selectedEntityIndex = 0;
      }
      DisplayEntity( entities[ selectedEntityIndex ] );
      ImGui::End();
    }

    if(
      ImGui::Button( "Copy Entity" ) ||
      gameInterface.gameInput->KeyboardDown( KeyboardKey::Control ) &&
      gameInterface.gameInput->KeyboardJustDown( KeyboardKey::C ) )
    {
      isEntityCopied = true;
      copiedEntity = entities[ selectedEntityIndex ];
      gameTransientState->messages.AddMessage(
        VA( "copied entity %i", selectedEntityIndex ) );
    }

    if(
      ImGui::Button( "Paste Entity" ) ||
      gameInterface.gameInput->KeyboardDown( KeyboardKey::Control ) &&
      gameInterface.gameInput->KeyboardJustDown( KeyboardKey::V ) )
    {
      if( isEntityCopied )
      {
        v3 color = copiedEntity.mColor;
        if( copiedEntity.mType == TacEntityType::Tree )
        {
          color = GetRandomTreeColor();
        }

        v3 spawnpoint = copiedEntity.mPos;
        if( groundClosestTri.collided )
        {
          spawnpoint = ray.pos + ray.dir * groundClosestTri.dist;
        }

        selectedEntityIndex = AddEntity(
          copiedEntity.mType,
          copiedEntity.mAssetID,
          copiedEntity.mScale,
          copiedEntity.mRot,
          spawnpoint,
          color );
        gameTransientState->messages.AddMessage(
          VA( "pasted entity %i", selectedEntityIndex ) );
      }
    }
  }

  gameTransientState->messages.Update( gameInterface.gameInput->dt );
  gameTransientState->messages.Draw();

  void* addr = ( void* )&entities[ 1 ].mPos;
  TacUnusedParameter( addr );

  static r32 minEntityDist = 0.3f;
  ImGui::DragFloat( "min entity dist", &minEntityDist, 0.01f );
  if( ImGui::Button( "randomize trees" ) )
  {
    // change color, scale, rot
    for( TacEntity& entity : entities )
    {
      if( entity.mType == TacEntityType::Tree )
      {
        entity.mColor = GetRandomTreeColor();
        entity.mScale = V3( 1.0f, 1.0f, 1.0f ) * RandReal( 1.0f, 2.0f );
        entity.mRot.y = RandReal( 0.0f, 360.0f );
        entity.transformDirty = true;
      }
    }

    // move trees away from each other
    for(
      u32 iEntity = 0;
      iEntity < entities.size();
      ++iEntity )
    {
      TacEntity& entity0 = entities[ iEntity ];
      if( entity0.mType != TacEntityType::Tree )
        continue;
      for(
        u32 jEntity = iEntity + 1;
        jEntity < entities.size();
        ++jEntity )
      {
        TacEntity& entity1 = entities[ jEntity ];
        if( entity1.mType != TacEntityType::Tree )
          continue;
        r32 dist = Distance( entity0.mPos, entity1.mPos );
        if( dist < minEntityDist )
        {
          v3 dPos = entity1.mPos - entity0.mPos;
          if( LengthSq( dPos ) < 0.01f )
          {
            entity0.mPos.x += minEntityDist;
          }
          else
          {
            r32 distanceNeeded =
              ( minEntityDist - Length( dPos ) ) / 2.0f;
            dPos = Normalize( dPos );
            entity0.mPos -= dPos * distanceNeeded;
            entity1.mPos += dPos * distanceNeeded;
          }
        }
      }
    }



    // snap trees to the ground
    //r32 y = ( ray.pos + ray.dir * groundClosestTri.dist ).y;
    TacEntity& ground = entities[ groundEntityIndex ];
    r32 y = ground.mPos.y + ground.mScale.y;
    for(
      u32 iEntity = 0;
      iEntity < entities.size();
      ++iEntity )
    {
      TacEntity& entity0 = entities[ iEntity ];
      if( entity0.mType == TacEntityType::Tree )
      {
        entity0.mPos.y = y - RandReal( 0, 5 );
      }
    }
  }

  if( playerEntityIndex )
  {
    TacEntity& player = entities[ playerEntityIndex ];
    if( ImGui::Button( "Set camera offset" ) )
    {
      playerToCameraOffset = mCamera.camPos - player.mPos;
    }
    if( ImGui::Checkbox(
      "Camera following player",
      ( bool* )&cameraFollowingPlayer ) )
    {
      Zero( cameraVel );
    }

    // follow camera
    if( cameraFollowingPlayer )
    {
      ImGui::DragFloat( "camera damping", &cameradamping, 0.01f );
      ImGui::DragFloat( "camera angular freq", &cameraangularFreq, 0.01f );
      v3 cameraTargetPosition = player.mPos + playerToCameraOffset;
      for( u32 iAxis = 0; iAxis < 3; ++iAxis )
      {
        Spring(
          mCamera.camPos[ iAxis ],
          cameraVel[ iAxis ],
          cameraTargetPosition[ iAxis ],
          cameradamping,
          cameraangularFreq,
          gameInterface.gameInput->dt );
      }
    }

    // player controls
    {
      static v3 baseAngle = { -90, 0, 0 };
      static r32 maxspeed = 15;
      static v2 vel = {};
      static r32 movementForce = 100;

      v2 accel = {};
      {
        if( gameInterface.gameInput->KeyboardDown( KeyboardKey::W ) )
        {
          accel += V2( 0.0f, -1.0f );
        }
        if( gameInterface.gameInput->KeyboardDown( KeyboardKey::A ) )
        {
          accel += V2( -1.0f, 0.0f );
        }
        if( gameInterface.gameInput->KeyboardDown( KeyboardKey::S ) )
        {
          accel += V2( 0.0f, 1.0f );
        }
        if( gameInterface.gameInput->KeyboardDown( KeyboardKey::D ) )
        {
          accel += V2( 1.0f, 0.0f );
        }
        accel *= movementForce;
      }

      r32 curSpeed = Length( vel );

      // flip turn
      if( curSpeed > 0.01f )
      {
        r32 aov = Dot( accel, vel );
        if( aov < -0.01f )
        {
          v2 normalizedvel = Normalize( vel );
          v2 accelpara = aov * vel / LengthSq( vel );
          v2 accelperp = accel - accelpara;
          accel = accelperp;
          //v2 normalizedaccel = Normalize( accel );
          //v2 velpara = aov * normalizedaccel;
          //v2 velperp = vel - velpara;
          //vel = velperp - velpara;
          //vel = velperp - velpara;
        }
      }


      // integrate velocity, and clamp to max speed
      vel += accel * gameInterface.gameInput->dt;
      curSpeed = Length( vel );
      if( curSpeed > maxspeed )
      {
        vel *= maxspeed / curSpeed;
      }
      curSpeed = Length( vel );

      // only apply friction when no input
      if(
        Length( accel ) < 0.01f &&
        curSpeed > 0.01f )
      {
        v2 oldvel = vel;
        v2 normalizedvel = vel / curSpeed;
        vel -= ( normalizedvel * movementForce ) * gameInterface.gameInput->dt;
        if( Dot( oldvel, vel ) < 0 )
        {
          Zero( vel );
        }
      }

      curSpeed = Length( vel );
      if( curSpeed > 0.01f )
      {
        // integrate position
        player.mPos.x += vel.x * gameInterface.gameInput->dt;
        player.mPos.z += vel.y * gameInterface.gameInput->dt;
        player.transformDirty = true;

        // rotate towards direction
        v3 baseAngleRads = baseAngle * DEG2RAD;
        player.mRot.x = baseAngleRads.x;
        player.mRot.y = baseAngleRads.y + Atan2( -vel.y, vel.x );
        player.mRot.z = baseAngleRads.z;
        player.transformDirty = true;
      }

      if( ImGui::CollapsingHeader( "player controls" ) )
      {
        float currentPlayerSpeed = Length( vel );
        float imguispeed = 0.01f;
        ImGui::DragFloat( "movement force", &movementForce, imguispeed, 0, 100 );
        ImGui::DragFloat( "cur speed", &currentPlayerSpeed, imguispeed );
        ImGui::DragFloat( "max speed", &maxspeed, imguispeed, 0, 100 );
        ImGui::DragFloat2( "vel", &vel.x, imguispeed );
        ImGui::DragFloat( "x", &player.mPos.x, imguispeed );
        ImGui::DragFloat( "y", &player.mPos.z, imguispeed );
        ImGui::DragFloat3( "base angle", &baseAngle.x, imguispeed );
      }
    }

    // get the closest tree
    u32 closestTreeIndex = 0;
    r32 closestTreeDistanceSq;
    for( u32 TreeIndex = 1; TreeIndex < entities.size(); ++TreeIndex )
    {
      TacEntity& entity = entities[ TreeIndex ];
      if( entity.mType == TacEntityType::Tree )
      {
        r32 distSq = DistanceSq( entity.mPos, player.mPos );
        if( !closestTreeIndex || distSq < closestTreeDistanceSq )
        {
          closestTreeIndex = TreeIndex;
          closestTreeDistanceSq = distSq;
        }
      }
    }

    if( closestTreeIndex )
    {
      TacEntity& closestTree = entities[ closestTreeIndex ];
      closestTree.mPos;

      // just draw a circle under the tree...
    }
  }

  ImGui::ShowMetricsWindow();


  renderGroup.Execute( gameInterface.renderer );
  EndTemporaryMemory( tempRenderMemory );
}

void TacGameState::Uninit( TacGameInterface& gameInterface )
{
  gameInterface.renderer->RemoveBlendState( blendStateHandle );
  gameInterface.renderer->RemoveCbuffer( cbufferHandlePerFrame );
  gameInterface.renderer->RemoveCbuffer( cbufferHandlePerObject );
  gameInterface.renderer->RemoveCbuffer( cbufferHandleSSAO );
  gameInterface.renderer->RemoveDepthBuffer( gbufferDepthStencil );
  gameInterface.renderer->RemoveDepthState( depthStateHandle );
  gameInterface.renderer->RemoveDepthState( noDepthTesting );
  gameInterface.renderer->RemoveIndexBuffer( ndcQuadIBO );
  gameInterface.renderer->RemoveRasterizerState( rasterizerStateHandle );
  gameInterface.renderer->RemoveShader( shaderHandle );
  gameInterface.renderer->RemoveShader( perPixelFrustumVectorsCreator );
  gameInterface.renderer->RemoveShader( gbufferFinalShader );
  gameInterface.renderer->RemoveSamplerState( linearSampler );
  gameInterface.renderer->RemoveSamplerState( pointSampler );
  gameInterface.renderer->RemoveTextureResoure( gbufferViewSpaceNormal );
  gameInterface.renderer->RemoveTextureResoure( gbufferDiffuse );
  gameInterface.renderer->RemoveTextureResoure( viewSpacePositionTexture );
  gameInterface.renderer->RemoveTextureResoure( randomVectorTexture );
  gameInterface.renderer->RemoveVertexFormat( vertexFormatHandle );
  gameInterface.renderer->RemoveVertexBuffer( ndcQuadVBO );

  {
    const u32 numRenderTargets = 1;
    TacTextureHandle renderTargets[ numRenderTargets ] =
    {
      gameInterface.renderer->GetBackbufferColor()
    };
    gameInterface.renderer->SetRenderTargets(
      renderTargets,
      numRenderTargets,
      gameInterface.renderer->GetBackbufferDepth() );
  }

  for(
    u32 iModel = 0;
    iModel < ( u32 )TacGameAssetID::Count;
    ++iModel )
  {
    TacGameAssetID id = ( TacGameAssetID )iModel;
    TacUnusedParameter( id );

    TacGameAssets::AsyncModelParams& param = gameTransientState->gameAssets.params[ iModel ];
    if( param.status != TacGameAssets::AsyncTaskStatus::Finished )
    {
      continue;
    }

    TacModel* model =
      param.model;
    // gameTransientState->gameAssets.GetModel( id );
    if( !model )
      continue;

    for( u32 i = 0; i < model->numSubModels; ++i )
    {
      TacSubModel& subModel = model->subModels[ i ];
      ModelBuffers& buffers = subModel.buffers;
      gameInterface.renderer->RemoveIndexBuffer(
        buffers.indexBufferHandle );
      for(
        u32 iVertexBuffer = 0;
        iVertexBuffer < buffers.numVertexBuffers;
        ++iVertexBuffer )
      {
        gameInterface.renderer->RemoveVertexBuffer(
          buffers.vertexBufferHandles[ iVertexBuffer ] );
      }
    }

    MemorymanagerDeallocate( *GetGlobalMemoryManager(), model );
  }
}

u32 TacGameState::AddEntity(
  TacEntityType type,
  TacGameAssetID assetID,
  v3 scale,
  v3 rot,
  v3 translate,
  v3 color )
{
  u32 index = entities.size();

  TacEntity entity = {};
  entity.mPos = translate;
  entity.mRot = rot;
  entity.mScale = scale;
  entity.mColor = color;
  entity.mType = type;
  entity.mAssetID = assetID;
  entity.transformDirty = true;

  entities.push_back( entity );
  return index;
}

void TacGameState::SaveEntities(
  TacThreadContext* thread,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  // todo: use json
  TacFile file = PlatformOpenFile(
    thread,
    entitiesfile.buffer,
    OpenFileDisposition::CreateAlways,
    FileAccess::Write,
    errors );
  if( errors.size )
    return;
  OnDestruct( PlatformCloseFile( thread, file, errors ); );
  SaveEntities( thread, file, errors );
  if( errors.size )
    return;
}

void TacGameState::SaveEntities(
  TacThreadContext* thread,
  TacFile file,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  u32 numEntities = entities.size();
  PlatformWriteEntireFile(
    thread,
    file,
    &numEntities,
    sizeof( u32 ),
    errors );
  if( errors.size )
    return;

  PlatformWriteEntireFile(
    thread,
    file,
    entities.data(),
    numEntities * sizeof( TacEntity ),
    errors );
  if( errors.size )
    return;
}

void TacGameState::LoadEntities(
  TacThreadContext* thread,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  // todo: use json
  TacFile file = PlatformOpenFile(
    thread,
    entitiesfile.buffer,
    OpenFileDisposition::OpenExisting,
    FileAccess::Read,
    errors );
  if( errors.size )
    return;

  OnDestruct( PlatformCloseFile( thread, file, errors ); );

  LoadEntities( thread, file, errors );
}

void TacGameState::LoadEntities(
  TacThreadContext* thread,
  TacFile file,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  OnDestruct( if( errors.size ) { entities.clear(); } );

  u32 numEntities;
  PlatformReadEntireFile(
    thread, file, &numEntities, sizeof( u32 ), errors );
  if( errors.size )
    return;

  entities.resize( numEntities );
  for( u32 i = 0; i < numEntities; ++i )
  {
    PlatformReadEntireFile(
      thread, file, &entities[ i ], sizeof( TacEntity ), errors );
    if( errors.size )
      return;
  }
}

void TacGameState::LoadCamera(
  TacThreadContext* thread,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  // todo: use json
  TacFile file = PlatformOpenFile(
    thread,
    camerafile.buffer,
    OpenFileDisposition::OpenExisting,
    FileAccess::Read,
    errors );
  if( errors.size )
    return;

  OnDestruct( PlatformCloseFile( thread, file, errors ); );

  LoadCamera( thread, file, errors );
  if( errors.size )
    return;
}

void TacGameState::LoadCamera(
  TacThreadContext* thread,
  TacFile file,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  PlatformReadEntireFile(
    thread,
    file,
    &mCamera,
    sizeof( Camera ),
    errors );
  if( errors.size )
    return;
}

void TacGameState::SaveCamera(
  TacThreadContext* thread,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  // todo: use json
  TacFile file = PlatformOpenFile(
    thread,
    camerafile.buffer,
    OpenFileDisposition::CreateAlways,
    FileAccess::Write,
    errors );
  if( errors.size )
    return;

  OnDestruct( PlatformCloseFile( thread, file, errors ); );

  SaveCamera( thread, file, errors );
}

void TacGameState::SaveCamera(
  TacThreadContext* thread,
  TacFile file,
  FixedString< DEFAULT_ERR_LEN >& errors )
{

  PlatformWriteEntireFile(
    thread,
    file,
    &mCamera,
    sizeof( Camera ),
    errors );
  if( errors.size )
    return;
}

void TacGameState::SaveScene(
  TacThreadContext* thread,

  FixedString< DEFAULT_ERR_LEN >& errors )
{
  // todo: use json
  TacFile file = PlatformOpenFile(
    thread,
    scenefile.buffer,
    OpenFileDisposition::CreateAlways,
    FileAccess::Write,
    errors );
  if( errors.size )
    return;

  OnDestruct( PlatformCloseFile( thread, file, errors ); );

  SaveCamera( thread, file, errors );
  if( errors.size )
    return;
  SaveEntities( thread, file, errors );
  if( errors.size )
    return;
}

void TacGameState::LoadScene(
  TacThreadContext* thread,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacFile file = {};

  OnDestruct( {
    PlatformCloseFile( thread, file, errors );

  if( entities.empty() )
  {
    AddEntity( TacEntityType::Null,
      TacGameAssetID::Cube,
      V3< r32 >( 1, 1, 1 ),
      V3< r32 >( 0, 0, 0 ),
      V3< r32 >( 0, 0, 0 ),
      V3< r32 >( 1, 1, 1 ) );
  }
  else
  {
    TacAssert( entities[ 0 ].mType == TacEntityType::Null );
  }

  if( errors.size )
  {
    mCamera.camDir = Normalize( V3( 0.0f, -0.2f, -1.0f ) );
    mCamera.camFOVYDeg = 90;
    mCamera.camNear = 1;
    mCamera.camFar = 100;
    mCamera.camPos = V3< r32 >( 0, 5, 10 );
    mCamera.camWorldUp = V3< r32 >( 0, 1, 0 );

    AddEntity(
      TacEntityType::Tree,
      TacGameAssetID::Cube,
      V3< r32 >( 1, 1, 1 ),
      V3< r32 >( 0, 0, 0 ),
      V3< r32 >( 0, 0, 0 ),
      V3< r32 >( 1, 0, 0 ) );
  }
  } );

  // todo: use json
  file = PlatformOpenFile(
    thread,
    scenefile.buffer,
    OpenFileDisposition::OpenExisting,
    FileAccess::Read,
    errors );
  if( errors.size )
    return;


  LoadCamera( thread, file, errors );
  if( errors.size )
    return;

  LoadEntities( thread, file, errors );
  if( errors.size )
    return;
}

struct LoadModelData
{
  TacGameAssets::AsyncModelParams* param;
  TacMemoryArena* allocator;
  const char* filepath;
  TacVertexFormat* vertexFormats;
  u32 numVertexFormats;
};

void QueueCallbackLoadModel(
  TacThreadContext* thread,
  void* data )
{
  TacUnusedParameter( thread );
  FixedString< DEFAULT_ERR_LEN > errors = {};

  LoadModelData* callbackData = ( LoadModelData* )data;
  TacMemoryArena* memoryArena = callbackData->allocator;

  OnDestruct(
    // make sure the task is completed after all other memory operations
    // ( so this OnDestruct happens AFTER all other OnDestructs )
    std::atomic_thread_fence( std::memory_order_release );
  callbackData->param->status =
    TacGameAssets::AsyncTaskStatus::PendingCompletion;
  );

  TacFile file = {};
  file = PlatformOpenFile(
    thread,
    callbackData->filepath,
    OpenFileDisposition::OpenExisting,
    FileAccess::Read,
    errors );
  TacAssert( !errors.size );
  OnDestruct(
    PlatformCloseFile(
      thread,
      file,
      errors );
  if( errors.size )
  {
    DisplayError(
      "Error during QueueCallbackLooadModel",
      0,
      __FILE__,
      __FUNCTION__ );
    CrashGame();
  }
  );

  u32 size = PlatformGetFileSize(
    thread,
    file,
    errors );
  TacAssert( !errors.size );

  uint8_t* memory = ( uint8_t* )PushSize( memoryArena, size );
  OnDestruct( PopSize( memoryArena, size ); );
  TacAssert( memory );

  PlatformReadEntireFile(
    thread,
    file,
    memory,
    size,
    errors );
  TacAssert( !errors.size );

  TacModelLoader modelLoader;
  OnDestruct( modelLoader.Release(); );
  modelLoader.OpenScene(
    memory,
    size,
    callbackData->filepath,
    errors );
  TacAssert( !errors.size );

  callbackData->param->loadedformat = modelLoader.AllocateVertexFormat(
    memoryArena,
    callbackData->vertexFormats,
    callbackData->numVertexFormats );
  TacAssert( !errors.size );

}

TacModel* TacGameAssets::GetModel(
  TacGameAssetID gameAssetID )
{
  TacModel* result = nullptr;
  u32 index = ( u32 )gameAssetID;
  AsyncModelParams& param = params[ index ];

  switch( param.status )
  {
    case AsyncTaskStatus::NotStarted:
    {
      param.iTaskArena = GetUnusedTaskArenaIndex();
      if( param.iTaskArena < numTaskArenas )
      {
        taskArenasUseds[ param.iTaskArena ] = true;
        TacMemoryArena* taskArena = &taskArenas[ param.iTaskArena ];
        param.model =
          ( TacModel* )MemoryManagerAllocate(
            *GetGlobalMemoryManager(),
            sizeof( TacModel ) );
        LoadModelData* callbackData = ( LoadModelData* )PushSize(
          taskArena,
          sizeof( LoadModelData ) );
        callbackData->allocator = taskArena;

        // set callbackdata filepath
        switch( gameAssetID )
        {
          case TacGameAssetID::Tree:
          {
            callbackData->filepath = "tacData/Models/tree.fbx";
          } break;
          case TacGameAssetID::Cube:
          {
            callbackData->filepath = "tacData/Models/cube.obj";
          } break;
          case TacGameAssetID::Arrow:
          {
            callbackData->filepath = "tacData/Models/arrowcube.fbx";
            //callbackData->filepath = "tacData/Models/chewtoy.fbx";
          } break;
          case TacGameAssetID::Sphere:
          {
            callbackData->filepath = "tacData/Models/sphere.fbx";
          } break;
          TacInvalidDefaultCase;
        }

        callbackData->numVertexFormats = numVertexFormats;
        callbackData->vertexFormats = vertexFormats;
        callbackData->param = &param;
        param.status = AsyncTaskStatus::Started;
        PushEntry(
          highPriorityQueue,
          QueueCallbackLoadModel,
          callbackData );
      }
    } break;
    case AsyncTaskStatus::Started:
    {
      // Waiting for the thread to set the status to PendingCompletion
    } break;
    case AsyncTaskStatus::PendingCompletion:
    {
      std::atomic_thread_fence( std::memory_order_consume );

      FixedString< DEFAULT_ERR_LEN > errors = {};

      ModelFormat& format = param.loadedformat;
      param.model->numSubModels = format.numsubformtas;
      for( u32 imodel = 0; imodel < format.numsubformtas; ++imodel )
      {
        SubModelFormat& subformat = format.subformats[ imodel ];
        TacSubModel& submodel = param.model->subModels[ imodel ];
        submodel.buffers = ModelBuffersCreate(
          renderer,
          subformat.numVertexes,
          subformat.vertexBuffers,
          format.vertexBufferStrides,
          format.numVertexBuffers,
          subformat.numIndexes,
          subformat.indexBuffer,
          format.indexFormat,
          subformat.numIndexes * sizeof( u32 ),
          errors );
        TacAssert( !errors.size );

        // set debug names
        {

          const char* assetIDString = assetIDstrings[ index ];
          TacUnusedParameter( assetIDString );
          renderer->SetName(
            submodel.buffers.indexBufferHandle,
            assetIDString );
          for(
            u32 iVertexBuffer = 0;
            iVertexBuffer < submodel.buffers.numVertexBuffers;
            ++iVertexBuffer )
          {
            renderer->SetName(
              submodel.buffers.vertexBufferHandles[ iVertexBuffer ],
              assetIDString );
          }
        }
      }

      // load the modelraycastinfo
      {
        TacModelRaycastInfo& modelRaycastInfo =
          modelRaycastInfos[ index ];

        TacVertexFormat positionVertexFormat = {};
        {
          b32 found = false;
          for( u32 i = 0; i < numVertexFormats; ++i )
          {
            TacVertexFormat& vertexFormat = vertexFormats[ i ];
            if( vertexFormat.mAttributeType == TacAttributeType::Position )
            {
              positionVertexFormat = vertexFormat;
              found = true;
              break;
            }
          }
          TacAssert( found );
        }
        u32 stride =
          format.vertexBufferStrides[ positionVertexFormat.mInputSlot ];

        u32 runningVertexCount = 0;
        u32 runningIndexCount = 0;
        for( u32 imodel = 0; imodel < format.numsubformtas; ++imodel )
        {
          SubModelFormat& subformat = format.subformats[ imodel ];
          runningVertexCount += subformat.numVertexes;
          runningIndexCount += subformat.numIndexes;
        }
        modelRaycastInfo.vertexes.resize( runningVertexCount );
        modelRaycastInfo.indexes.resize( runningIndexCount );

        runningVertexCount = 0;
        runningIndexCount = 0;
        u32 accumulatedVertexCount = 0;
        for( u32 imodel = 0; imodel < format.numsubformtas; ++imodel )
        {
          SubModelFormat& subformat = format.subformats[ imodel ];

          void* vertexBufferContainingPosition =
            subformat.vertexBuffers[ positionVertexFormat.mInputSlot ];

          TacAssert( positionVertexFormat.textureFormat ==
            TacTextureFormat::RGB32Float );
          //TacAssert(
          //  positionVertexFormat.mNumBytes == 4 );
          //TacAssert(
          //  positionVertexFormat.mVariableType == TacVariableType::real );
          //TacAssert(
          //  positionVertexFormat.mNumComponents == 3 );

          u8* bytedata =
            ( u8* )vertexBufferContainingPosition +
            positionVertexFormat.mAlignedByteOffset;
          for(
            u32 iVertex = 0;
            iVertex < subformat.numVertexes;
            ++iVertex, bytedata += stride )
          {
            v3* pos = ( v3* )bytedata;
            modelRaycastInfo.vertexes[ runningVertexCount++ ] = *pos;
          }

          for( u32 iindex = 0; iindex < subformat.numIndexes; ++iindex )
          {
            modelRaycastInfo.indexes[ runningIndexCount++ ] =
              subformat.indexBuffer[ iindex ] +
              accumulatedVertexCount;
          }

          accumulatedVertexCount += subformat.numVertexes;
        }

        modelRaycastInfo.boundingSphere = SphereExtents(
          modelRaycastInfo.vertexes.data(),
          modelRaycastInfo.vertexes.size() );
      }

      TacMemoryArena* taskArena = &taskArenas[ param.iTaskArena ];
      taskArena->used = 0;
      taskArenasUseds[ param.iTaskArena ] = false;
      param.status = AsyncTaskStatus::Finished;
    } break;
    case AsyncTaskStatus::Finished:
    {
      result = param.model;
    } break;
    TacInvalidDefaultCase;
  }
  return result;
}

TacModelRaycastInfo* TacGameAssets::GetModelRaycastInfo(
  TacGameAssetID gameAssetID )
{
  TacModelRaycastInfo* result = nullptr;
  u32 index = ( u32 )gameAssetID;
  AsyncModelParams& param = params[ index ];
  if( param.status == AsyncTaskStatus::Finished )
  {
    result = &modelRaycastInfos[ index ];
  }
  return result;
}

void MessageLoop::Draw()
{
  if( numMessages )
  {
    ImGui::Begin( "Messages", 0, ImGuiWindowFlags_AlwaysAutoResize );
    for( u32 imessage = 0; imessage < numMessages; ++imessage )
    {
      u32 index = ( curMessage + imessage ) % maxMessages;
      const char* message = messages[ index ].buffer;
      ImGui::Text( message );
    }
    if( numMessages > 0 &&
      ImGui::Button( "Clear messages" ) )
    {
      numMessages = 0;
    }
    ImGui::End();
  }
}

void MessageLoop::Update( r32 dt )
{
  elapsedMessageTime += dt;
  if( elapsedMessageTime > maxMessageTime )
  {
    PopMessage();
  }
}

void MessageLoop::AddMessage( const char* message )
{
  if( numMessages == maxMessages )
  {
    PopMessage();
  }

  u32 index = ( curMessage + numMessages++ ) % maxMessages;
  messages[ index ] = message;
  elapsedMessageTime = 0;
}

void MessageLoop::PopMessage()
{
  if( numMessages > 0 )
  {
    curMessage = ( curMessage + 1 ) % maxMessages;
    numMessages--;
    elapsedMessageTime = 0;
  }
}

void TacEmitter::Update( r32 dt )
{
  spawncounter += spawnrate * dt;
  spawncounter -= SpawnParticles( ( u32 )spawncounter );

  //static v3 wind1 = { 3, 1, 0 };
  //static v3 wind2 = { 3, 1, 0 };
  static v3 wind3 = { .07f, 0, 0 };
  //ImGui::DragFloat3( "wind1", &wind1.x, 0.01f );
  //ImGui::DragFloat3( "wind2", &wind2.x, 0.01f );
  ImGui::DragFloat3( "wind3", &wind3.x, 0.01f );
  static float damping = 0.99f;
  ImGui::DragFloat( "damping", &damping, 0.001f );

  u32 iParticle = 0;
  while( iParticle < numAlive )
  {
    TacParticle& p = particles[ iParticle ];
    p.cursize = p.maxsize * sizeByLife.GetPoint( 1.0f - p.curlife * p.maxlifeinverse ).y;
    p.vel.y -= accel * dt;
    p.vel += wind3 * dt;
    p.vel *= damping;
    //p.pos.x += wind1.x * sin( ( p.pos.y + RandReal( 0, wind1.z ) ) * wind1.y ) * dt;
    //p.pos.z += wind2.x * sin( ( p.pos.y + RandReal( 0, wind2.z ) ) * wind2.y ) * dt;
    p.pos += p.vel;
    p.curlife -= dt;
    if( p.curlife <= 0 )
    {
      p = particles[ --numAlive ];
      numDead++;
    }
    else
    {
      ++iParticle;
    }
  }
}
