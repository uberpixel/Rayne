//
//  TGApplication.cpp
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGApplication.h"

namespace TG
{
	class TestCacheable : public RN::Cacheable
	{
	public:
		TestCacheable(machine_uint tweight) :
			RN::Cacheable(1, tweight)
		{
			weight = (int)tweight;
			content = malloc(weight * sizeof(int));
		}
		
		virtual ~TestCacheable()
		{
			if(content)
				free(content);
		}
		
		virtual bool BeginContentAccess()
		{
			if(!content)
			{
				printf("Recreating content!\n");
				content = malloc(weight * sizeof(int));
			}
			
			return RN::Cacheable::BeginContentAccess();
		}
		
		virtual bool DiscardContent()
		{
			free(content);
			content = 0;
			
			return true;
		}
		
		virtual bool IsDiscarded() const
		{
			return (content == 0);
		}
		
		
		
		void Description()
		{
			printf("Weight %i, Discarded: %s\n", weight, content == 0 ? "yes" : "no");
		}
		
	private:
		int weight;
		void *content;
	};
	
	
	
	Application::Application()
	{
		_world = 0;
		RN::Texture::SetDefaultAnisotropyLevel(RN::Texture::MaxAnisotropyLevel());
	}
	
	Application::~Application()
	{}
	
	
	void Application::Start()
	{
#define DumpCache() do { \
		printf("-------------\n"); \
		printf("Weight limit: %i, total weight: %i\n", (int)cache->WeightLimit(), (int)cache->TotalWeight()); \
		for(auto i=test.begin(); i!=test.end(); i++) { \
			TestCacheable *cacheable = *i; \
			cacheable->Description(); \
		} \
	} while(0)
		
		RN::Cache *cache = new RN::Cache();
		
		cache->SetWeightLimit(128);
		
		std::vector<TestCacheable *> test;
		
		for(int i=0; i<20; i++)
		{
			TestCacheable *cacheable = new TestCacheable(10 + arc4random_uniform(64));
			
			test.push_back(cacheable);
			cache->AddCacheable(cacheable);
		}
		
		DumpCache();
		
		for(auto i=test.begin(); i!=test.end(); i++)
		{
			TestCacheable *cacheable = *i;
			if(cacheable->IsDiscarded())
			{
				if(arc4random_uniform(128) > 96)
				{
					cacheable->BeginContentAccess();
					cacheable->EndContentAccess();
				}
			}
		}

		DumpCache();
		
		//_world = new World();
	}
	
	void Application::WillExit()
	{
		if(_world)
		{
			delete _world;
			_world = 0;
		}
	}
	
	
	void Application::UpdateGame(float delta)
	{
	}
	
	void Application::UpdateWorld(float delta)
	{
	}
}

extern "C"
{
	RN::Application *RNApplicationCreate(RN::Kernel *kernel)
	{
		return static_cast<RN::Application *>(new TG::Application());
	}
}
