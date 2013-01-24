//
//  RNPhysicsPipeline.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysicsPipeline.h"

namespace RN
{
	PhysicsPipeline::PhysicsPipeline()
	{
		std::thread thread = std::thread(&PhysicsPipeline::WorkLoop, this);
		thread.detach();
	}
	
	void PhysicsPipeline::WorkOnTask(TaskID task, float delta)
	{
		// TODO: Run the physics simulation here
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	
	void PhysicsPipeline::WorkLoop()
	{
		while(!ShouldStop())
		{
			WaitForWork();
		}
		
		Thread::CurrentThread()->Exit();
		DidStop();
	}
}
