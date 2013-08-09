//
//  RNResource.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResource.h"

namespace RN
{
	RNDeclareMeta(Resource)
	RNDeclareMeta(AudioResource)
	
	Resource::Resource(size_t maxReader) :
		_semaphore(maxReader > 0 ? maxReader : static_cast<size_t>(-1)),
		_readers(0)
	{}
	
	Resource::~Resource()
	{}
	
	
	void Resource::FetchContent()
	{}
	void Resource::CanDiscardContent()
	{}
	bool Resource::HasDiscardedContent()
	{
		return false;
	}
	
	
	void Resource::BeginAccess()
	{
		std::unique_lock<std::mutex> lock(_writerLock);
		if(HasDiscardedContent())
		{
			FetchContent();
			lock.unlock();
		}
		
		_semaphore.Wait();
		_readers ++;
	}
	
	void Resource::EndAccess()
	{
		_readers --;
		_semaphore.Signal();
		
		if(_readers.load() == 0)
		{
			std::lock_guard<std::mutex> lock(_writerLock);
			CanDiscardContent();
		}
	}
	
	
	AudioResource::AudioResource(size_t maxReader) :
		Resource(maxReader)
	{}
}
