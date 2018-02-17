#pragma once

#include "tacGraphics\tacRenderer.h"
#include "tacGraphics\tacResourceManagerV2.h"
#include "tacGraphics\tac4Model.h"

#include "tacLibrary\tacPlatform.h"
#include "tacLibrary\tacMath.h"

#include "3rdparty\assimp\Importer.hpp"  // C++ importer interface
#include "3rdparty\assimp\scene.h"       // Output data structure
#include "3rdparty\assimp\postprocess.h" // Post processing flags

struct TacGameState
{
  // camera
  m4 view;
  m4 proj;
  v3 camPos;
  v3 camDir;
  v3 camWorldUp;
  v3 camPosOriginal;
  v3 camDirOriginal;
  v3 camWorldUpOriginal;
};
