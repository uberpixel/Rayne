//
//  RNSkeleton.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSkeleton.h"
#include "RNFile.h"
#include "RNDebug.h"

//#define RNDebugDrawSkeleton

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
			if(bone->prevFrame != 0)
			{
				bone->prevFrame->nextFrame = 0;
				bone->prevFrame = 0;
			}
			while(bone->nextFrame)
			{
				bone = bone->nextFrame;
				delete bone->prevFrame;
			}
			delete bone;
		}
	}
	
	void Animation::MakeLoop()
	{
		for(auto bone : bones)
		{
			AnimationBone *temp = bone.second;
			temp->prevFrame->nextFrame = new AnimationBone(temp->prevFrame, temp, temp->prevFrame->time+1+temp->time, temp->position, temp->scale, temp->rotation);
			temp->prevFrame = temp->prevFrame->nextFrame;
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
		finished = false;
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
		finished = false;
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
	
	bool Bone::Update(Bone *parent, float timestep, bool restart)
	{
		bool running = true;
		if(currFrame != 0 && nextFrame != 0)
		{
			if(finished && restart)
			{
				finished = false;
				currTime = 0.0f;
			}
			
			currTime += timestep;
			
			while(currTime > nextFrame->time)
			{
				if(currFrame->time > nextFrame->time)
				{
					if(restart)
					{
						currTime -= currFrame->time;
					}
					else
					{
						finished = true;
						running = false;
						currTime = currFrame->time;
						break;
					}
				}
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
#if defined(RNDebugDrawSkeleton)
			RN::Vector3 pos1 = finalMatrix.Transform(RN::Vector3())*0.0088889f;
			RN::Debug::AddLinePoint(pos1, RN::Color::Red());
#endif
			
			if(children[i]->Update(this, timestep, restart))
				running = true;
		}
		
#if defined(RNDebugDrawSkeleton)
		if(children.size() == 0)
		{
			RN::Vector3 pos1 = finalMatrix.Transform(RN::Vector3())*0.0088889f;
			RN::Debug::AddLinePoint(pos1, RN::Color::Red());
			RN::Debug::CloseLine();
		}
#endif
		
		finalMatrix = finalMatrix*invBaseMatrix;
		return running;
	}
	
	void Bone::SetAnimation(AnimationBone *animbone)
	{
		currTime = 0.0f;
		currFrame = animbone;
		nextFrame = animbone->nextFrame;
		timeDiff = nextFrame->time-currFrame->time;
	}
	
	
	Skeleton::Skeleton()
		: _tempanim(NULL)
	{
	}

	Skeleton::Skeleton(const std::string& path)
		: _tempanim(NULL)
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
		
		Init();
	}
	
	Skeleton::Skeleton(const Skeleton *other)
		: _tempanim(NULL)
	{
		for(int i = 0; i < other->bones.size(); i++)
			bones.push_back(other->bones[i]);
		
		animations = other->animations;
		for(auto anim : animations)
			anim.second->Retain();
		
		for(int i = 0; i < bones.size(); i++)
		{
			for(int n = 0; n < bones[i].tempChildren.size(); n++)
			{
				bones[i].children.push_back(&(bones[bones[i].tempChildren[n]]));
			}
		}
		
		for(int i = 0; i < bones.size(); i++)
		{
			_matrices.push_back(bones[i].finalMatrix);
		}
	}
	
	Skeleton::~Skeleton()
	{
		for (std::map<std::string, Animation*>::iterator it = animations.begin(); it != animations.end(); ++it)
		{
			it->second->Release();
		}
		
		if(_tempanim)
			_tempanim->Release();
	}
	
	void Skeleton::Init()
	{
		if(_matrices.size() > 0)
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
			_matrices.push_back(bones[i].finalMatrix);
			if(bones[i].isRoot)
			{
				bones[i].Init();
			}
		}
	}
	
	bool Skeleton::Update(float timestep, bool restart)
	{
		bool running = false;
		for(int i = 0; i < bones.size(); i++)
		{
			if(bones[i].isRoot)
			{
				if(bones[i].Update(0, timestep, restart))
					running = true;
			}
		}
		
		for(int i = 0; i < bones.size(); i++)
		{
			_matrices[i] = bones[i].finalMatrix;
		}
		
#if defined(RNDebugDrawSkeleton)
		RN::Debug::EndLine();
#endif
		
		return running;
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
	
	void Skeleton::CopyAnimation(const std::string &from, const std::string &to, float start, float end, bool loop)
	{
		Animation *fromanim = animations[from];
		Animation *toanim = new Animation(to);
		animations.insert(std::pair<std::string, Animation*>(to, toanim));
		toanim->Autorelease();
		toanim->Retain();
		
		for(auto firstbone : fromanim->bones)
		{
			auto bone = firstbone.second;
			float maxtime = bone->time;
			bool inrange = false;
			while(bone->time <= start && bone->time < end)
			{
				if(!bone->nextFrame || maxtime > bone->time)
				{
					inrange = false;
					break;
				}
				
				inrange = true;
				maxtime = bone->time;
				bone = bone->nextFrame;
			}
			
			if(inrange)
			{
				AnimationBone *newfirstbone = new AnimationBone(0, 0, bone->time-start, bone->position, bone->scale, bone->rotation);
				AnimationBone *newcurrbone = newfirstbone;
				while(bone->nextFrame && maxtime <= bone->time)
				{
					maxtime = bone->time;
					bone = bone->nextFrame;
					if(bone->time > end)
						break;
					
					newcurrbone = new AnimationBone(newcurrbone, 0, bone->time-start, bone->position, bone->scale, bone->rotation);
					newcurrbone->prevFrame->nextFrame = newcurrbone;
				}
				newcurrbone->nextFrame = newfirstbone;
				newfirstbone->prevFrame = newcurrbone;
				toanim->bones.insert(std::pair<int, AnimationBone*>(firstbone.first, newfirstbone));
			}
		}
		
		if(loop)
			toanim->MakeLoop();
	}
	
	void Skeleton::RemoveAnimation(const std::string &animname)
	{
		Animation *anim = animations[animname];
		animations.erase(animname);
		anim->Release();
	}
	
	void Skeleton::SetBlendAnimation(const std::string &to, float time)
	{
		//_tempanim =
	}
	
	Bone *Skeleton::GetBone(const std::string name)
	{
		for(int i = 0; i < bones.size(); i++)
			if(bones[i].name == name)
				return &bones[i];
		
		return NULL;
	}
	
	Skeleton *Skeleton::WithFile(const std::string& path)
	{
		Skeleton *skeleton = new Skeleton(path);
		return (Skeleton *)skeleton->Autorelease();
	}
		
	Skeleton *Skeleton::Empty()
	{
		Skeleton *skeleton = new Skeleton();
		return (Skeleton *)skeleton->Autorelease();
	}
	
	Skeleton *Skeleton::WithSkeleton(const Skeleton *other)
	{
		Skeleton *skeleton = new Skeleton(other);
		return (Skeleton *)skeleton->Autorelease();
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
			anim->Autorelease();
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
