//
//  RNPhysicsPipeline.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSICSPIPELINE_H__
#define __RAYNE_PHYSICSPIPELINE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNPipelineSegment.h"

namespace RN
{
	class PhysicsPipeline : public PipelineSegment
	{
	public:
		PhysicsPipeline();
		
		virtual void WorkOnTask(TaskID task);
		
	private:
		void WorkLoop();
	};
}

#endif /* __RAYNE_PHYSICSPIPELINE_H__ */
