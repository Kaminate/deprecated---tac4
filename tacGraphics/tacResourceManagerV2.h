#pragma once
#include "tacLibrary\tacFilesystem.h"
#include "tacLibrary\tacMath.h"
#include "tacRenderer.h"
#include "tac4Model.h"
#include "tac4Animation.h"
#include <vector>




struct TacResourceManagerV2
{
  TacTextureHandle defaultTextureBlack;
  TacTextureHandle defaultTextureDarkGrey;
  TacTextureHandle defaultTextureWhite;
  TacTextureHandle defaultTextureRed;
  TacTextureHandle defaultTextureGreen;
  TacTextureHandle defaultTextureBlue;
  TacTextureHandle defaultTextureNormal;
};

void ResourceManagerV2Init(
  TacResourceManagerV2& resources,
  TacRenderer* myRenderer,
  FixedString< DEFAULT_ERR_LEN >& errors );
