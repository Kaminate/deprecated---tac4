#include "tac3geometry.h"
#include "tacLibrary\tacDefines.h"

const PerComponentData v3Data = { 3, sizeof( r32 ), GL_FLOAT };
const PerComponentData v2Data = { 2, sizeof( r32 ), GL_FLOAT };

void LoadGeometry(
  GPUGeometry& geom,
  GLenum primitive,
  void* indexes,
  u32 indexCount,
  GLenum indexType,
  AttributeInput attributeData[ Attribute::eCount ] )
{
  geom.mIndexType = indexType;
  geom.mIndexCount = indexCount;
  geom.mprimitive = primitive;
  glGenVertexArrays(1, &geom.mVAO);
  glBindVertexArray(geom.mVAO);

  for( GLuint i = 0; i < (GLuint) Attribute::eCount; ++i )
  {
    glDisableVertexAttribArray( i );
    AttributeInput& data = attributeData[ i ];
    if( data.num )
    {
      u32 entireBufSize =
        data.attributeComponentData.mComponentSize *
        data.attributeComponentData.mComponentsPerVert *
        attributeData->num;
      glGenBuffers( 1, &geom.mFBOs[ i ] );
      glBindBuffer( GL_ARRAY_BUFFER, geom.mFBOs[ i ] );
      glBufferData(
        GL_ARRAY_BUFFER,
        entireBufSize,
        data.memory,
        GL_STATIC_DRAW );

      glEnableVertexAttribArray( i );
      glVertexAttribPointer(
        i,
        data.attributeComponentData.mComponentsPerVert,
        data.attributeComponentData.mComponentType,
        GL_FALSE,
        0,
        nullptr);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
  }

  unsigned indexComponentSize = 0;
  switch ( indexType )
  {
  case GL_UNSIGNED_BYTE: 
    indexComponentSize = sizeof( GLubyte );
    TacAssert( indexComponentSize == 1 );
    break;
  case GL_UNSIGNED_SHORT: 
    indexComponentSize = sizeof( GLushort );
    TacAssert( indexComponentSize == 2 );
    break;
  case GL_UNSIGNED_INT:
    indexComponentSize = sizeof( GLuint );
    TacAssert( indexComponentSize == 4 );
    break;
  default:
    TacAssert( false );
    break;
  }

  glGenBuffers( 1, &geom.mIndexBuffer );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geom.mIndexBuffer );
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    indexCount * indexComponentSize,
    indexes,
    GL_STATIC_DRAW );
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glBindVertexArray(0);
}


// A unit cube has a width of 1
// This cube is centered around the origin
struct CubeVertex
{
  u32 iPos;
  u32 iNor;
  u32 iUV;
};

struct CubeFace
{
  CubeFace (
    u32 iPos0,
    u32 iUV0,
    u32 iNor0,

    u32 iPos1,
    u32 iUV1,
    u32 iNor1,

    u32 iPos2,
    u32 iUV2,
    u32 iNor2 )
  {
    vert[ 0 ].iPos = iPos0; 
    vert[ 0 ].iUV = iUV0; 
    vert[ 0 ].iNor = iNor0; 

    vert[ 1 ].iPos = iPos1; 
    vert[ 1 ].iUV = iUV1; 
    vert[ 1 ].iNor = iNor1; 

    vert[ 2 ].iPos = iPos2; 
    vert[ 2 ].iUV = iUV2; 
    vert[ 2 ].iNor = iNor2; 
  };
  CubeVertex vert[ 3 ];
};


void LoadUnitSphere( GPUGeometry& geom, u32 numDivisions )
{
  v3 baseVertexes[ 12 ];

  // create 12 vertices of a icosahedron
  float t = ( 1.0f + 1.414f ) / 2.0f;
  int index = 0;
  baseVertexes[ index++ ] = V3( -1.0f,  t,  0.0f );
  baseVertexes[ index++ ] = V3(  1.0f,  t,  0.0f );
  baseVertexes[ index++ ] = V3( -1.0f, -t,  0.0f );
  baseVertexes[ index++ ] = V3(  1.0f, -t,  0.0f );

  baseVertexes[ index++ ] = V3(  0.0f, -1.0f,  t );
  baseVertexes[ index++ ] = V3(  0.0f,  1.0f,  t );
  baseVertexes[ index++ ] = V3(  0.0f, -1.0f, -t );
  baseVertexes[ index++ ] = V3(  0.0f,  1.0f, -t );

  baseVertexes[ index++ ] = V3(  t,  0.0f, -1.0f );
  baseVertexes[ index++ ] = V3(  t,  0.0f,  1.0f );
  baseVertexes[ index++ ] = V3( -t,  0.0f, -1.0f );
  baseVertexes[ index++ ] = V3( -t,  0.0f,  1.0f );
  for( int i = 0; i < ARRAYSIZE( baseVertexes ); ++i )
  {
    baseVertexes[ i ] = Normalize( baseVertexes[ i ] );
  }

  std::vector< v3u > faces;
  faces.resize( 20 );
  index = 0;
  faces[ index++ ] = V3( 0u, 11u, 5u );
  faces[ index++ ] = V3( 0u, 5u, 1u );
  faces[ index++ ] = V3( 0u, 1u, 7u );
  faces[ index++ ] = V3( 0u, 7u, 10u );
  faces[ index++ ] = V3( 0u, 10u, 11u );

  faces[ index++ ] = V3( 1u, 5u, 9u );
  faces[ index++ ] = V3( 5u, 11u, 4u );
  faces[ index++ ] = V3( 11u, 10u, 2u );
  faces[ index++ ] = V3( 10u, 7u, 6u );
  faces[ index++ ] = V3( 7u, 1u, 8u );

  faces[ index++ ] = V3( 3u, 9u, 4u );
  faces[ index++ ] = V3( 3u, 4u, 2u );
  faces[ index++ ] = V3( 3u, 2u, 6u );
  faces[ index++ ] = V3( 3u, 6u, 8u );
  faces[ index++ ] = V3( 3u, 8u, 9u );

  faces[ index++ ] = V3( 4u, 9u, 5u );
  faces[ index++ ] = V3( 2u, 4u, 11u );
  faces[ index++ ] = V3( 6u, 2u, 10u );
  faces[ index++ ] = V3( 8u, 6u, 7u );
  faces[ index++ ] = V3( 9u, 8u, 1u );

  std::vector< v3 > points;
  points.reserve( faces.size() * 3 );
  for( v3u& face : faces )
  {
    points.push_back( baseVertexes[ face[ 0 ] ] );
    points.push_back( baseVertexes[ face[ 1 ] ] );
    points.push_back( baseVertexes[ face[ 2 ] ] );
  }
  std::vector< v3 > dividiedPoints;
  for( u32 i = 0; i < numDivisions; ++i)
  {
    dividiedPoints.reserve(points.size() * 4);
    for(unsigned iTri = 0; iTri < points.size() / 3; ++iTri)
    {
      v3& outer0 = points[ iTri * 3 + 0 ];
      v3& outer1 = points[ iTri * 3 + 1 ];
      v3& outer2 = points[ iTri * 3 + 2 ];

      v3 outer01 = ( outer0 + outer1 ) / 2.0f;
      v3 outer02 = ( outer0 + outer2  ) / 2.0f;
      v3 outer12 = ( outer1 + outer2 ) / 2.0f;
      outer01 = Normalize( outer01 );
      outer02 = Normalize( outer02 );
      outer12 = Normalize( outer12 );

      dividiedPoints.push_back( outer0 );
      dividiedPoints.push_back( outer01 );
      dividiedPoints.push_back( outer02 );

      dividiedPoints.push_back( outer1 );
      dividiedPoints.push_back( outer12 );
      dividiedPoints.push_back( outer01 );

      dividiedPoints.push_back( outer2 );
      dividiedPoints.push_back( outer02 );
      dividiedPoints.push_back( outer12 );

      dividiedPoints.push_back( outer01 );
      dividiedPoints.push_back( outer12 );
      dividiedPoints.push_back( outer02 );
    }
    points.swap( dividiedPoints );
    dividiedPoints.clear();
  }

  std::vector< v3 > mCol;
  std::vector< v3 > mNor;
  std::vector< v2 > mUVs;
  mCol.reserve( points.size() );
  mNor.reserve( points.size()  );
  mUVs.reserve( points.size() );
  for(const v3 & p : points)
  {
    mCol.push_back( V3( 1.0f, 1.0f, 1.0f) );
    mNor.push_back( p );
    float r = Length( p );
    mUVs.push_back( V2( atan2( p.y, p.x ), acos( p.z / r) ) );
  }

  std::vector< u32 > mIndexes;
  for( unsigned i = 0; i < points.size(); ++i )
    mIndexes.push_back( i );



  AttributeInput attributeData[ ( u32 )Attribute::eCount ] = {};

  {
    AttributeInput* input = &attributeData[ ( u32 )Attribute::ePos ];
    input->attributeComponentData = v3Data;
    input->memory = points.data();
    input->num = points.size();
  }

  {
    AttributeInput* input = &attributeData[ ( u32 )Attribute::eCol ];
    input->attributeComponentData = v3Data;
    input->memory = mCol.data();
    input->num = mCol.size();
  }

  {
    AttributeInput* input = &attributeData[ ( u32 )Attribute::eUV ];
    input->attributeComponentData = v2Data;
    input->memory = mUVs.data();
    input->num = mUVs.size();
  }

  {
    AttributeInput* input = &attributeData[ ( u32 )Attribute::eNor ];
    input->attributeComponentData = v3Data;
    input->memory = mNor.data();
    input->num = mNor.size();
  }

  LoadGeometry(
    geom,
    GL_TRIANGLES,
    mIndexes.data(),
    mIndexes.size(),
    GL_UNSIGNED_INT,
    attributeData );
}


void LoadUnitCube( GPUGeometry& geom )
{
  v3 baseCorners[ 8 ] = {
    { -0.5f, -0.5f,  0.5f },
    {  0.5f, -0.5f,  0.5f },
    { -0.5f,  0.5f,  0.5f },
    {  0.5f,  0.5f,  0.5f },
    { -0.5f,  0.5f, -0.5f },
    {  0.5f,  0.5f, -0.5f },
    { -0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f, -0.5f } };
  v2 baseUVs[ 4 ] = {
    { 0, 0 },
    { 1, 0 },
    { 0, 1 },
    { 1, 1 } };

  v3 baseNormals[ 6 ] = {
    {  0,  0,  1 },
    {  0,  1,  0 },
    {  0,  0, -1 },
    {  0, -1,  0 },
    {  1,  0,  0 },
    { -1,  0,  0 } };

  CubeFace cubeFaces[ 12 ] = {
    CubeFace( 1,1,1, 2,2,1, 3,3,1 ),
    CubeFace( 3,3,1, 2,2,1, 4,4,1 ),

    CubeFace( 3,1,2, 4,2,2, 5,3,2 ),
    CubeFace( 5,3,2, 4,2,2, 6,4,2 ),

    CubeFace( 5,4,3, 6,3,3, 7,2,3 ),
    CubeFace( 7,2,3, 6,3,3, 8,1,3 ),

    CubeFace( 7,1,4, 8,2,4, 1,3,4 ),
    CubeFace( 1,3,4, 8,2,4, 2,4,4 ),

    CubeFace( 2,1,5, 8,2,5, 4,3,5 ),
    CubeFace( 4,3,5, 8,2,5, 6,4,5 ),

    CubeFace( 7,1,6, 1,2,6, 5,3,6 ),
    CubeFace( 5,3,6, 1,2,6, 3,4,6 ) };

  v3 mPos[36];
  v3 mCol[36];
  v2 mUVs[36];
  v3 mNor[36];
  u32 indexes[36];

  u32 iVert = 0;
  for( CubeFace& face : cubeFaces )
  {
    for( CubeVertex& vert : face.vert )
    {
      mPos[ iVert ] = baseCorners[ vert.iPos - 1 ];
      mCol[ iVert ] = V3( 1.0f, 1.0f, 1.0f );
      mUVs[ iVert ] = baseUVs[ vert.iUV - 1 ];
      mNor[ iVert ] = baseNormals[ vert.iNor - 1 ];
      indexes[ iVert ] = iVert;
      ++iVert;
    }
  }


  AttributeInput attributeData[ ( u32 )Attribute::eCount ] = {};

  {
    AttributeInput* input = &attributeData[ ( u32 )Attribute::ePos ];
    input->attributeComponentData = v3Data;
    input->memory = mPos;
    input->num = ArraySize( mPos );
  }

  {
    AttributeInput* input = &attributeData[ ( u32 )Attribute::eCol ];
    input->attributeComponentData = v3Data;
    input->memory = mCol;
    input->num = ArraySize( mCol );
  }

  {
    AttributeInput* input = &attributeData[ ( u32 )Attribute::eUV ];
    input->attributeComponentData = v2Data;
    input->memory = mUVs;
    input->num = ArraySize( mUVs );
  }

  {
    AttributeInput* input = &attributeData[ ( u32 )Attribute::eNor ];
    input->attributeComponentData = v3Data;
    input->memory = mNor;
    input->num = ArraySize( mNor );
  }

  LoadGeometry( geom, GL_TRIANGLES, indexes, ArraySize( indexes ), GL_UNSIGNED_INT, attributeData );
}

void LoadNDCQuad( GPUGeometry& geom )
{
  u32 indexes[] = { 0, 1, 2,  0, 2, 3 };
  AttributeInput attributeData[ ( u32 )Attribute::eCount ] = {};
  v3 positions[] = {
    V3( 1.0f, -1.0f, 0.0f ),
    V3( 1.0f, 1.0f, 0.0f ),
    V3( -1.0f, 1.0f, 0.0f ),
    V3( -1.0f, -1.0f, 0.0f )
  };
  AttributeInput* input = &attributeData[ ( u32 )Attribute::ePos ];
  input->attributeComponentData.mComponentSize = sizeof( r32 );
  input->attributeComponentData.mComponentsPerVert = 3;
  input->attributeComponentData.mComponentType = GL_FLOAT;
  input->memory = positions;
  input->num = ArraySize( positions );

  v3 colors[] = {
    V3( 1.0f, 0.0f, 0.0f ),
    V3( 0.0f, 1.0f, 0.0f ),
    V3( 0.0f, 0.0f, 1.0f ),
    V3( 1.0f, 1.0f, 1.0f ),
  };
  input = &attributeData[ ( u32 )Attribute::eCol ];
  input->attributeComponentData.mComponentSize = sizeof( r32 );
  input->attributeComponentData.mComponentsPerVert = 3;
  input->attributeComponentData.mComponentType = GL_FLOAT;
  input->memory = colors;
  input->num = ArraySize( colors );

  v2 baseUVs[ 4 ] = {
    { 1, 0 },
    { 1, 1 },
    { 0, 1 },
    { 0, 0 } };
  input = &attributeData[ ( u32 )Attribute::eUV ];
  input->attributeComponentData.mComponentSize = sizeof( r32 );
  input->attributeComponentData.mComponentsPerVert = 2;
  input->attributeComponentData.mComponentType = GL_FLOAT;
  input->memory = baseUVs;
  input->num = ArraySize( baseUVs );

  LoadGeometry( 
    geom,
    GL_TRIANGLES,
    indexes,
    ArraySize( indexes ),
    GL_UNSIGNED_INT,
    attributeData );
}

