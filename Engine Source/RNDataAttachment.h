//
//  RNDataAttachment.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DATAATTACHMENT_H__
#define __RAYNE_DATAATTACHMENT_H__

#include "RNBase.h"
#include "RNSceneNodeAttachment.h"
#include "RNGPUBuffer.h"

namespace RN
{
	class DataAttachment : public SceneNodeAttachment
	{
	public:
		RNAPI DataAttachment();
		RNAPI ~DataAttachment();
		
		RNAPI void SetBuffer(GPUBuffer *buffer);
		RNAPI void UpdateRenderingObject(RenderingObject &object) override;
		
	private:
		GPUBuffer *_buffer;
	};
}

#endif /* __RAYNE_DATAATTACHMENT_H__ */
