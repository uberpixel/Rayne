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
		AnimationBone(AnimationBone *prev, AnimationBone *next, const float frametime, const Vector3 &pos, const Vector3 &scal, const Quaternion &rot);
		
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
		Animation(const std::string &animname);
		~Animation();
		
		void MakeLoop();
		
		std::string name;
		std::map<int, AnimationBone*> bones;
	};
	
	class Bone
	{
	public:
		Bone(Vector3 &pos, std::string bonename, bool root);
		Bone(const Bone &other);
		
		void Init(Bone *parent = 0);
		bool Update(Bone *parent, float timestep, bool restart);
		
		void SetAnimation(AnimationBone *anim);
		
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
	};
	
	class Skeleton : public Object
	{
	public:
		Skeleton();
		Skeleton(const std::string& path);
		Skeleton(const Skeleton *other);
		
		virtual ~Skeleton();
		
		static Skeleton *WithFile(const std::string& path);
		static Skeleton *WithSkeleton(const Skeleton *other);
		static Skeleton *Empty();
		
		void Init();
		bool Update(float timestep, bool restart = true);
		void SetAnimation(const std::string &animname);
		void SetBlendAnimation(const std::string &to, float time = 0.0f);
		void CopyAnimation(const std::string &from, const std::string &to, float start, float end, bool loop = true);
		void RemoveAnimation(const std::string &animname);
		Bone *GetBone(const std::string name);
		uint16 NumBones() const { return bones.size(); }
		const std::vector<Matrix>& Matrices() const { return _matrices; }
		
		std::vector<Bone> bones;
		std::map<std::string, Animation*> animations;
		std::vector<Matrix> _matrices;
		
	private:
		void ReadSkeletonVersion1(File *file);
		void ReadBVH(File *file);
		
		Animation *_tempanim;
	};
}

#endif /* __RAYNE_SKELETON_H__ */
