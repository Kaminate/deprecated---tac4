#include "tac4Animation.h"
#include "tac4Model.h"

TacAnimationDriver::~TacAnimationDriver()
{

}
TacAnimationDriver::TacAnimationDriver(
  TacModelInstance* modelInstance )
{
  this->myModelInstance = modelInstance;
}

// Weighted interpolation between different animations
// @animations - an array of animation*
// @times - the time elapsed in the animation
// @weights - how much to weight each animation. Adds up to 1
void TacAnimationDriver::SetFinalPose(
  const TacAnimation** animations,
  const r32* times,
  const r32* weights,
  u32 num )
{
  TacUnusedParameter( weights);
  // TODO( N8 ): TEMP! We do not yet support animation BLENDING!
  TacAssert( num == 1 );

  const TacModel* m = myModelInstance->model;

  myModelInstance->CopyAnimatedTransforms();

  r32 time = times[ 0 ];
  //r32 weight = weights[ 0 ]; ( only used in animation BLENDING )
  const TacAnimation* myAnimation = animations[ 0 ];
  TacAssert( myAnimation );
  for( u32 iTrack = 0; iTrack < myAnimation->tracks.size(); ++iTrack )
  {
    const TacTrack& track = myAnimation->tracks[ iTrack ];
    track.nodeIndex;
    track.keyFrames;
    u32 iKey = 0;
    while( iKey < track.keyFrames.size() - 1 )
    {
      const TacKeyFrame& curKeyFrame = track.keyFrames[ iKey ];
      if( curKeyFrame.time > time )
      {
        break;
      }
      ++iKey;
    }
    u32 iPrevKey = iKey > 0 ? iKey - 1 : iKey;

    TacAssertIndex( iPrevKey, track.keyFrames.size() );
    TacAssertIndex( iKey, track.keyFrames.size() );
    const TacKeyFrame& prevFrame = track.keyFrames[ iPrevKey ];
    const TacKeyFrame& nextFrame = track.keyFrames[ iKey ];

    v3 scale = { 1, 1, 1 };
    m3 rotation;
    v3 translation;
    if( iPrevKey == iKey )
    {
      rotation = QuaternionToMatrix( prevFrame.rotationQuaternion );
      translation = prevFrame.translation;
    }
    else
    {
      // NOTE( N8 ): Avoid division by 0
      TacAssert( nextFrame.time - prevFrame.time > 0 );
      r32 t01 =
        ( time - prevFrame.time ) /
        ( nextFrame.time - prevFrame.time );
      rotation = QuaternionToMatrix( Slerp(
        prevFrame.rotationQuaternion,
        nextFrame.rotationQuaternion,
        t01 ) );
      translation = Lerp(
        prevFrame.translation,
        nextFrame.translation,
        t01 );
    }

    // set the matrix for the hierarachy
    // ( note the individual submodels bone count <= total node count )
    myModelInstance->animatedTransforms[ track.nodeIndex ] = M4Transform(
      scale,
      rotation, 
      translation );
  }

  myModelInstance->ConcatinateAnimatedTransforms();

  for( u32 iSubModel = 0; iSubModel < m->numSubModels; ++iSubModel )
  {
    const std::vector< TacBone >& bones = m->subModels[ iSubModel ].bones;

    TacSubModelInstance& mySubModelInstance =
      myModelInstance->subModelInstances[ iSubModel ];

    // The end result ( the matrix buffer sent to the shader )
    mySubModelInstance.finalMatrixes.resize( bones.size() );
    for( u32 iBone = 0; iBone < bones.size(); ++iBone )
    {
      const TacBone& myBone = bones[ iBone ];
      mySubModelInstance.finalMatrixes[ iBone ] =
        myModelInstance->animatedTransforms[ myBone.nodeIndex ] *
        myBone.offset;
    }
  }

}

