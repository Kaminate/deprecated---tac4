#pragma once

#include "tacGraphics/gl_core_4_4.h"
#include "tacLibrary/tacFilesystem.h"
#include "tacLibrary/tacString.h"
#include "3rdparty/freeimage/FreeImage.h"

class Image
{
public:
  Image(const Image & rhs);
  Image & operator =(const Image & rhs);
  Image();
  ~Image();
  void Clear();
  FixedString< DEFAULT_ERR_LEN > Load( const char* filepath );
  FixedString< DEFAULT_ERR_LEN >  Save( const char* filepath );
  FIBITMAP * mBitmap;
  int mWidth;
  int mHeight;
  int mPitch;
  int mBitsPerPixel;
  BYTE * mBytes;
};

class Texture
{
public:
  struct Properties
  {
    unsigned mWidth;
    unsigned mHeight;
    unsigned mDepth; // only used in 3d textures.......
    GLuint mTarget; // ie: GL_TEXTURE_2D
    GLuint mFormat; // ex: GL_BGRA, GL_RGB, GL_RED, GL_DEPTH_COMPONENT
    GLuint mType; // ex: GL_FLOAT
    GLuint mInternalFormat; // ex: GL_RGBA8
    // numMips?
  };
  void Clear();
  void Load( 
    const Properties & props,
    void * pixelData);
  FixedString< DEFAULT_ERR_LEN > Texture::Load(
    const char* filename,
    GLint internalFormat);
  void Texture::Load( 
    const Image & myImage,
    GLint internalFormat);


  GLuint mHandle;
  Properties mProperties;
};


// TODO: make this take GL_TEXTURE_2D as an argument
inline void SendTexture( GLuint textureLocation, GLuint textureHandle, u32 name )
{
  // https://open.gl/textures

  // The function glActiveTexture specifies which texture unit a
  // texture object is bound to when glBindTexture is called.
  glActiveTexture( GL_TEXTURE0 + name );
  glBindTexture( GL_TEXTURE_2D, textureHandle );

  // so it shouldn't matter when glUniform1i is called,
  // as long as glBindTexture is called after glActiveTexture
  glUniform1i( textureLocation, name );
}


inline void SendTheTexture(
  GLuint program,
  const char* samplerName,
  GLuint texHandle,
  int name )
{
  GLint iTexture = glGetUniformLocation( program, samplerName );
  TacAssert( iTexture != -1 );
  SendTexture( iTexture, texHandle, name );
}
inline void SendTheTexture(
  GLuint program,
  const char* samplerName,
  Texture& tex,
  int name )
{
  SendTheTexture( program, samplerName, tex.mHandle, name );
}

