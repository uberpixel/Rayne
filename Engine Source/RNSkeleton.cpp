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
	AnimationBone::AnimationBone(int32 numframes)
	{
		currentTime = 0.0f;
		currentFrame = 0;
		numFrames = numframes;
		frames = new AnimationBone::Frame[numframes];
	}
	
	AnimationBone::~AnimationBone()
	{
		delete[] frames;
	}
	
	void AnimationBone::Update(float delta, bool loop)
	{
		if(numFrames <= 1)
			return;
		
		finished = false;
		currentTime += delta;
		
		if(delta > 0.0f) //forwards
		{
			while(currentTime > frames[currentFrame+1].time)
			{
				currentFrame += 1;
				
				if(currentFrame > numFrames-2)
				{
					if(loop)
					{
						currentFrame = 0;
						currentTime -= frames[numFrames-1].time;
					}
					else
					{
						currentFrame = numFrames-2;
						currentTime = frames[numFrames-1].time;
						finished = true;
					}
				}
			}
		}
		else //backwards
		{
			while(currentTime < frames[currentFrame].time)
			{
				currentFrame -= 1;
				
				if(currentFrame < 0)
				{
					if(loop)
					{
						currentFrame = numFrames-2;
						currentTime += frames[numFrames-1].time;
					}
					else
					{
						currentFrame = 0;
						currentTime = 0.0f;
						finished = true;
					}
				}
			}
		}
	}
	
	void AnimationBone::SetTime(float time, bool loop)
	{
		currentTime = 0.0f;
		Update(time, loop);
	}
	
	void AnimationBone::SetIndex(int32 frame, bool loop)
	{
		currentFrame = frame;
		
		if(loop)
		{
			while(currentFrame > numFrames-1)
			{
				currentFrame -= numFrames-1;
			}
		}
		else
		{
			currentFrame = std::min(currentFrame, numFrames-1);
		}
		
		currentTime = frames[currentFrame].time;
	}
	
	const AnimationBone::Frame AnimationBone::GetInterpolatedFrame() const
	{
		Frame frame;
		
		float timeDiff = frames[currentFrame+1].time-frames[currentFrame].time;
		float blend = (currentTime-frames[currentFrame].time)/timeDiff;
		
		frame.time = currentTime;
		frame.position = frames[currentFrame].position.GetLerp(frames[currentFrame+1].position, blend);
		frame.scale = frames[currentFrame].scale.GetLerp(frames[currentFrame+1].scale, blend);
		frame.rotation = Quaternion::WithLerpSpherical(frames[currentFrame].rotation, frames[currentFrame+1].rotation, blend);
		
		return frame;
	}
	
	bool AnimationBone::IsFinished() const
	{
		return finished;
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
			delete bone;
		}
	}
	
	float Animation::GetLength()
	{
		float length = 0.0f;
		for(auto bone : bones)
		{
			if(bone.second->frames)
				length = fmaxf(length, bone.second->frames[bone.second->numFrames-1].time);
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
		rotation = Quaternion::WithIdentity();
		scale = Vector3(1.0, 1.0, 1.0);
		
		animationBone = nullptr;
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
		rotation = Quaternion::WithIdentity();
		scale = Vector3(1.0f, 1.0f, 1.0f);
		
		animationBone = nullptr;
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
		animationBone = nullptr;
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
	
	bool Bone::Update(Bone *parent, float delta, bool loop)
	{
		bool running = true;
		if(animationBone && animationBone->numFrames > 0)
		{
			animationBone->Update(delta, loop);
			AnimationBone::Frame interpolatedFrame = animationBone->GetInterpolatedFrame();
			
			position = interpolatedFrame.position;
			scale = interpolatedFrame.scale;
			rotation = interpolatedFrame.rotation;
			
			if(animationBone->numFrames <= 1)
			{
				running = false;
			}
			else
			{
				if(animationBone->IsFinished())
				{
					finished = true;
					running = false;
				}
			}
		}
		else
		{
			running = false;
		}
		
		//TODO: Remove absolute flag...
		if(!absolute || !animationBone)
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
			
			if(children[i]->Update(this, delta, loop))
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
		if(animbone)
		{
			animationBone = animbone;
		}
		else
		{
			position = Vector3();
			rotation = Quaternion::WithIdentity();
			scale = Vector3(1.0f, 1.0f, 1.0f);
			animationBone = nullptr;
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
	
	bool Skeleton::Update(float delta, bool restart)
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
				if(bones[i].Update(0, delta, restart))
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
	
/*	void Skeleton::CopyAnimation(const std::string &from, const std::string &to, float start, float end, bool loop)
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
	}*/
	
	void Skeleton::RemoveAnimation(const std::string &animname)
	{
		Animation *anim = animations[animname];
		if(anim == _curranim)
			_curranim = 0;
		
		animations.erase(animname);
		anim->Release();
	}
	
/*	void Skeleton::SetBlendAnimation(const std::string &to, float blendtime, float targettime)
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
	}*/
	
	std::vector<Bone *> Skeleton::GetBones(const std::string name)
	{
		std::vector<Bone *> out;
		for(int i = 0; i < bones.size(); i++)
			if(bones[i].name == name)
				out.push_back(&bones[i]);
		
		return out;
	}
	
	Skeleton *Skeleton::WithFile(const std::string &path)
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
