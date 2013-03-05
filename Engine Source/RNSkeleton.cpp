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
			delete[] bonename;
			
			Vector3 bonepos;
			file->ReadIntoBuffer(&bonepos.x, sizeof(Vector3));
			
			uint8 isroot = file->ReadUint8();
			
			uint16 numchildren = file->ReadUint16();
			for(int n = 0; n < numchildren; n++)
			{
				uint16 child = file->ReadUint16();
			}
		}
		
		uint16 numanims = file->ReadUint16();
		for(int i = 0; i < numanims; i++)
		{
			uint16 lenanimname = file->ReadUint16();
			char *animname = new char[lenanimname];
			file->ReadIntoBuffer(animname, lenanimname);
			delete[] animname;
			
			uint16 numanimbones = file->ReadUint16();
			for(int n = 0; n < numanimbones; n++)
			{
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
				}
			}
		}
	}
}
