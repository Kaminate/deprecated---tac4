#include "tacResourceManager.h"
#include "tacLibrary\tacMemoryManager.h"
#include "tacLibrary\tacString.h"

#include "3rdparty\assimp\Importer.hpp"  // C++ importer interface
#include "3rdparty\assimp\scene.h"       // Output data structure
#include "3rdparty\assimp\postprocess.h" // Post processing flags

#include <vector> 

void ProcessNode(
  aiNode* node,
  TacModel& modl,
  u32* assimp_to_tac_mesh_index,
  u32* assimp_to_tac_material_index,
  ResourceManager& resources,
  const aiScene* scene )
{
  TacAssert( node->mTransformation.IsIdentity() );

  for( u32 i = 0; i < node->mNumMeshes; ++i )
  {
    u32 assimpMeshIndex = node->mMeshes[ i ];
    u32 tacMeshIndex = assimp_to_tac_mesh_index[ assimpMeshIndex ];

    u32 assimpMaterialIndex =
      scene->mMeshes[ assimpMeshIndex ]->mMaterialIndex;
    u32 tacMaterialIndex =
      assimp_to_tac_material_index[ assimpMaterialIndex ];

    if( modl.numGeoms < TacModel::MAX_GEOMS )
    {
      modl.iGeoms[ modl.numGeoms ] = tacMeshIndex;
      modl.iMaterials[ modl.numGeoms ] = tacMaterialIndex;

      ++modl.numGeoms;
    }
  }


  for( u32 i = 0; i < node->mNumChildren; ++i )
  {
    ProcessNode(
      node->mChildren[ i ],
      modl,
      assimp_to_tac_mesh_index,
      assimp_to_tac_material_index,
      resources,
      scene );
  }
}

ResourceManager::Node& GetNode(
  ResourceManager& manager,
  ResourceManager::Node& node,
  const Filepath& path )
{
  u32 index = FindFirstOf( path.buffer, "\\/");

  // NOTE( N8 ): 
  //   if path is "foo/bar/baz.txt"
  //   then part is "foo"
  //   and remaining is "bar/baz.txt"

  Filepath part = {};
  part.size = TacStrCpy(
    part.buffer,
    ArraySize( part.buffer ),
    path.buffer,
    index );

  Filepath remaining = {};
  remaining = path.buffer + index + 1;

  if( node.path == part )
  {
    if( remaining.size == 0 )
    {
      return node;
    }
    else 
    {
      if( !node.child )
      {
        ResourceManager::Node& child = manager.nodes[
          node.child = manager.numNodes++ ];
        child.child = 0;
        child.index = 0;
        child.next = 0;
        child.path.size = TacStrCpy(
          child.path.buffer,
          ArraySize( child.path.buffer ),
          remaining.buffer,
          FindFirstOf( remaining.buffer, "\\/" ) );
        child.type = ResourceManager::Node::Type::eDirectory;
      }
      return GetNode( manager, manager.nodes[ node.child ], remaining );
    }
  }
  else
  {
    if( !node.next )
    {
      ResourceManager::Node& next = manager.nodes[
        node.next = manager.numNodes++ ];
      next.child = 0;
      next.index = 0;
      next.next = 0;
      next.path = part;
      next.type = ResourceManager::Node::Type::eDirectory;
    }
    return GetNode( manager, manager.nodes[ node.next ], path );
  }
}

TacModel LoadModelAssip(
  const Filepath& path,
  ResourceManager& resources,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  TacAssert( errors.size == 0 );

  ResourceManager::Node& node = GetNode(resources, resources.nodes[ 0 ], path );
  node.type = ResourceManager::Node::Type::eModel;

  if( !node.index )
  {


    Filepath modelFolder = FilepathGetFolder( path );
    TacModel modl = {};

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( 
      path.buffer, 
      aiProcess_Triangulate |
      aiProcess_GenSmoothNormals );

    if( scene && 
      ( ( scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ) == 0 ) &&
      scene->mRootNode )
    {
      std::vector< u32 > assimp_to_tac_mesh_index;
      assimp_to_tac_mesh_index.resize( scene->mNumMeshes );
      for( u32 i = 0; i < scene->mNumMeshes; ++i )
      {
        u32 iGeom = 0;
        if( resources.numGeom < ArraySize( resources.geometries ) )
        {
          aiMesh* mesh = scene->mMeshes[ i ];
          AttributeInput attributeData[ Attribute::eCount ] = {};
          TacAssert( mesh->HasPositions() )
          {
            AttributeInput& curInput = attributeData[ ( u32 )Attribute::ePos ];
            curInput.attributeComponentData.mComponentsPerVert = 3;
            curInput.attributeComponentData.mComponentSize = sizeof( float );
            curInput.attributeComponentData.mComponentType = GL_FLOAT;
            curInput.memory = mesh->mVertices;
            curInput.num = mesh->mNumVertices;
          }

          if( mesh->HasNormals() )
          {
            AttributeInput& curInput = attributeData[ ( u32 )Attribute::eNor ];
            curInput.attributeComponentData.mComponentsPerVert = 3;
            curInput.attributeComponentData.mComponentSize = sizeof( float );
            curInput.attributeComponentData.mComponentType = GL_FLOAT;
            curInput.memory = mesh->mNormals;
            curInput.num = mesh->mNumVertices;
          }

          // up to 8 texture coords per vertex
          if( mesh->HasTextureCoords( 0 ) )
          {
            AttributeInput& curInput = attributeData[ ( u32 )Attribute::eUV ];
            curInput.attributeComponentData.mComponentsPerVert = 3;
            curInput.attributeComponentData.mComponentSize = sizeof( float );
            curInput.attributeComponentData.mComponentType = GL_FLOAT;
            curInput.memory = mesh->mTextureCoords[ 0 ];
            curInput.num = mesh->mNumVertices;
          }

          TacAssert( mesh->HasFaces() );
          std::vector< u32 > indexes;
          indexes.reserve( mesh->mNumFaces * 3 );
          {
            for( u32 iFace = 0; iFace < mesh->mNumFaces; ++iFace )
            {
              aiFace& curFace = mesh->mFaces[ iFace ];
              if( curFace.mNumIndices == 3 ) // ignore point and line primitives
              {
                for( u32 iIndex = 0; iIndex < curFace.mNumIndices; ++iIndex )
                {
                  indexes.push_back( curFace.mIndices[ iIndex ] );
                }
              }
            }
          }

          GPUGeometry geom;
          LoadGeometry(
            geom,
            GL_TRIANGLES,
            indexes.data(),
            indexes.size(),
            GL_UNSIGNED_INT,
            attributeData );

          resources.geometries[ iGeom = resources.numGeom++ ] = geom;
        }
        assimp_to_tac_mesh_index[ i ] = iGeom;
      }

      std::vector< u32 > assimp_to_tac_material_index;
      assimp_to_tac_material_index.resize( scene->mNumMaterials );
      for( u32 i = 0; i < scene->mNumMaterials; ++i )
      {
        u32 tacMaterialIndex = 0;
        aiMaterial* curAssimpMaterial = scene->mMaterials[ i ];
        if( resources.numMaterials < ArraySize( resources.materials ) )
        {
          auto GetTexture = [](
            aiMaterial* curAssimpMaterial,
            ResourceManager& resources,
            const Filepath& modelFolder,
            aiTextureType textureType ) -> u32
          {
            u32 textureIndex = 0;
            u32 numTextures = curAssimpMaterial->GetTextureCount( textureType );
            if( numTextures != 0 )
            {
              aiString str;
              // NOTE( N8 ):
              //   assimp materials can have multiple textures of each type.
              //   we're only going to get the first one
              curAssimpMaterial->GetTexture( textureType, 0, &str );

              Filepath curTexturePath;
              curTexturePath = str.C_Str();



              Filepath curTexturePathFromRoot = FilepathAppendFile( modelFolder, curTexturePath );
              ResourceManager::Node& textureNode = GetNode(resources, resources.nodes[ 0 ],  curTexturePathFromRoot );
              textureNode.type = ResourceManager::Node::Type::eTexture;

              if( textureNode.index == 0 )
              {
                Texture newTexture = {};
                FixedString< DEFAULT_ERR_LEN > errorsstdstr = newTexture.Load( curTexturePathFromRoot.buffer, GL_RGBA8 );
                if( errorsstdstr.size == 0 )
                {
                  resources.textures[ textureNode.index = resources.numTextures++ ] = newTexture;
                }
              }
              textureIndex = textureNode.index;
            }

            return textureIndex;
          };

          Material& curTacMaterial = resources.materials[ tacMaterialIndex = resources.numMaterials++ ];
          curTacMaterial.iDiffuse = GetTexture( curAssimpMaterial, resources, modelFolder, aiTextureType_DIFFUSE );
          curTacMaterial.iNormal = GetTexture( curAssimpMaterial, resources, modelFolder, aiTextureType_NORMALS );
          curTacMaterial.iSpecular = GetTexture( curAssimpMaterial, resources, modelFolder, aiTextureType_SPECULAR );
          if( curTacMaterial.iDiffuse == 0 )
            curTacMaterial.iDiffuse = resources.defaultTextureWhite;
          if( curTacMaterial.iNormal == 0 )
            curTacMaterial.iNormal = resources.defaultTextureNormal;
          if( curTacMaterial.iSpecular == 0 )
            curTacMaterial.iSpecular = resources.defaultTextureDarkGrey;
        }
        assimp_to_tac_material_index[ i ] = tacMaterialIndex;
      }

      ProcessNode(
        scene->mRootNode,
        modl,
        assimp_to_tac_mesh_index.data(),
        assimp_to_tac_material_index.data(),
        resources,
        scene );
    }
    else
    {
      errors = importer.GetErrorString();
    }

    resources.models[ node.index = resources.numModels++ ] = modl;
  }

  return resources.models[ node.index ];
}

void ResourceManagerInit( ResourceManager& resources )
{
  auto LoadDefaultTexture = [](
    ResourceManager& resources, u32& index, u8 r, u8 g, u8 b )
  {
    Texture::Properties props = {};
    props.mFormat = GL_RGBA;
    props.mType = GL_UNSIGNED_BYTE;
    props.mInternalFormat = GL_RGBA8;
    props.mHeight = 1;
    props.mWidth = 1;
    props.mTarget = GL_TEXTURE_2D;

    u8 color[ 4 ] = { r, g, b, 255 };
    Texture unit;
    unit.Load( props, &color );

    resources.textures[ index = resources.numTextures++ ] = unit;
  };

  resources.numTextures = 0;
  LoadDefaultTexture( resources, resources.defaultTextureBlack, 0, 0, 0 );
  LoadDefaultTexture( resources, resources.defaultTextureDarkGrey, 30, 30, 30 );
  LoadDefaultTexture( resources, resources.defaultTextureWhite, 255, 255, 255 );
  LoadDefaultTexture( resources, resources.defaultTextureRed, 255, 0, 0 );
  LoadDefaultTexture( resources, resources.defaultTextureGreen, 0, 255, 0 );
  LoadDefaultTexture( resources, resources.defaultTextureBlue, 0, 0, 255 );
  LoadDefaultTexture( resources, resources.defaultTextureNormal, 128, 128, 255 );

  resources.numMaterials = 0;
  Material& defaultMaterial = resources.materials[ resources.numMaterials++ ];
  defaultMaterial.iDiffuse = resources.defaultTextureWhite;
  defaultMaterial.iNormal = resources.defaultTextureNormal;
  defaultMaterial.iSpecular = resources.defaultTextureDarkGrey;

  resources.numGeom = 0;
  LoadUnitCube( resources.geometries[ resources.defaultGeometryUnitCube = resources.numGeom++ ] );
  LoadUnitSphere( resources.geometries[ resources.defaultGeometryUnitSphere = resources.numGeom++ ], 4 );
  LoadNDCQuad( resources.geometries[ resources.defaultGeometryNDCQuad = resources.numGeom++ ] );

  resources.numModels = 0;
  TacModel& defaultModel = resources.models[ resources.numModels++ ];

  defaultModel.iGeoms[ defaultModel.numGeoms ] = resources.defaultGeometryUnitCube;
  defaultModel.iMaterials[ defaultModel.numGeoms ] = 0;
  defaultModel.numGeoms++;

  resources.numNodes = 0;
  ResourceManager::Node& root = resources.nodes[ resources.numNodes++ ];
  root.child = 0;
  root.index = 0;
  root.next = 0;
  root.path = "tacData";
  root.type = ResourceManager::Node::Type::eDirectory;

}
