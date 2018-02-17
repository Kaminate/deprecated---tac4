#include "tacModelConverter.h"

#include "tacLibrary\imgui\imgui.h"

#include "tacGraphics\tac3camera.h"
#include "tacGraphics\tacRenderer.h"
#include "tacGraphics\tac4Model.h"

#include <queue>
#include <iostream>
#include <stdio.h>
#include <vector>

m4 AssimpToTacMatrix( aiMatrix4x4 m )
{
  m4 result = 
  { 
    m.a1, m.a2, m.a3, m.a4,
    m.b1, m.b2, m.b3, m.b4,
    m.c1, m.c2, m.c3, m.c4,
    m.d1, m.d2, m.d3, m.d4,
  };
  return result;
}
v3 AssimpToTacVector3( aiVector3D v )
{
  v3 result = {v.x, v.y, v.z };
  return result;
}

// returns numNodes if not found
u32 FindNodeIndex (
  const TacNode* nodes,
  u32 numNodes,
  const char* name )
{
  u32 nodeIndex = 0;
  while( nodeIndex < numNodes )
  {
    if( nodes[ nodeIndex ].name == name )
      break;
    ++nodeIndex;
  }
  return nodeIndex;
}

const aiScene*
  OpenAIScene(
  const char* filepath,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(
    filepath,
    aiProcess_Triangulate |
    aiProcess_GenSmoothNormals );
  b32 readSuccess =
    scene && 
    // NOTE( N8 ):
    //   We don't care about the scene being incomplete anymore.
    //   It's okay if we have animation data but no geometry data
    //( ( scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ) == 0 ) &&
    scene->mRootNode;
  if( !readSuccess )
  {
    errors.size = sprintf_s(
      errors.buffer,
      "Assimp::Importer::ReadFile( %s ) failed\n",
      filepath );
    FixedStringAppend( errors, importer.GetErrorString() );
  }
  else
  {
    // NOTE( N8 ):
    //   thi must be called after GetErrorString because apparently it
    //   clears the error messages.
    scene = importer.GetOrphanedScene();
  }
  return scene;
}

void LoadAnimationsAssimp(
  const aiScene* scene,
  const std::vector< TacNode >& mySkeleton,
  std::vector< TacAnimation >& myAnimations,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  for( u32 iAnim = 0; iAnim < scene->mNumAnimations; ++iAnim )
  {
    aiAnimation* curAiAnimation = scene->mAnimations[ iAnim ];

    TacAnimation myAnimation;
    myAnimation.name = curAiAnimation->mName.C_Str();
    // TODO( N8 ): do we need this?
    myAnimation.duration = 
      ( decltype( myAnimation.duration ) ) 
      curAiAnimation->mDuration; // in ticks
    if( curAiAnimation->mTicksPerSecond != 0 )
    {
    }


    // NOTE( N8 ): add a track for each bone
    myAnimation.tracks.resize( curAiAnimation->mNumChannels );
    for(
      u32 iChannel = 0;
      iChannel < curAiAnimation->mNumChannels; 
    ++iChannel )
    {
      aiNodeAnim* curAiNodeAnim =
        curAiAnimation->mChannels[ iChannel ];
      curAiNodeAnim->mNodeName;

      TacTrack& t = myAnimation.tracks[ iChannel ];
      t.nodeIndex = FindNodeIndex(
        mySkeleton.data(),
        mySkeleton.size(),
        curAiNodeAnim->mNodeName.C_Str() );
      TacAssertIndex( t.nodeIndex, mySkeleton.size() );

      t.keyFrames.resize( curAiNodeAnim->mNumPositionKeys );

      // NOTE( N8 ): add all the key frames for each bone
      for(
        u32 iKey = 0;
        iKey < curAiNodeAnim->mNumPositionKeys;
      ++iKey )
      {
        TacAssert(
          ( curAiNodeAnim->mPositionKeys[ iKey ].mTime ==
          curAiNodeAnim->mRotationKeys[ iKey  ].mTime ) &&
          ( curAiNodeAnim->mPositionKeys[ iKey ].mTime ==
          curAiNodeAnim->mScalingKeys[ iKey  ].mTime ) );

        auto rot = curAiNodeAnim->mRotationKeys[ iKey ].mValue;
        auto scale = curAiNodeAnim->mScalingKeys[ iKey ].mValue;
        auto pos = curAiNodeAnim->mPositionKeys[ iKey ].mValue;

        TacKeyFrame& myKeyFrame = t.keyFrames[ iKey ];
        myKeyFrame.rotationQuaternion.x = rot.x;
        myKeyFrame.rotationQuaternion.y = rot.y;
        myKeyFrame.rotationQuaternion.z = rot.z;
        myKeyFrame.rotationQuaternion.w = rot.w;
        myKeyFrame.time = 
          ( decltype( myKeyFrame.time ) )
          curAiNodeAnim->mPositionKeys[ iKey ].mTime;
        myKeyFrame.translation = AssimpToTacVector3(
          curAiNodeAnim->mPositionKeys[ iKey ].mValue );
      }
    }

    myAnimations.push_back( myAnimation );

    // NOTE( N8 ): NOT IN USE ( EVEN BY ASSIMP )

    //for( u32 iMeshChannel = 0;
    //  iMeshChannel < curAiAnimation->mNumMeshChannels;
    //  ++iMeshChannel )
    //{
    //  aiMeshAnim* curAiMeshAnim =
    //    curAiAnimation->mMeshChannels[ iMeshChannel ];
    //  for(
    //    u32 iKey = 0;
    //    iKey < curAiMeshAnim->mNumKeys;
    //  ++iKey )
    //  {
    //    aiMeshKey& curAiMeshKey = curAiMeshAnim->mKeys[ iKey ];
    //  }
    //}
  }
}

struct Temp
{
  aiNode* mnode;
  Temp( aiNode* node) : mnode( node ){ }
};

void LoadSkeletonAssimp(
  const aiScene* scene,
  TacModel& myModel,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  u32 nextindex = 0;
  std::queue< Temp >nodestack;
  nodestack.push( Temp( scene->mRootNode ) );
  ++nextindex;
  while( !nodestack.empty() )
  {
    Temp t = nodestack.front();
    nodestack.pop();

    TacNode myTacNode;
    myTacNode.name = t.mnode->mName.C_Str();
    myTacNode.transformation = AssimpToTacMatrix( t.mnode->mTransformation );
    for( u32 ichild = 0; ichild < t.mnode->mNumChildren; ++ichild )
    {
      aiNode* child = t.mnode->mChildren[ ichild ];
      myTacNode.childIndexes.push_back( nextindex++ );
      nodestack.push( Temp( child ) );
    }
    myModel.skeleton.push_back( myTacNode );
  }

  for( u32 i = 0; i < scene->mNumMeshes; ++i )
  {
    aiMesh* myAiMesh = scene->mMeshes[ i ];

    std::vector< TacBone >& bones = myModel.subModels[ i ].bones;
    bones.resize( myAiMesh->mNumBones );

    // Create the skeleton
    for( u32 iBone = 0; iBone < myAiMesh->mNumBones; ++iBone )
    {
      aiBone* curAiBone = myAiMesh->mBones[ iBone ];
      curAiBone->mName;
      curAiBone->mOffsetMatrix;
      curAiBone->mWeights[ 0 ].mVertexId;

      TacBone& b = bones[ iBone ];
      b.name = curAiBone->mName.C_Str();
      b.offset = AssimpToTacMatrix( curAiBone->mOffsetMatrix );
      b.nodeIndex = FindNodeIndex(
        myModel.skeleton.data(),
        myModel.skeleton.size(),
        b.name.buffer );
      TacAssertIndex( b.nodeIndex, myModel.skeleton.size() );
    }
  }
}

void LoadModelAssip(
  const aiScene* scene,
  TacModel& myModel,
  TacRenderer* myRenderer,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  if( scene->mNumMeshes > ArraySize( myModel.subModels ) )
  {
    errors = "Not enough meshes";
    return;
  }

  myModel.numSubModels = scene->mNumMeshes;

  for( u32 i = 0; i < scene->mNumMeshes; ++i )
  {
    TacSubModel& mySubModel = myModel.subModels[ i ];
    aiMesh* myAiMesh = scene->mMeshes[ i ];

    // load the geometry
    {
      // buffer data
      const u32 MAXVBOS = 10;
      void* vertexBufferDatas[ MAXVBOS  ];
      u32 vertexBufferStrides[ MAXVBOS ];
      u32 numVertexBuffers = 0;

      // vertex data
      TacVertexFormat vertexFormats[ 10 ] = {};
      u32 numVertexFormats = 0;
      {
        auto AddAssimpBuffer = [&](
          void* bufferData,
          TacAttributeType myTacAttributeType,
          TacTextureFormat variableFormat,
          //TacVariableType variableType,
          //u32 variableNumBytes,
          //u32 attribNumComponents,
          u32 stride )
        {
          if( numVertexBuffers >= MAXVBOS )
          {
            errors = "Not enough vbos";
            return;
          }
          u32 vboIndex = numVertexBuffers++;
          vertexBufferDatas[ vboIndex ] = bufferData;
          vertexBufferStrides[ vboIndex ] = stride;

          if( numVertexFormats >= ArraySize( vertexFormats ) )
          {
            errors = "Not enough vertex formats";
            return;
          }
          TacVertexFormat& curVertexFormat =
            vertexFormats[ numVertexFormats++ ];
          curVertexFormat.mAttributeType = myTacAttributeType;
          curVertexFormat.textureFormat = variableFormat;
          //curVertexFormat.mVariableType = variableType;
          //curVertexFormat.mNumBytes = variableNumBytes;
          //curVertexFormat.mNumComponents = attribNumComponents;
          curVertexFormat.mInputSlot = vboIndex;
        };

        if( myAiMesh->HasPositions() )
        {
          AddAssimpBuffer(
            myAiMesh->mVertices,
            TacAttributeType::Position,
            TacTextureFormat::RGB32Float,
            //TacVariableType::real,
            //4,
            //3,
            sizeof( aiVector3D ) );
        }

        if( myAiMesh->HasVertexColors( 0 ) )
        {
          AddAssimpBuffer(
            myAiMesh->mColors[ 0 ],
            TacAttributeType::Color,
            TacTextureFormat::RGBA32Float,
            //TacVariableType::real,
            //4,
            //4,
            sizeof( aiColor4D ));
        }

        if( myAiMesh->HasNormals() )
        {
          AddAssimpBuffer(
            myAiMesh->mNormals,
            TacAttributeType::Normal,
            TacTextureFormat::RGB32Float,
            //TacVariableType::real,
            //4,
            //3,
            sizeof( aiVector3D ) );
        }

        if( myAiMesh->HasTextureCoords( 0 ) )
        {
          AddAssimpBuffer(
            myAiMesh->mTextureCoords[ 0 ],
            TacAttributeType::Texcoord,
            TacTextureFormat::RGB32Float,
            //TacVariableType::real,
            //4,
            //3,
            sizeof( aiVector3D ) );
        }

        if( myAiMesh->HasBones() )
        {
          const int BONES_PER_VERT = 4;
          u32 count = BONES_PER_VERT * myAiMesh->mNumVertices;
          u8* boneIndexes = new u8[ count ];
          u8* boneWeights = new u8[ count ];
          u8* boneCounts = new u8[ myAiMesh->mNumVertices ];

          if( !boneIndexes || !boneWeights || !boneCounts )
          {
            errors = "not enough memory to allocate for bones";
            return;
          }

          for( u32 iBone = 0; iBone < myAiMesh->mNumBones; ++iBone )
          {
            aiBone* bone = myAiMesh->mBones[ iBone ];

            for( u32 iWeight = 0; iWeight < bone->mNumWeights; ++iWeight )
            {
              aiVertexWeight& weight = bone->mWeights[ iWeight ];

              u32 index = BONES_PER_VERT * weight.mVertexId;
              u8* curBoneIndexes = &boneIndexes[ index ];
              u8* curBoneWeights = &boneWeights[ index ];
              u8& curBoneCount = boneCounts[ weight.mVertexId ];

              u8 boneWeight = ( u8 )( U8MAX * weight.mWeight );
              if( curBoneCount < BONES_PER_VERT )
              {
                curBoneIndexes[ curBoneCount ] = iBone;
                curBoneWeights[ curBoneCount ] = boneWeight;

                ++curBoneCount;
              }
              else
              {
                // NOTE( N8 ):

                //   Already BONES_PER_VERT bones for this vertex!
                //   Silently distribute the weights into the other bones
                // TODO( N8 ):
                //   make this throw a warning
                boneWeight /= BONES_PER_VERT;
                for( u32 i = 0; i < BONES_PER_VERT; ++i )
                {
                  curBoneWeights += boneWeight;
                }
              }
            }
          }

          AddAssimpBuffer(
            boneIndexes,
            TacAttributeType::BoneIndex,
            TacTextureFormat::RGBA8Uint,

            //TacVariableType::uint,
            //1,
            //BONES_PER_VERT,
            BONES_PER_VERT * sizeof( u8 ) );

          AddAssimpBuffer(
            boneWeights,
            TacAttributeType::BoneWeight,
            TacTextureFormat::RGBA8Unorm,
            //TacVariableType::unorm,
            //1,
            //BONES_PER_VERT,
            BONES_PER_VERT * sizeof( u8 ) );
        }

        // TODO( N8 ): TEMP HACK. add normals if none exist
        if( !myAiMesh->HasNormals() )
        {
          static v3 normals[ 1000 ] = {};
          TacAssert( ArraySize( normals ) >= myAiMesh->mNumVertices );
          for( u32 iPos = 0; iPos < myAiMesh->mNumVertices; ++iPos )
          {
            aiVector3D& curPos = myAiMesh->mVertices[ iPos ];
            v3& curNormal = normals[ iPos ];
            curNormal.x = curPos.x;
            curNormal.y = curPos.y;
            curNormal.z = curPos.z;
            if( LengthSq( curNormal ) > 0.1f )
            {
              curNormal = Normalize( curNormal );
            }
          }
          AddAssimpBuffer(
            normals,
            TacAttributeType::Normal,
            TacTextureFormat::RGB32Float,
            //TacVariableType::real,
            //4,
            //3,
            sizeof( v3 ) );
        }

        // TODO( N8 ): TEMP HACK. add vertex colors if none exist
        if( !myAiMesh->HasVertexColors( 0 ) )
        {
          static v4 colors[ 1000 ] = {};
          TacAssert( ArraySize( colors ) >= myAiMesh->mNumVertices );
          v3 minPos;
          v3 maxPos;
          aiVector3D& firstAIVert = myAiMesh->mVertices[ 0 ];
          for( u32 i = 0; i < 3; ++i )
          {
            minPos[i] = maxPos[i] = firstAIVert[i];
          }
          for( u32 iPos = 0; iPos < myAiMesh->mNumVertices; ++iPos )
          {
            aiVector3D& curPos = myAiMesh->mVertices[ iPos ];
            for( u32 i = 0; i < 3; ++i )
            {
              if(  curPos[i] <minPos[i] )
                minPos[i] = curPos[i];
              if( curPos[i] > maxPos[ i ])
                maxPos[i] = curPos[i];
            }
          }
          v3 dPos = maxPos - minPos;
          if( AbsoluteValue( dPos.x ) > 0.1f &&
            AbsoluteValue( dPos.y ) > 0.1f &&
            AbsoluteValue( dPos.z ) > 0.1f )
          {
            for( u32 iPos = 0; iPos < myAiMesh->mNumVertices; ++iPos )
            {
              aiVector3D& curPos = myAiMesh->mVertices[ iPos ];
              v4& curCol = colors[ iPos ];
              for( u32 i = 0; i < 3; ++i )
              {
                curCol[ i ];
                curCol[i] = ( curPos[i] - minPos[i] ) / dPos[i];
              }
              curCol.w = 1.0f;
            }
          }
          AddAssimpBuffer(
            colors,
            TacAttributeType::Color,
            TacTextureFormat::RGBA32Float,
            //TacVariableType::real,
            //4,
            //4,
            sizeof( v4 ) );
        }
      }

      // index data
      std::vector< u32 > indexes;
      indexes.resize( myAiMesh->mNumFaces * 3 );
      u32 index = 0;
      for( u32 i = 0; i < myAiMesh->mNumFaces; ++i )
      {
        aiFace& curFace = myAiMesh->mFaces[ i ];
        indexes[ index++ ] = curFace.mIndices[ 0 ];
        indexes[ index++ ] = curFace.mIndices[ 1 ];
        indexes[ index++ ] = curFace.mIndices[ 2 ];
      }

      const TacVariableType indexType = TacVariableType::uint;
      const u32 indexNumBytes = 4;

      mySubModel.buffers = ModelBuffersCreate(
        myRenderer,
        myAiMesh->mNumVertices,
        vertexBufferDatas,
        vertexBufferStrides,
        numVertexBuffers,
        indexes.size(),
        indexes.data(),
        TacTextureFormat::R32Uint,
        sizeof( u32 ) * indexes.size(),

        //indexType,
        //indexNumBytes,
        errors );
      if( errors.size )
        return;
    }

    // load the material
    {
      aiMaterial* curAssimpMaterial =
        scene->mMaterials[ myAiMesh->mMaterialIndex ];

      auto LoadTheAssimpTexture = [](
        aiMaterial* curAssimpMaterial,
        aiTextureType textureType,
        TacTextureHandle& myRendererID,
        TacRenderer* myRenderer,
        FixedString< DEFAULT_ERR_LEN >& errors )
      {
        // if the material doesn't have said texture, this function returns
        // and empty texturePath
        auto GetTexturePath = [](
          aiMaterial* curAssimpMaterial,
          aiTextureType textureType,
          Filepath& texturePath,
          FixedString< DEFAULT_ERR_LEN >& errors )
        {
          u32 numTextures = curAssimpMaterial->GetTextureCount( textureType );
          if( numTextures == 0 )
          {
            // not an error.
            return;
          }
          aiString str;
          // NOTE( N8 ):
          //   assimp materials can have multiple textures of each type.
          //   we're only going to get the first one
          if( curAssimpMaterial->GetTexture( textureType, 0, &str ) !=
            aiReturn_SUCCESS )
          {
            errors = "Could not get assimp material";
            return;
          }

          texturePath = str.C_Str();
        };

        Filepath texturePath = {};
        GetTexturePath(
          curAssimpMaterial,
          textureType,
          texturePath,
          errors );
        if( errors.size )
          return;

        // todo: actually load the texture now that we have the path
        myRendererID.mIndex = RENDERER_ID_NONE;
      };

      LoadTheAssimpTexture(
        curAssimpMaterial,
        aiTextureType_DIFFUSE,
        mySubModel.myDiffuseID,
        myRenderer,
        errors );
      if( errors.size )
        return;

      LoadTheAssimpTexture(
        curAssimpMaterial,
        aiTextureType_SPECULAR,
        mySubModel.mySpecularID,
        myRenderer,
        errors );
      if( errors.size )
        return;

      LoadTheAssimpTexture(
        curAssimpMaterial,
        aiTextureType_NORMALS,
        mySubModel.myNormalID,
        myRenderer,
        errors );
      if( errors.size )
        return;
    }
  };
}

// Game -------------------------------------------------------------------
extern "C" __declspec( dllexport )
  void GameOnLoad(
  TacGameInterface& onLoadInterface )
{
  ImGui::SetInternalState( onLoadInterface.imguiInternalState );

  if( !onLoadInterface.gameMemory->initialized )
  {
    onLoadInterface.gameMemory->initialized = true;
    if( sizeof( TacGameState ) >
      onLoadInterface.gameMemory->permanentStorageSize )
    {
      onLoadInterface.gameUnrecoverableErrors.size = sprintf_s(
        onLoadInterface.gameUnrecoverableErrors.buffer,
        "Not enough memory( size: %i ) to store GameState( size: %i )",
        onLoadInterface.gameMemory->permanentStorageSize,
        sizeof( TacGameState ) );
      return;
    }
    TacGameState* state =
      ( TacGameState* )onLoadInterface.gameMemory->permanentStorage;

    // world data ---------------------------------------------------------
    {
      state->camPosOriginal = state->camPos = V3( 6.4f, 10.0f, 11.0f );
      state->camDirOriginal = state->camDir =
        Normalize( V3( -0.13f, -0.127f, -0.9f ) );
      state->camWorldUpOriginal = state->camWorldUp = V3( 0.0f, 1.0f, 0.0f );
      r32 aspect = 
        ( r32 ) onLoadInterface.windowWidth / 
        ( r32 ) onLoadInterface.windowHeight;
      ComputeViewMatrix(
        state->view,
        state->camPos,
        state->camDir,
        state->camWorldUp );
      r32 a, b;
      onLoadInterface.renderer->GetPerspectiveProjectionAB(
        1000.0f, 0.1f, a, b );
      ComputePerspectiveProjMatrix(
        a, b, state->proj, 90.0f * DEG2RAD, aspect );
    }
  }
}

extern "C" __declspec( dllexport )
  void GameUpdateAndRender(
  TacGameInterface& updateInterface )
{
  TacGameInput& input = *updateInterface.gameInput;
  TacGameState* state =
    ( TacGameState* )updateInterface.gameMemory->permanentStorage;

  if( input.KeyboardDown( KeyboardKey::Escape ) )
  {
    updateInterface.running = false;
  }

  // imgui and update camera ----------------------------------------------
  {
    if( ImGui::CollapsingHeader( "general" ) )
    {
      static bool drawTestWindow;
      ImGui::Checkbox( "Show test window ", &drawTestWindow );
      if( drawTestWindow )
      {
        ImGui::ShowTestWindow( &drawTestWindow );
      }
    }
    v3 right, up;
    GetCamDirections( state->camDir, state->camWorldUp, right, up );
    if( ImGui::CollapsingHeader( "camera" ) )
    {
      ImGui::DragFloat3( "camPos", &state->camPos.x, 0.1f );
      ImGui::DragFloat3( "camDir", &state->camDir.x, 0.1f );
      ImGui::DragFloat3( "camWorldUp", &state->camWorldUp.x, 0.1f );
      ImGui::DragFloat3( "camRight", &right.x, 0.1f );
      ImGui::DragFloat3( "camUp", &up.x, 0.1f );
    }
    // mouse controls
    {
      // scroll wheel zoom
      state->camPos += state->camDir *
        ( r32 )updateInterface.gameInput->mouseWheelChange;

      // middle mouse drag look
      if( input.MouseDown( MouseButton:: eMiddleMouseButton ) )
      {
        // horizontal
        if( updateInterface.gameInput->mouseXRel != 0 )
        {
          r32 radiansPerPixel =
            10.0f * DEG2RAD /
            50.0f * updateInterface.gameInput->mouseXRel;
          r32 c = Cos( radiansPerPixel );
          r32 s = Sin( radiansPerPixel );

          v3 newDir = right * s + state->camDir * c;
          v3 newRight = right * c + state->camDir * -s;
          state->camDir = newDir;
          right = newRight;
        }

        // vertical
        if( updateInterface.gameInput->mouseYRel != 0 )
        {
          r32 radiansPerPixel =
            10.0f * DEG2RAD /
            50.0f * -updateInterface.gameInput->mouseYRel;
          r32 c = Cos( radiansPerPixel );
          r32 s = Sin( radiansPerPixel );

          v3 newDir = state->camDir * c + up * s;
          v3 newUp = state->camDir * -s + up * c;
          state->camDir = newDir;
          up = newUp;
        }

        // clamp vertical angle
        float camDiroWorldUp = Dot( state->camDir, state->camWorldUp );
        float maxAngle = 15.0f * DEG2RAD;
        if( AbsoluteValue( camDiroWorldUp ) > Cos( maxAngle ) )
        {
          v3 v = Cross( state->camWorldUp, right );

          r32 c = Cos( maxAngle );
          r32 s = Sin( maxAngle );
          if( camDiroWorldUp  > 0 )
          {
            state->camDir = state->camWorldUp * c + v * s;
          }
          else
          {
            state->camDir = -state->camWorldUp * c + v * s;
          }
          GetCamDirections( state->camDir, state->camWorldUp, right, up );
        }
      }
    }

    // wasd qe
    if( true )
    {
      v3 addition = {};
      auto ProjectOntoPlane = []( v3 plane, v3 dir )
      {
        v3 result = {};
        v3 newAddition =
          dir - plane * Dot( dir, plane );
        if( LengthSq( newAddition ) > 0.1f )
        {
          result += Normalize( newAddition );
        }
        return result;
      };
      v3 projectedRight = ProjectOntoPlane(
        state->camWorldUp,
        right );
      v3 projectedDir = ProjectOntoPlane(
        state->camWorldUp,
        state->camDir );

      if( input.KeyboardDown( KeyboardKey::W ) )
      {
        addition += projectedDir;
      }
      if( input.KeyboardDown( KeyboardKey::A ) )
      {
        addition -= projectedRight;
      }
      if( input.KeyboardDown( KeyboardKey::S ) )
      {
        addition -= projectedDir;
      }
      if( input.KeyboardDown( KeyboardKey::D ) )
      {
        addition += projectedRight;
      }
      if( input.KeyboardDown( KeyboardKey::Q ) )
      {
        addition -= state->camWorldUp;
      }
      if( input.KeyboardDown( KeyboardKey::E ) )
      {
        addition += state->camWorldUp;
      }
      if( LengthSq( addition ) > 0.1f )
      {
        addition = Normalize( addition ) * 1.11f;
        state->camPos += addition;
      }
    }

    // numpad
    {
      //The viewer should provide interactive camera operations
      //  The following interface (in the global space) on the Num-pad is recommended:
      //  “0” and “.” (clockwise and counterclockwise rotations around Y)
      b32 numpad0Down = input.KeyboardDown( KeyboardKey::Numpad0 );
      b32 periodDown = input.KeyboardDown( KeyboardKey::DecimalPoint );
      if( numpad0Down || periodDown )
      {
        r32 radius;
        r32 theta;
        r32 phi;
        SphericalCoordinatesTo(
          state->camPos.z,
          state->camPos.x,
          state->camPos.y,
          &radius,
          &theta,
          &phi );

        r32 speed = 1.0f;
        if( numpad0Down )
        {
          theta -= speed * input.dt;
        }
        if( periodDown )
        {
          theta += speed * input.dt;
        }
        SphericalCoordinatesFrom(
          radius,
          theta,
          phi,
          &state->camPos.z,
          &state->camPos.x,
          &state->camPos.y );
        v3 lookatPoint = { 0, 10, 0 };
        state->camDir = Normalize( lookatPoint - state->camPos );
      }

      v3 additionDir = {};
      //  “4”, “6”, “2” and “8” (translations in X and Z)
      if( input.KeyboardDown( KeyboardKey::Numpad4 ) )
        additionDir -= V3( 1.0f, 0.0f, 0.0f );
      if( input.KeyboardDown( KeyboardKey::Numpad6 ) )
        additionDir += V3( 1.0f, 0.0f, 0.0f );
      if( input.KeyboardDown( KeyboardKey::Numpad2 ) )
        additionDir += V3( 0.0f, 0.0f, 1.0f );
      if( input.KeyboardDown( KeyboardKey::Numpad8 ) )
        additionDir -= V3( 0.0f, 0.0f, 1.0f );
      state->camPos += additionDir * 1.0f;

      //  “+” and “-“ (Zoom in and out)
      v3 zoomAddition = {};
      r32 zoomMultiplier = 0;
      if( input.KeyboardDown( KeyboardKey::NumpadAdd ) )
      {
        zoomMultiplier += 1.0f;
      }
      if( input.KeyboardDown( KeyboardKey::NumpadSubtract ) )
      {
        zoomMultiplier -= 1.0f;
      }
      state->camPos += zoomMultiplier * 5.0f * input.dt * state->camDir; 

      //  “5” (initial setting).
      if( input.KeyboardDown( KeyboardKey::Numpad5 ) )
      {
        state->camDir = state->camDirOriginal;
        state->camPos = state->camPosOriginal;
        state->camWorldUp = state->camWorldUpOriginal;
      }

    }

    ComputeViewMatrix(
      state->view,
      state->camPos,
      state->camDir,
      state->camWorldUp );
  }

  // render ---------------------------------------------------------------
  {
    updateInterface.renderer->ClearColor( 
      updateInterface.renderer->GetBackbufferColor(),

      V4( 0.0f, 0.6f, 1.0f, 1.0f ) );
    updateInterface.renderer->ClearDepthStencil(
      updateInterface.renderer->GetBackbufferDepth(),
      true, 1.0f, false, 0 );
    updateInterface.renderer->SetScissorRect(
      0, 0, 
      ( r32 )updateInterface.gameInput->windowWidth,
      ( r32 )updateInterface.gameInput->windowHeight );
  }
}
