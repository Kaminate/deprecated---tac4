#include "tacMainWin32.h"
#include <chrono>
#include <thread>

globalVariable s64 g_Time;
globalVariable s64 g_TicksPerSecond;
globalVariable int VERTEX_BUFFER_SIZE = 20000;
globalVariable int INDEX_BUFFER_SIZE = 40000;

globalVariable TacVertexBufferHandle    g_ImguiVertexBuffer;
globalVariable TacIndexBufferHandle     g_ImguiIndexBuffer;
globalVariable TacDepthStateHandle      g_ImguiDepthState;
globalVariable TacShaderHandle          g_ImguiShader;
globalVariable TacSamplerStateHandle    g_ImguiSamplerState;
globalVariable TacBlendStateHandle      g_ImguiBlendState;
globalVariable TacRasterizerStateHandle g_ImguiRasterizerState;
globalVariable TacVertexFormatHandle    g_ImguiVertexFormatID;
globalVariable TacTextureHandle         g_ImguiTexture;
globalVariable TacCBufferHandle         g_ImguiCBuffer;

globalVariable TacRenderer* renderer;
globalVariable TacThreadContext thread;
globalVariable TacWin32State win32State;
globalVariable TacGameInput gameInput;
globalVariable TacGameInterface gameInterface;
globalVariable HWND g_hWnd;
globalVariable bool mouseTracking;

globalVariable KeyboardKey vkcodeMap[ 256 ];

void TrackMouseLeave()
{
  TRACKMOUSEEVENT mouseevent = {};
  mouseevent.cbSize = sizeof( TRACKMOUSEEVENT );
  mouseevent.dwFlags = 0x2; // TIME_LEAVE
  mouseevent.hwndTrack = g_hWnd;
  mouseevent.dwHoverTime = HOVER_DEFAULT;

  if( 0 == TrackMouseEvent( &mouseevent ) )
  {
    AppendWin32Error(
      gameInterface.gameUnrecoverableErrors,
      GetLastError() );
    return;
  }
}

LRESULT ImGui_ImplDX11_WndProcHandler(
  HWND,
  UINT msg,
  WPARAM wParam,
  LPARAM lParam )
{
  ImGuiIO& io = ImGui::GetIO();

  switch( msg )
  {
  case WM_LBUTTONDOWN:
    {
      io.MouseDown[0] = true;
    } break;
  case WM_LBUTTONUP:
    {
      io.MouseDown[0] = false; 
    } break;
  case WM_RBUTTONDOWN:
    {
      io.MouseDown[1] = true; 
    } break;
  case WM_RBUTTONUP:
    {
      io.MouseDown[1] = false; 
    } break;
  case WM_MOUSEWHEEL:
    {
      io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
    } break;
  case WM_MOUSEMOVE:
    {
      io.MousePos.x = (signed short)(lParam);
      io.MousePos.y = (signed short)(lParam >> 16); 
    } break;
  case WM_KEYDOWN:
    {
      if (wParam < 256)
        io.KeysDown[wParam] = 1;
    } break;
  case WM_KEYUP:
    {
      if (wParam < 256)
        io.KeysDown[wParam] = 0;
    } break;
  case WM_CHAR:
    {
      if (wParam > 0 && wParam < 0x10000)
        io.AddInputCharacter((unsigned short)wParam);
    } break;
  default:
    {
      return 0;
    } break;
  }
  return 1;
}

// Main ImGui rendering function
// (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
internalFunction void ImGui_ImplDX11_RenderDrawLists(
  ImDrawData* draw_data )
{
  renderer->SetDepthState( g_ImguiDepthState, 0 );
  renderer->SetIndexBuffer( g_ImguiIndexBuffer );
  renderer->SetVertexBuffers( &g_ImguiVertexBuffer, 1 );

  ImDrawVert* vtx_dst;
  ImDrawIdx* idx_dst;
  FixedString< DEFAULT_ERR_LEN > myErrors = {};

  renderer->MapVertexBuffer(
    ( void** )&vtx_dst,
    TacMap::WriteDiscard,
    myErrors );
  TacAssert( myErrors.size == 0 );

  renderer->MapIndexBuffer(
    ( void** )&idx_dst,
    TacMap::WriteDiscard,
    myErrors );
  TacAssert( myErrors.size == 0 );

  for( int n = 0; n < draw_data->CmdListsCount; n++ )
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    memcpy(
      vtx_dst,
      &cmd_list->VtxBuffer[0],
      cmd_list->VtxBuffer.size() * sizeof(ImDrawVert));
    memcpy(
      idx_dst,
      &cmd_list->IdxBuffer[0],
      cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx));
    vtx_dst += cmd_list->VtxBuffer.size();
    idx_dst += cmd_list->IdxBuffer.size();
  }
  renderer->UnmapVertexBuffer(myErrors );
  TacAssert( myErrors.size == 0 );
  renderer->UnmapIndexBuffer(myErrors );
  TacAssert( myErrors.size == 0 );

  // Setup viewport
  r32 w = ImGui::GetIO().DisplaySize.x;
  r32 h = ImGui::GetIO().DisplaySize.y;
  renderer->SetViewport( 0, 0, w, h );

  renderer->SetActiveShader( g_ImguiShader );
  renderer->SetVertexFormat( g_ImguiVertexFormatID );
  // send projection matrix
  {
    const float L = 0.0f;
    const float R = ImGui::GetIO().DisplaySize.x;
    const float B = ImGui::GetIO().DisplaySize.y;
    const float T = 0.0f;
    const float mvp[4][4] = 
    {
      { 2.0f/(R-L),   0.0f,           0.0f,       0.0f},
      { 0.0f,         2.0f/(T-B),     0.0f,       0.0f,},
      { 0.0f,         0.0f,           0.5f,       0.0f },
      { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
    };
    renderer->SendUniform(
      "ProjectionMatrix",
      ( void* )mvp,
      sizeof( mvp ) );
  }
  renderer->SetIndexBuffer( g_ImguiIndexBuffer );
  renderer->SetVertexBuffers( &g_ImguiVertexBuffer, 1 );
  renderer->SetPrimitiveTopology(
    TacPrimitive::TriangleList );
  renderer->SetSamplerState( "sampler0", g_ImguiSamplerState );

  // Setup render state
  renderer->SetBlendState( g_ImguiBlendState );
  renderer->SetRasterizerState( g_ImguiRasterizerState );

  // Render command lists
  int vtx_offset = 0;
  int idx_offset = 0;
  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->UserCallback)
      {
        pcmd->UserCallback(cmd_list, pcmd);
      }
      else
      {
        TacTextureHandle myRendererID;
        myRendererID.mIndex = ( s32 )pcmd->RendererID;
        renderer->SetTexture( "texture0", myRendererID );
        renderer->Apply();
        renderer->SetScissorRect(
          pcmd->ClipRect.x,
          pcmd->ClipRect.y,
          pcmd->ClipRect.z,
          pcmd->ClipRect.w );
        renderer->DrawIndexed(
          pcmd->ElemCount,
          idx_offset,
          vtx_offset);
      }
      idx_offset += pcmd->ElemCount;
    }
    vtx_offset += cmd_list->VtxBuffer.size();
  }
}

internalFunction void ImGui_ImplDX11_CreateFontsTexture(
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  ImGuiIO& io = ImGui::GetIO();

  // Build
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

  {
    TacImage myImage = {};
    //myImage.mBitsPerPixel = 8;
    myImage.mData = pixels;
    myImage.mWidth = width;
    myImage.mHeight = height;
    myImage.mPitch = width * 4;
    myImage.mTextureFormat = TacTextureFormat::RGBA8Unorm;
    //myImage.variableType = TacVariableType::unorm;
    //myImage.variableBytesPerComponent = 1;
    //myImage.variableNumComponents = 4;
    myImage.textureDimension = 2;
    g_ImguiTexture = renderer->AddTextureResource(
      myImage,
      TacTextureUsage::Immutable,
      TacBinding::ShaderResource,
      errors );
    if( errors.size )
      return;
    gameInterface.renderer->SetName(
      g_ImguiTexture,
      STRINGIFY( g_ImguiTexture ) );

    TacAssert(
      sizeof( io.Fonts->TexID ) >=
      sizeof( g_ImguiTexture.mIndex ) );
    io.Fonts->TexID = ( void* )g_ImguiTexture.mIndex;
  }

  // create sampler state
  g_ImguiSamplerState = renderer->AddSamplerState(
    TacAddressMode::Wrap,
    TacAddressMode::Wrap,
    TacAddressMode::Wrap,
    TacComparison::Always,
    TacFilter::Linear,
    errors );
  if( errors.size )
    return;

  // Cleanup
  // (don't clear theinput data if you want to append new fonts later)
  io.Fonts->ClearInputData();
  io.Fonts->ClearTexData();
}

internalFunction void ImGui_ImplDX11_CreateDeviceObjects(
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  // blending setup
  {
    g_ImguiBlendState = renderer->AddBlendState(
      TacBlendConstants::SrcA,
      TacBlendConstants::OneMinusSrcA,
      TacBlendMode::Add,
      TacBlendConstants::OneMinusSrcA,
      TacBlendConstants::Zero,
      TacBlendMode::Add,
      errors );
    if( errors.size )
      return;
    gameInterface.renderer->SetName(
      g_ImguiBlendState,
      STRINGIFY( g_ImguiBlendState ) );
  }

  // rasterizer state
  {
    g_ImguiRasterizerState =
      renderer->AddRasterizerState(
      TacFillMode::Solid,
      TacCullMode::None,
      false,
      true,
      false,
      errors );
    if( errors.size )
      return;
    gameInterface.renderer->SetName(
      g_ImguiRasterizerState,
      STRINGIFY( g_ImguiRasterizerState ) );
  }

  ImGui_ImplDX11_CreateFontsTexture( errors );
  if( errors.size )
    return;

  // shader
  {

    static const char* vertexShader = 
      "cbuffer vertexBuffer : register(b0)\n"
      "{\n"
      "  float4x4 ProjectionMatrix;\n"
      "};\n"
      "struct VS_INPUT\n"
      "{\n"
      "  float2 pos : POSITION;\n"
      "  float4 col : COLOR;\n"
      "  float2 uv  : TEXCOORD;\n"
      "};\n"
      "\n"
      "struct PS_INPUT\n"
      "{\n"
      "  float4 pos : SV_POSITION;\n"
      "  float4 col : COLOR;\n"
      "  float2 uv  : TEXCOORD;\n"
      "};\n"
      "\n"
      "PS_INPUT main(VS_INPUT input)\n"
      "{\n"
      "  PS_INPUT output;\n"
      "  output.pos = mul(\n"
      "    ProjectionMatrix,\n"
      "    float4(input.pos.xy, 0.f, 1.f));\n"
      "  output.col =input.col;\n"
      "  output.uv  =input.uv;\n"
      "  return output;\n"
      "}\n";

    static const char* pixelShader = 
      "struct PS_INPUT\n"
      "{\n"
      "  float4 pos : SV_POSITION;\n"
      "  float4 col : COLOR;\n"
      "  float2 uv  : TEXCOORD;\n"
      "};\n"
      "sampler sampler0 : register( ps, s[ 0 ] );\n"
      "Texture2D texture0;\n"
      "\n"
      "float4 main(PS_INPUT input) : SV_Target\n"
      "{\n"
      "  float4 out_col =\n"
      "    input.col *\n"
      "    texture0.Sample(sampler0,input.uv); \n"
      "  return out_col; \n"
      "}\n";


    const char* shaderStrings[ ( u32 )TacShaderType::Count ] = {};
    shaderStrings[ ( u32 )TacShaderType::Vertex ] = vertexShader;
    shaderStrings[ ( u32 )TacShaderType::Fragment ] = pixelShader;

    const char* entryPoints[ ( u32 )TacShaderType::Count ] = {};
    entryPoints[ ( u32 )TacShaderType::Vertex ] = "main";
    entryPoints[ ( u32 )TacShaderType::Fragment ] = "main";

    const char* shaderModels[ ( u32 )TacShaderType::Count ] = {};
    shaderModels[ ( u32 )TacShaderType::Vertex ] = "vs_5_0";
    shaderModels[ ( u32 )TacShaderType::Fragment ] = "ps_5_0";

    g_ImguiShader = renderer->LoadShaderFromString(
      shaderStrings,
      entryPoints,
      shaderModels,
      errors );
    if( errors.size )
      return;
    gameInterface.renderer->SetName(
      g_ImguiShader,
      STRINGIFY( g_ImguiShader ) );
  }

  // vertex format
  {
    TacVertexFormat myTacVertexFormats[ 3 ];
    {
      TacVertexFormat* vertexFormat = myTacVertexFormats;
      vertexFormat->mInputSlot = 0;
      vertexFormat->mAttributeType =
        TacAttributeType::Position;
      vertexFormat->textureFormat = TacTextureFormat::RG32Float;
      //vertexFormat->mVariableType = TacVariableType::real;
      //vertexFormat->mNumBytes = 4;
      //vertexFormat->mNumComponents = 2;
      vertexFormat->mAlignedByteOffset = OffsetOf( ImDrawVert, pos );

      ++vertexFormat;
      vertexFormat->mInputSlot = 0;
      vertexFormat->mAttributeType =
        TacAttributeType::Texcoord;
      vertexFormat->textureFormat = TacTextureFormat::RG32Float;
      //vertexFormat->mVariableType = TacVariableType::real;
      //vertexFormat->mNumBytes = 4;
      //vertexFormat->mNumComponents = 2;
      vertexFormat->mAlignedByteOffset = OffsetOf( ImDrawVert, uv );

      ++vertexFormat;
      vertexFormat->mInputSlot = 0;
      vertexFormat->mAttributeType =
        TacAttributeType::Color;
      vertexFormat->textureFormat = TacTextureFormat::RGBA8Unorm;
      //vertexFormat->mVariableType = TacVariableType::unorm;
      //vertexFormat->mNumBytes = 1;
      //vertexFormat->mNumComponents = 4;
      vertexFormat->mAlignedByteOffset = OffsetOf( ImDrawVert, col );
    }

    g_ImguiVertexFormatID = renderer->AddVertexFormat(
      myTacVertexFormats,
      ArraySize( myTacVertexFormats ),
      g_ImguiShader,
      errors );
    if( errors.size )
      return;
    gameInterface.renderer->SetName(
      g_ImguiVertexFormatID,
      STRINGIFY( g_ImguiVertexFormatID ) );
  }

  // constant buffer
  {
    TacConstant constants[ 1 ] = {};
    TacConstant& c0 = constants[ 0 ];
    c0.name = "ProjectionMatrix";
    c0.size = 64;

    g_ImguiCBuffer = renderer->LoadCbuffer(
      "vertexBuffer",
      constants,
      ArraySize( constants ),
      errors );
    if( errors.size )
      return;
    gameInterface.renderer->SetName(
      g_ImguiCBuffer,
      STRINGIFY( g_ImguiCBuffer ) );

    renderer->AddCbuffer(
      g_ImguiShader,
      g_ImguiCBuffer,
      0,
      TacShaderType::Vertex,
      errors );
    if( errors.size )
      return;
  }

  // sampler
  {
    renderer->AddSampler(
      "sampler0",
      g_ImguiShader,
      TacShaderType::Fragment,
      0,
      errors );
    if( errors.size )
      return;
  }

  // texture sampler
  {
    renderer->AddTexture(
      "texture0",
      g_ImguiShader,
      TacShaderType::Fragment,
      0,
      errors );
    if( errors.size )
      return;
  }

  // depth state
  {

    g_ImguiDepthState = renderer->AddDepthState(
      false,
      false,
      TacDepthFunc::Less,
      errors  );
    if( errors.size )
      return;
    gameInterface.renderer->SetName(
      g_ImguiDepthState,
      STRINGIFY( g_ImguiDepthState ) );
  }

  // Vertex buffer
  {
    g_ImguiVertexBuffer = renderer->AddVertexBuffer(
      TacBufferAccess::Dynamic,
      nullptr,
      VERTEX_BUFFER_SIZE,
      sizeof( ImDrawVert ),
      errors );
    if( errors.size )
      return;
    gameInterface.renderer->SetName(
      g_ImguiVertexBuffer,
      STRINGIFY( g_ImguiVertexBuffer ) );
  }


  // index buffer
  {
    TacAssert( sizeof( ImDrawIdx ) == 2 );

    g_ImguiIndexBuffer = renderer->AddIndexBuffer(
      TacBufferAccess::Dynamic,
      nullptr,
      INDEX_BUFFER_SIZE,
      TacTextureFormat::R16Uint,
      INDEX_BUFFER_SIZE * 2,
      //TacVariableType::uint,
      //2,
      errors );
    if( errors.size )
      return;
    gameInterface.renderer->SetName(
      g_ImguiIndexBuffer,
      STRINGIFY( g_ImguiIndexBuffer ) );
  }
}

internalFunction bool ImGui_ImplDX11_Init( HWND g_hWnd )
{
  if( !QueryPerformanceFrequency( ( LARGE_INTEGER* )&g_TicksPerSecond ) )
    return false;
  if( !QueryPerformanceCounter( ( LARGE_INTEGER* )&g_Time ) )
    return false;

  // Keyboard mapping.
  // ImGui will use those indices to peek into the io.KeyDown[] array
  // that we will update during the application lifetime.
  ImGuiIO& io = ImGui::GetIO();
  io.KeyMap[ ImGuiKey_Tab ] = VK_TAB; 
  io.KeyMap[ ImGuiKey_LeftArrow ] = VK_LEFT;
  io.KeyMap[ ImGuiKey_RightArrow ] = VK_RIGHT;
  io.KeyMap[ ImGuiKey_UpArrow ] = VK_UP;
  io.KeyMap[ ImGuiKey_DownArrow ] = VK_DOWN;
  io.KeyMap[ ImGuiKey_PageUp ] = VK_PRIOR;
  io.KeyMap[ ImGuiKey_PageDown ] = VK_NEXT;
  io.KeyMap[ ImGuiKey_Home ] = VK_HOME;
  io.KeyMap[ ImGuiKey_End ] = VK_END;
  io.KeyMap[ ImGuiKey_Delete ] = VK_DELETE;
  io.KeyMap[ ImGuiKey_Backspace ] = VK_BACK;
  io.KeyMap[ ImGuiKey_Enter ] = VK_RETURN;
  io.KeyMap[ ImGuiKey_Escape ] = VK_ESCAPE;
  io.KeyMap[ ImGuiKey_A ] = 'A';
  io.KeyMap[ ImGuiKey_C ] = 'C';
  io.KeyMap[ ImGuiKey_V ] = 'V';
  io.KeyMap[ ImGuiKey_X ] = 'X';
  io.KeyMap[ ImGuiKey_Y ] = 'Y';
  io.KeyMap[ ImGuiKey_Z ] = 'Z';

  io.RenderDrawListsFn = ImGui_ImplDX11_RenderDrawLists;
  io.ImeWindowHandle = g_hWnd;

  return true;
}

internalFunction void ImGui_ImplDX11_Shutdown()
{
  renderer->RemoveVertexBuffer( g_ImguiVertexBuffer );
  renderer->RemoveIndexBuffer( g_ImguiIndexBuffer );
  renderer->RemoveDepthState( g_ImguiDepthState );
  renderer->RemoveShader( g_ImguiShader );
  renderer->RemoveSamplerState( g_ImguiSamplerState );
  renderer->RemoveBlendState( g_ImguiBlendState );
  renderer->RemoveRasterizerState( g_ImguiRasterizerState );
  renderer->RemoveVertexFormat( g_ImguiVertexFormatID );
  renderer->RemoveTextureResoure( g_ImguiTexture );
  renderer->RemoveCbuffer( g_ImguiCBuffer );

  ImGui::GetIO().Fonts->TexID = 0;
  ImGui::Shutdown();
}

internalFunction void ImGui_ImplDX11_NewFrame( HWND g_hWnd )
{
  ImGuiIO& io = ImGui::GetIO();

  // Setup display size (every frame to accommodate for window resizing)
  RECT rect;
  GetClientRect(g_hWnd, &rect);
  io.DisplaySize = ImVec2(
    (float)(rect.right - rect.left),
    (float)(rect.bottom - rect.top));

  // Setup time step
  INT64 current_time;
  QueryPerformanceCounter((LARGE_INTEGER *)&current_time); 
  io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
  g_Time = current_time;

  // Read keyboard modifiersinputs
  io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
  // io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
  // io.MousePos : filled by WM_MOUSEMOVE events
  // io.MouseDown : filled by WM_*BUTTON* events
  // io.MouseWheel : filled by WM_MOUSEWHEEL events

  // Hide OS mouse cursor if ImGui is drawing it
  SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

  // Start the frame
  ImGui::NewFrame();
}

LRESULT CALLBACK WndProc(
  HWND hwnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam )
{
  if( ImGui_ImplDX11_WndProcHandler( hwnd, uMsg, wParam, lParam ) )
  {
    if(
      ImGui::IsMouseHoveringAnyWindow() ||
      ImGui::IsAnyItemActive() ||
      ImGui::IsAnyItemHovered() )
    {
      return true;
    }
  }

  LRESULT result = 0;

  switch( uMsg )
  {
  case WM_DESTROY:
  case WM_CLOSE:
  case WM_QUIT:
    {
      gameInterface.running = false;
    } break;
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP:
    {
      b32 wasDown = ( lParam & 1 << 30 ) != 0;
      b32 isDown = ( lParam & 1 << 31 ) == 0;
      if( isDown != wasDown )
      {
        u32 vkCode = ( u32 )wParam;
        KeyboardKey key = vkcodeMap[ vkCode ];
        gameInput.keyboardKeyEndedDown[ ( u32 )key ] = isDown;

        // alt codes
        b32 altDown = ( lParam & 1 << 29 ) != 0;
        if( isDown && altDown )
        {
          //if( vkCode == VK_RETURN )
          //{
          //  ToggleFullscreen( hwnd );
          //}

          if( vkCode == VK_F4 )
          {
            gameInterface.running = false;
          }
        }
      }
    } break;
  case WM_ACTIVATEAPP:
    {
      BYTE alpha = wParam == TRUE ? 255 : 64;

      SetLayeredWindowAttributes(
        hwnd,
        RGB( 0, 0, 0 ),
        alpha,
        LWA_ALPHA );

    } break;
    // mouse buttons
  case WM_LBUTTONDOWN:
    {
      u32 mouseButtonIndex = ( u32 )MouseButton::eLeftMouseButton;
      gameInput.mouseButtonEndedDown[ mouseButtonIndex ] = true;
    } break;
  case WM_LBUTTONUP:
    {
      u32 mouseButtonIndex = ( u32 )MouseButton::eLeftMouseButton;
      gameInput.mouseButtonEndedDown[ mouseButtonIndex ] = false;
    } break;
  case WM_RBUTTONDOWN:
    {
      u32 mouseButtonIndex = ( u32 )MouseButton::eRightMouseButton;
      gameInput.mouseButtonEndedDown[ mouseButtonIndex ] = true;
    } break;
  case WM_RBUTTONUP:
    {
      u32 mouseButtonIndex = ( u32 )MouseButton::eRightMouseButton;
      gameInput.mouseButtonEndedDown[ mouseButtonIndex ] = false;
    } break;
  case WM_MBUTTONDOWN:
    {
      u32 mouseButtonIndex = ( u32 )MouseButton::eMiddleMouseButton;
      gameInput.mouseButtonEndedDown[ mouseButtonIndex ] = true;
    } break;
  case WM_MBUTTONUP:
    {
      u32 mouseButtonIndex = ( u32 )MouseButton::eMiddleMouseButton;
      gameInput.mouseButtonEndedDown[ mouseButtonIndex ] = false;
    } break;
    //else if( vkCode == VK_LBUTTON )
    //{
    //  u32 mouseButtonIndex =  ( u32 )MouseButton::eLeftMouseButton;
    //}
    //else if( vkCode == VK_RBUTTON )
    //{
    //  u32 mouseButtonIndex =  ( u32 )MouseButton::eRightMouseButton;
    //  input.mouseButtonEndedDown[ mouseButtonIndex ] = isDown;
    //}
    //else if( vkCode == VK_MBUTTON )
    //{
    //  u32 mouseButtonIndex =  ( u32 )MouseButton::eMiddleMouseButton;
    //  input.mouseButtonEndedDown[ mouseButtonIndex ] = isDown;
    //}
  case WM_MOUSEMOVE:
    {
      if( !mouseTracking )
      {
        mouseTracking = true;
        TrackMouseLeave();
      }
      u32 newX = (signed short)(lParam);
      u32 newY = (signed short)(lParam >> 16); 
      gameInput.mouseXRel += newX - gameInput.mouseX;
      gameInput.mouseYRel += newY - gameInput.mouseY;
      gameInput.mouseX = newX;
      gameInput.mouseY = newY;
    } break;
  case WM_MOUSEWHEEL:
    {
      short wheelDeltaParam = GET_WHEEL_DELTA_WPARAM( wParam );
      short ticks = wheelDeltaParam / WHEEL_DELTA;
      gameInput.mouseWheelChange += ticks;
    } break;
    //case WM_SETCURSOR:
    //{
    //  if( DEBUGGlobalShowCursor )
    //  {
    //    result = DefWindowProc( hwnd, uMsg, wParam, lParam );
    //  }
    //  else
    //  {
    //    SetCursor( nullptr );
    //  }
    //} break;
  case WM_MOUSELEAVE:
    {
      mouseTracking = false;
      for( u32 i = 0; i < ( u32 )KeyboardKey::eCount; ++i )
      {
        gameInput.keyboardKeyEndedDown[ i ] = 0;
      }
      for( u32 i = 0; i < ( u32 )MouseButton::eCount; ++i )
      {
        gameInput.mouseButtonEndedDown[ i ] = 0;
      }
    } break;
  default:
    {
      result = DefWindowProc( hwnd, uMsg, wParam, lParam );
    } break;
  }

  return result;

}

internalFunction Win32GameCode Win32LoadGameCode(
  const char* srcDLLpath,
  const char* tmpDLLpath,
  const char* srcPDBpath,
  const char* tmpPDBpath,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  Win32GameCode result = {};
  result.onUpdate = GameUpdateStub;
  result.onLoad = GameOnLoadStub;

  if( 0 == CopyFile( srcPDBpath, tmpPDBpath, FALSE ) )
  {
    errors.size = sprintf_s(
      errors.buffer,
      "failed to copy from %s to %s\n",
      srcPDBpath,
      tmpPDBpath );
    AppendWin32Error( errors, GetLastError() );
    return result;
  }
  if( 0 == CopyFile( srcDLLpath, tmpDLLpath, FALSE ) )
  {
    errors.size = sprintf_s(
      errors.buffer,
      "failed to copy from %s to %s\n",
      srcDLLpath,
      tmpDLLpath );
    AppendWin32Error( errors, GetLastError() );
    return result;
  }

  TacFile handle = PlatformOpenFile(
    &thread,
    tmpDLLpath,
    OpenFileDisposition::OpenExisting,
    FileAccess::Read,
    errors );
  if( errors.size == 0 )
  {
    result.lastWrite = PlatformGetFileModifiedTime(
      &thread,
      handle,
      errors );
    PlatformCloseFile( &thread, handle, errors );

    result.dll = LoadLibrary( tmpDLLpath );
    if( result.dll )
    {
      const char* gameUpdateName =
        STRINGIFY( TAC_GAME_UPDATE_FUNCTION_NAME );
      result.onUpdate = ( decltype( &GameUpdateStub ) )
        GetProcAddress( ( HMODULE )result.dll, gameUpdateName );

      const char* gameOnLoadName =
        STRINGIFY( TAC_GAME_LOAD_FUNCTION_NAME );
      result.onLoad = ( decltype( &GameOnLoadStub ) )
        GetProcAddress( ( HMODULE )result.dll, gameOnLoadName );

      const char* gameOnExitName =
        STRINGIFY( TAC_GAME_EXIT_FUNCTION_NAME );
      result.onExit = ( decltype( &GameOnExitStub ) )
        GetProcAddress( ( HMODULE )result.dll, gameOnExitName );
    }
  }

  if( !result.onUpdate || !result.onLoad )
  {
    result.onUpdate  = GameUpdateStub;
    result.onLoad  = GameOnLoadStub;
  }

  return result;
}

internalFunction void Win32UnloadGameCode(
  Win32GameCode* gameCode )
{
  if( gameCode->dll )
  {
    FreeLibrary( ( HMODULE )gameCode->dll );
    gameCode->dll = nullptr;
    gameCode->onUpdate = GameUpdateStub;
    gameCode->onLoad = GameOnLoadStub;
    gameCode->onExit = GameOnExitStub;
  }
}

// Input Record -----------------------------------------------------------

internalFunction void InputRecordBegin(
  TacThreadContext* thread, 
  TacWin32State& win32state,
  u32 index,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  win32State.recordingHandle = PlatformOpenFile(
    thread,
    win32State.filename,
    OpenFileDisposition::CreateAlways,
    FileAccess::Write,
    errors );
  if( errors.size )
    return;

  PlatformWriteEntireFile(
    thread,
    win32State.recordingHandle,
    win32state.memory.permanentStorage,
    win32state.memory.permanentStorageSize,
    errors );
  if( errors.size )
    return;

  TacAssert( index );
  win32state.iRecord = index;
}

internalFunction void InputRecordUpdate(
  TacThreadContext* thread, 
  TacWin32State& win32state,
  TacGameInput& input,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  PlatformWriteEntireFile(
    thread,
    win32state.recordingHandle,
    &input,
    sizeof( TacGameInput ),
    errors );
}

internalFunction void InputRecordEnd(
  TacThreadContext* thread, 
  TacWin32State& win32state,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  PlatformCloseFile( thread, win32state.recordingHandle, errors );
  if( errors.size )
    return;
  win32state.iRecord = 0;
}

// Input Playback ---------------------------------------------------------

internalFunction void InputPlaybackBegin(
  TacThreadContext* thread, 
  TacWin32State& win32state,
  u32 index,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  win32State.recordingHandle = PlatformOpenFile(
    thread,
    win32State.filename,
    OpenFileDisposition::OpenExisting,
    FileAccess::Read,
    errors );
  if( errors.size )
    return;

  PlatformReadEntireFile(
    thread,
    win32State.recordingHandle,
    win32State.memory.permanentStorage,
    win32State.memory.permanentStorageSize,
    errors );
  if( errors.size )
    return;

  TacAssert( index );
  win32state.iPlayback = index;
}

internalFunction void InputPlaybackEnd(
  TacThreadContext* thread, 
  TacWin32State& win32state,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  PlatformCloseFile( thread, win32state.recordingHandle, errors );
  if( errors.size )
    return;
  win32state.iPlayback = 0;
}

internalFunction void InputPlaybackUpdate(
  TacThreadContext* thread, 
  TacWin32State& win32state,
  TacGameInput& input,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  bool reachedEOF = PlatformReadEntireFile(
    thread,
    win32state.recordingHandle,
    &input,
    sizeof( TacGameInput ),
    errors );
  if( errors.size )
    return;

  if( reachedEOF )
  {
    u32 index = win32state.iPlayback;
    InputPlaybackEnd( thread, win32state, errors );
    if( errors.size )
      return;
    InputPlaybackBegin( thread, win32state, index, errors );
    if( errors.size )
      return;
  }
}

// public functions -------------------------------------------------------

void PrintStringCallback(
  TacThreadContext* thread,
  void* data )
{
  TacUnusedParameter( thread );
  const char* str = ( const char* )data;
  char buffer[ 256 ];
  sprintf_s(
    buffer,
    "thread %d - %s\n",
    thread->logicalThreadIndex,
    str );
  OutputDebugString( buffer );
}

void RunGame( 
  HINSTANCE hInstance,
  int nCmdShow,
  const char* dllname,
  FixedString< DEFAULT_ERR_LEN >& unrecoverableErrors )
{
  // setup the vkcodemap
  {
    // alphabet
    {
      vkcodeMap[ 'A' ] = KeyboardKey::A;
      vkcodeMap[ 'B' ] = KeyboardKey::B;
      vkcodeMap[ 'C' ] = KeyboardKey::C;
      vkcodeMap[ 'D' ] = KeyboardKey::D;
      vkcodeMap[ 'E' ] = KeyboardKey::E;
      vkcodeMap[ 'F' ] = KeyboardKey::F;
      vkcodeMap[ 'G' ] = KeyboardKey::G;
      vkcodeMap[ 'H' ] = KeyboardKey::H;
      vkcodeMap[ 'I' ] = KeyboardKey::I;
      vkcodeMap[ 'J' ] = KeyboardKey::J;
      vkcodeMap[ 'K' ] = KeyboardKey::K;
      vkcodeMap[ 'L' ] = KeyboardKey::L;
      vkcodeMap[ 'M' ] = KeyboardKey::M;
      vkcodeMap[ 'N' ] = KeyboardKey::N;
      vkcodeMap[ 'O' ] = KeyboardKey::O;
      vkcodeMap[ 'P' ] = KeyboardKey::P;
      vkcodeMap[ 'Q' ] = KeyboardKey::Q;
      vkcodeMap[ 'R' ] = KeyboardKey::R;
      vkcodeMap[ 'S' ] = KeyboardKey::S;
      vkcodeMap[ 'T' ] = KeyboardKey::T;
      vkcodeMap[ 'U' ] = KeyboardKey::U;
      vkcodeMap[ 'V' ] = KeyboardKey::V;
      vkcodeMap[ 'W' ] = KeyboardKey::W;
      vkcodeMap[ 'X' ] = KeyboardKey::X;
      vkcodeMap[ 'Y' ] = KeyboardKey::Y;
      vkcodeMap[ 'Z' ] = KeyboardKey::Z;

    }

    // numbers
    {
      vkcodeMap[ '0' ] = KeyboardKey::Num0;
      vkcodeMap[ '1' ] = KeyboardKey::Num1;
      vkcodeMap[ '2' ] = KeyboardKey::Num2;
      vkcodeMap[ '3' ] = KeyboardKey::Num3;
      vkcodeMap[ '4' ] = KeyboardKey::Num4;
      vkcodeMap[ '5' ] = KeyboardKey::Num5;
      vkcodeMap[ '6' ] = KeyboardKey::Num6;
      vkcodeMap[ '7' ] = KeyboardKey::Num7;
      vkcodeMap[ '8' ] = KeyboardKey::Num8;
      vkcodeMap[ '9' ] = KeyboardKey::Num9;
    }

    // numpad
    {
      vkcodeMap[ VK_NUMPAD0 ] = KeyboardKey::Numpad0;
      vkcodeMap[ VK_NUMPAD1 ] = KeyboardKey::Numpad1;
      vkcodeMap[ VK_NUMPAD2 ] = KeyboardKey::Numpad2;
      vkcodeMap[ VK_NUMPAD3 ] = KeyboardKey::Numpad3;
      vkcodeMap[ VK_NUMPAD4 ] = KeyboardKey::Numpad4;
      vkcodeMap[ VK_NUMPAD5 ] = KeyboardKey::Numpad5;
      vkcodeMap[ VK_NUMPAD6 ] = KeyboardKey::Numpad6;
      vkcodeMap[ VK_NUMPAD7 ] = KeyboardKey::Numpad7;
      vkcodeMap[ VK_NUMPAD8 ] = KeyboardKey::Numpad8;
      vkcodeMap[ VK_NUMPAD9 ] = KeyboardKey::Numpad9;
    }

    // arrows
    {
      vkcodeMap[ VK_UP ] = KeyboardKey::UpArrow;
      vkcodeMap[ VK_DOWN ] = KeyboardKey::DownArrow;
      vkcodeMap[ VK_LEFT ] = KeyboardKey::LeftArrow;
      vkcodeMap[ VK_RIGHT ] = KeyboardKey::RightArrow;
    }

    // etc
    {
      vkcodeMap[ VK_ESCAPE ] = KeyboardKey::Escape ;
      vkcodeMap[ VK_BACK ] = KeyboardKey::Backspace;
      vkcodeMap[  VK_DELETE ] = KeyboardKey::Delete;
      vkcodeMap[ VK_SPACE ] = KeyboardKey::Space ;
      vkcodeMap[ VK_OEM_PLUS ] = KeyboardKey::Plus;
      vkcodeMap[ VK_OEM_MINUS ] = KeyboardKey::Minus;
      vkcodeMap[ VK_OEM_PERIOD ] = KeyboardKey::Period;
      vkcodeMap[ VK_OEM_COMMA ] = KeyboardKey::Comma;
      vkcodeMap[ VK_DECIMAL ] = KeyboardKey::DecimalPoint;
      vkcodeMap[ VK_ADD ] = KeyboardKey::NumpadAdd;
      vkcodeMap[ VK_SUBTRACT ] = KeyboardKey::NumpadSubtract;
      vkcodeMap[ VK_CONTROL ] = KeyboardKey::Control;
    }


  }

  gameInterface.gameInput = &gameInput;

  // init memory ----------------------------------------------------------
  {
#ifdef TACDEBUG
    LPVOID baseAddr = ( LPVOID )Terabytes( ( uint64_t ) 2 );
#else
    LPVOID baseAddr = 0;
#endif
    win32State.memory.permanentStorageSize = Megabytes( 4 );
    win32State.memory.transientStorageSize = Megabytes( 64 );
    u32 totalMemorySize
      = win32State.memory.permanentStorageSize 
      + win32State.memory.transientStorageSize;

    void* memoryBlock = VirtualAlloc(
      baseAddr,
      totalMemorySize,
      MEM_RESERVE | MEM_COMMIT,
      PAGE_READWRITE );
    if( !memoryBlock )
    {
      unrecoverableErrors.size = sprintf_s(
        unrecoverableErrors.buffer,
        "Failed to allocate %i bytes",
        totalMemorySize );
      AppendFileInfo( unrecoverableErrors );
      return;
    }

    win32State.memory.permanentStorage = memoryBlock;
    win32State.memory.transientStorage
      = ( uint8_t* )memoryBlock
      + win32State.memory.permanentStorageSize;

    gameInterface.gameMemory = &win32State.memory;
  }

  // reload
  win32State.filename = "tacData/playback.tacbinary";

  // init window ----------------------------------------------------------
  UINT width = 1366;
  UINT height = 768;
  gameInterface.windowWidth = ( u32 )width;
  gameInterface.windowHeight = ( u32 )height;
  {
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIconSm = wcex.hIcon = ( HICON )LoadImage(
      hInstance,
      MAKEINTRESOURCE( TACICON ),
      IMAGE_ICON,
      32,
      32,
      LR_DEFAULTSIZE );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszClassName = "TacWindowClass";
    if( !RegisterClassEx( &wcex ) )
    {
      unrecoverableErrors = "RegisterClassEx() failed\n";
      AppendWin32Error( unrecoverableErrors, GetLastError() );
      AppendFileInfo( unrecoverableErrors );
      return;
    }

    RECT rc = { 0, 0, ( LONG )width, ( LONG )height };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );

    g_hWnd = CreateWindowA(
      wcex.lpszClassName,
      "Tac",
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
      CW_USEDEFAULT, CW_USEDEFAULT,
      rc.right - rc.left,
      rc.bottom - rc.top,
      nullptr,
      nullptr,
      hInstance,
      nullptr );
    if( !g_hWnd )
    {
      unrecoverableErrors = "CreateWindowA() failed\n";
      AppendWin32Error( unrecoverableErrors, GetLastError() );
      AppendFileInfo( unrecoverableErrors );
      return;
    }

    ShowWindow( g_hWnd, nCmdShow );
  }

  // mouse tracking
  {
    mouseTracking = true;

    TRACKMOUSEEVENT mouseevent = {};
    mouseevent.cbSize = sizeof( TRACKMOUSEEVENT );
    mouseevent.dwFlags = 0x2; // TIME_LEAVE
    mouseevent.hwndTrack = g_hWnd;
    mouseevent.dwHoverTime = HOVER_DEFAULT;
    if( 0 == TrackMouseEvent( &mouseevent ) )
    {
      AppendWin32Error( unrecoverableErrors, GetLastError() );
      return;
    }
  }

  // renderer -------------------------------------------------------------
  void* rendererBlock = VirtualAlloc(
    0,
    sizeof( RendererDX11 ),
    MEM_RESERVE | MEM_COMMIT,
    PAGE_READWRITE );
  if( !rendererBlock )
  {
    unrecoverableErrors = "Failed to allocate renderer";
    AppendFileInfo( unrecoverableErrors );
    return;
  }
  renderer = new( rendererBlock ) RendererDX11(
    g_hWnd,
    unrecoverableErrors );
  if( unrecoverableErrors.size )
    return;
  OnDestruct( renderer->~TacRenderer(); );
  gameInterface.renderer = renderer;

  // imgui ----------------------------------------------------------------
  ImGui_ImplDX11_Init( g_hWnd );
  ImGui_ImplDX11_CreateDeviceObjects( unrecoverableErrors );
  if( unrecoverableErrors.size )
    return;
  OnDestruct( ImGui_ImplDX11_Shutdown(); );
  gameInterface.imguiInternalState = ImGui::GetInternalState();

  // game code ------------------------------------------------------------
  Win32GameCode gameCode = {};
  gameCode.onLoad = GameOnLoadStub;
  gameCode.onUpdate = GameUpdateStub;
  gameCode.onExit = GameOnExitStub;

  FixedString< 128 > srcDLL;
  FixedString< 128 > tmpDLL;
  FixedString< 128 > srcPDB;
  FixedString< 128 > tmpPDB;
  {

    srcDLL = dllname;
    FixedStringAppend( srcDLL, ".dll" );
    tmpDLL = dllname;
    FixedStringAppend( tmpDLL, "Open.dll" );
    srcPDB = dllname;
    FixedStringAppend( srcPDB, ".pdb" );
    tmpPDB = dllname;
    FixedStringAppend( tmpPDB, "Open.pdb" );
  }
  gameInterface.running = true;

  // threading ------------------------------------------------------------
  const u32 numWorkerThreads = 3;
  TacWorkQueue highPriorityQueue;
  highPriorityQueue.Init( numWorkerThreads, &gameInterface.running );
  win32State.memory.highPriorityQueue = &highPriorityQueue;
  thread.logicalThreadIndex = numWorkerThreads;
  gameInterface.thread = &thread;
  if( false )
  {
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  0" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  1" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  2" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  3" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  4" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  5" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  6" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  7" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  8" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String  9" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 10" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 11" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 12" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 13" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 14" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 15" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 16" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 17" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 18" );
    PushEntry( &highPriorityQueue, PrintStringCallback, "String 19" );
    CompleteAllWork( &highPriorityQueue, &thread );
  }

  // framerate /input ----------------------------------------------------
  gameInput.dt = 1.0f / 60.0f;
  gameInput.windowWidth = width;
  gameInput.windowHeight = height;
  LARGE_INTEGER largeInt;
  QueryPerformanceCounter( &largeInt );
  LONGLONG lastTimeCheck_win32 = largeInt.QuadPart;
  QueryPerformanceFrequency( &largeInt );
  const LONGLONG ticsPerSecond = largeInt.QuadPart;

  // game loop ------------------------------------------------------------

  while( gameInterface.running )
  { 
    r64 secsUntilNextUpdate = gameInput.dt;
    // prepare input for next frame
    gameInput.mouseXRel = 0;
    gameInput.mouseYRel = 0;
    gameInput.mouseWheelChange = 0;
    for( u32 i = 0; i < ( u32 )MouseButton::eCount; ++i )
    {
      gameInput.mouseButtonEndedDownPrev[ i ] =
        gameInput.mouseButtonEndedDown[ i ];
    }
    for( u32 i = 0; i < ( u32 )KeyboardKey::eCount; ++i )
    {
      gameInput.keyboardKeyEndedDownPrev[ i ] =
        gameInput.keyboardKeyEndedDown[ i ];
    }

    MSG msg = { 0 };
    while( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }

    // handle errors that came from wndproc
    if( gameInterface.gameUnrecoverableErrors.size )
    {
      unrecoverableErrors = gameInterface.gameUnrecoverableErrors;
      return;
    }

    if( win32State.iRecord )
    {
      InputRecordUpdate(
        &thread,
        win32State,
        gameInput,
        unrecoverableErrors );
      if( unrecoverableErrors.size )
        return;
    }
    if( win32State.iPlayback )
    {
      InputPlaybackUpdate(
        &thread,
        win32State,
        gameInput,
        unrecoverableErrors );
      if( unrecoverableErrors.size )
        return;
    }

    ImGui_ImplDX11_NewFrame( g_hWnd );

    gameCode.onUpdate( gameInterface );
    gameInput.elapsedTime += gameInput.dt;
    gameInput.elapsedFrames++;
    if( gameInterface.gameUnrecoverableErrors.size )
    {
      unrecoverableErrors = gameInterface.gameUnrecoverableErrors;
      return;
    }

    if( win32State.iPlayback )
    {
      if( ImGui::Button( "Stop Playback" ) )
      {
        InputPlaybackEnd( &thread, win32State, unrecoverableErrors );
        if( unrecoverableErrors.size )
        {
          return;
        }
      }
    }
    else if( win32State.iRecord )
    {
      if( ImGui::Button( "Stop Recording" ) )
      {
        InputRecordEnd( &thread, win32State, unrecoverableErrors );
        if( unrecoverableErrors.size )
        {
          return;
        }
      }
    }
    else
    {
      if( ImGui::Button( "Start Recording" ) )
      {
        InputRecordBegin( &thread, win32State, 1, unrecoverableErrors );
        if( unrecoverableErrors.size )
        {
          return;
        }
      }
      if( ImGui::Button( "Start Playback" ) )
      {
        InputPlaybackBegin( &thread, win32State, 1, unrecoverableErrors );
        if( unrecoverableErrors.size )
        {
          return;
        }
      }
    }

    ImGui::Render();

    gameInterface.renderer->SwapBuffers();


    const double secondsPerReload = 1.0f; 
    static double secondsLeftTillNextReload = 0;
    secondsLeftTillNextReload -= gameInput.dt;
    if( secondsLeftTillNextReload <= 0 )
    {
      secondsLeftTillNextReload = secondsPerReload;

      do
      {
        FixedString< DEFAULT_ERR_LEN > reloadErrors = {};
        TacFile handle = PlatformOpenFile(
          &thread,
          srcDLL.buffer,
          OpenFileDisposition::OpenExisting,
          FileAccess::Read,
          reloadErrors  );
        if( reloadErrors.size )
          break;

        time_t lastWrite = PlatformGetFileModifiedTime(
          &thread,
          handle,
          reloadErrors );
        if( reloadErrors.size )
          break;

        PlatformCloseFile( &thread, handle, reloadErrors );
        if( reloadErrors.size )
          break;

        if( lastWrite == gameCode.lastWrite )
          break;

        Win32UnloadGameCode( &gameCode );
        gameCode = Win32LoadGameCode(
          srcDLL.buffer,
          tmpDLL.buffer,
          srcPDB.buffer,
          tmpPDB.buffer,
          reloadErrors );
        if( reloadErrors.size )
          break;

        gameCode.onLoad( gameInterface );
        if( gameInterface.gameUnrecoverableErrors.size )
        {
          unrecoverableErrors = gameInterface.gameUnrecoverableErrors;
          return;
        }

      } while( false );
    }

    do 
    {
      // see how much time is left until the next frame
      QueryPerformanceCounter( &largeInt );
      secsUntilNextUpdate -=
        ( r64 )( largeInt.QuadPart - lastTimeCheck_win32 ) /
        ( r64 )ticsPerSecond;
      lastTimeCheck_win32 = largeInt.QuadPart;

      // don't eat all resources if we have ample time remaining
      if( secsUntilNextUpdate * 1000.0 > 2.0 )
      {
        Sleep( 1 );
      }
    } while ( secsUntilNextUpdate > 0 );

  }

  gameCode.onExit( gameInterface );
}

void HandleErrors( FixedString< DEFAULT_ERR_LEN >& mainErrors )
{
  if( mainErrors.size == 0 )
    return;
  OutputDebugString( mainErrors.buffer );
  std::ofstream ofs( "tacErrors.txt" );
  if( ofs.is_open() )
  {
    ofs << mainErrors.buffer << std::endl;
    ofs.close();
  }
  FixedString< DEFAULT_ERR_LEN > msgBoxStr;
  msgBoxStr = mainErrors;
  FixedStringAppend(
    msgBoxStr,
    "\n\tPlease perform the following steps:"
    "\n\t1. Press <Ctrl + C> ( you will hear a ding )"
    "\n\t2. Open Skype"
    "\n\t3. Find Nathan"
    "\n\t4. Press <Ctrl + V> and hit Enter" );
  MessageBox( nullptr, msgBoxStr.buffer, "Error", MB_OK );
}

int WINAPI wWinMain(
  _In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPWSTR lpCmdLine,
  _In_ int nCmdShow )
{
  UNREFERENCED_PARAMETER( hPrevInstance );
  UNREFERENCED_PARAMETER( lpCmdLine );
  FixedString< DEFAULT_ERR_LEN > mainErrors = {};
  const char* projects[] = 
  {
    "tacModelConverter",
    "tacDemo",
  };
  RunGame( hInstance, nCmdShow, projects[ 1 ], mainErrors );
  HandleErrors( mainErrors );
  return 0;
}
