#pragma once

#include "tacLibrary\tacPlatform.h"
#include "tacLibrary\tacMath.h"
#include "tacGraphics\tacRenderer.h"
#include "tacGraphics\tacResourceManagerV2.h"

struct TacGameState
{
  TacShaderHandle myShader;
  m4 world;
  m4 world2;
  m4 view;
  m4 proj;
  v3 camPos;
  v3 camDir;
  v3 camWorldUp;
  TacDepthStateHandle myRendererID;

  TacResourceManagerV2 myResourceManager;
};
