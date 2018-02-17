#pragma once

#include "tacLibrary\tacPlatform.h"

// Using a separate struct for each resource type is for type safety
#define RENDERER_ID_NONE 0
#define DEFINE_HANDLE( name )\
struct name\
{\
  s32 mIndex;\
};
DEFINE_HANDLE( TacShaderHandle )
  DEFINE_HANDLE( TacVertexBufferHandle )
  DEFINE_HANDLE( TacIndexBufferHandle )
  DEFINE_HANDLE( TacSamplerStateHandle )
  DEFINE_HANDLE( TacTextureHandle )
  DEFINE_HANDLE( TacDepthBufferHandle )
  DEFINE_HANDLE( TacCBufferHandle )
  DEFINE_HANDLE( TacBlendStateHandle )
  DEFINE_HANDLE( TacRasterizerStateHandle )
  DEFINE_HANDLE( TacDepthStateHandle )
  DEFINE_HANDLE( TacVertexFormatHandle )

  enum class TacAttributeType
{
  Position,
  Normal,
  Texcoord,
  Color,
  BoneIndex,
  BoneWeight,
};

enum class TacVariableType
{
  unorm,
  snorm,
  uint,
  sint,
  real,
  count,
};

enum class TacTextureFormat
{
  Unknown,
  RGBA8Unorm,
  RGBA8Snorm,
  RGBA8Uint,
  R16Uint,
  R32Uint,
  R32Float,
  RG32Float,
  RGB32Float,
  RGBA16Float,
  RGBA32Float,
  D24S8,
};

struct TacVertexFormat
{
  u32 mInputSlot;
  TacAttributeType mAttributeType;
  //TacVariableType mVariableType;
  //u32 mNumBytes;
  //u32 mNumComponents;
  TacTextureFormat textureFormat;

  // offset of the variable from the vertex buffer. OffsetOf()
  u32 mAlignedByteOffset;
};

enum class TacDepthFunc
{
  Less,
  LessOrEqual,
};

struct TacConstant
{
  FixedString< 32 > name;
  u32 offset;
  u32 size;
};

enum class TacAddressMode
{
  Wrap,
  Clamp,
  Border,
};

enum class TacComparison
{
  Always,
  Never,
};

enum class TacFilter
{
  Point,
  Linear
};

enum class TacShaderType
{
  Vertex,
  Fragment,
  Count,
};

enum class TacBufferAccess
{
  Static,
  Default,
  Dynamic,
};

enum class TacMap
{
  Read,
  Write,
  ReadWrite,
  WriteDiscard,
};

enum class TacPrimitive
{
  TriangleList
};

enum class TacBlendMode
{
  Add,
};

enum class TacBlendConstants
{
  One,
  Zero,
  SrcRGB,
  SrcA,
  OneMinusSrcA,
};

enum class TacFillMode
{
  Solid,
  Wireframe
};

enum class TacCullMode
{
  None,
  Back,
  Front
};

enum class TacAttribute
{
  Pos = 0,
  Col = 1, 
  UV = 2, 
  Nor = 3 , 
  BoneId = 4 , 
  BoneWeight = 5,
  Tangents = 6,
  Bitangents = 7,
  Count
};

enum class TacTextureUsage
{
  Default,
  Immutable
};

enum class TacBinding
{
  ShaderResource = ( 1 << 0 ),
  RenderTarget = ( 1 << 1 ),
};
inline TacBinding operator | ( TacBinding a, TacBinding b )
{
  u32 result = ( u32 )a | ( u32 )b;
  return ( TacBinding )result;
}
inline u32 operator & ( TacBinding a, TacBinding b )
{
  u32 result = ( u32 )a & ( u32 )b;
  return result;
}


struct TacImage
{
public:
  u32 mWidth; // used in 3d, 2d, 1d textures
  u32 mHeight; // used in 3d, 2d, textures
  u32 mDepth; // used in 3d textures

  // the distance, in bytes, from one row of pixels to the next,
  // including padding
  u32 mPitch;
  //u32 mBitsPerPixel;
  u32 textureDimension; // 1 = 1d, 2 = 2d, 3 = 3d

  u8* mData;

  TacTextureFormat mTextureFormat;
  //TacVariableType variableType;
  //u32 variableBytesPerComponent;
  //u32 variableNumComponents;
};

struct TacRenderer
{
  virtual ~TacRenderer(){}

  // vertex buffer --------------------------------------------------------

  virtual TacVertexBufferHandle AddVertexBuffer(
    TacBufferAccess access,
    void* data,
    u32 numVertexes,
    u32 stride,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveVertexBuffer( TacVertexBufferHandle vertexBuffer ) = 0;

  virtual void SetVertexBuffers(
    const TacVertexBufferHandle* vertexBuffers,
    u32 numVertexBuffers ) = 0;

  virtual void MapVertexBuffer(
    void** data,
    TacMap mapType,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void UnmapVertexBuffer(
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void SetName(
    TacVertexBufferHandle handle,
    const char* name ) = 0;

  // index buffer ---------------------------------------------------------

  virtual TacIndexBufferHandle AddIndexBuffer(
    TacBufferAccess access,
    void* data,
    u32 numIndexes,
  TacTextureFormat dataType,
  u32 totalBufferSize,
    //TacVariableType variableType,
    //u32 numBytes,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveIndexBuffer( TacIndexBufferHandle indexBuffer ) = 0;

  virtual void SetIndexBuffer( TacIndexBufferHandle indexBuffer  ) = 0;

  virtual void MapIndexBuffer(
    void** data,
    TacMap mapType,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void UnmapIndexBuffer(
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;
  virtual void SetName(
    TacIndexBufferHandle handle,
    const char* name ) = 0;

  // clear ----------------------------------------------------------------

  virtual void ClearColor(
    TacTextureHandle textureHandle,
    v4 rgba ) = 0;

  virtual void ClearDepthStencil(
    TacDepthBufferHandle depthBufferHandle,
    b32 clearDepth,
    r32 depth,
    b32 clearStencil,
    u8 stencil ) = 0;

  // shader ---------------------------------------------------------------

  virtual TacShaderHandle LoadShader(
    const char* paths[ ( u32 )TacShaderType::Count ],
    const char* entryPoints[ ( u32 )TacShaderType::Count ],
    const char* shaderModels[ ( u32 )TacShaderType::Count ],
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveShader( TacShaderHandle shader ) = 0;

  virtual void ReloadShader( TacShaderHandle shader ) = 0;

  virtual TacShaderHandle LoadShaderFromString(
    const char* shaderStr[ ( u32 )TacShaderType::Count ],
    const char* entryPoints[ ( u32 )TacShaderType::Count ],
    const char* shaderModels[ ( u32 )TacShaderType::Count ],
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void SetActiveShader( TacShaderHandle shader ) = 0;

  virtual TacShaderHandle GetCurrentlyBoundShader() = 0;
  virtual void SetName(
    TacShaderHandle handle,
    const char* name ) = 0;

  // sampler state --------------------------------------------------------

  virtual TacSamplerStateHandle AddSamplerState(
    const TacAddressMode u,
    const TacAddressMode v,
    const TacAddressMode w,
    TacComparison compare,
  TacFilter filter,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveSamplerState( TacSamplerStateHandle  samplerState ) = 0;

  virtual void AddSampler( 
    const char* samplerName,
    TacShaderHandle shader,
    TacShaderType shaderType,
    u32 samplerIndex,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void SetSamplerState(
    const char* samplerName,
    TacSamplerStateHandle samplerState ) = 0;
  virtual void SetName(
    TacSamplerStateHandle handle,
    const char* name ) = 0;

  // texture --------------------------------------------------------------

  virtual TacTextureHandle AddTextureResource(
    const TacImage& myImage,
    TacTextureUsage textureUsage,
    TacBinding binding,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveTextureResoure( TacTextureHandle texture ) = 0;

  virtual void AddTexture(
    const char* textureName,
    TacShaderHandle shader,
    TacShaderType shaderType,
    u32 samplerIndex,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void SetTexture(
    const char* textureName,
    TacTextureHandle texture ) = 0;

  virtual void SetName(
    TacTextureHandle handle,
    const char* name ) = 0;

  // render targets -------------------------------------------------------
  virtual TacDepthBufferHandle AddDepthBuffer(
    u32 width,
    u32 height,
  TacTextureFormat textureFormat,
    //u32 numDepthBits,
    //u32 numStencilBits,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;
  virtual void RemoveDepthBuffer( TacDepthBufferHandle depthBufer ) = 0;

  virtual void SetRenderTargets(
    TacTextureHandle* renderTargets,
    u32 numRenderTargets,
    TacDepthBufferHandle depthBuffer ) = 0;

  virtual TacTextureHandle GetBackbufferColor() = 0;
  virtual TacDepthBufferHandle GetBackbufferDepth() = 0;
  virtual void SetName(
    TacDepthBufferHandle handle,
    const char* name ) = 0;

  // cbuffer --------------------------------------------------------------

  virtual TacCBufferHandle LoadCbuffer( 
    const char* name,
    TacConstant* constants,
    u32 numConstants,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveCbuffer( TacCBufferHandle cbuffer ) = 0;

  virtual void AddCbuffer(
    TacShaderHandle shader,
    TacCBufferHandle cbuffer,
    u32 cbufferRegister,
    TacShaderType myShaderType,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;
  virtual void SetName(
    TacCBufferHandle handle,
    const char* name ) = 0;

  // blend state ----------------------------------------------------------

  virtual TacBlendStateHandle AddBlendState(
    TacBlendConstants srcRGB,
    TacBlendConstants dstRGB,
    TacBlendMode blendRGB,
    TacBlendConstants srcA,
    TacBlendConstants dstA,
    TacBlendMode blendA,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveBlendState( TacBlendStateHandle blendState ) = 0;

  virtual void SetBlendState(
    TacBlendStateHandle blendState,
    v4 blendFactorRGBA = V4( 1.0f, 1.0f, 1.0f, 1.0f ),
    u32 sampleMask = 0xffffffff ) = 0;
  virtual void SetName(
    TacBlendStateHandle handle,
    const char* name ) = 0;

  // rasterizer state -----------------------------------------------------

  virtual TacRasterizerStateHandle AddRasterizerState(
    TacFillMode fillMode,
    TacCullMode cullMode,
    b32 frontCounterClockwise,
    b32 scissor,
    b32 multisample,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveRasterizerState(
    TacRasterizerStateHandle  rasterizerState ) = 0;

  virtual void SetRasterizerState(
    TacRasterizerStateHandle rasterizerState ) = 0;
  virtual void SetName(
    TacRasterizerStateHandle handle,
    const char* name ) = 0;

  // depth state ----------------------------------------------------------

  virtual TacDepthStateHandle AddDepthState(
    b32 depthTest,
    b32 depthWrite,
    TacDepthFunc depthFunc,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveDepthState( TacDepthStateHandle depthState ) = 0;

  // stencilRef
  //   Reference value to perform against when doing a depth-stencil test
  virtual void SetDepthState(
    TacDepthStateHandle depthState,
    u32 stencilRef = 0 ) = 0;
  virtual void SetName(
    TacDepthStateHandle handle,
    const char* name ) = 0;

  // vertex format --------------------------------------------------------

  virtual TacVertexFormatHandle AddVertexFormat(
    TacVertexFormat* vertexFormats,
    u32 numVertexFormats,
    TacShaderHandle shader,
    FixedString< DEFAULT_ERR_LEN >& errors ) = 0;

  virtual void RemoveVertexFormat( TacVertexFormatHandle vertexFormat ) = 0;

  virtual void SetVertexFormat( TacVertexFormatHandle vertexFormat ) = 0;
  virtual void SetName(
    TacVertexFormatHandle handle,
    const char* name ) = 0;

  // etc ------------------------------------------------------------------

  virtual void DebugBegin( const char* section ) = 0;
  virtual void DebugMark( const char* remark ) = 0;
  virtual void DebugEnd() = 0;

  virtual void Draw() = 0;

  virtual void DrawIndexed(
    u32 elementCount,
    u32 idxOffset,
    u32 vtxOffset ) = 0;
  virtual void SendUniform( const char* name, void* data, u32 size ) = 0;

  virtual void Apply() = 0;

  virtual void SwapBuffers() = 0;

  virtual void SetViewport(
    r32 xRelBotLeftCorner,
    r32 yRelBotLeftCorner,
    r32 wIncreasingRight,
    r32 hIncreasingUp ) = 0;

  virtual void SetPrimitiveTopology( TacPrimitive primitive ) = 0;

  virtual void SetScissorRect(
    float x1,
    float y1,
    float x2,
    float y2 ) = 0;

  virtual void GetPerspectiveProjectionAB(
    r32 f,
    r32 n,
    r32& A,
    r32& B ) = 0;
};
