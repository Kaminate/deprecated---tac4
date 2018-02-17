#pragma once

#include "tacRenderer.h"
#include "tacLibrary/tacPlatformWin32.h"
#include "tacLibrary/tacMath.h"
#include "tacLibrary/tacFilesystem.h"
#include "tacGraphics/tac4Animation.h"
#include <vector>

struct ModelBuffers
{
  TacIndexBufferHandle indexBufferHandle;
  TacVertexBufferHandle vertexBufferHandles[ 10 ];
  u32 numVertexBuffers;
};

// TODO: replace vertexFormats with myRendererID
// 10 arguments... too many?
ModelBuffers ModelBuffersCreate(
  TacRenderer* myRenderer,
  u32 mNumVertices,
  void** vertexBufferDatas,
  u32* vertexBufferStrides,
  u32 numVertexBuffers,
  u32 numIndexes,
  void* indexesData,
  TacTextureFormat indexTextureFormat,
  u32 indexesTotalBufferSize,
  //TacVariableType indexVariableType,
  //u32 indexNumBytes,
  FixedString< DEFAULT_ERR_LEN >& errors );


void ModelBuffersRender(
  TacRenderer* myRenderer,
  const ModelBuffers& buffers );

struct TacSubModel
{
  ModelBuffers buffers;
  TacTextureHandle myDiffuseID;
  TacTextureHandle myNormalID;
  TacTextureHandle mySpecularID;
  std::vector< TacBone > bones;
};
#define MAX_SUBMODELS 10

struct TacModel
{
  TacSubModel subModels[ MAX_SUBMODELS ];
  u32 numSubModels;
  std::vector< TacNode > skeleton;
  std::vector< TacAnimation > animations;
};

struct TacSubModelInstance
{
  // NOTE( N8 ):
  //   These matrixes are sent to the vertex shader.
  //   They transform a vertex from model space to
  //   animated model space
  //
  //   The size of this matrix is equal to
  //   the number of bones in the submodel
  //
  //   v4[ animated position in model space ] =
  //     m4[ finalMatrix ] *
  //     v4[ position in model space ]
  //   
  //   where m4[ finalMatrix ] = 
  //     m4[ anim bone to model space ] *
  //     m4[ model to bone space ] *
  //
  //  In other words, a vertex transform through
  //    1. Model space 
  //    2a. Bone space ( relative to the bone in bind pose )
  //    2b. Bone space ( relative to the bone in animated pose )
  //    3. Model space
  std::vector< m4 > finalMatrixes;

  TacTextureHandle diffuseOverride;
  TacTextureHandle normalOverride;
  TacTextureHandle specularOverride;
};

struct TacModelInstance
{
  m4 world;
  v3 scale;
  v3 rotation;
  v3 translation;

  std::vector< TacSubModelInstance > subModelInstances;
  const TacModel* model;

  TacAnimationDriver* myAnimationDriver;

  // transforms from current node space to model space
  std::vector< m4 > animatedTransforms;
  void CopyAnimatedTransforms();
  void ConcatinateAnimatedTransforms();
};

