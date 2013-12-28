//
//  RNSkeleton.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SKELETON_H__
#define __RAYNE_SKELETON_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNMesh.h"
#include "RNMaterial.h"
#include "RNArray.h"
#include "RNQuaternion.h"
#include "RNMatrix.h"

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
		RNAPI Animation(const std::string &animname);
		RNAPI ~Animation();
		
		void MakeLoop();
		float GetLength();
		
		std::string name;
		std::map<int, AnimationBone*> bones;
	};
	
	class Bone
	{
	public:
		RNAPI Bone(const Vector3 &pos, const std::string bonename, bool root);
		RNAPI Bone(const Bone &other);
		
		RNAPI void Init(Bone *parent = 0);
		RNAPI bool Update(Bone *parent, float timestep, bool restart);
		
		RNAPI void SetAnimation(AnimationBone *anim);
		
		Matrix relBaseMatrix;
		Matrix invBaseMatrix;
		
		Vector3 position;
		Quaternion rotation;
		Vector3 scale;
		
		Matrix finalMatrix;
		
		std::string name;
		bool isRoot;
		
		std::vector<Bone*> children;
		std::vector<uint16> tempChildren;
		
		AnimationBone *currFrame;
		AnimationBone *nextFrame;
		
		float currTime;
		float timeDiff;
		bool finished;
		
	private:
	};
	
	class Skeleton : public Object
	{
	public:
		RNAPI Skeleton();
		RNAPI Skeleton(const Skeleton *other);
		
		RNAPI ~Skeleton() override;
		
		RNAPI static Skeleton *WithFile(const std::string& path);
		RNAPI static Skeleton *WithSkeleton(const Skeleton *other);
		RNAPI static Skeleton *Empty();
		
		RNAPI void Init();
		RNAPI bool Update(float timestep, bool restart = true);
		RNAPI void SetTime(float time);
		RNAPI void SetProgress(float progress);
		RNAPI void SetAnimation(const std::string &animname);
		RNAPI void SetAnimation(Animation *anim);
		RNAPI void SetBlendAnimation(const std::string &to, float blendtime, float targettime = 0.0f);
		RNAPI void CopyAnimation(const std::string &from, const std::string &to, float start, float end, bool loop = true);
		RNAPI void RemoveAnimation(const std::string &animname);
		
		RNAPI Bone *GetBone(const std::string name);
		RNAPI uint16 GetBoneCount() const { return bones.size(); }
		RNAPI const std::vector<Matrix>& GetMatrices() const { return _matrices; }
		
		std::vector<Bone> bones;
		std::map<std::string, Animation*> animations;
		std::vector<Matrix> _matrices;
		
	private:
		Animation *_blendanim;
		float _blendtime;
		Animation *_curranim;
	};
}

#endif /* __RAYNE_SKELETON_H__ */
