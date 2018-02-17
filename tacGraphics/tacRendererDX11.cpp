#include "tacRendererDX11.h"

#include <initguid.h>
#include <dxgidebug.h>

// helper -----------------------------------------------------------------

DXGI_FORMAT GetDXGIFormat( TacTextureFormat textureFormat )
{
  switch( textureFormat )
  {
  case TacTextureFormat::Unknown:
    return DXGI_FORMAT_UNKNOWN;
  case TacTextureFormat::RGBA8Unorm:
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case TacTextureFormat::R32Float:
    return DXGI_FORMAT_R32_FLOAT;
  case TacTextureFormat::RGBA8Snorm:
    return DXGI_FORMAT_R8G8B8A8_SNORM;
  case TacTextureFormat::RGBA8Uint:
    return DXGI_FORMAT_R8G8B8A8_UINT;
  case TacTextureFormat::R16Uint:
    return DXGI_FORMAT_R16_UINT;
  case TacTextureFormat::R32Uint:
    return DXGI_FORMAT_R32_UINT;
  case TacTextureFormat::RG32Float:
    return DXGI_FORMAT_R32G32_FLOAT;
  case TacTextureFormat::RGB32Float:
    return DXGI_FORMAT_R32G32B32_FLOAT;
  case TacTextureFormat::RGBA16Float:
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
  case TacTextureFormat::RGBA32Float:
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
  case TacTextureFormat::D24S8:
    return DXGI_FORMAT_D24_UNORM_S8_UINT;
    TacInvalidDefaultCase;
  }
  return DXGI_FORMAT_UNKNOWN;
}

internalFunction void TacCompileShaderFromFile(
  const char* filepath, 
  const char* entrypoint,
  const char* shaderModel,
  ID3DBlob** ppBlobOut,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  HRESULT hr = S_OK;
  DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef TACDEBUG
  dwShaderFlags |= D3DCOMPILE_DEBUG;
  dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  Filepath filepathType;
  filepathType = filepath;

  WCHAR wcharFilepath[ ArraySize( filepathType.buffer ) ];
  for( u32 i = 0;  i < filepathType.size; ++i )
    wcharFilepath[ i ] = btowc( filepathType.buffer[ i ] );
  wcharFilepath[ filepathType.size ] = 0;

  CComPtr< ID3DBlob > pErrorBlob;
  hr = D3DCompileFromFile(
    wcharFilepath,
    nullptr, // D3D_SHADER_MACRO* pDefines,
    nullptr, // ID3DInclude* pInclude,
    entrypoint,
    shaderModel,
    dwShaderFlags,
    0,
    ppBlobOut,
    &pErrorBlob );
  if( FAILED( hr ) )
  {
    filepathType = FilepathGetFile( filepathType );
    errors.size = sprintf_s(
      errors.buffer,
      "Failed to compile shader %s\n",
      filepathType.buffer );
    if( pErrorBlob.p )
    {
      FixedStringAppend(
        errors,
        ( const char* )pErrorBlob->GetBufferPointer() );
      return;
    }
  }
}

internalFunction void TacCompileShaderFromString(
  ID3DBlob** ppBlobOut,
  void* shaderStr,
  u32 shaderStrSize,
  const char* entryPoint,
  const char* shaderModel,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  HRESULT hr = S_OK;
  DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef TACDEBUG
  dwShaderFlags |= D3DCOMPILE_DEBUG;
  dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  CComPtr< ID3DBlob > pErrorBlob;
  hr = D3DCompile(
    shaderStr,
    shaderStrSize,
    nullptr,
    nullptr, // D3D_SHADER_MACRO* pDefines,
    nullptr, // ID3DInclude* pInclude,
    entryPoint,
    shaderModel,
    dwShaderFlags,
    0,
    ppBlobOut,
    &pErrorBlob );
  if( FAILED( hr ) )
  {
    if( pErrorBlob.p )
    {
      FixedStringAppend(
        errors,
        ( const char* )pErrorBlob->GetBufferPointer() );
    }
  }
}

internalFunction D3D11_USAGE GetUsage( TacBufferAccess access )
{
  switch ( access )
  {
  case TacBufferAccess::Default: return D3D11_USAGE_DEFAULT;
  case TacBufferAccess::Dynamic: return D3D11_USAGE_DYNAMIC;
  case TacBufferAccess::Static: return D3D11_USAGE_IMMUTABLE;
  default: TacAssert( false ); return ( D3D11_USAGE )0;
  }
}

internalFunction D3D11_MAP GetD3D11_MAP( TacMap mapType )
{
  D3D11_MAP result = D3D11_MAP_READ;
  switch (mapType)
  {
  case TacMap::Read:
    result = D3D11_MAP_READ;
    break;
  case TacMap::Write:
    result = D3D11_MAP_WRITE;
    break;
  case TacMap::ReadWrite:
    result = D3D11_MAP_READ_WRITE;
    break;
  case TacMap::WriteDiscard:
    result = D3D11_MAP_WRITE_DISCARD;
    break;
  default:
    TacAssert( false );
    break;
  }
  return result;
}

internalFunction TacSampler* GetSampler(
  const char* name,
  TacSampler* samplers,
  u32 numSamplers )
{
  TacSampler* result = nullptr;
  for( u32 i = 0; i < numSamplers; ++i )
  {
    TacSampler* shaderSampler = &samplers[ i ];
    if( TacStrCmp( shaderSampler->mName.buffer, name ) == 0 )
    {
      result = shaderSampler;
      break;
    }
  }
  return result;
}

// memory clean up helpers ------------------------------------------------
internalFunction void ReleaseDepthBuffer(
  TacDepthBufferDX11& depthBufferDX11 )
{
  if( depthBufferDX11.mHandle )
  {
    depthBufferDX11.mHandle->Release();
  }

  if( depthBufferDX11.mDSV )
  {
    depthBufferDX11.mDSV->Release();
  }

  Clear( depthBufferDX11 );
}

internalFunction void ReleaseTexture(
  TacTextureDX11& textureDX11 )
{
  if( textureDX11.mHandle )
  {
    textureDX11.mHandle->Release();
  }

  if( textureDX11.mSrv )
  {
    textureDX11.mSrv->Release();
  }

  if( textureDX11.mRTV )
  {
    textureDX11.mRTV->Release();
  }

  Clear( textureDX11 );
}

internalFunction void ReleaseShader(
  TacShaderDX11& shader )
{
  if( shader.mVertexShader )
    shader.mVertexShader->Release();

  if( shader.mPixelShader )
    shader.mPixelShader->Release();

  Clear( shader );
}

internalFunction void ReleaseVertexBuffer(
  TacVertexBufferDX11& vertexBufferDX11 )
{
  if( vertexBufferDX11.mHandle )
  {
    vertexBufferDX11.mHandle->Release();
  }

  Clear( vertexBufferDX11 );
}

internalFunction void ReleaseIndexBuffer(
  TacIndexBufferDX11& indexBufferDX11 )
{
  if( indexBufferDX11.mHandle )
  {
    indexBufferDX11.mHandle->Release();
  }

  Clear( indexBufferDX11 );
}

internalFunction void ReleaseSamplerState(
  TacSamplerStateDX11& samplerStateDX11 )
{
  if( samplerStateDX11.mHandle )
  {
    samplerStateDX11.mHandle->Release();
  }
  Clear( samplerStateDX11 );
}

internalFunction void ReleaseDepthState(
  TacDepthStateDX11& depthStateDX11 )
{
  if( depthStateDX11.mHandle )
  {
    depthStateDX11.mHandle->Release();
  }
  Clear( depthStateDX11 );
}

internalFunction void ReleaseBlendState(
  TacBlendStateDX11& blendStateDX11 )
{
  if( blendStateDX11.mHandle )
  {
    blendStateDX11.mHandle->Release();
  }

  Clear( blendStateDX11 );
}


internalFunction void ReleaseRasterizerState(
  TacRasterizerStateDX11& rasterizerStateDX11 )
{
  if( rasterizerStateDX11.mHandle )
  {
    rasterizerStateDX11.mHandle->Release();
  }

  Clear( rasterizerStateDX11 );
}

// RendererDX11 -----------------------------------------------------------

template< typename T >
void AddEmpty( std::vector< T >& v )
{
  T empty = { 0 };
  v.push_back( empty );
}

RendererDX11::RendererDX11(
  HWND g_hWnd,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  mCurrentShader.mIndex = RENDERER_ID_NONE;
  mCurrentVertexBuffer.mIndex = RENDERER_ID_NONE;
  mCurrentIndexBuffer.mIndex = RENDERER_ID_NONE;
  backBufferColorIndex.mIndex = RENDERER_ID_NONE;
  backBufferDepthIndex.mIndex = RENDERER_ID_NONE;

  // Initialize null indexes
  {
    AddEmpty( mVertexFormats );
    AddEmpty( mVertexBuffers );
    AddEmpty( mIndexBuffers );
    AddEmpty( mShaders );
    AddEmpty( mSamplerStates );
    AddEmpty( mTextures );
    AddEmpty( mCbuffers );
    AddEmpty( mBlendStates );
    AddEmpty( mRasterizerStates );
    AddEmpty( mDepthStates );
    AddEmpty( mDepthBuffers );
  }

  RECT myrect;
  if( 0 == GetClientRect( g_hWnd, &myrect ) )
  {
    errors = "GetClientRect failed\n";
    AppendWin32Error( errors, GetLastError() );
    return;
  }
  u32 width = ( u32 )( myrect.right - myrect.left );
  u32 height = ( u32 )( myrect.bottom - myrect.top );

  HRESULT hr = S_OK;

  // Init Device ----------------------------------------------------------
  {

    UINT createDeviceFlags = 0;
#ifdef TACDEBUG
    // apparently this requires windows 10 sdk now
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
      D3D_DRIVER_TYPE_HARDWARE,
      D3D_DRIVER_TYPE_WARP,
      D3D_DRIVER_TYPE_REFERENCE,
    };

    D3D_FEATURE_LEVEL featureLevels[] =
    {
      D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0,
    };

    // NOTE( N8 ):
    // get the driver type, feature level, device, and device context

    for(
      UINT driverTypeIndex = 0;
      driverTypeIndex < ARRAYSIZE( driverTypes );
    driverTypeIndex++ )
    {
      D3D_DRIVER_TYPE driverType= driverTypes[driverTypeIndex];
      D3D_FEATURE_LEVEL g_featureLevel;

      hr = D3D11CreateDevice(
        nullptr,
        driverType,
        nullptr,
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE( featureLevels ),
        D3D11_SDK_VERSION,
        &mDevice,
        &g_featureLevel,
        &mDeviceContext );

      if ( hr == E_INVALIDARG )
      {
        // DirectX 11.0 platforms will not recognize
        // D3D_FEATURE_LEVEL_11_1 so we need to retry without it
        hr = D3D11CreateDevice(
          nullptr,
          driverType,
          nullptr,
          createDeviceFlags,
          &featureLevels[1],
          ARRAYSIZE( featureLevels ) - 1,
          D3D11_SDK_VERSION,
          &mDevice,
          &g_featureLevel,
          &mDeviceContext );
      }

      if( SUCCEEDED( hr ) )
        break;
    }
    if( FAILED( hr ) )
    {
      errors = "Failed to create device";
      return;
    }



    CComPtr< IDXGIDevice > dxgiDevice;
    hr = mDevice->QueryInterface(
      __uuidof( IDXGIDevice ),
      reinterpret_cast< void** >( &dxgiDevice ) );
    if( FAILED( hr ) )
    {
      errors = "Failed to get IDXGIDevice";
      return;
    }

#if TACDEBUG
    hr = mDevice->QueryInterface(
      __uuidof( ID3D11InfoQueue ),
      ( void** )&infoQueueDEBUG );
    if( FAILED( hr ) )
    {
      errors = "Failed to get ID3D11InfoQueue";
      return;
    }

    hr = mDeviceContext->QueryInterface(
      __uuidof( ID3DUserDefinedAnnotation ),
      ( void** )&userAnnotationDEBUG );
    if( FAILED( hr ) )
    {
      errors = "Failed to get ID3DUserDefinedAnnotation";
      AppendInfoQueueMessage( hr, errors );
      return;
    }
#endif

    // Obtain DXGI factory from device
    // (since we used nullptr for pAdapter above)
    CComPtr< IDXGIAdapter > adapter;
    hr = dxgiDevice->GetAdapter(&adapter);
    if( FAILED( hr ) )
    {
      errors = "Failed to get IDXGIAdapter";
      return;
    }

    CComPtr< IDXGIFactory1 > dxgiFactory;
    hr = adapter->GetParent(
      __uuidof( IDXGIFactory1 ),
      reinterpret_cast<void**>(&dxgiFactory) );
    if( FAILED( hr ) )
    {
      errors = "Failed to get IDXGIFactory1";
      return;
    }

    // Create swap chain
    {
      CComPtr< IDXGIFactory2 > dxgiFactory2;
      hr = dxgiFactory->QueryInterface(
        __uuidof(IDXGIFactory2),
        reinterpret_cast<void**>(&dxgiFactory2) );
      if ( SUCCEEDED( hr ) )
      {
        TacAssert( dxgiFactory2.p );
        // DirectX 11.1 or later
        CComPtr< ID3D11Device1 > g_pd3dDevice1;
        hr = mDevice->QueryInterface(
          __uuidof(ID3D11Device1),
          reinterpret_cast<void**>(&g_pd3dDevice1) );
        if( FAILED( hr ) )
        {
          errors = "Failed to get ID3D11Device1";
          return;
        }

        CComPtr< ID3D11DeviceContext1 > g_pImmediateContext1;
        hr = mDeviceContext->QueryInterface(
          __uuidof(ID3D11DeviceContext1),
          reinterpret_cast<void**>(&g_pImmediateContext1) );
        if( FAILED( hr ) )
        {
          errors = "Failed to get ID3D11DeviceContext1";
          return;
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        CComPtr< IDXGISwapChain1 > g_pSwapChain1;
        hr = dxgiFactory2->CreateSwapChainForHwnd(
          mDevice,
          g_hWnd,
          &sd,
          nullptr,
          nullptr,
          &g_pSwapChain1 );
        if( FAILED( hr ) )
        {
          errors = "Failed to create IDXGISwapChain1";
          return;
        }
        hr = g_pSwapChain1->QueryInterface(
          __uuidof(IDXGISwapChain),
          reinterpret_cast<void**>(&mSwapChain) );
        if( FAILED( hr ) )
        {
          errors = "Failed to get IDXGISwapChain";
          return;
        }
      }
      else
      {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 2;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain( mDevice, &sd, &mSwapChain );
        if( FAILED( hr ) )
        {
          errors = "Failed to get IDXGISwapChain";
          return;
        }
      }
    }

    // We'll handle alt-enter ourselves thank you very much
    hr = dxgiFactory->MakeWindowAssociation(
      g_hWnd,
      DXGI_MWA_NO_WINDOW_CHANGES | 
      DXGI_MWA_NO_ALT_ENTER );
    if (FAILED(hr))
    {
      errors = "MakeWindowAssociation failed";
      return;
    }

    // backbuffer color
    {
      TacTextureDX11 backbufferColorDX11 = {};
      backbufferColorDX11.mWidth = width;
      backbufferColorDX11.mHeight = height;

      OnDestruct( if( errors.size ) { ReleaseTexture( backbufferColorDX11 ); } );

      CComPtr< ID3D11Texture2D > backBufferColorTexture;
      hr = mSwapChain->GetBuffer(
        0,
        __uuidof( ID3D11Texture2D ),
        reinterpret_cast<void**>( &backBufferColorTexture ) );
      if( FAILED( hr ) )
      {
        errors = "Failed to get backbuffer";
        return;
      }

      hr = mDevice->CreateRenderTargetView(
        backBufferColorTexture,
        nullptr,
        &backbufferColorDX11.mRTV );
      if( FAILED( hr ) )
      {
        errors =
          "Failed to create backbuffer render target view";
        return;
      }

      backBufferColorIndex.mIndex = mTextures.size();
      mTextures.push_back( backbufferColorDX11 );
      SetName( backBufferColorIndex, STRINGIFY( backBufferColorIndex ) );
    }

    // backbuffer depth
    {
      TacDepthBufferDX11 backBufferDepthDX11 = {};
      backBufferDepthDX11.mWidth = width;
      backBufferDepthDX11.mHeight = height;
      backBufferDepthDX11.textureFormat = TacTextureFormat::D24S8;
      //backBufferDepthDX11.numDepthBits = 24;
      //backBufferDepthDX11.numStencilBits = 8;

      OnDestruct( if( errors.size ) { ReleaseDepthBuffer( backBufferDepthDX11 ); } );

      CComPtr< ID3D11Texture2D > backBufferDepthTexture;
      D3D11_TEXTURE2D_DESC descDepth = {};
      descDepth.Width = width;
      descDepth.Height = height;
      descDepth.MipLevels = 1;
      descDepth.ArraySize = 1;
      descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      descDepth.SampleDesc.Count = 1;
      descDepth.SampleDesc.Quality = 0;
      descDepth.Usage = D3D11_USAGE_DEFAULT;
      descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
      hr = mDevice->CreateTexture2D(
        &descDepth,
        nullptr,
        &backBufferDepthTexture );
      if( FAILED( hr ) )
      {
        errors = "CreateTexture2D failed";
        return;
      }

      // create the depth stencil view
      {
        D3D11_DEPTH_STENCIL_VIEW_DESC descDSVDesc = {};
        descDSVDesc.Format = descDepth.Format;
        descDSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        hr = mDevice->CreateDepthStencilView(
          backBufferDepthTexture,
          &descDSVDesc,
          &backBufferDepthDX11.mDSV );
        if( FAILED( hr ) )
        {
          errors = "CreateDepthStencilView failed";
          AppendWin32Error( errors, hr );
          AppendInfoQueueMessage( hr, errors );
          return;
        }

        backBufferDepthIndex.mIndex = mDepthBuffers.size();
        mDepthBuffers.push_back( backBufferDepthDX11 );
        SetName(
          backBufferDepthIndex,
          STRINGIFY( backBufferDepthIndex ) );
      }
    }

    SetRenderTargets( &backBufferColorIndex, 1, backBufferDepthIndex );
    SetViewport( 0, 0, ( r32 )width, ( r32 )height );
  }

  // set primitive topology -----------------------------------------------
  SetPrimitiveTopology( TacPrimitive::TriangleList );

  // rasterizer state -----------------------------------------------------
  CComPtr< ID3D11RasterizerState > g_pRasterizerState;
  {
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.FrontCounterClockwise = TRUE;
    rasterizerDesc. DepthClipEnable = TRUE;
    mDevice->CreateRasterizerState( &rasterizerDesc, &g_pRasterizerState );
    mDeviceContext->RSSetState( g_pRasterizerState );
  }

  // initialize current arrays
  for(
    u32 iShaderType = 0;
    iShaderType < ( u32 )TacShaderType::Count;
  ++iShaderType )
  {
    mCurrentSamplersDirty[ iShaderType ] = false;
    mCurrentTexturesDirty[ iShaderType ] = false;


    for( u32 iSampler = 0; iSampler < SAMPLERS_PER_SHADER; ++iSampler )
    {
      mCurrentSamplers[ iShaderType ][ iSampler ].mIndex =
        RENDERER_ID_NONE;
    }

    for( u32 iTexture = 0; iTexture < TEXTURES_PER_SHADER; ++iTexture )
    {
      mCurrentTextures[ iShaderType ][ iTexture  ].mIndex =
        RENDERER_ID_NONE;
    }
  }
}

RendererDX11::~RendererDX11()
{
  RemoveDepthBuffer( backBufferDepthIndex );
  RemoveTextureResoure( backBufferColorIndex );

  if( mSwapChain )
    mSwapChain->Release();

  if( mDevice )
    mDevice->Release();

  if( mDeviceContext )
    mDeviceContext->Release();

#if TACDEBUG
  if( infoQueueDEBUG )
    infoQueueDEBUG->Release();

  if( userAnnotationDEBUG )
    userAnnotationDEBUG->Release();

  if( HMODULE Dxgidebughandle = GetModuleHandle( "Dxgidebug.dll" ) )
  {
    HRESULT ( WINAPI* myDXGIGetDebugInterface)(
      REFIID riid,
      void   **ppDebug );

    myDXGIGetDebugInterface =
      ( HRESULT( WINAPI * )( REFIID, void** ) )GetProcAddress(
      Dxgidebughandle,
      "DXGIGetDebugInterface" );

    if( myDXGIGetDebugInterface )
    {
      CComPtr< IDXGIDebug > myIDXGIDebug;
      HRESULT hr = myDXGIGetDebugInterface(
        __uuidof( myIDXGIDebug ),
        ( void** )&myIDXGIDebug );

      if( SUCCEEDED( hr ) )
      {
        OutputDebugStringA("/////////////////////////////////////\n");
        OutputDebugStringA("// Tac reporting live objects...   //\n");
        myIDXGIDebug->ReportLiveObjects(
          DXGI_DEBUG_ALL,
          DXGI_DEBUG_RLO_ALL );
        OutputDebugStringA("// Tac done reporting live objects //\n");
        OutputDebugStringA("/////////////////////////////////////\n");
      }
    }
  }
#endif
}


// vertex buffer --------------------------------------------------------

TacVertexBufferHandle RendererDX11::AddVertexBuffer(
  TacBufferAccess access,
  void* data,
  u32 numVertexes,
  u32 stride,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacVertexBufferHandle result;
  result.mIndex = RENDERER_ID_NONE;

  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = numVertexes * stride;
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  bd.Usage = GetUsage( access );
  if( access == TacBufferAccess::Dynamic )
  {
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; 
  }

  TacVertexBufferDX11 vertexBuffer = {};
  OnDestruct( if( errors.size ) { ReleaseVertexBuffer( vertexBuffer ); } );

  HRESULT hr;
  if( data )
  {
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;
    hr = mDevice->CreateBuffer(
      &bd,
      &initData,
      &vertexBuffer.mHandle );
  }
  else
  {
    hr = mDevice->CreateBuffer(
      &bd,
      nullptr,
      &vertexBuffer.mHandle );
  }

  if( FAILED( hr ) )
  {
    errors = "CreateBuffer failed";
    AppendInfoQueueMessage( hr, errors );
    AppendFileInfo( errors );
    return result;
  }

  vertexBuffer.mNumVertexes = numVertexes;
  vertexBuffer.mStride = stride;

  if( mVertexBufferIndexes.empty() )
  {
    result.mIndex = mVertexBuffers.size();
    mVertexBuffers.push_back( vertexBuffer );
  }
  else
  {
    mVertexBuffers[ result.mIndex = mVertexBufferIndexes.back() ] =
      vertexBuffer;
    mVertexBufferIndexes.pop_back();
  }

  return result;
}

void RendererDX11::RemoveVertexBuffer(
  TacVertexBufferHandle vertexBuffer )
{
  TacAssert( vertexBuffer.mIndex != RENDERER_ID_NONE );
  TacAssertIndex( vertexBuffer.mIndex, mVertexBuffers.size() );
  ReleaseVertexBuffer( mVertexBuffers[ vertexBuffer.mIndex ] );
  mVertexBufferIndexes.push_back( vertexBuffer.mIndex );
}

void RendererDX11::SetVertexBuffers(
  const TacVertexBufferHandle* vertexBufferIDs,
  u32 numRendererIDs )
{
  u32 startSlot = 0;

  const u32 NUM_VBOS = 16;
  ID3D11Buffer* vertexBufferHandles[ NUM_VBOS  ];
  u32 strides[ NUM_VBOS ];// = {};
  u32 offsets[ NUM_VBOS ] = {};
  u32 numToSend = Minimum(
    numRendererIDs,
    ArraySize( vertexBufferHandles ) );
  for( u32 i = 0; i < numToSend; ++i )
  {
    TacVertexBufferHandle curID = vertexBufferIDs[ i ];
    TacAssert( curID.mIndex != RENDERER_ID_NONE );
    TacAssertIndex( ( u32 )curID.mIndex, mVertexBuffers.size() );
    TacVertexBufferDX11& myVertexBufferDX11 = mVertexBuffers[ curID.mIndex ];

    strides[ i ] = myVertexBufferDX11.mStride;
    vertexBufferHandles[ i ] = myVertexBufferDX11.mHandle;
  }

  mCurrentVertexBuffer = vertexBufferIDs[ 0 ];
  mDeviceContext->IASetVertexBuffers(
    startSlot,
    numToSend,
    vertexBufferHandles,
    strides,
    offsets );
}

void RendererDX11::MapVertexBuffer(
  void** data,
  TacMap mapType,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  if( mCurrentVertexBuffer.mIndex == RENDERER_ID_NONE )
  {
    errors = "No vertex buffer set, call Renderer::SetVertexBuffer";
    AppendFileInfo( errors );
    return;
  }

  MapResource(
    data,
    mVertexBuffers[ mCurrentVertexBuffer.mIndex ].mHandle,
    GetD3D11_MAP( mapType ),
    errors );
}

void RendererDX11::UnmapVertexBuffer(
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  if( mCurrentVertexBuffer.mIndex == RENDERER_ID_NONE )
  {
    errors = "No vertex buffer set, call Renderer::SetVertexBuffer";
    AppendFileInfo( errors );
    return;
  }

  mDeviceContext->Unmap(
    mVertexBuffers[ mCurrentVertexBuffer.mIndex ].mHandle,
    0 );
}

void RendererDX11::SetName(
  TacVertexBufferHandle handle,
  const char* name )
{
  mVertexBuffers[ handle.mIndex ].mHandle->SetPrivateData(
    WKPDID_D3DDebugObjectName,
    TacStrLen( name ),
    name );
}

// index buffer -----------------------------------------------------------


TacIndexBufferHandle RendererDX11::AddIndexBuffer(
  TacBufferAccess access,
  void* data,
  u32 numIndexes,
  TacTextureFormat dataType,
  u32 totalBufferSize,
  //TacVariableType variableType,
  //u32 numBytes,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacIndexBufferHandle result;
  result.mIndex = RENDERER_ID_NONE;

  D3D11_BUFFER_DESC bd = {};
  bd.ByteWidth = totalBufferSize;//numIndexes * numBytes;
  bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  bd.Usage = GetUsage( access );
  if( access == TacBufferAccess::Dynamic )
  {
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  }

  TacIndexBufferDX11 indexBuffer = {};
  OnDestruct( if( errors.size ) { ReleaseIndexBuffer( indexBuffer ); } );

  HRESULT hr;
  if( data )
  {
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;
    hr = mDevice->CreateBuffer(
      &bd,
      &initData,
      &indexBuffer.mHandle );
  }
  else
  {
    hr = mDevice->CreateBuffer(
      &bd,
      nullptr,
      &indexBuffer.mHandle );
  }

  if( FAILED( hr ) )
  {
    errors = "CreateBuffer failed\n";
    AppendInfoQueueMessage( hr, errors );
    AppendFileInfo( errors );
    return result;
  }

  //indexBuffer.mVariableType = variableType;
  //indexBuffer.mNumBytes = numBytes;
  indexBuffer.mIndexFormat = dataType;
  indexBuffer.mNumIndexes = numIndexes;

  if( mIndexBufferIndexes.empty() )
  {
    result.mIndex = mIndexBuffers.size();
    mIndexBuffers.push_back( indexBuffer );
  }
  else
  {
    mIndexBuffers[ result.mIndex = mIndexBufferIndexes.back() ] =
      indexBuffer;
    mIndexBufferIndexes.pop_back();
  }

  return result;
}

void RendererDX11::RemoveIndexBuffer(
  TacIndexBufferHandle indexBuffer )
{
  TacAssert( indexBuffer.mIndex != RENDERER_ID_NONE );
  TacAssertIndex( indexBuffer.mIndex, mIndexBuffers.size() );
  ReleaseIndexBuffer( mIndexBuffers[ indexBuffer.mIndex ] );
  mIndexBufferIndexes.push_back( indexBuffer.mIndex );
}

void RendererDX11::SetIndexBuffer(
  TacIndexBufferHandle indexBuffer )
{
  if( mCurrentIndexBuffer.mIndex == indexBuffer.mIndex )
  {
    return;
  }

  mCurrentIndexBuffer = indexBuffer;

  if( mCurrentIndexBuffer.mIndex != RENDERER_ID_NONE )
  {
    TacIndexBufferDX11& indexBufferDX11 =
      mIndexBuffers[ mCurrentIndexBuffer.mIndex ];
    mDeviceContext->IASetIndexBuffer(
      indexBufferDX11.mHandle,
      GetDXGIFormat( indexBufferDX11.mIndexFormat ),// GetDXGIFormat( indexBuffer.mVariableType, indexBuffer.mNumBytes, 1 ),
      0 );
  }
  else
  {
    mDeviceContext->IASetIndexBuffer(
      nullptr, 
      DXGI_FORMAT_UNKNOWN,
      0 );
  }
}

void RendererDX11::MapIndexBuffer(
  void** data,
  TacMap mapType,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  if( mCurrentIndexBuffer.mIndex == RENDERER_ID_NONE )
  {
    errors = "No index buffer set, call Renderer::SetIndexBuffer";
    AppendFileInfo( errors );
    return;
  }

  MapResource(
    data,
    mIndexBuffers[ mCurrentIndexBuffer.mIndex ].mHandle,
    GetD3D11_MAP( mapType ),
    errors );
}

void RendererDX11::UnmapIndexBuffer(
  FixedString< DEFAULT_ERR_LEN >& errors )
{

  if( mCurrentIndexBuffer.mIndex == RENDERER_ID_NONE )
  {
    errors = "No index buffer set, call Renderer::SetIndexBuffer";
    AppendFileInfo( errors );
    return;
  }
  mDeviceContext->Unmap(
    mIndexBuffers[ mCurrentIndexBuffer.mIndex ].mHandle,
    0 );
}

void RendererDX11::SetName(
  TacIndexBufferHandle handle,
  const char* name )
{
  mIndexBuffers[ handle.mIndex ].mHandle->SetPrivateData(
    WKPDID_D3DDebugObjectName,
    TacStrLen( name ),
    name );
}
// clear ----------------------------------------------------------------

void RendererDX11::ClearColor(
  TacTextureHandle textureHandle,
  v4 rgba )
{
  TacAssertIndex( textureHandle.mIndex, mTextures.size() );
  TacTextureDX11& textureDX11 = mTextures[ textureHandle.mIndex ];
  TacAssert( textureDX11.mRTV );
  mDeviceContext->ClearRenderTargetView(
    textureDX11.mRTV,
    &rgba.x );
}

void RendererDX11::ClearDepthStencil(
  TacDepthBufferHandle depthBufferHandle,
  b32 clearDepth,
  r32 depth,
  b32 clearStencil,
  u8 stencil )
{
  UINT clearFlags = 0;
  if( clearDepth ) clearFlags |= D3D11_CLEAR_DEPTH;
  if( clearStencil ) clearFlags |= D3D11_CLEAR_STENCIL;

  TacAssertIndex( depthBufferHandle.mIndex, mDepthBuffers.size() );
  TacDepthBufferDX11& depthBufferDX11 =
    mDepthBuffers[ depthBufferHandle.mIndex ];
  TacAssert( depthBufferDX11.mDSV );
  mDeviceContext->ClearDepthStencilView(
    depthBufferDX11.mDSV, clearFlags, depth, stencil );
}


// Shader ---------------------------------------------------------------

TacShaderHandle RendererDX11::LoadShader(
  const char* shaderPaths[ ( u32 )TacShaderType::Count ],
  const char* entryPoints[ ( u32 )TacShaderType::Count ],
  const char* shaderModels[ ( u32 )TacShaderType::Count ],
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacShaderHandle result;
  result.mIndex = RENDERER_ID_NONE;

  TacShaderDX11 newShader = {};
  OnDestruct( if( errors.size ) { ReleaseShader( newShader ); } );

  for( u32 i = 0; i < ( u32 )TacShaderType::Count; ++i )
  {
    PerShaderData& myPerShaderData =
      newShader.mPerShaderData[ i ];
    myPerShaderData.mShaderPath = shaderPaths[ i ];
    myPerShaderData.mShaderModel = shaderModels[ i ];
    myPerShaderData.mEntryPoint = entryPoints[ i ];
  }

  while( true )
  {
    ReloadShader(
      newShader,
      errors );
    if( errors.size )
    {
#if TACDEBUG
      DisplayError( errors.buffer, __LINE__, __FILE__, __FUNCTION__ );
      //__debugbreak();
      errors = "";
      continue;
#else
      return result;
#endif
    }
    else
    {
      break;
    }
  }

  if( mShaderIndexes.empty() )
  {
    result.mIndex = mShaders.size();
    mShaders.push_back( newShader );
  }
  else
  {
    mShaders[ result.mIndex = mShaderIndexes.back() ] = newShader;
    mShaderIndexes.pop_back();
  }

  // set debug names


  return result;
}

void RendererDX11::RemoveShader( TacShaderHandle shader )
{
  ReleaseShader( mShaders[ shader.mIndex ] );
  mShaderIndexes.push_back( shader.mIndex );
}

void RendererDX11::SetActiveShader( TacShaderHandle shaderID )
{
  if( shaderID.mIndex != mCurrentShader.mIndex )
  {
    if( ( mCurrentShader = shaderID ).mIndex == RENDERER_ID_NONE )
    {
      mDeviceContext->VSSetShader( nullptr, nullptr, 0 );
      mDeviceContext->GSSetShader( nullptr, nullptr, 0 );
      mDeviceContext->PSSetShader( nullptr, nullptr, 0 );
      //g_pImmediateContext->IASetInputLayout( nullptr );
    }
    else
    {
      TacShaderDX11& shader = mShaders[ mCurrentShader.mIndex ];
      mDeviceContext->VSSetShader( shader.mVertexShader, nullptr, 0 );
      mDeviceContext->GSSetShader( nullptr, nullptr, 0 );
      mDeviceContext->PSSetShader( shader.mPixelShader, nullptr, 0 );
      //g_pImmediateContext->IASetInputLayout( shader.vertexLayout );

      // set vertex constant buffers

      //ID3D11Buffer* cbuffers[ 10 ];
      //auto CopyCBuffers = [](
      //  TacCBufferDX11* allcbufferdx11s,
      //  ID3D11Buffer** buffers,
      //  u32 bufferArraysize,
      //  TacCBufferHandle* cbufferHandles,
      //  u32 numcbufferHandles )
      //{
      //  TacAssert( numcbufferHandles < bufferArraysize );
      //  for( u32 i = 0; i < numcbufferHandles; ++i )
      //  {
      //    TacCBufferHandle id = cbufferHandles[ i ];
      //    TacCBufferDX11& cbuffer = allcbufferdx11s[ id.mIndex ];
      //    buffers[ i ] = cbuffer.mHandle;
      //  }
      //};

      typedef decltype( &ID3D11DeviceContext::VSSetConstantBuffers ) SetCBufferType;
      SetCBufferType pointers[ 3 ];
      pointers[ ( u32 )TacShaderType::Vertex ] = &ID3D11DeviceContext::VSSetConstantBuffers;
      pointers[ ( u32 )TacShaderType::Fragment ] = &ID3D11DeviceContext::PSSetConstantBuffers;

      for(
        u32 iShaderType = 0;
        iShaderType < ( u32 )TacShaderType::Count;
      ++iShaderType )
      {
        PerShaderData* shaderData = &shader.mPerShaderData[ iShaderType ];

        for( u32 i = 0; i < shaderData->mNumCBufferBindings; ++i )
        {
          PerShaderData::CBufferBinding& bufferBinding =
            shaderData->mCBufferBindings[ i ];

          TacCBufferDX11& bufferdx11 =
            mCbuffers[ bufferBinding.mCBufferHandle.mIndex ];

          SetCBufferType pointer = pointers [ iShaderType ];
          ( mDeviceContext->*pointer )(
            bufferBinding.mRegister,
            1,
            &bufferdx11.mHandle );
        }
      }
    }
  }
}

TacShaderHandle RendererDX11::GetCurrentlyBoundShader()
{
  return mCurrentShader;
}

void RendererDX11::ReloadShader( TacShaderHandle shader )
{
  TacAssert( shader.mIndex );
  TacAssertIndex( shader.mIndex, mShaders.size() );
  TacShaderDX11& shaderDX11 = mShaders[ shader.mIndex ];

  // free previous resources, but keep our data
  TacShaderDX11 copy = shaderDX11;
  copy.mVertexShader = nullptr;
  copy.mPixelShader = nullptr;
  ReleaseShader( shaderDX11 );
  shaderDX11 = copy;

  FixedString< DEFAULT_ERR_LEN > errors;
  do
  {
    errors = "";
    ReloadShader(
      shaderDX11,
      errors );
    if( errors.size )
    {
      DisplayError( errors.buffer, __LINE__, __FILE__, __FUNCTION__ );
    }
  } while( errors.size );

}

TacShaderHandle RendererDX11::LoadShaderFromString(
  const char* shaderStr[ ( u32 )TacShaderType::Count ],
  const char* entryPoints[ ( u32 )TacShaderType::Count ],
  const char* shaderModels[ ( u32 )TacShaderType::Count ],
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacShaderHandle result;
  result.mIndex = RENDERER_ID_NONE;

  TacShaderDX11 newShader = { 0 };
  OnDestruct( if( errors.size ) { ReleaseShader( newShader ); } );

  for( u32 i = 0; i < ( u32 )TacShaderType::Count; ++i )
  {
    if( !shaderStr[ i ] )
      continue;

    PerShaderData& myPerShaderData =
      newShader.mPerShaderData[ i ];
    myPerShaderData.mShaderModel = shaderModels[ i ];
    myPerShaderData.mEntryPoint = entryPoints[ i ];
  }

  ReloadShaderFromString(
    newShader,
    shaderStr,
    errors );
  if( errors.size )
  {
    return result;
  }

  if( mShaderIndexes.empty() )
  {
    result.mIndex = mShaders.size();
    mShaders.push_back( newShader );
  }
  else
  {
    mShaders[ result.mIndex = mShaderIndexes.back() ] = newShader;
    mShaderIndexes.pop_back();
  }

  return result;
}

void RendererDX11::SetName(
  TacShaderHandle handle,
  const char* name )
{
  TacShaderDX11& newShader = mShaders[ handle.mIndex ];

  ID3D11DeviceChild* children[ ( u32 )TacShaderType::Count ] =
  {
    newShader.mVertexShader,
    newShader.mPixelShader,
  };
  for( u32 i = 0; i < ( u32 )TacShaderType::Count; ++i )
  {
    ID3D11DeviceChild* child = children[ i ];
    if( !child )
      continue;

    child->SetPrivateData(
      WKPDID_D3DDebugObjectName,
      TacStrLen( name ),
      name );
  }
}

// non virtual ---

void RendererDX11::ReloadShader(
  TacShaderDX11& shader,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  PerShaderData& vertexData =
    shader.mPerShaderData[ ( u32 )TacShaderType::Vertex ];

  if( vertexData.mShaderPath.size )
  {
    CComPtr< ID3DBlob > pVSBlob;
    TacCompileShaderFromFile(
      vertexData.mShaderPath.buffer,
      vertexData.mEntryPoint.buffer,
      vertexData.mShaderModel.buffer,
      &pVSBlob,
      errors );
    if( errors.size )
      return;

    HRESULT hr = mDevice->CreateVertexShader( 
      pVSBlob->GetBufferPointer(),
      pVSBlob->GetBufferSize(),
      nullptr,
      &shader.mVertexShader );
    if( FAILED( hr ) )
    {
      errors = "CreateVertexShader failed";
      return;
    }

    hr = D3DGetInputSignatureBlob(
      pVSBlob->GetBufferPointer(),
      pVSBlob->GetBufferSize(),
      &shader.mInputSig );
    TacAssert( SUCCEEDED( hr ) );
  }

  PerShaderData& fragmentData =
    shader.mPerShaderData[ ( u32 )TacShaderType::Fragment ];

  if( fragmentData.mShaderPath.size )
  {
    CComPtr< ID3DBlob > pPSBlob;
    TacCompileShaderFromFile(
      fragmentData.mShaderPath.buffer,
      fragmentData.mEntryPoint.buffer,
      fragmentData.mShaderModel.buffer,
      &pPSBlob,
      errors );
    if( errors.size )
      return;

    HRESULT hr = mDevice->CreatePixelShader(
      pPSBlob->GetBufferPointer(),
      pPSBlob->GetBufferSize(),
      nullptr,
      &shader.mPixelShader );
    if( FAILED( hr ) )
    {
      errors = "CreatePixelShader failed";
      return;
    }
  }
}

void RendererDX11::ReloadShaderFromString(
  TacShaderDX11& shader,
  const char* shaderStrings[ ( u32 )TacShaderType::Count ],
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  if( shaderStrings[ ( u32 )TacShaderType::Vertex ] &&
    *shaderStrings[ ( u32 )TacShaderType::Vertex ] )
  {
    PerShaderData& vertexData =
      shader.mPerShaderData[ ( u32 )TacShaderType::Vertex ];

    CComPtr< ID3DBlob > pVSBlob;

    TacCompileShaderFromString(
      &pVSBlob,
      ( void* )shaderStrings[ ( u32 )TacShaderType::Vertex ],
      TacStrLen( shaderStrings[ ( u32 )TacShaderType::Vertex ] ),
      vertexData.mEntryPoint.buffer,
      vertexData.mShaderModel.buffer,
      errors );
    if( errors.size )
      return;

    HRESULT hr = mDevice->CreateVertexShader( 
      pVSBlob->GetBufferPointer(),
      pVSBlob->GetBufferSize(),
      nullptr,
      &shader.mVertexShader );
    if( FAILED( hr ) )
    {
      errors = "CreateVertexShader failed";
      return;
    }

    hr = D3DGetInputSignatureBlob(
      pVSBlob->GetBufferPointer(),
      pVSBlob->GetBufferSize(),
      &shader.mInputSig );
    TacAssert( SUCCEEDED( hr ) );
  }

  if( shaderStrings[ ( u32 )TacShaderType::Fragment ] &&
    *shaderStrings[ ( u32 )TacShaderType::Fragment ] )
  {
    PerShaderData& fragmentData =
      shader.mPerShaderData[ ( u32 )TacShaderType::Fragment ];
    CComPtr< ID3DBlob > pPSBlob;
    TacCompileShaderFromString(
      &pPSBlob,
      ( void* )shaderStrings[ ( u32 )TacShaderType::Fragment ],
      TacStrLen( shaderStrings[ ( u32 )TacShaderType::Fragment ] ),
      fragmentData.mEntryPoint.buffer,
      fragmentData.mShaderModel.buffer,
      errors );
    if( errors.size )
      return;

    HRESULT hr = mDevice->CreatePixelShader(
      pPSBlob->GetBufferPointer(),
      pPSBlob->GetBufferSize(),
      nullptr,
      &shader.mPixelShader );
    if( FAILED( hr ) )
    {
      errors = "CreatePixelShader failed";
      return;
    }
  }
}

// sampler state --------------------------------------------------------

TacSamplerStateHandle RendererDX11::AddSamplerState(
  const TacAddressMode u,
  const TacAddressMode v,
  const TacAddressMode w,
  TacComparison compare,
  TacFilter filter,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacSamplerStateHandle id;
  id.mIndex = RENDERER_ID_NONE;

  auto GetAddressMode = []( TacAddressMode mode )
  {
    switch( mode )
    {
    case TacAddressMode::Wrap:
      return D3D11_TEXTURE_ADDRESS_WRAP;
      break;
    case TacAddressMode::Clamp:
      return D3D11_TEXTURE_ADDRESS_CLAMP;
      break;
    case TacAddressMode::Border:
      return D3D11_TEXTURE_ADDRESS_BORDER;
      break;
      TacInvalidDefaultCase;
    }
    return D3D11_TEXTURE_ADDRESS_WRAP;
  };

  auto GetCompare = []( TacComparison compare )
  {
    switch( compare )
    {
    case TacComparison::Always: return D3D11_COMPARISON_ALWAYS;
    case TacComparison::Never: return D3D11_COMPARISON_NEVER;
      TacInvalidDefaultCase;
    }
    return D3D11_COMPARISON_ALWAYS;
  };

  auto GetFilter = []( TacFilter filter )
  {
    switch( filter )
    {
    case TacFilter::Linear:
      return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    case TacFilter::Point:
      return D3D11_FILTER_MIN_MAG_MIP_POINT;
      TacInvalidDefaultCase;
    }
    return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  };

  D3D11_SAMPLER_DESC desc = {};
  desc.Filter = GetFilter( filter );
  desc.AddressU = GetAddressMode( u );
  desc.AddressV = GetAddressMode( v );
  desc.AddressW = GetAddressMode( w );
  desc.ComparisonFunc = GetCompare( compare );
  desc.BorderColor[ 0 ] = 1;
  desc.BorderColor[ 1 ] = 0;
  desc.BorderColor[ 2 ] = 0;
  desc.BorderColor[ 3 ] = 1;

  TacSamplerStateDX11 samplerState = {};
  OnDestruct( if( errors.size ) { ReleaseSamplerState( samplerState ); } );

  HRESULT hr = mDevice->CreateSamplerState(
    &desc,
    &samplerState.mHandle );
  if( FAILED( hr ) )
  {
    errors = "ID3D11Device::CreateSamplerState failed\n";
    AppendInfoQueueMessage( hr, errors );
    AppendFileInfo( errors );
    return id;
  }

  if( mSamplerStateIndexes.empty() )
  {
    id.mIndex = mSamplerStates.size();
    mSamplerStates.push_back( samplerState );
  }
  else
  {
    mSamplerStates[ id.mIndex = mSamplerStateIndexes.back() ] =
      samplerState;
    mSamplerStateIndexes.pop_back();
  }

  return id;
}

void RendererDX11::RemoveSamplerState(
  TacSamplerStateHandle samplerState )
{
  ReleaseSamplerState( mSamplerStates[ samplerState.mIndex ] );
  mSamplerStateIndexes.push_back( samplerState.mIndex );
}

void RendererDX11::AddSampler(
  const char* name,
  TacShaderHandle shaderHandle,
  TacShaderType shaderType,
  u32 samplerIndex,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacAssert( shaderHandle.mIndex != RENDERER_ID_NONE );

  TacShaderDX11& shaderDX11 = mShaders[ shaderHandle.mIndex ];
  if( shaderDX11.mNumSamplers >= ArraySize( shaderDX11.mSamplers ) )
  {
    errors = "Not enough samplers";
    return;
  }

  TacSampler& mySampler = shaderDX11.mSamplers[ shaderDX11.mNumSamplers++ ];
  mySampler.mName = name;
  mySampler.mSamplerIndex = samplerIndex;
  mySampler.mShaderType = shaderType;
}

void RendererDX11::SetSamplerState(
  const char* samplerName,
  TacSamplerStateHandle samplerState )
{
  TacAssert( mCurrentShader.mIndex != RENDERER_ID_NONE );

  // get the sampler from the shader
  TacShaderDX11& shader = mShaders[ mCurrentShader.mIndex ];
  TacSampler* foundSampler = GetSampler(
    samplerName,
    shader.mSamplers,
    shader.mNumSamplers );
  TacAssert( foundSampler );

  u32 iShaderType = ( u32 )foundSampler->mShaderType;
  mCurrentSamplers[ iShaderType ][ foundSampler->mSamplerIndex ] =
    samplerState;
  mCurrentSamplersDirty[ iShaderType ] = true;
}


void RendererDX11::SetName(
  TacSamplerStateHandle handle,
  const char* name )
{
  mSamplerStates[ handle.mIndex ].mHandle->SetPrivateData(
    WKPDID_D3DDebugObjectName,
    TacStrLen( name ),
    name );
}


// texture --------------------------------------------------------------
TacTextureHandle RendererDX11::AddTextureResource(
  const TacImage& myImage,
  TacTextureUsage textureUsage,
  TacBinding binding,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacTextureHandle textureHandle;
  textureHandle.mIndex = RENDERER_ID_NONE;

  TacTextureDX11 textureDX11 = {};
  textureDX11.mWidth = myImage.mWidth;
  textureDX11.mHeight = myImage.mHeight;
  textureDX11.mDepth = myImage.mDepth;

  OnDestruct( if( errors.size ) { ReleaseTexture(textureDX11 ); } );

  CreateTexture(
    myImage,
    &textureDX11.mHandle,
    textureUsage,
    binding,
    errors );
  if( errors.size )
    return textureHandle;

  if( binding & TacBinding::RenderTarget )
  {
    HRESULT hr = mDevice->CreateRenderTargetView(
      textureDX11.mHandle,
      nullptr,
      &textureDX11.mRTV );
    if( FAILED( hr ) )
    {
      errors = "ID3D11Device::CreateRenderTargetView failed\n";
      AppendInfoQueueMessage( hr, errors );
      AppendFileInfo( errors );
      return textureHandle;
    }
  }

  if( binding & TacBinding::ShaderResource )
  {
    CreateShaderResourceView(
      textureDX11.mHandle,
      &textureDX11.mSrv,
      errors );
    if( errors.size )
      return textureHandle;
  }

  if( mTextureIndexes.empty() )
  {
    textureHandle.mIndex = mTextures.size();
    mTextures.push_back( textureDX11 );
  }
  else
  {
    mTextures[ textureHandle.mIndex = mTextureIndexes.back() ] = textureDX11;
    mTextureIndexes.pop_back();
  }

  return textureHandle;
}

void RendererDX11::RemoveTextureResoure( TacTextureHandle texture )
{
  ReleaseTexture( mTextures[ texture.mIndex ] );
  mTextureIndexes.push_back( texture.mIndex );
}

void RendererDX11::AddTexture(
  const char* name,
  TacShaderHandle shaderID,
  TacShaderType shaderType,
  u32 samplerIndex,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacAssert( shaderID.mIndex != RENDERER_ID_NONE );
  TacShaderDX11& shader = mShaders[ shaderID.mIndex ];


#if TACDEBUG
  for( u32 i = 0; i < shader.mNumTextures; ++i )
  {
    TacSampler& sampler = shader.mTextures[ i ]; 

    // make sure we don't overwrite a texture
    if( sampler.mShaderType == shaderType )
    {
      TacAssert( sampler.mSamplerIndex != samplerIndex );
    }
  }
#endif



  if( shader.mNumTextures >= ArraySize( shader.mTextures ) )
  {
    errors = "Not enough textures";
    return;
  }

  TacSampler& mySampler = shader.mTextures[ shader.mNumTextures++ ];
  mySampler.mName = name;
  mySampler.mSamplerIndex = samplerIndex;
  mySampler.mShaderType = shaderType;
}

void RendererDX11::SetTexture(
  const char* textureName,
  TacTextureHandle texture )
{
  TacAssert( mCurrentShader.mIndex != RENDERER_ID_NONE );

  // get the sampler from the shader
  TacShaderDX11& shader = mShaders[ mCurrentShader.mIndex ];
  TacSampler* foundSampler = GetSampler(
    textureName,
    shader.mTextures,
    shader.mNumTextures );
  TacAssert( foundSampler );

  u32 shaderTypeIndex =  ( u32 )foundSampler->mShaderType ;
  mCurrentTextures[ shaderTypeIndex ][ foundSampler->mSamplerIndex ] =
    texture;
  mCurrentTexturesDirty[ shaderTypeIndex ] = true;
}

void RendererDX11::SetName(
  TacTextureHandle handle,
  const char* name )
{
  TacTextureDX11& textureDX11 = mTextures[ handle.mIndex ]; 
  ID3D11DeviceChild* children[] =
  {
    textureDX11.mHandle,
    textureDX11.mRTV,
    textureDX11.mSrv,
  };

  for( ID3D11DeviceChild* child : children )
  {
    if( !child )
      continue;

    child->SetPrivateData(
      WKPDID_D3DDebugObjectName,
      TacStrLen( name ),
      name );
  }
}

// non-virtual ---

void RendererDX11::CreateTexture(
  const TacImage& myImage,
  ID3D11Resource** resource,
  TacTextureUsage textureUsage,
  TacBinding binding,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  DXGI_FORMAT format = GetDXGIFormat(
    myImage.mTextureFormat );
  //myImage.variableType,
  //myImage.variableBytesPerComponent,
  //myImage.variableNumComponents );

  D3D11_USAGE Usage = D3D11_USAGE_DEFAULT;
  switch( textureUsage )
  {
  case TacTextureUsage::Default: Usage = D3D11_USAGE_DEFAULT; break;
  case TacTextureUsage::Immutable: Usage = D3D11_USAGE_IMMUTABLE; break;
    TacInvalidDefaultCase;
  }
  UINT BindFlags = 0;
  if( binding & TacBinding::RenderTarget )
  {
    BindFlags |= D3D11_BIND_RENDER_TARGET;
  }
  if( binding & TacBinding::ShaderResource )
  {
    BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  }


  if( myImage.textureDimension == 1 )
  {
    D3D11_TEXTURE1D_DESC texDesc = {};
    texDesc.Width = myImage.mWidth;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = format;
    texDesc.Usage = Usage;//D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = BindFlags;//D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = myImage.mData;
    subResource.SysMemPitch = myImage.mPitch;

    HRESULT hr = mDevice->CreateTexture1D(
      &texDesc,
      &subResource,
      ( ID3D11Texture1D** )resource );

    if( FAILED( hr ) )
    {
      errors = "ID3D11Device::CreateTexture1D failed\n";
      AppendFileInfo( errors );
      AppendInfoQueueMessage( hr, errors );
      return;
    }
  }
  else if( myImage.textureDimension == 2 )
  {
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = myImage.mWidth;
    texDesc.Height = myImage.mHeight;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = format;
    texDesc.Usage = Usage;//D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = BindFlags;//D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = myImage.mData;
    subResource.SysMemPitch = myImage.mPitch;

    D3D11_SUBRESOURCE_DATA* pSubResource =
      myImage.mData ? &subResource : nullptr;

    HRESULT hr = mDevice->CreateTexture2D(
      &texDesc,
      pSubResource,
      ( ID3D11Texture2D** )resource );

    if( FAILED( hr ) )
    {
      errors = "ID3D11Device::CreateTexture2D failed\n";
      AppendFileInfo( errors );
      AppendInfoQueueMessage( hr, errors );
      return;
    }
  }
  else if( myImage.textureDimension == 3 )
  {
    D3D11_TEXTURE3D_DESC texDesc = {};
    texDesc.Width = myImage.mWidth;
    texDesc.Height = myImage.mHeight;
    texDesc.Depth = myImage.mDepth;
    texDesc.MipLevels = 1;
    texDesc.Format = format;
    texDesc.Usage = Usage;//D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = BindFlags;//D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = myImage.mData;
    subResource.SysMemPitch = myImage.mPitch;

    HRESULT hr = mDevice->CreateTexture3D(
      &texDesc,
      &subResource,
      ( ID3D11Texture3D** )resource );

    if( FAILED( hr ) )
    {
      errors = "ID3D11Device::CreateTexture3D failed\n";
      AppendFileInfo( errors );
      AppendInfoQueueMessage( hr, errors );
      return;
    }
  }
  else
  {
    TacInvalidCodePath;
  }
}

void RendererDX11::CreateShaderResourceView(
  ID3D11Resource* resource,
  ID3D11ShaderResourceView** srv,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  D3D11_RESOURCE_DIMENSION type;
  resource->GetType( &type );

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  switch( type )
  {
  case D3D11_RESOURCE_DIMENSION_UNKNOWN:
    {
      TacAssert( false );
    } break;
  case D3D11_RESOURCE_DIMENSION_BUFFER:
    {
      TacAssert( false );
    } break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
    {
      TacAssert( false );
    } break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
    {
      D3D11_TEXTURE2D_DESC desc2d;
      ( ( ID3D11Texture2D* )resource )->GetDesc( &desc2d );
      srvDesc.Format = desc2d.Format;
      srvDesc.ViewDimension = desc2d.SampleDesc.Count > 1
        ? D3D11_SRV_DIMENSION_TEXTURE2DMS
        : D3D11_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MipLevels = desc2d.MipLevels;
    } break;
  case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
    {
      TacAssert( false );
    } break;
  default:
    {
      TacAssert( false );
    } break;
  }

  HRESULT hr = mDevice->CreateShaderResourceView(
    resource,
    &srvDesc,
    srv );
  if( FAILED( hr ) )
  {
    errors = "ID3D11Device::CreateShaderResourceView failed\n";
    AppendInfoQueueMessage( hr, errors );
    AppendFileInfo( errors );
    return;
  }
}

// render targets -------------------------------------------------------
TacDepthBufferHandle RendererDX11::AddDepthBuffer(
  u32 width,
  u32 height,
  TacTextureFormat textureFormat,
  //u32 numDepthBits,
  //u32 numStencilBits,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacDepthBufferHandle result = {};

  TacDepthBufferDX11 depthBufferDX11 = {};
  depthBufferDX11.mWidth = width;
  depthBufferDX11.mHeight = height;
  depthBufferDX11.textureFormat = textureFormat;
  //depthBufferDX11.numDepthBits = numDepthBits;
  //depthBufferDX11.numStencilBits = numStencilBits;

  OnDestruct( if( errors.size ) { ReleaseDepthBuffer(depthBufferDX11 ); } );

  //TacAssert( numDepthBits == 24 );
  //TacAssert( numStencilBits == 8 );

  D3D11_TEXTURE2D_DESC texture2dDesc = {};
  texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  texture2dDesc.Format = 
    GetDXGIFormat( textureFormat );
  //DXGI_FORMAT_D24_UNORM_S8_UINT;
  texture2dDesc.Height = height;
  texture2dDesc.Width = width;
  texture2dDesc.SampleDesc.Count = 1;
  texture2dDesc.SampleDesc.Quality = 0;
  texture2dDesc.ArraySize = 1;
  texture2dDesc.MipLevels = 1;

  HRESULT hr = mDevice->CreateTexture2D(
    &texture2dDesc,
    nullptr, 
    ( ID3D11Texture2D** )&depthBufferDX11.mHandle );
  if( FAILED( hr ) )
  {
    errors = "Create Depth Buffer Texture failed";
    AppendInfoQueueMessage( hr, errors );
    AppendFileInfo( errors );
    return result;
  }


  D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
  desc.Format = texture2dDesc.Format;
  desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  hr = mDevice->CreateDepthStencilView(
    depthBufferDX11.mHandle,
    &desc,
    &depthBufferDX11.mDSV );
  if( FAILED( hr ) )
  {
    errors = "Create Depth Buffer depth stencil view failed";
    AppendInfoQueueMessage( hr, errors );
    AppendFileInfo( errors );
    return result;
  }

  if( !mDepthBufferIndexes.empty() )
  {
    mDepthBuffers[ result.mIndex = mDepthBufferIndexes.back() ] =
      depthBufferDX11;
    mDepthBufferIndexes.pop_back();
  }
  else
  {
    result.mIndex = mDepthBuffers.size();
    mDepthBuffers.push_back( depthBufferDX11 );
  }

  return result;
}

void RendererDX11::RemoveDepthBuffer( TacDepthBufferHandle depthBuffer )
{
  ReleaseDepthBuffer( mDepthBuffers[ depthBuffer.mIndex ] );
  mDepthBufferIndexes.push_back( depthBuffer.mIndex );
}

void RendererDX11::SetRenderTargets(
  TacTextureHandle* renderTargets,
  u32 numRenderTargets,
  TacDepthBufferHandle depthBuffer )
{
  const u32 maxRenderTargets = 16;
  TacAssertIndex( numRenderTargets, maxRenderTargets );
  ID3D11RenderTargetView* renderTargetViews[ maxRenderTargets ];

  for( u32 i = 0; i < numRenderTargets; ++i )
  {
    u32 iTexture = renderTargets[ i ].mIndex;
    TacAssertIndex( iTexture, mTextures.size() );
    TacAssert( iTexture != RENDERER_ID_NONE );
    TacTextureDX11& textureDX11 = mTextures[ iTexture ];
    renderTargetViews[ i ] = textureDX11.mRTV;
  }

  ID3D11DepthStencilView *pDepthStencilView = nullptr;
  if( depthBuffer.mIndex != RENDERER_ID_NONE )
  {
    pDepthStencilView = mDepthBuffers[ depthBuffer.mIndex ].mDSV;
  }

  mDeviceContext->OMSetRenderTargets(
    numRenderTargets,
    renderTargetViews,
    pDepthStencilView );
}

void RendererDX11::SetName(
  TacDepthBufferHandle handle,
  const char* name )
{
  TacDepthBufferDX11& depthBufferDX11 = mDepthBuffers[ handle.mIndex ];
  ID3D11DeviceChild* children[] = 
  {
    depthBufferDX11.mDSV,
    depthBufferDX11.mHandle,
  };
  for( ID3D11DeviceChild* child : children )
  {
    if( !child )
      continue;

    child->SetPrivateData(
      WKPDID_D3DDebugObjectName,
      TacStrLen( name ),
      name );
  }
}

TacTextureHandle RendererDX11::GetBackbufferColor()
{
  return backBufferColorIndex;
}

TacDepthBufferHandle RendererDX11::GetBackbufferDepth()
{
  return backBufferDepthIndex;
}

// cbuffer --------------------------------------------------------------

TacCBufferHandle RendererDX11::LoadCbuffer(
  const char* name,
  TacConstant* constants,
  u32 numConstants,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacCBufferHandle cBufferHandle;
  cBufferHandle.mIndex = RENDERER_ID_NONE;


  // check that there is enough cpu memory to hold the constants
  u32 spaceNeeded = 0;
  {
    for( u32 i = 0; i < numConstants; ++i )
    {
      TacConstant* c = &constants[ i ];
      u32 endPoint = c->offset + c->size;
      if( endPoint > spaceNeeded )
      {
        spaceNeeded = endPoint;
      }
    }

    // constant buffers must be a multiple of 16
    spaceNeeded = RoundUpToNearestMultiple( spaceNeeded, 16 );

    if( spaceNeeded > SizeOfMember( TacCBufferDX11, mMemory ) )
    {
      errors = "Not enough cpu cbuffer memory";
      AppendFileInfo( errors );
      return cBufferHandle;
    }
  }

  ID3D11Buffer* cbufferhandle;

  D3D11_BUFFER_DESC bd = {};
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = spaceNeeded;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bd.CPUAccessFlags = 0;
  HRESULT hr = mDevice->CreateBuffer( &bd, nullptr, &cbufferhandle );
  if( FAILED( hr ) )
  {
    errors = "CreateBuffer failed";
    AppendInfoQueueMessage( hr, errors );
    AppendFileInfo( errors );
    return cBufferHandle;
  }

  TacCBufferDX11 cBufferDX11 = {};
  for( u32 i = 0; i < numConstants; ++i )
  {
    cBufferDX11.mConstants[ i ] = constants[ i ];
  }
  cBufferDX11.mDirty = false;
  cBufferDX11.mHandle = cbufferhandle;
  cBufferDX11.mName = name;
  cBufferDX11.mNumConstants = numConstants;
  cBufferDX11.mSize = spaceNeeded;

  if( mCbufferIndexes.empty() )
  {
    cBufferHandle.mIndex = mCbuffers.size();
    mCbuffers.push_back( cBufferDX11 );
  }
  else
  {
    mCbuffers[ cBufferHandle.mIndex = mCbufferIndexes.back() ] = cBufferDX11;
    mCbufferIndexes.pop_back();
  }

  return cBufferHandle;
}

void RendererDX11::RemoveCbuffer( TacCBufferHandle cbuffer )
{
  TacCBufferDX11& cBufferDX11 = 
    mCbuffers[ cbuffer.mIndex ];

  cBufferDX11.mHandle->Release();
  mCbufferIndexes.push_back( cbuffer.mIndex );

  TacCBufferDX11 empty = {};
  cBufferDX11 = empty;
}

void RendererDX11::AddCbuffer( 
  TacShaderHandle shaderID, 
  TacCBufferHandle cbufferID,
  u32 cbufferRegister,
  TacShaderType myShaderType,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacAssert( shaderID.mIndex != RENDERER_ID_NONE );
  TacAssert( cbufferID.mIndex != RENDERER_ID_NONE );

  TacShaderDX11& shader = mShaders[ shaderID.mIndex ];
  TacCBufferDX11& cbuffer = mCbuffers[ cbufferID.mIndex ];

  PerShaderData& myPerShaderData = shader.mPerShaderData[ ( u32 )myShaderType ];
  if( myPerShaderData.mNumCBufferBindings >
    ArraySize( myPerShaderData.mCBufferBindings ) )
  {
    errors = "Not enough cbuffers";
    AppendFileInfo( errors );
    return;
  }

  PerShaderData::CBufferBinding& bufferBinding =
    myPerShaderData.mCBufferBindings
    [
      myPerShaderData.mNumCBufferBindings++
    ];
  bufferBinding.mCBufferHandle = cbufferID;
  bufferBinding.mRegister = cbufferRegister;




  for( u32 iConstant = 0; iConstant < cbuffer.mNumConstants; ++iConstant )
  {
    TacConstant& myConstant = cbuffer.mConstants[ iConstant ];


    bool exists = false;
    // search for the constant in the shader constnats
    for(
      u32 iConstantFinder = 0;
      iConstantFinder < shader.mNumConstantFinders;
    ++iConstantFinder )
    {
      ConstantFinder& curConstantFinder =
        shader.mConstantFinders[ iConstantFinder ];

      exists = 
        ( curConstantFinder.mName == myConstant.name ) &&
        ( curConstantFinder.mCBufferHandle.mIndex == cbufferID.mIndex ) &&
        ( curConstantFinder.mConstantIndex == iConstant );
      if( exists )
      {
        break;
      }
    }

    if( !exists )
    {

      if( shader.mNumConstantFinders ==
        ArraySize( shader.mConstantFinders ) )
      {
        errors = "Too many constants";
        AppendFileInfo( errors );
        return;
      }

      ConstantFinder& myConstantFinder =
        shader.mConstantFinders[ shader.mNumConstantFinders++ ];

      myConstantFinder.mConstantIndex = iConstant;
      myConstantFinder.mCBufferHandle = cbufferID;
      myConstantFinder.mName = myConstant.name;
    }




    //ConstantFinder& myConstantFinder =
    //  shader.mConstantFinders[ shader.mNumConstantFinders++ ];

    //myConstantFinder.mConstantIndex = i;
    //myConstantFinder.mCBufferHandle = cbufferID;
    //myConstantFinder.mName = myConstant.name;
  }





  //if( shader.mNumConstantFinders + cbuffer.mNumConstants >
  //  ArraySize( shader.mConstantFinders ) )
  //{
  //  errors = "Too many constants";
  //  AppendFileInfo( errors );
  //  return;
  //}
  //for( u32 i = 0; i < cbuffer.mNumConstants; ++i )
  //{
  //  TacConstant& myConstant = cbuffer.mConstants[ i ];
  //  ConstantFinder& myConstantFinder =
  //    shader.mConstantFinders[ shader.mNumConstantFinders++ ];
  //  myConstantFinder.mConstantIndex = i;
  //  myConstantFinder.mCBufferHandle = cbufferID;
  //  myConstantFinder.mName = myConstant.name;
  //}
}

void RendererDX11::SetName(
  TacCBufferHandle handle,
  const char* name )
{
  mCbuffers[ handle.mIndex ].mHandle->SetPrivateData(
    WKPDID_D3DDebugObjectName,
    TacStrLen( name ),
    name );
}

// blend state ----------------------------------------------------------

TacBlendStateHandle RendererDX11::AddBlendState(
  TacBlendConstants srcRGB,
  TacBlendConstants dstRGB,
  TacBlendMode blendRGB,
  TacBlendConstants srcA,
  TacBlendConstants dstA,
  TacBlendMode blendA,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  auto GetBlend = []( TacBlendConstants c )
  {
    switch( c )
    {
    case TacBlendConstants::OneMinusSrcA:
      return D3D11_BLEND_INV_SRC_ALPHA;
    case TacBlendConstants::SrcA:
      return D3D11_BLEND_SRC_ALPHA;
    case TacBlendConstants::SrcRGB:
      return D3D11_BLEND_SRC_COLOR;
    case TacBlendConstants::Zero:
      return D3D11_BLEND_ZERO;
    case TacBlendConstants::One:
      return D3D11_BLEND_ONE;
    default:
      TacAssert( false );
      return D3D11_BLEND_ZERO;
    }
  };

  auto GetBlendOp = []( TacBlendMode mode )
  {
    switch (mode)
    {
    case TacBlendMode::Add:
      return D3D11_BLEND_OP_ADD;
    default:
      TacAssert( false );
      return D3D11_BLEND_OP_ADD;
    }
  };

  TacBlendStateHandle result;
  result.mIndex = RENDERER_ID_NONE;

  D3D11_BLEND_DESC desc = {};
  desc.RenderTarget[ 0 ].BlendEnable = true;
  desc.RenderTarget[ 0 ].SrcBlend = GetBlend( srcRGB );
  desc.RenderTarget[ 0 ].DestBlend = GetBlend( dstRGB );
  desc.RenderTarget[ 0 ].BlendOp = GetBlendOp( blendRGB );
  desc.RenderTarget[ 0 ].SrcBlendAlpha = GetBlend( srcA );
  desc.RenderTarget[ 0 ].DestBlendAlpha = GetBlend( dstA );
  desc.RenderTarget[ 0 ].BlendOpAlpha = GetBlendOp( blendA );
  desc.RenderTarget[ 0 ].RenderTargetWriteMask =
    D3D11_COLOR_WRITE_ENABLE_ALL;

  TacBlendStateDX11 blendState = {};
  HRESULT hr = mDevice->CreateBlendState(
    &desc,
    &blendState.mHandle );
  if( FAILED( hr ) )
  {
    errors = "ID3D11Device::CreateBlendState failed\n";
    AppendFileInfo( errors );
    AppendInfoQueueMessage( hr, errors );
    return result;
  }

  if( mBlendStateIndexes.empty() )
  {
    result.mIndex = mBlendStates.size();
    mBlendStates.push_back( blendState );
  }
  else
  {
    mBlendStates[ result.mIndex = mBlendStateIndexes.back() ] =
      blendState;
    mBlendStateIndexes.pop_back();
  }

  return result;
}

void RendererDX11::RemoveBlendState( TacBlendStateHandle blendState )
{
  TacBlendStateDX11& blendStateDX11 = mBlendStates[ blendState.mIndex ];
  ReleaseBlendState( blendStateDX11 );
  mBlendStateIndexes.push_back( blendState.mIndex );
}

void RendererDX11::SetBlendState(
  TacBlendStateHandle blendState,
  v4 blendFactorRGBA,
  u32 sampleMask )
{
  TacAssert( blendState.mIndex != RENDERER_ID_NONE );

  mDeviceContext->OMSetBlendState(
    mBlendStates[ blendState.mIndex ].mHandle,
    &blendFactorRGBA.x,
    sampleMask );
}

void RendererDX11::SetName(
  TacBlendStateHandle handle,
  const char* name )
{
  TacBlendStateDX11& blendStateDX11 = mBlendStates[ handle.mIndex ];
  blendStateDX11.mHandle->SetPrivateData(
    WKPDID_D3DDebugObjectName,
    TacStrLen( name ),
    name );
}
// rasterizer state -----------------------------------------------------

TacRasterizerStateHandle RendererDX11::AddRasterizerState(
  TacFillMode fillMode,
  TacCullMode cullMode,
  b32 frontCounterClockwise,
  b32 scissor,
  b32 multisample,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacRasterizerStateHandle result;
  result.mIndex = RENDERER_ID_NONE;

  D3D11_RASTERIZER_DESC desc = {};
  {
    switch( fillMode )
    {
    case TacFillMode::Solid:
      {
        desc.FillMode = D3D11_FILL_SOLID;
      } break;
    case TacFillMode::Wireframe:
      {
        desc.FillMode = D3D11_FILL_WIREFRAME;
      } break;
    default:
      {
        TacAssert( false );
      } break;
    }
    switch (cullMode)
    {
    case TacCullMode::None:
      {
        desc.CullMode = D3D11_CULL_NONE;
      } break;
    case TacCullMode::Back:
      {
        desc.CullMode = D3D11_CULL_BACK;
      } break;
    case TacCullMode::Front:
      {
        desc.CullMode = D3D11_CULL_FRONT;
      } break;
    default:
      {
        TacAssert( false );
      } break;
    }
    desc.ScissorEnable = scissor;
    desc.MultisampleEnable = multisample;
    desc.DepthClipEnable = true;
    desc.FrontCounterClockwise = frontCounterClockwise;
  }

  TacRasterizerStateDX11 rasterizerState = {};
  HRESULT hr = mDevice->CreateRasterizerState(
    &desc,
    &rasterizerState.mHandle );
  if( FAILED( hr ) )
  {
    errors = "ID3D11Device::CreateRasterizerState failed\n";
    AppendFileInfo( errors );
    AppendInfoQueueMessage( hr, errors );
    return result;
  }

  if( mRasterizerStateIndexes.empty() )
  {
    result.mIndex = mRasterizerStates.size();
    mRasterizerStates.push_back( rasterizerState );
  }
  else
  {
    mRasterizerStates[ result.mIndex = mRasterizerStateIndexes.back() ] =
      rasterizerState;
    mRasterizerStateIndexes.pop_back();
  }
  return result;
}

void RendererDX11::RemoveRasterizerState(
  TacRasterizerStateHandle rasterizerState )
{
  TacRasterizerStateDX11& rasterizerStateDX11 = 
    mRasterizerStates[ rasterizerState.mIndex ];
  ReleaseRasterizerState( rasterizerStateDX11 );
  mRasterizerStateIndexes.push_back( rasterizerState.mIndex );
}

void RendererDX11::SetRasterizerState(
  TacRasterizerStateHandle rasterizerState )
{
  TacAssertIndex( rasterizerState.mIndex, mRasterizerStates.size() );

  mDeviceContext->RSSetState(
    mRasterizerStates[ rasterizerState.mIndex ].mHandle );
}

void RendererDX11::SetName(
  TacRasterizerStateHandle handle,
  const char* name )
{
  TacRasterizerStateDX11& rasterizerStateDX11 =
    mRasterizerStates[ handle.mIndex ];

  rasterizerStateDX11.mHandle->SetPrivateData(
    WKPDID_D3DDebugObjectName,
    TacStrLen( name ),
    name );
}
// depth state ----------------------------------------------------------

TacDepthStateHandle RendererDX11::AddDepthState(
  b32 depthTest,
  b32 depthWrite,
  TacDepthFunc depthFunc,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacDepthStateHandle depthStateHandle;
  depthStateHandle.mIndex = RENDERER_ID_NONE;

  D3D11_DEPTH_STENCIL_DESC desc = {};
  {
    desc.DepthEnable = depthTest;
    desc.DepthWriteMask = depthWrite
      ? D3D11_DEPTH_WRITE_MASK_ALL
      : D3D11_DEPTH_WRITE_MASK_ZERO;

    switch( depthFunc )
    {
    case TacDepthFunc::Less:
      {
        desc.DepthFunc = D3D11_COMPARISON_LESS;
      } break;
    case TacDepthFunc::LessOrEqual:
      {
        desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
      } break;
      TacInvalidDefaultCase;
    }
  }

  TacDepthStateDX11 depthState;
  HRESULT hr = mDevice->CreateDepthStencilState(
    &desc,
    &depthState.mHandle );
  if( FAILED( hr ) )
  {
    errors = "ID3D11Device::CreateDepthStencilState failed\n";
    AppendFileInfo( errors );
    AppendInfoQueueMessage( hr, errors );
    return depthStateHandle;
  }

  if( mDepthStatesIndexes.empty() )
  {
    depthStateHandle.mIndex = mDepthStates.size();
    mDepthStates.push_back( depthState );
  }
  else
  {
    mDepthStates[ depthStateHandle.mIndex = mDepthStatesIndexes.back() ] =
      depthState;
    mDepthStatesIndexes.pop_back();
  }

  return depthStateHandle;
}

void RendererDX11::RemoveDepthState( TacDepthStateHandle depthState )
{
  TacAssert( depthState.mIndex != RENDERER_ID_NONE );
  TacAssertIndex( depthState.mIndex, mDepthStates.size() );
  ReleaseDepthState( mDepthStates[ depthState.mIndex ] );
  mDepthStatesIndexes.push_back( depthState.mIndex );
}

void RendererDX11::SetDepthState(
  TacDepthStateHandle depthState,
  u32 stencilRef )
{
  TacAssert( depthState.mIndex != RENDERER_ID_NONE );
  TacAssertIndex( depthState.mIndex, mDepthStates.size() );

  mDeviceContext->OMSetDepthStencilState(
    mDepthStates[ depthState.mIndex ].mHandle,
    stencilRef );
}

void RendererDX11::SetName(
  TacDepthStateHandle handle,
  const char* name )
{
#if TACDEBUG
  TacDepthStateDX11& depthStateDX11 = mDepthStates[ handle.mIndex ];
  if( depthStateDX11.mHandle )
  {
    const u32 buffersize = 256;
    UINT pDataSize = buffersize;
    char data[ buffersize ] = {};

    depthStateDX11.mHandle->GetPrivateData( 
      WKPDID_D3DDebugObjectName,
      &pDataSize,
      &data );

    FixedString< 256 > newname = {};
    if( pDataSize )
    {
      FixedStringAppend( newname, data, pDataSize );
      FixedStringAppend( newname, " --and--" );

      D3D11_MESSAGE_ID hide[] =
      {
        D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS 
      };
      D3D11_INFO_QUEUE_FILTER filter = {};
      filter.DenyList.NumIDs = ArraySize( hide );
      filter.DenyList.pIDList = hide;
      infoQueueDEBUG->PushStorageFilter( &filter );
    }
    FixedStringAppend( newname, name );

    depthStateDX11.mHandle->SetPrivateData(
      WKPDID_D3DDebugObjectName,
      newname.size,
      newname.buffer );
    if( pDataSize )
    {
      infoQueueDEBUG->PopStorageFilter();
    }
  }
#else
  TacUnusedParameter( handle );
  TacUnusedParameter( name );
#endif
}
// Vertex format --------------------------------------------------------

TacVertexFormatHandle RendererDX11::AddVertexFormat(
  TacVertexFormat* vertexFormat,
  u32 numTacVertexFormats,
  TacShaderHandle shaderID,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacVertexFormatHandle vertexFormatHandle;
  vertexFormatHandle.mIndex = RENDERER_ID_NONE;


  std::vector< D3D11_INPUT_ELEMENT_DESC > inputElementDescs;
  inputElementDescs.resize( numTacVertexFormats );

  // copy from the tac vertex format to the dx11 vertex format
  for( u32 i = 0; i < numTacVertexFormats; ++i )
  {
    D3D11_INPUT_ELEMENT_DESC& curDX11Input = inputElementDescs[ i ];
    TacVertexFormat& curTacFormat = vertexFormat[ i ];

    auto GetSemanticName = []( TacAttributeType attribType )->const char*
    {
      switch( attribType )
      {
      case TacAttributeType::Position: return "POSITION";
      case TacAttributeType::Normal: return "NORMAL";
      case TacAttributeType::Texcoord: return "TEXCOORD";
      case TacAttributeType::Color: return "COLOR";
      case TacAttributeType::BoneIndex: return "BONEINDEX";
      case TacAttributeType::BoneWeight: return "BONEWEIGHT";
        TacInvalidDefaultCase;
      }
      return nullptr;
    };

    curDX11Input.Format = GetDXGIFormat( curTacFormat.textureFormat );
    //curTacFormat.mVariableType,
    //curTacFormat.mNumBytes,
    //curTacFormat.mNumComponents );
    curDX11Input.InputSlot = curTacFormat.mInputSlot;
    curDX11Input.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    curDX11Input.InstanceDataStepRate = 0;
    curDX11Input.SemanticIndex = 0;
    curDX11Input.SemanticName = GetSemanticName(
      curTacFormat.mAttributeType );
    curDX11Input.AlignedByteOffset = curTacFormat.mAlignedByteOffset;
  }

  // Create the input layout
  TacVertexFormatDX11 vertexFormatDX11 = {};
  {
    TacAssertIndex( shaderID.mIndex, mShaders.size() );
    TacShaderDX11& myShaderDX11 = mShaders[ shaderID.mIndex ];
    const void *pShaderBytecodeWithInputSignature =
      myShaderDX11.mInputSig->GetBufferPointer();
    SIZE_T BytecodeLength
      = myShaderDX11.mInputSig->GetBufferSize();

    HRESULT hr = mDevice->CreateInputLayout(
      inputElementDescs.data(),
      numTacVertexFormats,
      pShaderBytecodeWithInputSignature,
      BytecodeLength,
      &vertexFormatDX11.mHandle );

    if( FAILED( hr ) )
    {
      errors = "ID3D11Device::CreateInputLayout failed\n";
      AppendInfoQueueMessage( hr, errors );
      AppendFileInfo( errors );
      return vertexFormatHandle;
    }
  }

  if( mVertexFormatIndexes.empty() )
  {
    vertexFormatHandle.mIndex = mVertexFormats.size();
    mVertexFormats.push_back( vertexFormatDX11 );
  }
  else
  {
    mVertexFormats[ vertexFormatHandle.mIndex = mVertexFormatIndexes.back() ] =
      vertexFormatDX11;
    mVertexFormatIndexes.pop_back();
  }

  return vertexFormatHandle;
}

void RendererDX11::RemoveVertexFormat(
  TacVertexFormatHandle vertexFormat )
{
  TacVertexFormatDX11& vertexFormatDX11 = 
    mVertexFormats[ vertexFormat.mIndex ];

  vertexFormatDX11.mHandle->Release();
  mVertexFormatIndexes.push_back( vertexFormat.mIndex );

  TacVertexFormatDX11 empty = {};
  vertexFormatDX11 = empty;
}

void RendererDX11::SetVertexFormat(
  TacVertexFormatHandle myVertexFormatID )
{
  TacAssertIndex( myVertexFormatID.mIndex, mVertexFormats.size() );

  TacVertexFormatDX11& myVertexFormatDX11 =
    mVertexFormats[ myVertexFormatID.mIndex ];

  mDeviceContext->IASetInputLayout( myVertexFormatDX11.mHandle );
}

void RendererDX11::SetName(
  TacVertexFormatHandle handle,
  const char* name )
{
  mVertexFormats[ handle.mIndex ].mHandle->SetPrivateData(
    WKPDID_D3DDebugObjectName,
    TacStrLen( name ),
    name );
}
// etc ------------------------------------------------------------------
void RendererDX11::DebugBegin( const char* section )
{
#if TACDEBUG
  userAnnotationDEBUG->BeginEvent( ( LPCWSTR )section );
#else
  TacUnusedParameter( section );
#endif
}
void RendererDX11::DebugMark( const char* remark )
{
#if TACDEBUG
  userAnnotationDEBUG->SetMarker( ( LPCWSTR )remark );
#else
  TacUnusedParameter( remark );
#endif
}
void RendererDX11::DebugEnd()
{
#if TACDEBUG
  userAnnotationDEBUG->EndEvent();
#endif
}

void RendererDX11::Draw()
{
  TacIndexBufferDX11& indexBuffer = mIndexBuffers[ mCurrentIndexBuffer.mIndex ];
  mDeviceContext->DrawIndexed( indexBuffer.mNumIndexes, 0, 0 );
}

void RendererDX11::DrawIndexed(
  u32 elementCount,
  u32 idxOffset,
  u32 vtxOffset )
{
  mDeviceContext->DrawIndexed( elementCount, idxOffset, vtxOffset );
}

void RendererDX11::SendUniform(
  const char* name,
  void* data,
  u32 size )
{
  TacAssert( mCurrentShader.mIndex != RENDERER_ID_NONE );

  TacShaderDX11& shader = mShaders[ mCurrentShader.mIndex ];

  ConstantFinder* myFinder;
  ConstantFinder* endFinder =
    shader.mConstantFinders + shader.mNumConstantFinders;

  for( myFinder = shader.mConstantFinders;
    myFinder != endFinder;
    ++myFinder )
  {
    if( TacStrCmp( myFinder->mName.buffer, name ) == 0 )
    {
      break;
    }
  }

  TacAssert( myFinder != endFinder );

  TacCBufferDX11& cbuffer = mCbuffers[ myFinder->mCBufferHandle.mIndex ];
  TacConstant& myConstant =
    cbuffer.mConstants[ myFinder->mConstantIndex ];


  memcpy(
    cbuffer.mMemory + myConstant.offset,
    data,
    size );
  cbuffer.mDirty = true;
}

void RendererDX11::Apply()
{
  TacAssert( mCurrentShader.mIndex != RENDERER_ID_NONE );
  TacShaderDX11& shader = mShaders[ mCurrentShader.mIndex ];

  typedef decltype( &ID3D11DeviceContext::VSSetShaderResources )
    SetShaderResourcesFn;
  SetShaderResourcesFn setShaderResourcesFns[ ( u32 )TacShaderType::Count ] = {};
  setShaderResourcesFns[ ( u32 )TacShaderType::Vertex ] =
    &ID3D11DeviceContext::VSSetShaderResources;
  setShaderResourcesFns[ ( u32 )TacShaderType::Fragment ] =
    &ID3D11DeviceContext::PSSetShaderResources;

  typedef decltype( &ID3D11DeviceContext::VSSetSamplers ) SetSamplerFn;
  SetSamplerFn setSamplerFns[ ( u32 )TacShaderType::Count ] = {};
  setSamplerFns[ ( u32 )TacShaderType::Vertex ] =
    &ID3D11DeviceContext::VSSetSamplers;
  setSamplerFns[ ( u32 )TacShaderType::Fragment ] =
    &ID3D11DeviceContext::PSSetSamplers;

  for( u32 iShaderType = 0;
    iShaderType < ( u32 )TacShaderType::Count;
    ++iShaderType )
  {
    PerShaderData& myPerShaderData =
      shader.mPerShaderData[ iShaderType ];

    // update dirty constant buffers
    for( u32 i = 0; i < myPerShaderData.mNumCBufferBindings; ++i )
    {
      PerShaderData::CBufferBinding id = myPerShaderData.mCBufferBindings[ i ];

      TacCBufferDX11& mycbufferStuff = mCbuffers[ id.mCBufferHandle.mIndex ];
      if( mycbufferStuff.mDirty )
      {
        mycbufferStuff.mDirty = false;

        mDeviceContext->UpdateSubresource(
          mycbufferStuff.mHandle,
          0,
          nullptr,
          mycbufferStuff.mMemory,
          0,
          0 );
      }
    }

    // update samplers
    if( mCurrentSamplersDirty[ iShaderType ] )
    {
      mCurrentSamplersDirty[ iShaderType ] = false;
      TacSamplerStateHandle* shaderTypeCurrentSamplers =
        mCurrentSamplers[ iShaderType ];

      ID3D11SamplerState* ppSamplers[ SAMPLERS_PER_SHADER ] = {};
      for( u32 i = 0; i < SAMPLERS_PER_SHADER; ++i )
      {
        TacSamplerStateHandle myRendererID = shaderTypeCurrentSamplers[ i ];
        if( myRendererID.mIndex != RENDERER_ID_NONE )
        {
          ppSamplers[ i ] = mSamplerStates[ myRendererID.mIndex ].mHandle;
        }
      }

      u32 startSlot = 0;
      u32 numSamplers = SAMPLERS_PER_SHADER;

      SetSamplerFn fn = setSamplerFns[ iShaderType ];
      ( mDeviceContext->*fn )( startSlot, numSamplers, ppSamplers );

    }

    // update textures
    if( mCurrentTexturesDirty[ iShaderType ] )
    {
      mCurrentTexturesDirty[ iShaderType ] = false;

      TacTextureHandle* shaderTypeCurrentTextures =
        mCurrentTextures[ iShaderType ];

      ID3D11ShaderResourceView* ppSRVs[ TEXTURES_PER_SHADER ] = {};
      for( u32 i = 0; i < TEXTURES_PER_SHADER; ++i )
      {
        TacTextureHandle myRendererID = shaderTypeCurrentTextures[ i ];
        if( myRendererID.mIndex != RENDERER_ID_NONE )
        {
          ppSRVs[ i ] = mTextures[ myRendererID.mIndex ].mSrv;
        }
      }

      u32 startSlot = 0;
      u32 numSRVs = TEXTURES_PER_SHADER;
      SetShaderResourcesFn fn = setShaderResourcesFns[ iShaderType ];
      ( mDeviceContext->*fn )( startSlot, numSRVs, ppSRVs );
    }
  }
}

void RendererDX11::SwapBuffers()
{
  mSwapChain->Present( 0, 0 );
}

void RendererDX11::SetViewport(
  r32 xRelBotLeftCorner,
  r32 yRelBotLeftCorner,
  r32 wIncreasingRight,
  r32 hIncreasingUp )
{
  r32 rtvHeight = ( r32 )mTextures[ backBufferColorIndex.mIndex ].mHeight;

  D3D11_VIEWPORT vp;
  vp.Width = wIncreasingRight;
  vp.Height = hIncreasingUp;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = xRelBotLeftCorner;
  vp.TopLeftY = rtvHeight - ( yRelBotLeftCorner + hIncreasingUp );
  mDeviceContext->RSSetViewports( 1, &vp );
}

void RendererDX11::SetPrimitiveTopology( TacPrimitive primitive )
{
  switch ( primitive )
  {
  case TacPrimitive::TriangleList:
    mDeviceContext->IASetPrimitiveTopology( 
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    break;
  default:
    TacAssert( false );
    break;
  }
}

void RendererDX11::SetScissorRect(
  float x1,
  float y1,
  float x2,
  float y2 )
{
  const D3D11_RECT r = {
    ( LONG )x1,
    ( LONG )y1,
    ( LONG )x2,
    ( LONG )y2 };

  mDeviceContext->RSSetScissorRects( 1, &r ); 
}

// private functions and data -------------------------------------------

void RendererDX11::MapResource(
  void** data,
  ID3D11Resource* resource,
  D3D11_MAP d3d11mapType,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  D3D11_MAPPED_SUBRESOURCE mappedResource;
  HRESULT hr = mDeviceContext->Map(
    resource, 0, d3d11mapType, 0, &mappedResource );
  if( FAILED( hr ) )
  {
    errors = "ID3D11DeviceContext::Map failed\n";
    AppendInfoQueueMessage( hr, errors );
    AppendFileInfo( errors );
    return;
  }

  *data = mappedResource.pData;
}

void RendererDX11::AppendInfoQueueMessage(
  HRESULT hr,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
#if TACDEBUG
  // NOTE( N8 ): first call of GetMessage() to get the messageLen
  SIZE_T messageLen = 0;
  hr = infoQueueDEBUG->GetMessageA( 0, nullptr, &messageLen );
  if( FAILED( hr ) )
  {
    FixedStringAppend( errors, "\nFailed to get info queue message\n" );
    return;
  }

  // NOTE( N8 ):
  //   Now that we have the size of the message, we can actually
  //   retrieve it with the second call to GetMessage()
  char* data = new char[ messageLen ];
  if( data )
  {
    D3D11_MESSAGE* pMessage = ( D3D11_MESSAGE* )data;
    hr = infoQueueDEBUG->GetMessageA( 0, pMessage, &messageLen );
    FixedStringAppend( errors, "\n" );
    // NOTE( N8 ): length may include the null terminator
    if( pMessage->pDescription[ pMessage->DescriptionByteLength - 1 ]
    == '\0' )
    {
      FixedStringAppend( errors, pMessage->pDescription );
    }
    else
    {
      FixedStringAppend(
        errors,
        pMessage->pDescription,
        pMessage->DescriptionByteLength );
    }
    FixedStringAppend( errors, "\n" );
    delete[] data;
  }

#else
  TacUnusedParameter( hr );
  TacUnusedParameter( errors );
#endif
}

void RendererDX11::GetPerspectiveProjectionAB(
  r32 f,
  r32 n,
  r32& A,
  r32& B )
{
  TacAssert( f >= n ); // make sure you didnt switch them

  // In directx, ( A, B ) maps ( -n, -f ) to ( 0, 1 )
  B = n * ( A = f / ( n - f ) );
}

