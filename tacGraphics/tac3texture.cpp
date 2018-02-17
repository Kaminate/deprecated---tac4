#include "tac3texture.h"
#include "tacLibrary/tacPlatform.h"
#include <stdio.h>
void Texture::Clear()
{
  glDeleteTextures( 1, &mHandle );
  mHandle = 0;
  // NOTE( N8 ):
  // not clearing mProperties, so the texture can easily be reloaded
}

void Texture::Load( 
  const Properties & props,
  void * pixelData ) 
{
  mProperties = props;
  glGenTextures(1, &mHandle);
  glBindTexture(props.mTarget, mHandle);
  switch (props.mTarget)
  {
  case GL_TEXTURE_1D: 
    glTexImage1D(
      props.mTarget,
      0,
      props.mInternalFormat,
      props.mWidth,
      0,
      props.mFormat,
      props.mType,
      pixelData);
    break;
  case GL_TEXTURE_2D: 
    glTexImage2D(
      props.mTarget,
      0, // level
      props.mInternalFormat,
      props.mWidth,
      props.mHeight,
      0, // border
      props.mFormat,
      props.mType,
      pixelData);
    break;
  case GL_TEXTURE_3D: 
    glTexImage3D(
      props.mTarget,
      0,
      props.mInternalFormat,
      props.mWidth,
      props.mHeight,props.mDepth,
      0,
      props.mFormat,
      props.mType,
      pixelData);
    break;
  }
  glTexParameterf(mProperties.mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(mProperties.mTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(mProperties.mTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(mProperties.mTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(mProperties.mTarget, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(mProperties.mTarget, GL_TEXTURE_MAX_LEVEL, 0);
  glBindTexture(props.mTarget, 0);
}

FixedString< DEFAULT_ERR_LEN > Texture::Load(
  const char* filename,
  GLint internalFormat)
{
  Image myImage;
  FixedString< DEFAULT_ERR_LEN > errors = myImage.Load(filename);
  if(errors.size == 0 )
  {
    Load(myImage, internalFormat);
  }
  return errors;
}

void Texture::Load( 
  const Image & myImage,
  GLint internalFormat )
{
  Clear();
  mProperties.mInternalFormat = internalFormat;
  mProperties.mTarget = GL_TEXTURE_2D;
  mProperties.mDepth = 0;
  mProperties.mWidth = myImage.mWidth;
  mProperties.mHeight = myImage.mHeight;

  // NOTE( N8 ): During Image::Load, FreeImage converts to 32bit bgra
  TacAssert( myImage.mBitsPerPixel == 32 );
  mProperties.mFormat = GL_BGRA;

  glGenTextures(1, &mHandle);
  glBindTexture(mProperties.mTarget, mHandle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(
    mProperties.mTarget,
    mProperties.mDepth,
    mProperties.mInternalFormat, // actual format of how to store tex
    mProperties.mWidth,
    mProperties.mHeight,
    0,
    mProperties.mFormat,
    GL_UNSIGNED_BYTE, // pixel data format
    myImage.mBytes);

  glTexParameterf(mProperties.mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(mProperties.mTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(mProperties.mTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(mProperties.mTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(mProperties.mTarget, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(mProperties.mTarget, GL_TEXTURE_MAX_LEVEL, 0);

  glBindTexture(mProperties.mTarget, 0);
}

static void FreeImageErrorHandler(
  FREE_IMAGE_FORMAT fif,
  const char *message ) 
{
  printf("\n*** "); 
  if(fif != FIF_UNKNOWN) {
    printf("%s Format\n", FreeImage_GetFormatFromFIF(fif));
  }
  printf(message);
  printf(" ***\n");
  __debugbreak();
  TacAssert( false );
}

FixedString< DEFAULT_ERR_LEN > Image::Load( const char* filename )
{
  static bool once = [](){
    FreeImage_SetOutputMessage(FreeImageErrorHandler);
    return true;
  }();

  Clear();

  FixedString< DEFAULT_ERR_LEN > errors = {};
  do
  {
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename, 0);
    if(fif == FIF_UNKNOWN) 
      fif = FreeImage_GetFIFFromFilename(filename);
    if(fif == FIF_UNKNOWN)
    {
      errors = "unknown image type: ";
      FixedStringAppend( errors, filename );
      break;
    }

    if(FreeImage_FIFSupportsReading(fif))
    {
      mBitmap = FreeImage_Load(fif, filename);
    }
    if( mBitmap )
    {
      FIBITMAP* bitmap32 = FreeImage_ConvertTo32Bits(mBitmap);
      if( bitmap32 )
      {
        FreeImage_Unload( mBitmap );
        mBitmap = bitmap32;
      }
      else
      {
        errors.size = sprintf_s(
          errors.buffer,
          "Image failed to convert to 32bits "
          "(non standard bitmap type ): %s", filename );
        break;
      }
    }
    else
    {
      errors.size = sprintf_s( errors.buffer, "Image failed to load: %s", filename );
      break;
    }

    mWidth = FreeImage_GetWidth(mBitmap);
    if(mWidth == 0)
    {
      errors.size = sprintf_s( errors.buffer, "Image failed to get width: %s", filename );
      break;
    }
    mHeight = FreeImage_GetHeight(mBitmap);
    if(mHeight == 0)
    {
      errors.size = sprintf_s( errors.buffer, "Image failed to get height: %s", filename );
      break;
    }

    mPitch = FreeImage_GetPitch(mBitmap);

    mBitsPerPixel= FreeImage_GetBPP(mBitmap);

    mBytes = FreeImage_GetBits(mBitmap);
    if(!mBytes)
    {
      errors.size = sprintf_s( errors.buffer, "Image failed to get bits: %s", filename );
      break;
    }

  } while (false);

  return errors;
}
Image::Image() : mBitmap(nullptr)
{
  Clear();
}
Image::Image( const Image & rhs ):
  mBitmap(FreeImage_Clone(rhs.mBitmap)),
  mWidth(rhs.mWidth),
  mHeight(rhs.mHeight),
  mPitch(rhs.mPitch),
  mBitsPerPixel(rhs.mBitsPerPixel),
  mBytes(FreeImage_GetBits(mBitmap))
{
}
Image::~Image()
{
  Clear();
}
void Image::Clear()
{
  if(mBitmap)
    FreeImage_Unload(mBitmap);
  mBitmap = nullptr;
  mWidth = 0;
  mHeight = 0;
  mPitch = 0;
  mBitsPerPixel = 0;
  mBytes = nullptr;
}
Image & Image::operator=( const Image & rhs )
{
  Clear();
  mBitmap = FreeImage_Clone(rhs.mBitmap);
  mWidth = rhs.mWidth;
  mHeight = rhs.mHeight;
  mPitch = rhs.mPitch;
  mBitsPerPixel = rhs.mBitsPerPixel;
  mBytes = FreeImage_GetBits(mBitmap);
  return *this;
}

FixedString< DEFAULT_ERR_LEN > Image::Save( const char* filepath )
{
  FixedString< DEFAULT_ERR_LEN > errors = {};

  FREE_IMAGE_FORMAT fif 
    = FreeImage_GetFIFFromFilename( filepath );
  if(fif == FIF_UNKNOWN)
  {
    errors.size = sprintf_s(
      errors.buffer,
      "Failed to save image %s\n"
      "Unknown image format", filepath );
  }
  else
  {
    bool success = 0 != FreeImage_Save( fif, mBitmap, filepath );

    if(!success)
    {
      errors.size = sprintf_s(
        errors.buffer,
        "Failed to save %s",
        filepath );
    }
  }
  return errors;
}


