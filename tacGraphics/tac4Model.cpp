#include "tac4Model.h"
#include <queue>

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
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  ModelBuffers buffers = {};
  [&]()
  {
    // initialize stuff
    {
      buffers.indexBufferHandle.mIndex = RENDERER_ID_NONE;

      // initialze vertexbuffers to null
      if( numVertexBuffers > ArraySize( buffers.vertexBufferHandles ) )
      {
        errors = "Not enough vertex buffers\n";
        AppendFileInfo( errors );
        return;
      }
      buffers.numVertexBuffers = numVertexBuffers;
      for( u32 i = 0; i < numVertexBuffers; ++i )
      {
        buffers.vertexBufferHandles[ i ].mIndex = RENDERER_ID_NONE;
      }
    }


    // destroy stuff if there were initialization errors
    b32 freeData = true;
    OnDestruct(
      if( freeData )
      {
        for( u32 i = 0; i < numVertexBuffers; ++i )
        {
          TacVertexBufferHandle& vboID = buffers.vertexBufferHandles[ i ];
          // todo: free --> 
          vboID.mIndex = RENDERER_ID_NONE;
        }
        // todo: free -->
        buffers.indexBufferHandle.mIndex = RENDERER_ID_NONE;
      }
    );


    // create stuff
    {

      // Create Vertex buffers
      for( u32 i = 0; i < numVertexBuffers; ++i )
      {
        buffers.vertexBufferHandles[ i ] = myRenderer->AddVertexBuffer(
          TacBufferAccess::Default,
          vertexBufferDatas[ i ],
          mNumVertices,
          vertexBufferStrides[ i ],
          errors );
        if( errors.size )
          return;
      }

      // Create index buffers
      buffers.indexBufferHandle = myRenderer->AddIndexBuffer(
        TacBufferAccess::Default,
        indexesData,
        numIndexes,
        indexTextureFormat,
        indexesTotalBufferSize,
        
        
        //indexVariableType,
        //indexNumBytes,
        errors );
      if( errors.size )
        return;

    }

    freeData = false;
  }();
  return buffers;
}

// this looks like shit
void ModelBuffersRender(
  TacRenderer* myRenderer,
  const ModelBuffers& buffers )
{
  myRenderer->SetIndexBuffer( buffers.indexBufferHandle );
  myRenderer->SetVertexBuffers(
    buffers.vertexBufferHandles,
    buffers.numVertexBuffers );
  myRenderer->Draw();
}


void TacModelInstance::CopyAnimatedTransforms()
{
  const std::vector< TacNode >& nodes = model->skeleton;
  animatedTransforms.resize( nodes.size() );
  for( u32 iNode = 0; iNode < nodes.size(); ++iNode )
  {
    animatedTransforms[ iNode ] = nodes[ iNode ].transformation;
  }
}

// NOTE( N8 )
//   make everything from node->parent to node->parent->parent->...,
//   so it all ends up node->model
void TacModelInstance::ConcatinateAnimatedTransforms()
{
  const std::vector< TacNode >& nodes = model->skeleton;

  std::queue< u32 > indexes;
  indexes.push( 0 );
  while( !indexes.empty() )
  {
    u32 index = indexes.front();
    indexes.pop();

    const TacNode& node = nodes[ index ];
    const m4& transform = animatedTransforms[ index ];
    for( u32 childIndex : node.childIndexes )
    {
      m4& childTransform = animatedTransforms[ childIndex ];
      childTransform = transform * childTransform;

      indexes.push( childIndex );
    }
  }
}

