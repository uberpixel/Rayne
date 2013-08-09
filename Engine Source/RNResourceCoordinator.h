//
//  RNResourceCoordinator.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RESOURCECOORDINATOR_H__
#define __RAYNE_RESOURCECOORDINATOR_H__

#include "RNBase.h"
#include "RNFile.h"
#include "RNString.h"
#include "RNDictionary.h"
#include "RNResource.h"

namespace RN
{
	class ResourceReader;
	class ResourceCoordinator : public Singleton<ResourceCoordinator>
	{
	public:
		ResourceCoordinator();
		~ResourceCoordinator() override;
		
		void RegisterReader(ResourceReader *reader);
		void UnregisterReader(String *uuid);
		
		Resource *OpenResource(const std::string& file, Dictionary *info);
		
	private:
		std::mutex _readerLock;
		Dictionary _reader;
	};
	
	
	
	class ResourceReader : public Object
	{
	public:
		ResourceReader(String *uuid);
		~ResourceReader() override;
		
		String *GetUUID() const { return _uuid; }
		
		virtual bool CanHandleFile(File *file, uint32& priority) = 0;
		virtual Resource *ResourceForFile(File *file, Dictionary *info) = 0;
		
	private:
		String *_uuid;
		
		RNDefineMeta(ResourceReader, Object)
	};
}

#endif /* __RAYNE_RESOURCECOORDINATOR_H__ */
