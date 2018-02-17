#pragma once
#include "tacLibrary\tacString.h"

struct RenderTexture
{
public:
	enum class DepthType
  {
		Depth24,
		Depth16,
		NoDepth
	};
	enum class ColorType
  {
		RGB888,
		RGBA8888,
		HDR = 0xFFFF,
		RG16F,
		RGB16F,
		RGBA16F,
	};

	int m_width;
	int m_height;
	unsigned int m_format;
	unsigned int m_type;
	unsigned int m_fboId;
	unsigned int m_texId;
	unsigned int m_depthRBId;
};
void RenderTextureInit(
  RenderTexture& tex,
  int w,
  int h,
  RenderTexture::ColorType color,
  RenderTexture::DepthType depth,
  FixedString< DEFAULT_ERR_LEN >& errors,
  bool useFiltering = true );
void RenderTextureFreeResources( RenderTexture& tex );
void RenderTextureActivate( RenderTexture& tex );
void GetCurrentlyBoundFBOErrorString( FixedString< DEFAULT_ERR_LEN >& errors );
