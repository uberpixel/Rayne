//
//  RNSGAAssetLoader.cpp
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Rendering/RNRenderer.h"
#include "../System/RNFileManager.h"
#include "../Threads/RNWorkQueue.h"
#include "RNSGAAssetLoader.h"
#include "RNAssetManager.h"

namespace RN
{
	RNDefineMeta(SGAAssetLoader, AssetLoader)
	
	static SGAAssetLoader *__assetLoader;
	
	void SGAAssetLoader::Register()
	{
		uint8 magic[] = { 0x5A, 0x4E, 0xDA, 0x16 };
		
		Config config(Skeleton::GetMetaClass());
		config.SetExtensions(Set::WithObjects({RNCSTR("sga")}));
		config.SetMagicBytes(Data::WithBytes(magic, 4), 0);
		config.supportsBackgroundLoading = true;
		
		__assetLoader = new SGAAssetLoader(config);
		
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		coordinator->RegisterAssetLoader(__assetLoader);
	}
	
	SGAAssetLoader::SGAAssetLoader(const Config &config) :
	AssetLoader(config)
	{}
	
	Asset *SGAAssetLoader::Load(File *file, const LoadOptions &options)
	{
		Skeleton *skeleton = new Skeleton();
		
		file->Seek(5); // Skip over magic bytes and version number
		
		uint16 lenskeletonname = file->ReadUint16();
		char *skeletonname = new char[lenskeletonname];
		file->Read(skeletonname, lenskeletonname);
		delete[] skeletonname;
		
		uint16 bonecount = file->ReadUint16();
		for(int i = 0; i < bonecount; i++)
		{
			uint16 lenbonename = file->ReadUint16();
			char *bonename = new char[lenbonename];
			file->Read(bonename, lenbonename);
			
			Vector3 bonepos;
			file->Read(&bonepos.x, sizeof(Vector3));
			
			uint8 isroot = file->ReadUint8();
			
			Bone bone(bonepos, bonename, isroot);
			delete[] bonename;
			
			uint16 numchildren = file->ReadUint16();
			for(int n = 0; n < numchildren; n++)
			{
				uint16 child = file->ReadUint16();
				bone.tempChildren.push_back(child);
			}
			
			skeleton->bones.push_back(bone);
		}
		
		uint16 numanims = file->ReadUint16();
		for(int i = 0; i < numanims; i++)
		{
			uint16 lenanimname = file->ReadUint16();
			char *animname = new char[lenanimname];
			file->Read(animname, lenanimname);
			
			Animation *anim = new Animation(animname);
			anim->Autorelease();
			anim->Retain();
			skeleton->animations.insert(std::pair<std::string, Animation*>(animname, anim));
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
					file->Read(&animbonepos.x, sizeof(Vector3));
					Vector3 animbonescale;
					file->Read(&animbonescale.x, sizeof(Vector3));
					Quaternion animbonerot;
					file->Read(&animbonerot.x, sizeof(Quaternion));
					
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
				anim->bones.insert(std::pair<size_t, AnimationBone*>(static_cast<size_t>(boneid), animbone));
			}
		}
		
		skeleton->Init();
		
		return skeleton->Autorelease();
	}
	
	bool SGAAssetLoader::SupportsLoadingFile(File *file) const
	{
		file->Seek(4); // Skip the magic bytes, they've already been checked
		uint32 version = file->ReadUint8();
		
		return (version == 1);
	}
}
