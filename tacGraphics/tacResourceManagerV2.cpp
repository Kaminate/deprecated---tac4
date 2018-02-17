#include "tacResourceManagerV2.h"
#include <queue> // TODO( N8 ): TEMP

void ResourceManagerV2Init(
  TacResourceManagerV2& resources,
  TacRenderer* myRenderer,
  FixedString< DEFAULT_ERR_LEN >& errors )
{
  // Load default textures
  {
    auto LoadDefaultTexture = [](
      TacResourceManagerV2& resources,
      TacRenderer* myRenderer,
      TacTextureHandle& index,
      u8 r,
      u8 g,
      u8 b,
      FixedString< DEFAULT_ERR_LEN >& errors )
    {
      u8 data[ 4 ] = { r, g, b, 255 };
      TacImage myImage;
      myImage.mTextureFormat = TacTextureFormat::RGBA8Unorm;
      //myImage.mBitsPerPixel = 8;
      myImage.mData = ( u8* )&data;
      myImage.mWidth = 1;
      myImage.mHeight = 1;
      myImage.mPitch = 4;
      myImage.textureDimension = 2;
      //myImage.variableType = TacVariableType::unorm;
      //myImage.variableBytesPerComponent = 1;
      //myImage.variableNumComponents = 4;
      index = resources.defaultTextureBlack =
        myRenderer->AddTextureResource(
        myImage,
        TacTextureUsage::Immutable,
        TacBinding::ShaderResource,
        errors );
    };

    LoadDefaultTexture(
      resources,
      myRenderer,
      resources.defaultTextureBlack,
      0, 0, 0,
      errors );
    if( errors.size )
      return;

    LoadDefaultTexture(
      resources,
      myRenderer,
      resources.defaultTextureDarkGrey,
      30, 30, 30,
      errors );
    if( errors.size )
      return;

    LoadDefaultTexture(
      resources,
      myRenderer,
      resources.defaultTextureWhite,
      255, 255, 255,
      errors );
    if( errors.size )
      return;

    LoadDefaultTexture(
      resources,
      myRenderer,
      resources.defaultTextureRed,
      255, 0, 0,
      errors );
    if( errors.size )
      return;

    LoadDefaultTexture(
      resources,
      myRenderer,
      resources.defaultTextureGreen,
      0, 255, 0,
      errors );
    if( errors.size )
      return;

    LoadDefaultTexture(
      resources,
      myRenderer,
      resources.defaultTextureBlue,
      0, 0, 255,
      errors );
    if( errors.size )
      return;

    LoadDefaultTexture(
      resources,
      myRenderer,
      resources.defaultTextureNormal,
      128, 128, 255,
      errors );
    if( errors.size )
      return;
  }

  //resources.numMaterials = 0;
  //Material& defaultMaterial = resources.materials[ resources.numMaterials++ ];
  //defaultMaterial.iDiffuse = resources.defaultTextureWhite;
  //defaultMaterial.iNormal = resources.defaultTextureNormal;
  //defaultMaterial.iSpecular = resources.defaultTextureDarkGrey;

  //resources.numGeom = 0;
  //LoadUnitCube( resources.geometries[ resources.defaultGeometryUnitCube = resources.numGeom++ ] );
  //LoadUnitSphere( resources.geometries[ resources.defaultGeometryUnitSphere = resources.numGeom++ ], 4 );
  //LoadNDCQuad( resources.geometries[ resources.defaultGeometryNDCQuad = resources.numGeom++ ] );

  //resources.numModels = 0;
  //Model& defaultModel = resources.models[ resources.numModels++ ];

  //defaultModel.iGeoms[ defaultModel.numGeoms ] = resources.defaultGeometryUnitCube;
  //defaultModel.iMaterials[ defaultModel.numGeoms ] = 0;
  //defaultModel.numGeoms++;

  //ResourceManagerV2::Node& root = resources.nodes[ resources.numNodes++ ];
  //root.path = "tacData";
  //root.type = ResourceManagerV2::Node::Type::eDirectory;
}
