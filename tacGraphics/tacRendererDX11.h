#pragma once

// tac
#include "tacRenderer.h"
#include "tacLibrary/tacPlatformWin32.h"
#include "tacLibrary/tacMath.h"
#include "tacLibrary/tacFilesystem.h"

// stl
#include <vector>

// directx
#include <d3d11_1.h>
#include <d3dcompiler.h>

struct ConstantFinder
{
  FixedString< 32 > mName;
  TacCBufferHandle mCBufferHandle;
  u32 mConstantIndex;
};

struct PerShaderData
{
  struct CBufferBinding
  {
    TacCBufferHandle mCBufferHandle;
    u32 mRegister;
  };
  CBufferBinding mCBufferBindings[ 5 ];
  u32 mNumCBufferBindings;
  Filepath mShaderPath;
  FixedString< 16 > mEntryPoint;
  FixedString< 8 > mShaderModel;
};

struct TacSampler
{
  FixedString< 32 > mName;
  TacShaderType mShaderType;
  u32 mSamplerIndex;
};

// DX11

struct TacCBufferDX11
{
  FixedString< 32 > mName;
  ID3D11Buffer* mHandle;

  u8 mMemory[ 4 * 16 * 100 ]; // 6kb
  u32 mSize;

  b32 mDirty;

  TacConstant mConstants[ 10 ];
  u32 mNumConstants;
};

struct TacTextureDX11
{
  ID3D11Resource* mHandle;
  s32 mWidth;
  s32 mHeight;
  s32 mDepth;
  ID3D11ShaderResourceView* mSrv;
  ID3D11RenderTargetView* mRTV;
};

struct TacDepthBufferDX11
{
  ID3D11Resource* mHandle;
  s32 mWidth;
  s32 mHeight;
  TacTextureFormat textureFormat;
  //u32 numDepthBits;
  //u32 numStencilBits;
  ID3D11DepthStencilView* mDSV;
};

struct TacShaderDX11
{
  ID3D11VertexShader* mVertexShader;
  ID3D11PixelShader* mPixelShader;
  ID3DBlob* mInputSig;

  ConstantFinder mConstantFinders[ 10 ];
  u32 mNumConstantFinders;

  PerShaderData mPerShaderData[ ( u32 )TacShaderType::Count ];

  TacSampler mTextures[ 10 ];
  u32 mNumTextures;

  TacSampler mSamplers[ 10 ];
  u32 mNumSamplers;
};

struct TacVertexBufferDX11
{
  ID3D11Buffer* mHandle;
  u32 mNumVertexes;
  // sizeof a vertex
  u32 mStride;
};

struct TacIndexBufferDX11
{
  ID3D11Buffer* mHandle;
  TacTextureFormat mIndexFormat;
  //TacVariableType mVariableType;
  //u32 mNumBytes;
  u32 mNumIndexes;
};

struct TacSamplerStateDX11
{
  ID3D11SamplerState* mHandle;
};

struct TacDepthStateDX11
{
  ID3D11DepthStencilState* mHandle;
};

struct TacBlendStateDX11
{
  ID3D11BlendState* mHandle;
};

struct TacRasterizerStateDX11
{
  ID3D11RasterizerState* mHandle;
};

struct RendererDX11 : public TacRenderer
{
  RendererDX11(
    HWND g_hWnd,
    FixedString< DEFAULT_ERR_LEN >& errors );

  ~RendererDX11();

  // vertex buffer --------------------------------------------------------

  TacVertexBufferHandle AddVertexBuffer(
    TacBufferAccess access,
    void* data,
    u32 numVertexes,
    u32 stride,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveVertexBuffer( TacVertexBufferHandle vertexBuffer ) override;

  void SetVertexBuffers(
    const TacVertexBufferHandle* myVertexBufferIDs,
    u32 numVertexBufferIDs ) override;

  void MapVertexBuffer(
    void** data,
    TacMap mapType,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void UnmapVertexBuffer(
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void SetName(
    TacVertexBufferHandle handle,
    const char* name ) override;

  TacVertexBufferHandle mCurrentVertexBuffer;
  std::vector< TacVertexBufferDX11 > mVertexBuffers;
  std::vector< u32 > mVertexBufferIndexes;

  // index buffer ---------------------------------------------------------

  TacIndexBufferHandle AddIndexBuffer(
    TacBufferAccess access,
    void* data,
    u32 numIndexes,
    TacTextureFormat dataType,
    u32 totalBufferSize,
    //TacVariableType variableType,
    //u32 numBytes,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveIndexBuffer( TacIndexBufferHandle indexBuffer ) override;

  void SetIndexBuffer( TacIndexBufferHandle indexBuffer ) override;

  void MapIndexBuffer(
    void** data,
    TacMap mapType,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void UnmapIndexBuffer(
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void SetName(
    TacIndexBufferHandle handle,
    const char* name ) override;
  TacIndexBufferHandle mCurrentIndexBuffer;
  std::vector< TacIndexBufferDX11 > mIndexBuffers;
  std::vector< u32 > mIndexBufferIndexes;

  // clear ----------------------------------------------------------------

  void ClearColor(
    TacTextureHandle textureHandle,
    v4 rgba ) override;

  void ClearDepthStencil(
    TacDepthBufferHandle depthBufferHandle,
    b32 clearDepth,
    r32 depth,
    b32 clearStencil,
    u8 stencil ) override;

  // Shader ---------------------------------------------------------------

  TacShaderHandle LoadShader(
    const char* shaderPaths[ ( u32 )TacShaderType::Count ],
    const char* entryPoints[ ( u32 )TacShaderType::Count ],
    const char* shaderModels[ ( u32 )TacShaderType::Count ],
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveShader( TacShaderHandle shader ) override;
  void ReloadShader( TacShaderHandle shader ) override;

  TacShaderHandle LoadShaderFromString(
    const char* shaderStr[ ( u32 )TacShaderType::Count ],
    const char* entryPoints[ ( u32 )TacShaderType::Count ],
    const char* shaderModels[ ( u32 )TacShaderType::Count ],
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void SetActiveShader( TacShaderHandle shaderID ) override;

  TacShaderHandle GetCurrentlyBoundShader() override;

  // non virtual ---
  void ReloadShader(
    TacShaderDX11& shader,
    FixedString< DEFAULT_ERR_LEN >& errors );

  void ReloadShaderFromString(
    TacShaderDX11& shader,
    const char* shaderStrings[ ( u32 )TacShaderType::Count ],
    FixedString< DEFAULT_ERR_LEN >& errors );

  void SetName(
    TacShaderHandle handle,
    const char* name ) override;

  std::vector< TacShaderDX11 > mShaders;
  std::vector< u32 > mShaderIndexes;
  TacShaderHandle mCurrentShader;

  // sampler state --------------------------------------------------------

  TacSamplerStateHandle AddSamplerState(
    const TacAddressMode u,
    const TacAddressMode v,
    const TacAddressMode w,
    TacComparison compare,
    TacFilter filter,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveSamplerState( TacSamplerStateHandle samplerState ) override;

  void AddSampler(
    const char* name,
    TacShaderHandle shader,
    TacShaderType shaderType,
    u32 samplerIndex,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void SetSamplerState(
    const char* samplerName,
    TacSamplerStateHandle samplerState ) override;

  void SetName(
    TacSamplerStateHandle handle,
    const char* name ) override;

  static const u32 SAMPLERS_PER_SHADER = 16;
  std::vector< TacSamplerStateDX11 > mSamplerStates;
  std::vector< u32 > mSamplerStateIndexes;
  TacSamplerStateHandle mCurrentSamplers
    [ ( u32 )TacShaderType::Count ][ SAMPLERS_PER_SHADER ];
  b32 mCurrentSamplersDirty[ ( u32 )TacShaderType::Count ];

  // texture --------------------------------------------------------------

  TacTextureHandle AddTextureResource(
    const TacImage& myImage,
    TacTextureUsage textureUsage,
    TacBinding binding,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveTextureResoure( TacTextureHandle texture ) override;

  void AddTexture(
    const char* name,
    TacShaderHandle shaderID,
    TacShaderType shaderType,
    u32 samplerIndex,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void SetTexture(
    const char* textureName,
    TacTextureHandle texture ) override;

  // non-virtual ---
  void CreateTexture(
    const TacImage& image,
    ID3D11Resource** texture,
    TacTextureUsage textureUsage,
    TacBinding binding,
    FixedString< DEFAULT_ERR_LEN >& errors );

  void CreateShaderResourceView(
    ID3D11Resource* resource,
    ID3D11ShaderResourceView** srv,
    FixedString< DEFAULT_ERR_LEN >& errors );

  void SetName(
    TacTextureHandle handle,
    const char* name ) override;

  static const u32 TEXTURES_PER_SHADER = 16;
  std::vector< TacTextureDX11 > mTextures;
  std::vector< u32 > mTextureIndexes;
  TacTextureHandle mCurrentTextures
    [ ( u32 )TacShaderType::Count ][ TEXTURES_PER_SHADER ];
  b32 mCurrentTexturesDirty[ ( u32 )TacShaderType::Count ];

  // render targets -------------------------------------------------------
  TacDepthBufferHandle AddDepthBuffer(
    u32 width,
    u32 height,
    TacTextureFormat textureFormat,
    //u32 numDepthBits,
    //u32 numStencilBits,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;
  void RemoveDepthBuffer( TacDepthBufferHandle depthBufer ) override;
  std::vector< TacDepthBufferDX11 > mDepthBuffers;
  std::vector< u32 > mDepthBufferIndexes;

  void SetRenderTargets(
    TacTextureHandle* renderTargets,
    u32 numRenderTargets,
    TacDepthBufferHandle depthBuffer ) override;

  void SetName(
    TacDepthBufferHandle handle,
    const char* name ) override;

  TacTextureHandle GetBackbufferColor() override;
  TacDepthBufferHandle GetBackbufferDepth() override;

  // cbuffer --------------------------------------------------------------

  TacCBufferHandle LoadCbuffer(
    const char* name,
    TacConstant* constants,
    u32 numConstants,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveCbuffer( TacCBufferHandle cbuffer ) override;

  void AddCbuffer(
    TacShaderHandle shader,
    TacCBufferHandle cbuffer,
    u32 cbufferRegister,
    TacShaderType myShaderType,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void SetName(
    TacCBufferHandle handle,
    const char* name ) override;

  std::vector< TacCBufferDX11 > mCbuffers;
  std::vector< u32 > mCbufferIndexes;

  // blend state ----------------------------------------------------------

  TacBlendStateHandle AddBlendState(
    TacBlendConstants srcRGB,
    TacBlendConstants dstRGB,
    TacBlendMode blendRGB,
    TacBlendConstants srcA,
    TacBlendConstants dstA,
    TacBlendMode blendA,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveBlendState( TacBlendStateHandle blendState ) override;

  void SetBlendState(
    TacBlendStateHandle myRendererID,
    v4 blendFactorRGBA,
    u32 sampleMask ) override;

  void SetName(
    TacBlendStateHandle handle,
    const char* name ) override;

  std::vector< TacBlendStateDX11 > mBlendStates;
  std::vector< u32 > mBlendStateIndexes;

  // rasterizer state -----------------------------------------------------

  TacRasterizerStateHandle AddRasterizerState(
    TacFillMode fillMode,
    TacCullMode cullMode,
    b32 frontCounterClockwise,
    b32 scissor,
    b32 multisample,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveRasterizerState(
    TacRasterizerStateHandle rasterizerState ) override;

  void SetRasterizerState(
    TacRasterizerStateHandle myRendererID ) override;
  void SetName(
    TacRasterizerStateHandle handle,
    const char* name ) override;

  std::vector< TacRasterizerStateDX11 > mRasterizerStates;
  std::vector< u32 > mRasterizerStateIndexes;

  // depth state ----------------------------------------------------------

  TacDepthStateHandle AddDepthState(
    b32 depthTest,
    b32 depthWrite,
    TacDepthFunc depthFunc,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveDepthState( TacDepthStateHandle depthState ) override;

  void SetDepthState(
    TacDepthStateHandle depthState,
    u32 stencilRef ) override;
  void SetName(
    TacDepthStateHandle handle,
    const char* name ) override;

  std::vector< TacDepthStateDX11 > mDepthStates;
  std::vector< u32 > mDepthStatesIndexes;

  // Vertex format --------------------------------------------------------

  // there is one vertex format per shader input.
  // todo: the shader should store its vertex format id
  TacVertexFormatHandle AddVertexFormat(
    TacVertexFormat* vertexFormats,
    u32 numVertexFormats,
    TacShaderHandle shaderID,
    FixedString< DEFAULT_ERR_LEN >& errors ) override;

  void RemoveVertexFormat( TacVertexFormatHandle vertexFormat ) override;

  void SetVertexFormat( TacVertexFormatHandle myVertexFormatID ) override;
  void SetName(
    TacVertexFormatHandle handle,
    const char* name ) override;

  struct TacVertexFormatDX11
  {
    ID3D11InputLayout* mHandle;
  };
  std::vector< TacVertexFormatDX11 > mVertexFormats;
  std::vector< u32 > mVertexFormatIndexes;

  // etc ------------------------------------------------------------------
  void DebugBegin( const char* section ) override;
  void DebugMark( const char* remark ) override;
  void DebugEnd() override;

  void Draw() override;
  void DrawIndexed(
    u32 elementCount,
    u32 idxOffset,
    u32 vtxOffset ) override;
  void SendUniform( const char* name, void* data, u32 size ) override;
  void Apply() override;
  void SwapBuffers() override;
  void SetViewport(
    r32 xRelBotLeftCorner,
    r32 yRelBotLeftCorner,
    r32 wIncreasingRight,
    r32 hIncreasingUp ) override;
  void SetPrimitiveTopology( TacPrimitive primitive ) override;
  void SetScissorRect(
    float x1,
    float y1,
    float x2,
    float y2 ) override;
  void GetPerspectiveProjectionAB(
    r32 f,
    r32 n,
    r32& A,
    r32& B ) override;

  // private functions and data -------------------------------------------

  ID3D11Device* mDevice;
  ID3D11DeviceContext* mDeviceContext;
  IDXGISwapChain* mSwapChain;
  //ID3D11DepthStencilView* mDSV;

  TacTextureHandle backBufferColorIndex;
  TacDepthBufferHandle backBufferDepthIndex;

  void MapResource(
    void** data,
    ID3D11Resource* resource,
    D3D11_MAP d3d11mapType,
    FixedString< DEFAULT_ERR_LEN >& errors );

  void AppendInfoQueueMessage(
    HRESULT hr,
    FixedString< DEFAULT_ERR_LEN >& errors );
#if TACDEBUG
  ID3D11InfoQueue* infoQueueDEBUG;
  ID3DUserDefinedAnnotation* userAnnotationDEBUG;
#endif
};
