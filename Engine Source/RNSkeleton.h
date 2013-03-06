//
//  RNSkeleton.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
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
		
		std::string name;
		std::map<int, AnimationBone*> bones;
	};
	
	class Bone
	{
	public:
		Bone(Vector3 &pos, std::string bonename, bool root);
		Bone(const Bone &other);
		
		void Init(Bone *parent = 0);
		void Update(Bone *parent = 0, float timestep = 0.01f);
		
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
	};
	
	class Skeleton : public Object
	{
	public:
		Skeleton();
		Skeleton(const std::string& path);
		
		virtual ~Skeleton();
		
		static Skeleton *WithFile(const std::string& path);
		static Skeleton *Empty();
		
		void Init();
		void Update(float timestep);
		void SetAnimation(const std::string &animname);
		uint16 NumBones() const {return bones.size();}
		float *Matrices() const {return matrices;}
		
		std::vector<Bone> bones;
		std::map<std::string, Animation*> animations;
		float *matrices;
		
	private:
		void ReadSkeletonVersion1(File *file);
		void ReadBVH(File *file);
	};
}

#endif /* __RAYNE_SKELETON_H__ */
