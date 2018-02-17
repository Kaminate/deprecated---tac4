#include "tacGame.h"
#include "tacLibrary\tacPlatform.h"
#include "tacLibrary\tacMath.h"
#include "tacLibrary\imgui\imgui.h"
#include "tacGraphics\tac3camera.h"
#include "tacGraphics\tacRenderer.h"
#include <stdio.h>

extern "C" __declspec( dllexport )
void GameOnLoad(
TacGameInterface& onLoadInterface )
{
  ImGui::SetInternalState( onLoadInterface.imguiInternalState );

  if( !onLoadInterface.gameMemory->initialized )
  {
    onLoadInterface.gameMemory->initialized = true;
    if( sizeof( TacGameState ) >
      onLoadInterface.gameMemory->permanentStorageSize )
    {
      onLoadInterface.gameUnrecoverableErrors.size = sprintf_s(
        onLoadInterface.gameUnrecoverableErrors.buffer,
        "Not enough memory( size: %i ) to store GameState( size: %i )",
        onLoadInterface.gameMemory->permanentStorageSize,
        sizeof( TacGameState ) );
      return;
    }
    TacGameState* state =
      ( TacGameState* )onLoadInterface.gameMemory->permanentStorage;

    ResourceManagerV2Init(
      state->myResourceManager,
      onLoadInterface.renderer,
      onLoadInterface.gameUnrecoverableErrors );
    if( onLoadInterface.gameUnrecoverableErrors.size )
      return;

    // world data ---------------------------------------------------------
    {
      Identity( state->world ); 
      Identity( state->world2 ); 

      state->camPos = V3( 0.0f, 3.0f, 5.0f );
      state->camDir = -Normalize( state->camPos );
      state->camWorldUp = V3( 0.0f, 1.0f, 0.0f );
      r32 aspect =
        ( r32 ) onLoadInterface.windowWidth /
        ( r32 ) onLoadInterface.windowHeight;
      ComputeViewMatrix(
        state->view,
        state->camPos,
        state->camDir,
        state->camWorldUp );
      r32 f = 100.0f;
      r32 n = 0.1f;
      r32 a, b;
      onLoadInterface.renderer->GetPerspectiveProjectionAB( f, n, a, b );
      ComputePerspectiveProjMatrix(
        a, b, state->proj, 90.0f * DEG2RAD, aspect );
    }

    // compile shaders ----------------------------------------------------
    {
      const char* shaderPaths[ ( u32 )TacShaderType::Count ] = {};
      const char* entryPoints[ ( u32 )TacShaderType::Count ] = {};
      const char* shaderModels[ ( u32 )TacShaderType::Count ] = {};

      shaderPaths[ ( u32 )TacShaderType::Vertex ] = 
        "tacData/Shaders/Directx/Tutorial02.fx";
      entryPoints[ ( u32 )TacShaderType::Vertex ] = "VS";
      shaderModels[ ( u32 )TacShaderType::Vertex ] = "vs_4_0";

      shaderPaths[ ( u32 )TacShaderType::Fragment ] =
        "tacData/Shaders/Directx/Tutorial02.fx";
      entryPoints[ ( u32 )TacShaderType::Fragment ] = "PS";
      shaderModels[ ( u32 )TacShaderType::Fragment ] = "ps_4_0";

      state->myShader = onLoadInterface.renderer->LoadShader(
        shaderPaths,
        entryPoints,
        shaderModels,
        onLoadInterface.gameUnrecoverableErrors );
      if( onLoadInterface.gameUnrecoverableErrors.size )
        return;
    }

    // constant buffer ----------------------------------------------------
    {
      TacConstant constants[ 3 ] = {};
      TacConstant* c = constants;
      u32 offset = 0;

      c->name = "World";
      c->size = 64;
      c->offset = offset;
      offset += c->size;
      ++c;

      c->name = "View";
      c->size = 64;
      c->offset = offset;
      offset += c->size;
      ++c;

      c->name = "Projection";
      c->size = 64;
      c->offset = offset;
      offset += c->size;
      ++c;

      TacCBufferHandle myRendererID = onLoadInterface.renderer->LoadCbuffer(
        "ConstantBuffer",
        constants,
        ArraySize( constants ),
        onLoadInterface.gameUnrecoverableErrors );
      if( onLoadInterface.gameUnrecoverableErrors.size )
        return;

      onLoadInterface.renderer->AddCbuffer(
        state->myShader,
        myRendererID,
        0,
        TacShaderType::Vertex,
        onLoadInterface.gameUnrecoverableErrors );
      if( onLoadInterface.gameUnrecoverableErrors.size )
        return;
    }

    // depth state --------------------------------------------------------
    {
    state->myRendererID = onLoadInterface.renderer->AddDepthState(
      true,
      true,
      TacDepthFunc::Less,
      onLoadInterface.gameUnrecoverableErrors  );
    if( onLoadInterface.gameUnrecoverableErrors.size )
      return;
    }
  }
}

extern "C" __declspec( dllexport ) 
  void GameUpdateAndRender(
  TacGameInterface& updateInterface )
{
  TacGameState* state = ( TacGameState* )updateInterface.gameMemory->permanentStorage;

  if( updateInterface.gameInput->keyboardKeyEndedDown[ ( u32 )KeyboardKey::Escape ] )
  {
    updateInterface.running = false;
  }


  // update entities ------------------------------------------------------
  {
    state->world = M4Transform(
      V3< r32 >( 1, 1, 1 ),
      V3< r32 >( ( r32 )updateInterface.gameInput->elapsedTime, 0, 0 ),
      V3< r32 >( 0, 0, 0 ) );
    state->world2 =
      M4RotRadY( ( r32 )updateInterface.gameInput->elapsedTime ) *
      M4Translate( V3< r32 >( 3, 0, 0 ) );
  }

  // update camera --------------------------------------------------------
  {
    v3 addition = {};
    if( updateInterface.gameInput->keyboardKeyEndedDown[ ( u32 )KeyboardKey::W ] )
    {
      addition.y += 1.0f;
    }
    if( updateInterface.gameInput->keyboardKeyEndedDown[ ( u32 )KeyboardKey::A ] )
    {
      addition.x -= 1.0f;
    }
    if( updateInterface.gameInput->keyboardKeyEndedDown[ ( u32 )KeyboardKey::S ] )
    {
      addition.y -= 1.0f;
    }
    if( updateInterface.gameInput->keyboardKeyEndedDown[ ( u32 )KeyboardKey::D ] )
    {
      addition.x += 1.0f;
    }
    if( LengthSq( addition ) > 0.1f )
    {
      addition = Normalize( addition ) * 0.01f;
      state->camPos += addition;
      state->camDir = -Normalize( state->camPos );
      ComputeViewMatrix(
        state->view,
        state->camPos,
        state->camDir,
        state->camWorldUp );
    }
  }

  // render ---------------------------------------------------------------
  updateInterface.renderer->ClearColor(
    updateInterface.renderer->GetBackbufferColor(),
    V4( 1.0f, 0.5f, 0.0f, 1.0f ) );

  updateInterface.renderer->ClearDepthStencil( 
    updateInterface.renderer->GetBackbufferDepth(),
    true, 1.0f, false, 0 );
  {
    updateInterface.renderer->SetActiveShader( state->myShader );
    updateInterface.renderer->SendUniform(
      "World", &state->world, sizeof( state->world ) );
    updateInterface.renderer->SendUniform(
      "View", &state->view, sizeof( state->view ) );
    updateInterface.renderer->SendUniform(
      "Projection", &state->proj, sizeof( state->proj ) );
    updateInterface.renderer->SetDepthState( state->myRendererID, 0 );
    updateInterface.renderer->SetScissorRect(
      0, 0, 
      ( r32 )updateInterface.gameInput->windowWidth, 
      ( r32 )updateInterface.gameInput->windowHeight );
    updateInterface.renderer->Apply();
    // draw model
    updateInterface.renderer->SendUniform(
      "World", &state->world2, sizeof( state->world2 ) );
    updateInterface.renderer->Apply();
    // draw model
  }

  localPersist bool showTestWindow = true;
  if( showTestWindow )
  {
    ImGui::SetNextWindowPos(
      ImVec2( 20, 20 ),
      ImGuiSetCond_Once );
    ImGui::ShowTestWindow( &showTestWindow );
  }
}
