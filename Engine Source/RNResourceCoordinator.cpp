//
//  RNResourceCoordinator.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResourceCoordinator.h"
#include "RNScopeGuard.h"

namespace RN
{
	RNDeclareMeta(ResourceReader)
	
	ResourceCoordinator::ResourceCoordinator()
	{}
	
	ResourceCoordinator::~ResourceCoordinator()
	{}
	
	
	
	void ResourceCoordinator::RegisterReader(ResourceReader *reader)
	{
		std::lock_guard<std::mutex> lock(_readerLock);
		if(_reader.ObjectForKey(reader->GetUUID()))
		{
			throw Exception(Exception::Type::InconsistencyException, "A reader with the specified UUID exists already!");
			return;
		}
		
		_reader.SetObjectForKey(reader, reader->GetUUID());
	}
	
	void ResourceCoordinator::UnregisterReader(String *uuid)
	{
		std::lock_guard<std::mutex> lock(_readerLock);
		_reader.RemoveObjectForKey(uuid);
	}
	
	
	Resource *ResourceCoordinator::OpenResource(const std::string& tfile, Dictionary *info)
	{
		File *file;
		try
		{
			file = new File(tfile);
		}
		catch(Exception e)
		{
			return nullptr;
		}
		
		
		ScopeGuard guard([&]{
			file->Release();
		});
		
		ResourceReader *bestMatch = nullptr;
		uint32 bestScore = 0;
		
		std::lock_guard<std::mutex> lock(_readerLock);
		_reader.Enumerate([&](Object *object, Object *key, bool *stop) {
			
			ResourceReader *reader = static_cast<ResourceReader *>(object);
			
			file->Seek(0);
			uint32 score;
			
			if(reader->CanHandleFile(file, score))
			{
				if(score > bestScore || !bestMatch)
				{
					bestScore = score;
					bestMatch = reader;
				}
			}
		});
		
		if(!bestMatch)
			return nullptr;
		
		file->Seek(0);
		return bestMatch->ResourceForFile(file, info)->Autorelease();
	}
	
	
	
	ResourceReader::ResourceReader(String *uuid)
	{
		RN_ASSERT(uuid, "UUID mustn't be NULL!");
		
		_uuid = uuid->Retain();
	}
	
	ResourceReader::~ResourceReader()
	{
		_uuid->Release();
	}
}
