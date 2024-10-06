//
//  RNSkeleton.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSkeleton.h"
#include "../Objects/RNDictionary.h"
#include "../System/RNFile.h"
#include "../Assets/RNAssetManager.h"
#include "../Debug/RNLogger.h"

namespace RN
{
	RNDefineMeta(Skeleton, Asset)
	RNDefineMeta(Animation, Object)
	
	AnimationBone::AnimationBone(AnimationBone *prev, AnimationBone *next, const float frametime, const Vector3 &pos, const Vector3 &scal, const Quaternion &rot)
	{
		time = frametime;
		position = pos;
		scale = scal;
		rotation = rot;
		
		prevFrame = prev;
		nextFrame = next;
	}
	
	Animation::Animation(const String *animname)
	{
		name = animname->Copy();
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
		
		name->Release();
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
	
	Bone::Bone(const Vector3 &pos, const String *bonename, bool root)
	{
		invBaseMatrix = Matrix::WithTranslation(pos*(-1.0f));
		relBaseMatrix = Matrix::WithTranslation(pos);
		
		name = bonename->Copy();
		isRoot = root;
		
		position = Vector3(0.0f, 0.0f, 0.0f);
		rotation = Quaternion::WithIdentity();
		scale = Vector3(1.0, 1.0, 1.0);
		
		currFrame = 0;
		nextFrame = 0;
		currTime = 0.0f;
		finished = false;
		absolute = false;
	}
	
	Bone::Bone(const Matrix &basemat, const String *bonename, bool root)
	{
		invBaseMatrix = basemat;
		relBaseMatrix = basemat.GetInverse();
		
		name = bonename->Copy();
		isRoot = root;
		
		position = Vector3(0.0f, 0.0f, 0.0f);
		rotation = Quaternion::WithIdentity();
		scale = Vector3(1.0f, 1.0f, 1.0f);
		
		currFrame = 0;
		nextFrame = 0;
		currTime = 0.0f;
		finished = false;
		absolute = false;
	}
	
	Bone::Bone(const Bone &other)
	{
		relBaseMatrix = other.relBaseMatrix;
		invBaseMatrix = other.invBaseMatrix;
		position = other.position;
		rotation = other.rotation;
		scale = other.scale;
		finalMatrix = other.finalMatrix;
		name = other.name->Retain();
		isRoot = other.isRoot;
		tempChildren = other.tempChildren;
		currFrame = 0;
		nextFrame = 0;
		currTime = 0.0f;
		finished = false;
		absolute = other.absolute;
	}
	
	Bone::~Bone()
	{
		name->Release();
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
			
			if(currFrame != nextFrame) //bone is animated
			{
				float blend = 0.0f;
				if(timestep >= 0.0f)
				{
					if(nextFrame == currFrame->prevFrame) nextFrame = currFrame->nextFrame;
					while(currTime > nextFrame->time)
					{
						if(currFrame->time > nextFrame->time)
						{
							if(restart)
							{
								currTime = std::fmod(currTime, animationLength);
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
					
					blend = (currTime-currFrame->time)/timeDiff;
				}
				else
				{
					if(nextFrame == currFrame->nextFrame) nextFrame = currFrame->prevFrame;
					while(currTime < nextFrame->time)
					{
						if(currFrame->time < nextFrame->time)
						{
							if(restart)
							{
								currTime = animationLength + std::fmod(currTime, animationLength);
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
						nextFrame = nextFrame->prevFrame;
						timeDiff = currFrame->time - nextFrame->time;
					}
					
					blend = (currFrame->time-currTime)/timeDiff;
				}
			
				position = currFrame->position.GetLerp(nextFrame->position, blend);
				scale = currFrame->scale.GetLerp(nextFrame->scale, blend);
				rotation = Quaternion::WithLerpSpherical(currFrame->rotation, nextFrame->rotation, blend);
			}
			else //bone is not animated
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
		
		if(absolute)
		{
			finalMatrix = Matrix::WithIdentity();
		}
		else
		{
			finalMatrix = relBaseMatrix;
		}
		
		finalMatrix.Translate(position);
		finalMatrix.Rotate(rotation);
		finalMatrix.Scale(scale);
		
		if(parent != 0 && !absolute)
		{
			finalMatrix = parent->finalMatrix * finalMatrix;
		}
		
		for(int i = 0; i < children.size(); i++)
		{
			if(children[i]->Update(this, timestep, restart))
				running = true;
		}
		
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
			animationLength = animbone->prevFrame->time; //Get the time of the last frame
		}
		else
		{
			position = Vector3();
			rotation = Quaternion::WithIdentity();
			scale = Vector3(1.0f, 1.0f, 1.0f);
			currFrame = nullptr;
			nextFrame = nullptr;
			timeDiff = 0.0f;
			animationLength = 0.0f;
		}
	}
	
	
	Skeleton::Skeleton()
		: _blendanim(0), _curranim(0)
	{
		animations = new Dictionary();
	}
	
	Skeleton::Skeleton(const Skeleton *other)
		: _blendanim(0), _curranim(0)
	{
		for(int i = 0; i < other->bones.size(); i++)
			bones.push_back(other->bones[i]);
		
		animations = other->animations->Retain();
		
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
		animations->Release();
		
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
	
	void Skeleton::SetAnimation(const String *animname)
	{
		Animation *anim = animations->GetObjectForKey<Animation>(animname);
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
		
		Update(0.0f); //Update to get all bones in a good state
	}
	
	void Skeleton::CopyAnimation(const String *from, const String *to, float start, float end, bool loop)
	{
		Animation *fromanim = animations->GetObjectForKey<Animation>(from);
		Animation *toanim = new Animation(to);
		animations->SetObjectForKey(toanim->Autorelease(), to);
		
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
	
	void Skeleton::RemoveAnimation(const String *animname)
	{
		Animation *anim = animations->GetObjectForKey<Animation>(animname);
		if(anim == _curranim)
			_curranim = nullptr;
		
		animations->RemoveObjectForKey(animname);
	}
	
	void Skeleton::SetBlendAnimation(const String *to, float blendtime, float targettime)
	{
		_curranim = animations->GetObjectForKey<Animation>(to);
		_blendanim = new Animation(RNSTR("blend_to_" << to));
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
	
	std::vector<Bone *> Skeleton::GetBones(const String *name)
	{
		std::vector<Bone *> out;
		for(int i = 0; i < bones.size(); i++)
			if(bones[i].name->IsEqual(name))
				out.push_back(&bones[i]);
		
		return out;
	}
	
	Skeleton *Skeleton::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<Skeleton>(name, settings);
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
