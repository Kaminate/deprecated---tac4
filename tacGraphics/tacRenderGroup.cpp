#include "tacRenderGroup.h"
#include "tac4Model.h"

void TacRenderGroup::PushModel( TacModel* model )
{
  if( !model )
    return;
  PushData( TacRenderEntryType::Model );
  PushData( model );
}

void TacRenderGroup::PushClearColor( TacTextureHandle textureHandle, v4 rgba )
{
  PushData( TacRenderEntryType::ClearColor );
  PushData( textureHandle );
  PushData( rgba );
}

void TacRenderGroup::PushClearDepthStencil(
  TacDepthBufferHandle depthBufferHandle,
  b32 clearDepth,
  r32 depth,
  b32 clearStencil,
  u8 stencil )
{
  PushData( TacRenderEntryType::ClearDepthStencil );
  PushData( depthBufferHandle );
  PushData( clearDepth );
  PushData( depth );
  PushData( clearStencil );
  PushData( stencil );
}


void TacRenderGroup::PushRenderTargets(
  TacTextureHandle* textureHandles,
  u32 numTextureHandles,
  TacDepthBufferHandle depthBufferHandle )
{
  PushData( TacRenderEntryType::SetRenderTargets );
  PushData( numTextureHandles );
  PushData( textureHandles, numTextureHandles * sizeof( TacTextureHandle ) );
  PushData( depthBufferHandle );
}

void TacRenderGroup::PushBlendState(
  TacBlendStateHandle blendStateHandle,
  v4 blendFactorRGBA,
  u32 sampleMask )
{
  PushData( TacRenderEntryType::SetBlendState );
  PushData( blendStateHandle );
  PushData( blendFactorRGBA );
  PushData( sampleMask );
}

void TacRenderGroup::PushDepthState( TacDepthStateHandle depthStateHandle, u32 stencilRef )
{
  PushData( TacRenderEntryType::SetDepthState );
  PushData( depthStateHandle );
  PushData( stencilRef );
}


void TacRenderGroup::PushUniform( const char* name, void* data, u32 size )
{
  PushData( TacRenderEntryType::Uniform );
  PushData( name );
  PushData( size );
  PushData( data, size );
}

void TacRenderGroup::PushShader( TacShaderHandle shaderHandle )
{
  PushData( TacRenderEntryType::SetShader );
  PushData( shaderHandle );
}
void TacRenderGroup::PushPrimitive( TacPrimitive primitive )
{
  PushData( TacRenderEntryType::SetPrimitive );
  PushData( primitive );
}
void TacRenderGroup::PushVertexFormat( TacVertexFormatHandle vertexFormatHandle )
{
  PushData( TacRenderEntryType::SetVertexFormat );
  PushData( vertexFormatHandle );
}
void TacRenderGroup::PushRasterizerState( TacRasterizerStateHandle rasterizerStateHandle )
{
  PushData( TacRenderEntryType::SetRasterizerState );
  PushData( rasterizerStateHandle );
}

void TacRenderGroup::PushViewport(
  r32 xRelBotLeftCorner,
  r32 yRelBotLeftCorner,
  r32 wIncreasingRight,
  r32 hIncreasingUp )
{
  PushData( TacRenderEntryType::SetViewport );
  PushData( xRelBotLeftCorner );
  PushData( yRelBotLeftCorner );
  PushData( wIncreasingRight );
  PushData( hIncreasingUp );
}

void TacRenderGroup::PushApply()
{
  PushData( TacRenderEntryType::Apply );
}

void TacRenderGroup::PushDraw()
{
  PushData( TacRenderEntryType::Draw );
}

void TacRenderGroup::PushVertexBuffers(
  TacVertexBufferHandle* vertexBufferHandles,
  u32 numVertexBufferHandles )
{
  PushData( TacRenderEntryType::SetVertexBuffers );
  PushData( numVertexBufferHandles );
  PushData(
    vertexBufferHandles,
    sizeof( TacVertexBufferHandle ) * numVertexBufferHandles );
}

void TacRenderGroup::PushIndexBuffer(
  TacIndexBufferHandle indexBufferHandle )
{
  PushData( TacRenderEntryType::SetIndexBuffer );
  PushData( indexBufferHandle );
}
void TacRenderGroup::PushSamplerState(
  const char* name,
  TacSamplerStateHandle samplerStateHandle )
{
  PushData( TacRenderEntryType::SetSamplerState );
  PushData( name );
  PushData( samplerStateHandle );

}
void TacRenderGroup::PushTexture(
  const char* name,
  TacTextureHandle textureHandle )
{
  PushData( TacRenderEntryType::SetTexture );
  PushData( name );
  PushData( textureHandle );
}

struct Popper
{
  template< typename T >
  void Pop( T*& t, u32 count = 1 )
  {
    t = ( T* )runningAddress;
    runningAddress += sizeof( T ) * count;
  }
  u8* runningAddress;
};

void TacRenderGroup::Execute( TacRenderer* renderer )
{
  Popper p;
  p.runningAddress = mPushBufferMemory;
  const u8* endAddress =
    mPushBufferMemory +
    mPushBufferMemorySize;

  TacRenderEntryType* renderEntryType;
  while( p.runningAddress < endAddress )
  {
    p.Pop( renderEntryType );
    switch( *renderEntryType )
    {
      case TacRenderEntryType::ClearColor:
      {
        TacTextureHandle* textureHandle;
        p.Pop( textureHandle );
        v4* clearcolor;
        p.Pop( clearcolor );
        renderer->ClearColor(
          *textureHandle,
          *clearcolor );
      } break;
      case TacRenderEntryType::ClearDepthStencil:
      {
        TacDepthBufferHandle* depthBufferHandle;
        p.Pop( depthBufferHandle );
        b32* clearDepth;
        p.Pop( clearDepth );
        r32* depth;
        p.Pop( depth );
        b32* clearStencil;
        p.Pop( clearStencil );
        u8* stencil;
        p.Pop( stencil );
        renderer->ClearDepthStencil(
          *depthBufferHandle,
          *clearDepth,
          *depth,
          *clearStencil,
          *stencil );
      } break;
      case TacRenderEntryType::SetBlendState:
      {
        TacBlendStateHandle* blendStateHandle;
        p.Pop( blendStateHandle );
        v4* blendFactorRGBA;
        p.Pop( blendFactorRGBA );
        u32* sampleMask;
        p.Pop( sampleMask );
        renderer->SetBlendState(
          *blendStateHandle,
          *blendFactorRGBA,
          *sampleMask );
      } break;
      case TacRenderEntryType::SetDepthState:
      {
        TacDepthStateHandle* depthStateHandle;
        p.Pop( depthStateHandle );
        u32* stencilRef;
        p.Pop( stencilRef );
        renderer->SetDepthState(
          *depthStateHandle,
          *stencilRef );
      } break;
      case TacRenderEntryType::SetRenderTargets:
      {
        TacTextureHandle* textureHandles;
        u32* numTextureHandles;
        TacDepthBufferHandle* depthBufferHandle;
        p.Pop( numTextureHandles );
        p.Pop( textureHandles, *numTextureHandles );
        p.Pop( depthBufferHandle );
        renderer->SetRenderTargets(
          textureHandles,
          *numTextureHandles,
          *depthBufferHandle );
      } break;
      case TacRenderEntryType::Model:
      {
        TacModel** pmodel;
        p.Pop( pmodel );
        renderer->Apply();
        TacModel* model = *pmodel;
        for(
          u32 iSubModel = 0;
          iSubModel < model->numSubModels;
          ++iSubModel )
        {
          ModelBuffersRender(
            renderer,
            model->subModels[ iSubModel ].buffers );
        }
      } break;
      case TacRenderEntryType::Uniform:
      {
        const char** name;
        u32* size;
        u8* data;
        p.Pop( name );
        p.Pop( size );
        p.Pop( data, *size );
        renderer->SendUniform(
          *name,
          data,
          *size );
      } break;
      case TacRenderEntryType::SetShader:
      {
        TacShaderHandle* shaderHandle;
        p.Pop( shaderHandle );
        renderer->SetActiveShader( *shaderHandle );
      } break;
      case TacRenderEntryType::SetPrimitive:
      {
        TacPrimitive* primitive;
        p.Pop( primitive );
        renderer->SetPrimitiveTopology( *primitive );
      } break;
      case TacRenderEntryType::SetVertexFormat:
      {
        TacVertexFormatHandle* vertexFormatHandle;
        p.Pop( vertexFormatHandle );
        renderer->SetVertexFormat( *vertexFormatHandle );
      } break;
      case TacRenderEntryType::SetRasterizerState:
      {
        TacRasterizerStateHandle* rasterizerStateHandle;
        p.Pop( rasterizerStateHandle );
        renderer->SetRasterizerState( *rasterizerStateHandle );
      } break;
      case TacRenderEntryType::SetViewport:
      {
        r32* xRelBotLeftCorner;
        p.Pop( xRelBotLeftCorner );
        r32* yRelBotLeftCorner;
        p.Pop( yRelBotLeftCorner );
        r32* wIncreasingRight;
        p.Pop( wIncreasingRight );
        r32* hIncreasingUp;
        p.Pop( hIncreasingUp );
        renderer->SetViewport(
          *xRelBotLeftCorner,
          *yRelBotLeftCorner,
          *wIncreasingRight,
          *hIncreasingUp );
      } break;
      case TacRenderEntryType::Apply:
      {
        renderer->Apply();
      } break;
      case TacRenderEntryType::Draw:
      {
        renderer->Draw();
      } break;
      case TacRenderEntryType::SetVertexBuffers:
      {
        u32* numVertexBufferHandles;
        TacVertexBufferHandle* vertexBufferHandles;
        p.Pop( numVertexBufferHandles );
        p.Pop(
          vertexBufferHandles,
          *numVertexBufferHandles );
        renderer->SetVertexBuffers(
          vertexBufferHandles,
          *numVertexBufferHandles );
      } break;
      case TacRenderEntryType::SetIndexBuffer:
      {
        TacIndexBufferHandle* indexBufferHandle;
        p.Pop( indexBufferHandle );
        renderer->SetIndexBuffer(
          *indexBufferHandle );
      } break;
      case TacRenderEntryType::SetSamplerState:
      {
        const char** name;
        p.Pop( name );
        TacSamplerStateHandle* samplerStateHandle;
        p.Pop( samplerStateHandle );
        renderer->SetSamplerState(
          *name,
          *samplerStateHandle );
      } break;
      case TacRenderEntryType::SetTexture:
      {
        const char** name;
        p.Pop( name );
        TacTextureHandle* textureHandle;
        p.Pop( textureHandle );
        renderer->SetTexture(
          *name,
          *textureHandle );
      } break;
      TacInvalidDefaultCase;
    }
  }
}
