//
//  RNSkeleton.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSkeleton.h"
#include "RNFile.h"

namespace RN
{
	AnimationBone::AnimationBone(AnimationBone *prev, AnimationBone *next, const float frametime, const Vector3 &pos, const Vector3 &scal, const Quaternion &rot)
	{
		time = frametime;
		position = pos;
		scale = scal;
		rotation = rot;
		
		prevFrame = prev;
		nextFrame = next;
	}
	
	Animation::Animation(const std::string &animname)
	{
		name = animname;
	}
	
	Animation::~Animation()
	{
		for (std::map<int, AnimationBone*>::iterator it = bones.begin(); it != bones.end(); ++it)
		{
			AnimationBone *bone = it->second;
			AnimationBone *first = bone;
			while(bone)
			{
				bone = bone->nextFrame;
				delete bone->prevFrame;
				if(bone == first)
					break;
			}
			if(bone != first)
				delete bone;
		}
	}
	
	Bone::Bone(Vector3 &pos, std::string bonename, bool root)
	{
		invBaseMatrix.MakeTranslate(pos*(-1.0f));
		
		name = bonename;
		isRoot = root;
		
		position = pos;
		rotation.MakeIdentity();
		scale = Vector3(1.0, 1.0, 1.0);
		
		currFrame = 0;
		nextFrame = 0;
		currTime = 0.0f;
	}
	
	Bone::Bone(const Bone &other)
	{
		relBaseMatrix = other.relBaseMatrix;
		invBaseMatrix = other.invBaseMatrix;
		position = other.position;
		rotation = other.rotation;
		scale = other.scale;
		finalMatrix = other.finalMatrix;
		name = other.name;
		isRoot = other.isRoot;
		tempChildren = other.tempChildren;
		currFrame = 0;
		nextFrame = 0;
		currTime = 0.0f;
	}
	
	void Bone::Init(Bone *parent)
	{
		for(int i = 0; i < children.size(); i++)
		{
			children[i]->Init(this);
		}
		
		if(parent != 0)
			position -= parent->position;
		relBaseMatrix.MakeTranslate(position);
		position = 0.0f;
	}
	
	void Bone::Update(Bone *parent, float timestep)
	{
		if(currFrame != 0 && nextFrame != 0)
		{
			currTime += timestep;
			while(currTime > nextFrame->time)
			{
				if(currFrame->time > nextFrame->time)
					currTime = nextFrame->time;
				currFrame = nextFrame;
				nextFrame = nextFrame->nextFrame;
				timeDiff = nextFrame->time-currFrame->time;
			}
			
			float blend = (currTime-currFrame->time)/timeDiff;
			position = currFrame->position.Lerp(nextFrame->position, blend);
			scale = currFrame->scale.Lerp(nextFrame->scale, blend);
			rotation.MakeLerpS(currFrame->rotation, nextFrame->rotation, blend);
		}
		
		finalMatrix = relBaseMatrix;
		finalMatrix.Translate(position);
		finalMatrix.Scale(scale);
		finalMatrix.Rotate(rotation);
		if(parent != 0)
		{
			finalMatrix = parent->finalMatrix*finalMatrix;
		}
		
		for(int i = 0; i < children.size(); i++)
		{
			children[i]->Update(this, timestep);
		}
		
		finalMatrix = finalMatrix*invBaseMatrix;
	}
	
	void Bone::SetAnimation(AnimationBone *animbone)
	{
		currTime = 0.0f;
		currFrame = animbone;
		nextFrame = animbone->nextFrame;
		timeDiff = nextFrame->time-currFrame->time;
	}
	
	
	Skeleton::Skeleton()
	{
	}

	Skeleton::Skeleton(const std::string& path)
	{
		File *file = new File(path);
		
		uint32 magic = file->ReadUint32();
		
		if(magic == 383405658)
		{
			uint32 version = file->ReadUint8();
			
			switch(version)
			{
				case 1:
					ReadSkeletonVersion1(file);
					break;
					
				default:
					break;
			}
		}
		
		file->Release();
	}
	
	Skeleton::~Skeleton()
	{
		for (std::map<std::string, Animation*>::iterator it = animations.begin(); it != animations.end(); ++it)
		{
			it->second->Release();
		}
	}
	
	void Skeleton::Init()
	{
		if(_matrices->Count() > 0)
			return;
		
		for(int i = 0; i < bones.size(); i++)
		{
			for(int n = 0; n < bones[i].tempChildren.size(); n++)
			{
				bones[i].children.push_back(&(bones[bones[i].tempChildren[n]]));
			}
		}
		
		for(int i = 0; i < bones.size(); i++)
		{
			_matrices->AddObject(bones[i].finalMatrix);
			if(bones[i].isRoot)
			{
				bones[i].Init();
			}
		}
	}
	
	void Skeleton::Update(float timestep)
	{
		for(int i = 0; i < bones.size(); i++)
		{
			if(bones[i].isRoot)
			{
				bones[i].Update(0, timestep);
			}
		}
		
		for(int i = 0; i < bones.size(); i++)
		{
			_matrices->ReplaceObjectAtIndex(i, bones[i].finalMatrix);
		}
	}
	
	void Skeleton::SetAnimation(const std::string &animname)
	{
		Animation *anim = animations[animname];
		if(!anim)
		{
			return;
		}
		for(int i = 0; i < bones.size(); i++)
		{
			AnimationBone *temp = anim->bones[i];
			if(temp)
				bones[i].SetAnimation(temp);
		}
	}
	
	Skeleton *Skeleton::WithFile(const std::string& path)
	{
		Skeleton *skeleton = new Skeleton(path);
		return skeleton->Autorelease<Skeleton>();
	}
		
	Skeleton *Skeleton::Empty()
	{
		Skeleton *skeleton = new Skeleton();
		return skeleton->Autorelease<Skeleton>();
	}
	
	void Skeleton::ReadSkeletonVersion1(File *file)
	{
		uint16 lenskeletonname = file->ReadUint16();
		char *skeletonname = new char[lenskeletonname];
		file->ReadIntoBuffer(skeletonname, lenskeletonname);
		delete[] skeletonname;
		
		uint16 bonecount = file->ReadUint16();
		for(int i = 0; i < bonecount; i++)
		{
			uint16 lenbonename = file->ReadUint16();
			char *bonename = new char[lenbonename];
			file->ReadIntoBuffer(bonename, lenbonename);
			
			Vector3 bonepos;
			file->ReadIntoBuffer(&bonepos.x, sizeof(Vector3));
			
			uint8 isroot = file->ReadUint8();
			
			Bone bone(bonepos, bonename, isroot);
			delete[] bonename;
			
			uint16 numchildren = file->ReadUint16();
			for(int n = 0; n < numchildren; n++)
			{
				uint16 child = file->ReadUint16();
				bone.tempChildren.push_back(child);
			}
			
			bones.push_back(bone);
		}
		
		uint16 numanims = file->ReadUint16();
		for(int i = 0; i < numanims; i++)
		{
			uint16 lenanimname = file->ReadUint16();
			char *animname = new char[lenanimname];
			file->ReadIntoBuffer(animname, lenanimname);
			
			Animation *anim = new Animation(animname);
			anim->Autorelease<Animation>();
			anim->Retain();
			animations.insert(std::pair<std::string, Animation*>(animname, anim));
			delete[] animname;
			
			uint16 numanimbones = file->ReadUint16();
			for(int n = 0; n < numanimbones; n++)
			{
				AnimationBone *animbone = 0;
				uint16 boneid = file->ReadUint16();
				uint32 numframes = file->ReadUint32();
				for(int f = 0; f < numframes; f++)
				{
					float time = file->ReadFloat();
					Vector3 animbonepos;
					file->ReadIntoBuffer(&animbonepos.x, sizeof(Vector3));
					Vector3 animbonescale;
					file->ReadIntoBuffer(&animbonescale.x, sizeof(Vector3));
					Quaternion animbonerot;
					file->ReadIntoBuffer(&animbonerot.x, sizeof(Quaternion));
					
					animbone = new AnimationBone(animbone, 0, time, animbonepos, animbonescale, animbonerot);
				}
				
				AnimationBone *lastbone = animbone;
				while(animbone->prevFrame != 0)
				{
					animbone->prevFrame->nextFrame = animbone;
					animbone = animbone->prevFrame;
				}
				animbone->prevFrame = lastbone;
				lastbone->nextFrame = animbone;
				anim->bones.insert(std::pair<int, AnimationBone*>(boneid, animbone));
			}
		}
	}
}
