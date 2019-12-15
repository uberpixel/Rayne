//
//  RNOpenALResourceAttachment.´h
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENALRESOURCEATTACHMENT_H_
#define __RAYNE_OPENALRESOURCEATTACHMENT_H_

#include "RNOpenAL.h"

namespace RN
{
	class OpenALResourceAttachment : public RN::Object
	{
	public:
		OpenALResourceAttachment(RN::AudioAsset *resource);
		~OpenALResourceAttachment();
			
		uint32 GetBufferID() const { return _bufferID; }
			
		static OpenALResourceAttachment *GetAttachmentForResource(RN::AudioAsset *resource);
			
	private:
		int GetALFormat(short channels, short bitsPerSample);
		uint32 _bufferID;
	};
}

#endif /* defined(__RAYNE_OPENALRESOURCEATTACHMENT_H_) */
