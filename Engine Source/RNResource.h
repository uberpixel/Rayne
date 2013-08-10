//
//  RNResource.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RESOURCE_H__
#define __RAYNE_RESOURCE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNSemaphore.h"

namespace RN
{
	class Resource : public Object
	{
	public:
		~Resource() override;
		
		void BeginAccess();
		void EndAccess();
		
	protected:
		Resource(size_t maxReader);
		
		virtual void FetchContent();
		virtual void CanDiscardContent();
		virtual bool HasDiscardedContent();
		
	private:
		Semaphore _semaphore;
		std::atomic<uint32> _readers;
		std::mutex _writerLock;
		
		RNDefineMeta(Resource, Object)
	};
	
	class AudioResource : public Resource
	{
	public:
		struct PCMPackage
		{
			uint32 bitRate;
			std::vector<std::vector<uint8>> samples;
			size_t length;
		};
		
		virtual double GetDuration() = 0;
		virtual PCMPackage GetPCMData(size_t maxBytes) = 0;
		
		virtual uint32 GetChannels() = 0;
		virtual uint32 GetBitRate() = 0;
		
		virtual size_t GetPCMSampleCount() = 0;
		
		virtual bool IsSeekable() = 0;
		virtual void Seek(double time) = 0;
		
	protected:
		AudioResource(size_t maxReader);
		
		RNDefineMeta(AudioResource, Resource)
	};
}

#endif /* __RAYNE_RESOURCE_H__ */
