#pragma once
#include "tacLibrary\tacPlatform.h"
#include "tacRenderer.h"
struct TacModel;

enum class TacRenderEntryType
{
  ClearColor,
  ClearDepthStencil,
  SetRenderTargets,
  Uniform,
  Model,
  SetBlendState,
  SetDepthState,
  SetShader,
  SetPrimitive,
  SetVertexFormat,
  SetRasterizerState,
  SetViewport,
  SetVertexBuffers,
  SetIndexBuffer,
  Apply,
  Draw,
  SetSamplerState,
  SetTexture,
};

struct TacRenderGroup
{
  u8* mPushBufferMemory;
  u32 mPushBufferMemorySize;
  u32 mPushBufferMemorySizeMax;

  // push commands
  void PushModel( TacModel* model );
  void PushClearColor( TacTextureHandle textureHandle, v4 rgba );
  void PushClearDepthStencil(
    TacDepthBufferHandle depthBufferHandle, 
    b32 clearDepth = true,
    r32 depth = 1.0f,
    b32 clearStencil = false,
    u8 stencil = 0.0f );
  void PushRenderTargets(
    TacTextureHandle* textureHandles,
    u32 numTextureHandles,
    TacDepthBufferHandle depthBufferHandle );
  void PushBlendState(
    TacBlendStateHandle blendStateHandle,
    v4 blendFactorRGBA = V4( 1.0f, 1.0f, 1.0f, 1.0f ),
    u32 sampleMask = 0xffffffff );
  void PushDepthState( TacDepthStateHandle depthStateHandle, u32 stencilRef = 0 );
  void PushUniform( const char* name, void* data, u32 size );
  void PushShader( TacShaderHandle shaderHandle );
  void PushPrimitive( TacPrimitive primitive );
  void PushVertexFormat( TacVertexFormatHandle vertexFormatHandle );
  void PushRasterizerState( TacRasterizerStateHandle rasterizerStateHandle );
  void PushViewport(
    r32 xRelBotLeftCorner,
    r32 yRelBotLeftCorner,
    r32 wIncreasingRight,
    r32 hIncreasingUp );
  void PushApply();
  void PushDraw();
  void PushVertexBuffers(
      TacVertexBufferHandle* vertexBufferHandles,
      u32 numVertexBufferHandles );
  void PushIndexBuffer(
      TacIndexBufferHandle indexBufferHandle );

  void PushSamplerState(
    const char* name,
    TacSamplerStateHandle samplerStateHandle );
  void PushTexture(
    const char* name,
    TacTextureHandle textureHandle );

  // execute all stored commands
  void Execute( TacRenderer* renderer );

  // helper functions
  void PushData( const void* source, u32 size )
  {
    u8* result = nullptr;
    if( mPushBufferMemorySize + size <= mPushBufferMemorySizeMax )
    {
      result = mPushBufferMemory + mPushBufferMemorySize;
      mPushBufferMemorySize += size;
      memcpy( result, source, size );
    }
    else
    {
      TacInvalidCodePath;
    }
  }
  template< typename T>
  void PushData( const T& t )
  {
    PushData( &t, sizeof( T ) );
  }
};

