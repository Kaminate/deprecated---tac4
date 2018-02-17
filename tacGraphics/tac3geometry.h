#pragma once
#include "tacLibrary\tacMath.h"
#include "tacGraphics\gl_core_4_4.h"
#include <map>
#include <vector>

// correspond to binding points in the vao
enum class Attribute
{
  ePos = 0,
  eCol = 1, 
  eUV = 2, 
  eNor = 3 , 
  eBoneId = 4 , 
  eBoneWeight = 5,
  eTangents = 6,
  eBitangents = 7,
  eCount
};
struct PerComponentData
{
  u32 mComponentsPerVert; // for Attribute::ePos = 3 floats
  u32 mComponentSize;     // for Attribute::ePos = sizeof( float )
  GLenum mComponentType;  // for Attribute::ePos = GL_FLOAT
};

const extern PerComponentData v3Data;
const extern PerComponentData v2Data;

struct GPUGeometry
{
  GLuint mFBOs[ Attribute::eCount ];
  GLuint mVAO;

  // http://docs.gl/gl4/glDrawElements:
  // must be GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, or GL_UNSIGNED_INT.
  GLenum mIndexType; 
  GLuint mIndexBuffer;
  u32 mIndexCount;

  GLenum mprimitive;
};

struct AttributeInput
{
  void* memory;
  unsigned num;
  PerComponentData attributeComponentData;
};

void LoadGeometry(
  GPUGeometry& geom,
  GLenum primitive,
  void* indexes,
  u32 indexCount,
  GLenum indexType,
  AttributeInput attributeData[ ( u32 )Attribute::eCount ] );

void LoadUnitCube( GPUGeometry& geom );
void LoadNDCQuad( GPUGeometry& geom );
void LoadUnitSphere( GPUGeometry& geom, u32 numDivisions );



inline void DrawGPUGeometry( const GPUGeometry& geom )
{
  glBindVertexArray( geom.mVAO );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geom.mIndexBuffer );
  glDrawElements(
    geom.mprimitive,
    geom.mIndexCount,
    geom.mIndexType,
    nullptr );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
  glBindVertexArray( 0 );
}
