//
//  RNSkeleton.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSkeleton.h"
#include "RNFile.h"
#include "RNDebug.h"
#include "RNDictionary.h"
#include "RNResourceCoordinator.h"

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
		for (std::map<size_t, AnimationBone*>::iterator it = bones.begin(); it != bones.end(); ++it)
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
	
	float Animation::GetLength()
	{
		float length = 0.0f;
		for(auto bone : bones)
		{
			if(bone.second->prevFrame)
				length = fmaxf(length, bone.second->prevFrame->time);
		}
		return length;
	}
	
	Bone::Bone(const Vector3 &pos, const std::string bonename, bool root, bool absolute)
	{
		invBaseMatrix = Matrix::WithTranslation(pos*(-1.0f));
		relBaseMatrix = Matrix::WithTranslation(pos);
		
		name = bonename;
		isRoot = root;
		
		position = Vector3(0.0f, 0.0f, 0.0f);
		rotation.MakeIdentity();
		scale = Vector3(1.0, 1.0, 1.0);
		
		currFrame = 0;
		nextFrame = 0;
		currTime = 0.0f;
		finished = false;
		this->absolute = absolute;
	}
	
	Bone::Bone(const Matrix &basemat, const std::string bonename, bool root, bool absolute)
	{
		invBaseMatrix = basemat;
		relBaseMatrix = basemat.GetInverse();
		
		name = bonename;
		isRoot = root;
		
		position = Vector3(0.0f, 0.0f, 0.0f);
		rotation.MakeIdentity();
		scale = Vector3(1.0f, 1.0f, 1.0f);
		
		currFrame = 0;
		nextFrame = 0;
		currTime = 0.0f;
		finished = false;
		this->absolute = absolute;
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
		absolute = other.absolute;
	}
	
	void Bone::Init(Bone *parent)
	{
		for(int i = 0; i < children.size(); i++)
		{
			children[i]->Init(this);
		}
		
		if(parent != 0)
			relBaseMatrix = parent->invBaseMatrix*relBaseMatrix;
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
			
			if(currFrame != nextFrame) //bone not animated
			{
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
				position = currFrame->position.GetLerp(nextFrame->position, blend);
				scale = currFrame->scale.GetLerp(nextFrame->scale, blend);
				rotation.MakeLerpSpherical(currFrame->rotation, nextFrame->rotation, blend);
			}
			else
			{
				position = currFrame->position;
				scale = currFrame->scale;
				rotation = currFrame->rotation;
				running = false;
			}
		}
		else
		{
			running = false;
		}
		
		//TODO: Remove absolute flag...
		if(!absolute || !currFrame)
		{
			finalMatrix = relBaseMatrix;
			finalMatrix.Translate(position);
		}
		else
		{
			finalMatrix = Matrix::WithTranslation(position);
		}
		finalMatrix.Rotate(rotation);
		finalMatrix.Scale(scale);
		
		if(parent != 0)
		{
			finalMatrix = parent->finalMatrix*finalMatrix;
		}
		
		for(int i = 0; i < children.size(); i++)
		{
#if defined(RNDebugDrawSkeleton)
			Vector3 pos1 = finalMatrix.Transform(Vector3())*0.0088889f;
			Debug::AddLinePoint(pos1, Color::Red());
#endif
			
			if(children[i]->Update(this, timestep, restart))
				running = true;
		}
		
#if defined(RNDebugDrawSkeleton)
		if(children.size() == 0)
		{
			Vector3 pos1 = finalMatrix.Transform(Vector3())*0.0088889f;
			Debug::AddLinePoint(pos1, Color::Red());
			Debug::CloseLine();
		}
#endif
		
		finalMatrix = finalMatrix*invBaseMatrix;
		return running;
	}
	
	void Bone::SetAnimation(AnimationBone *animbone)
	{
		currTime = 0.0f;
		if(animbone)
		{
			currFrame = animbone;
			nextFrame = animbone->nextFrame;
			timeDiff = nextFrame->time-currFrame->time;
		}
		else
		{
			position = Vector3();
			rotation.MakeIdentity();
			scale = Vector3(1.0f, 1.0f, 1.0f);
			currFrame = nullptr;
			nextFrame = nullptr;
			timeDiff = 0.0f;
		}
	}
	
	
	Skeleton::Skeleton()
		: _blendanim(0), _curranim(0)
	{
	}
	
	Skeleton::Skeleton(const Skeleton *other)
		: _blendanim(0), _curranim(0)
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
			if(it->second)
				it->second->Release();
		}
		
		if(_blendanim)
			_blendanim->Release();
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
		if(_blendanim)
		{
			restart = false;
		}
		
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
		Debug::EndLine();
#endif
		
		if(!running && _blendanim)
		{
			_blendanim->Release();
			_blendanim = 0;
			SetTime(_blendtime);
		}
		
		return running;
	}
	
	void Skeleton::SetTime(float time)
	{
		SetAnimation(_curranim);
		Update(time);
	}
	
	void Skeleton::SetProgress(float progress)
	{
		if(_curranim)
		{
			float length = _curranim->GetLength();
			SetTime(progress*length);
		}
	}
	
	void Skeleton::SetAnimation(const std::string &animname)
	{
		Animation *anim = animations[animname];
		SetAnimation(anim);
	}
	
	void Skeleton::SetAnimation(Animation *anim)
	{
		if(!anim)
		{
			return;
		}
		for(int i = 0; i < bones.size(); i++)
		{
			AnimationBone *temp = anim->bones[i];
			bones[i].SetAnimation(temp);
		}
		
		if(anim != _blendanim)
		{
			_curranim = anim;
			if(_blendanim != 0)
			{
				_blendanim->Release();
				_blendanim = 0;
			}
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
				if(!bone->nextFrame || maxtime > bone->time || bone->nextFrame == bone)
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
				toanim->bones.insert(std::pair<size_t, AnimationBone*>(firstbone.first, newfirstbone));
			}
		}
		
		if(loop)
			toanim->MakeLoop();
	}
	
	void Skeleton::RemoveAnimation(const std::string &animname)
	{
		Animation *anim = animations[animname];
		if(anim == _curranim)
			_curranim = 0;
		
		animations.erase(animname);
		anim->Release();
	}
	
	void Skeleton::SetBlendAnimation(const std::string &to, float blendtime, float targettime)
	{
		_curranim = animations[to];
		_blendanim = new Animation("blend_to_"+to);
		_blendanim->Autorelease();
		_blendanim->Retain();
		
		for(auto bone : _curranim->bones)
		{
			Bone &currbone = bones[bone.first];
			AnimationBone *frombone = new AnimationBone(0, 0, 0.0f, currbone.position, currbone.scale, currbone.rotation);
			
			Bone tempbone(currbone);
			tempbone.SetAnimation(bone.second);
			tempbone.Update(0, targettime, true);
			
			AnimationBone *tobone = new AnimationBone(frombone, frombone, blendtime, tempbone.position, tempbone.scale, tempbone.rotation);
			frombone->prevFrame = tobone;
			frombone->nextFrame = tobone;
			_blendanim->bones.insert(std::pair<size_t, AnimationBone *>(bone.first, frombone));
		}
		
		_blendtime = targettime;
		SetAnimation(_blendanim);
	}
	
	std::vector<Bone *> Skeleton::GetBones(const std::string name)
	{
		std::vector<Bone *> out;
		for(int i = 0; i < bones.size(); i++)
			if(bones[i].name == name)
				out.push_back(&bones[i]);
		
		return out;
	}
	
	Skeleton *Skeleton::WithFile(const std::string& path)
	{
		Dictionary *settings = new Dictionary();
		settings->Autorelease();
		
		Skeleton *skeleton = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Skeleton>(RNSTR(path.c_str()), settings);
		return skeleton;
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
}
