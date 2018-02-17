#include "tac3rendertexture.h"
#include "tacGraphics\gl_core_4_4.h"
#include "tacLibrary\tacString.h"

void RenderTextureInit(
  RenderTexture& tex,
  int width,
  int height,
  RenderTexture::ColorType color,
  RenderTexture::DepthType depth,
  FixedString< DEFAULT_ERR_LEN >& errors,
  bool ms_useFiltering )
{
  GLenum err;
  tex.m_width = width;
  tex.m_height = height;
  glGenFramebuffers( 1, &tex.m_fboId);
  glBindFramebuffer( GL_FRAMEBUFFER, tex.m_fboId );
  err = glGetError();

  if( color >= RenderTexture::ColorType::HDR )
  {
    // Create the HDR render target
    switch( color )
    {
    case RenderTexture::ColorType::RGB16F:
      tex.m_format = GL_RGB;
      tex.m_type = GL_HALF_FLOAT;
      break;
    default:
    case RenderTexture::ColorType::RGBA16F:
      tex.m_format = GL_RGBA;
      tex.m_type = GL_HALF_FLOAT;
      break;
    }
    glGenTextures( 1, &tex.m_texId );
    glBindTexture( GL_TEXTURE_2D, tex.m_texId );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ms_useFiltering ? GL_LINEAR : GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ms_useFiltering ? GL_LINEAR : GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    //note that internalformat and format have both to be GL_RGB or GL_RGBA for creating floating point texture.
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, 0 );
    err = glGetError();
    glBindTexture( GL_TEXTURE_2D, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.m_texId, 0 );
    err = glGetError();
  }
  else
  {
    // Create the LDR render target
    tex.m_type = GL_UNSIGNED_BYTE;
    glGenTextures( 1, &tex.m_texId );
    glBindTexture( GL_TEXTURE_2D, tex.m_texId );
    switch ( color )
    {
    case RenderTexture::ColorType::RGBA8888:
      tex.m_format = GL_RGBA;
      break;
    default:
    case RenderTexture::ColorType::RGB888:
      tex.m_format = GL_RGB;
      break;
    }
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    glTexImage2D( GL_TEXTURE_2D, 0, tex.m_format, width, height, 0, tex.m_format, tex.m_type, 0 );

    glBindTexture( GL_TEXTURE_2D, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.m_texId, 0 );
  }
  // Create depth buffer
  if( depth != RenderTexture::DepthType::NoDepth )
  {
    glGenRenderbuffers( 1, &tex.m_depthRBId );
    glBindRenderbuffer( GL_RENDERBUFFER, tex.m_depthRBId );
    err = glGetError();
    unsigned int depth_format = 0;
    switch ( depth )
    {
    case RenderTexture::DepthType::Depth24:
      depth_format = GL_DEPTH_COMPONENT24;
      break;
    default:
    case RenderTexture::DepthType::Depth16:
      depth_format = GL_DEPTH_COMPONENT16;
      break;
    }
    glRenderbufferStorage( GL_RENDERBUFFER, depth_format, width, height );
    err = glGetError();
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tex.m_depthRBId );
    err = glGetError();
    glBindRenderbuffer( GL_RENDERBUFFER, 0 );    
  }
  GetCurrentlyBoundFBOErrorString( errors );

  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}
void RenderTextureFreeResources( RenderTexture& tex  )
{
  glDeleteFramebuffers( 1, &tex.m_fboId );
  tex.m_fboId = 0;

  glDeleteRenderbuffers( 1, &tex.m_depthRBId );
  tex.m_depthRBId = 0;
  
  glDeleteTextures( 1, &tex.m_texId );
  tex.m_texId = 0;
}

void RenderTextureActivate( RenderTexture& tex )
{
  glBindFramebuffer( GL_FRAMEBUFFER, tex.m_fboId ); 
  glViewport( 0, 0, tex.m_width, tex.m_height );
}

void GetCurrentlyBoundFBOErrorString( FixedString< DEFAULT_ERR_LEN >& errors )
{ 
  GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
  switch( status )
  {
#define SetStatusString( fboCase ) case fboCase: errors = #fboCase; break
    SetStatusString( GL_FRAMEBUFFER_UNDEFINED );
    SetStatusString( GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT );
    SetStatusString( GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT  );
    SetStatusString( GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER );
    SetStatusString( GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER );
    SetStatusString( GL_FRAMEBUFFER_UNSUPPORTED );
    SetStatusString( GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE  );
    SetStatusString( GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS );
case 0: errors = "GL_INVALID_ENUM is generated if target is "
          "not GL_DRAW_FRAMEBUFFER, "
          "GL_READ_FRAMEBUFFER or GL_FRAMEBUFFER."; break;
case GL_FRAMEBUFFER_COMPLETE: break;// do nothing
default: errors = "???"; break;
#undef SetStatusString
  }
}
