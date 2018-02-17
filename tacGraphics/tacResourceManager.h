#pragma once
#include "tac3geometry.h"
#include "tac3texture.h"
#include "tacLibrary\tacFilesystem.h"
#include "tacLibrary\tacMath.h"
#include "tacGraphics\gl_core_4_4.h"

struct TacMemoryManager;

//struct Renderer
//{
//  struct Impl;
//  char implData[ 100 ];
//};
//void RendererInitFunctions();

struct TacModel
{
  static const u32 MAX_GEOMS = 32;
  u32 iGeoms[ MAX_GEOMS ];
  u32 iMaterials[ MAX_GEOMS ];

  // NOTE( N8 ): indexes into both iGeoms array and iMaterials array
  u32 numGeoms;


  //Filepath path;
};

struct Light
{
  m4 world;
  r32 radius;
  v3 diffuse;

  b32 castsShadows;
  GLuint shadowFBO;
  Texture shadowDepth;
  Texture shadowDepthColorAttachment;

  r32 shadowMapCameraNear;
  r32 shadowMapCameraFar;
  r32 shadowMapCameraFOVYRad;
  m4 shadowMapCameraViewMatrix;
  m4 shadowMapCameraProjMatrix;
  v3 shadowMapCameraWSViewDir;
  v3 shadowMapCameraWorldUp;
};

struct MeshRenderer
{
  m4 world;
  m4 worldInverseTranspose;
  TacModel modl;
};

struct Material
{
  u32 iDiffuse;
  u32 iNormal;
  u32 iSpecular;
};

struct ResourceManager
{
  // index 0 is resolved for defaults

  GPUGeometry geometries[ 32 ];
  u32 numGeom;
  u32 defaultGeometryNDCQuad;
  u32 defaultGeometryUnitCube;
  u32 defaultGeometryUnitSphere;

  Texture textures[ 32 ];
  u32 numTextures;

  u32 defaultTextureBlack;
  u32 defaultTextureDarkGrey;
  u32 defaultTextureWhite;
  u32 defaultTextureRed;
  u32 defaultTextureGreen;
  u32 defaultTextureBlue;
  u32 defaultTextureNormal;

  Material materials[ 32 ];
  u32 numMaterials;

  TacModel models[ 32 ];
  u32 numModels;

  struct Node
  {
    enum class Type 
    { 
      eDirectory = 0,
      eTexture,
      eModel, 
    } type;
    u32 index;
    u32 next;
    u32 child;
    Filepath path;
  };
  Node nodes[ 512 ];
  u32 numNodes;
};
void ResourceManagerInit( ResourceManager& resources );
ResourceManager::Node& GetFolder( const char* path );

TacModel LoadModelAssip(
  TacMemoryManager& memory,
  const Filepath& path,
  ResourceManager& resources,
  FixedString< DEFAULT_ERR_LEN >& errors );

