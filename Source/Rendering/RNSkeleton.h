//
//  RNSkeleton.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SKELETON_H__
#define __RAYNE_SKELETON_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Assets/RNAsset.h"
#include "../Math/RNMatrix.h"
#include "../Math/RNQuaternion.h"
#include "../Math/RNVector.h"

namespace RN
{
	class AnimationBone
	{
	public:
		RNAPI AnimationBone(AnimationBone *prev, AnimationBone *next, const float frametime, const Vector3 &pos, const Vector3 &scal, const Quaternion &rot);
		
		float time;
		
		Vector3 position;
		Vector3 scale;
		Quaternion rotation;
		
		AnimationBone *nextFrame;
		AnimationBone *prevFrame;
	};
	
	class Animation : public Object
	{
	public:
		RNAPI Animation(const String *animname);
		RNAPI ~Animation();
		
		RNAPI void MakeLoop();
		RNAPI float GetLength();
		
		String *name;
		std::map<size_t, AnimationBone*> bones;
		
		__RNDeclareMetaInternal(Animation)
	};
	
	class Bone
	{
	public:
		RNAPI Bone(const Vector3 &pos, const String *bonename, bool root);
		RNAPI Bone(const Matrix &basemat, const String *bonename, bool root);
		RNAPI Bone(const Bone &other);
		
		RNAPI ~Bone();
		
		RNAPI void Init(Bone *parent = 0);
		RNAPI bool Update(Bone *parent, float timestep, bool restart);
		
		RNAPI void SetAnimation(AnimationBone *anim);
		
		Matrix relBaseMatrix;
		Matrix invBaseMatrix;
		
		Vector3 position;
		Quaternion rotation;
		Vector3 scale;
		
		Matrix finalMatrix;
		
		String *name;
		bool isRoot;
		
		std::vector<Bone*> children;
		std::vector<uint16> tempChildren;
		
		AnimationBone *currFrame;
		AnimationBone *nextFrame;
		
		float currTime;
		float timeDiff;
		bool finished;
		bool absolute;
		
	private:
	};
	
	class Skeleton : public Asset
	{
	public:
		RNAPI Skeleton();
		RNAPI Skeleton(const Skeleton *other);
		
		RNAPI ~Skeleton() override;
		
		RNAPI static Skeleton *WithName(const String *name, const Dictionary *settings = nullptr);
		RNAPI static Skeleton *WithSkeleton(const Skeleton *other);
		RNAPI static Skeleton *Empty();
		
		RNAPI void Init();
		RNAPI bool Update(float timestep, bool restart = true);
		RNAPI void SetTime(float time);
		RNAPI void SetProgress(float progress);
		RNAPI void SetAnimation(const String *animname);
		RNAPI void SetAnimation(Animation *anim);
		RNAPI void SetBlendAnimation(const String *to, float blendtime, float targettime = 0.0f);
		RNAPI void CopyAnimation(const String *from, const String *to, float start, float end, bool loop = true);
		RNAPI void RemoveAnimation(const String *animname);
		
		RNAPI std::vector<Bone *> GetBones(const String *name);
		RNAPI uint16 GetBoneCount() const { return bones.size(); }
		RNAPI const std::vector<Matrix>& GetMatrices() const { return _matrices; }
		
		std::vector<Bone> bones;
		Dictionary *animations;
		std::vector<Matrix> _matrices;
		
	private:
		Animation *_blendanim;
		float _blendtime;
		Animation *_curranim;
		
		__RNDeclareMetaInternal(Skeleton)
	};
}

#endif /* __RAYNE_SKELETON_H__ */
