//
//  RNCacheable.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCache.h"

#define kRNCacheWeightFactor 1.5f
#define kRNCacheCostFactor 0.35f

namespace RN
{
	Cacheable::Cacheable()
	{
		_owner = 0;
		_accessCount = 0;
		
		_cost   = 1;
		_weight = 1;
		_age    = 0;
		
		UpdateAgingFactor();
	}
	
	Cacheable::Cacheable(machine_uint cost, machine_uint weight)
	{
		RN_ASSERT0(cost > 0);
		RN_ASSERT0(weight > 0);
		
		_owner = 0;
		_accessCount = 0;
		
		_cost   = cost;
		_weight = weight;
		_age    = 0;
		
		UpdateAgingFactor();
	}
	
	Cacheable::~Cacheable()
	{
		if(_owner)
			_owner->RemoveCacheable(this);
	}
	
	
	bool Cacheable::BeginContentAccess()
	{
		_lock.Lock();
		
		if((_accessCount ++) == 0)
		{
			BecameDirty();
		}
		
		_lock.Unlock();
		return true;
	}
	
	void Cacheable::EndContentAccess()
	{
		_lock.Lock();
		bool canEvict = ((-- _accessCount) == 0);
		_lock.Unlock();
		
		if(canEvict && _owner)
			_owner->CleanCacheables();
	}
	
	
	void Cacheable::BecameDirty()
	{
		if(_lock.TryLock())
		{
			if(_owner)
				_owner->CacheableBecameDirty(this);
			
			_lock.Unlock();
		}
		else
		{
			if(_owner)
				_owner->CacheableBecameDirty(this);
		}
	}
	
	void Cacheable::SetCost(machine_uint cost)
	{
		RN_ASSERT0(cost > 0);
		
		_lock.Lock();
		_cost = cost;
		
		UpdateAgingFactor();
		BecameDirty();
		
		_lock.Unlock();
	}
	
	void Cacheable::SetWeight(machine_uint weight)
	{
		RN_ASSERT0(weight > 0);
		
		_lock.Lock();
		_weight = weight;
		
		UpdateAgingFactor();
		BecameDirty();
		
		_lock.Unlock();
	}
	
	void Cacheable::UpdateAgingFactor()
	{
		float factor = (((float)_weight) * kRNCacheWeightFactor) / (((float)_cost) * kRNCacheCostFactor);
		
		volatile machine_uint ageFactor = MIN(1, (machine_uint)(ceilf(factor)));
		_agingFactor = ageFactor;
	}
}
