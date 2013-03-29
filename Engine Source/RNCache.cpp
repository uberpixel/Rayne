//
//  RNCache.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCache.h"

namespace RN
{
	Cache::Cache()
	{
		_weightLimit = 0;
		_totalWeight = 0;
	}
	
	Cache::~Cache()
	{}
	
	
	void Cache::CleanCacheables()
	{
		_lock.Lock();
		
		if(_weightLimit > 0 && _totalWeight > _weightLimit)
			__EvictCacheables(_totalWeight - _weightLimit);
		
		_lock.Unlock();
	}
	
	void Cache::UpdateCacheable(Cacheable *cacheable)
	{
		auto iterator = _evicted.find(cacheable);
		if(iterator != _evicted.end())
		{
			cacheable->_weight.SynchronizePast();
			cacheable->_age = 0;
			
			if(!cacheable->IsDiscarded())
			{
				_evicted.erase(iterator);
				_caches.push_back(cacheable);
				
				_totalWeight += cacheable->_weight;
				
				if(_weightLimit > 0 && _totalWeight > _weightLimit)
					__EvictCacheables(_totalWeight - _weightLimit);
			}
			
			
			return;
		}
		else
		{
			cacheable->_age = 0;
			
			if(cacheable->IsDiscarded())
			{
				_evicted.insert(cacheable);
				_totalWeight -= cacheable->_weight.AccessPast();
				
				cacheable->_weight.SynchronizePast();
				
				for(auto i=_caches.begin(); i!=_caches.end(); i++)
				{
					if(cacheable == *i)
					{
						_caches.erase(i);
						break;
					}
				}
			}
			else
			{
				if(!cacheable->_weight.IsSynchronWithPast())
				{
					_totalWeight += cacheable->_weight.Diff<machine_int>();
					cacheable->_weight.SynchronizePast();
					
					if(_weightLimit > 0 && _totalWeight > _weightLimit)
						__EvictCacheables(_totalWeight - _weightLimit);
				}
			}
		}
	}
	
	void Cache::UpdateCacheables()
	{
		_cacheLock.Lock();
		
		if(_dirtyCaches.size() > 0)
		{
			std::unordered_set<Cacheable *> _remainder;
			
			for(auto i=_dirtyCaches.begin(); i!=_dirtyCaches.end(); i++)
			{
				Cacheable *cacheable = *i;
				
				if(cacheable->_lock.TryLock())
				{
					UpdateCacheable(cacheable);
					cacheable->_lock.Unlock();
				}
				else
				{
					_remainder.insert(cacheable);
				}
			}
			
			std::swap(_dirtyCaches, _remainder);
		}
		
		_cacheLock.Unlock();
	}
	
	void Cache::CacheableBecameDirty(Cacheable *cacheable)
	{
		// Try to process the change right away, otherwise queue it up for later processing
		if(_lock.TryLock())
		{
			UpdateCacheable(cacheable);
			UpdateCacheables();
			
			AgeCacheables();
			
			_lock.Unlock();
			return;
		}
		
		_cacheLock.Lock();
		_dirtyCaches.insert(cacheable);
		_cacheLock.Unlock();
	}	
	

	
	void Cache::__EvictCacheables(machine_uint weight)
	{
		std::vector<Cacheable *> evicted;
		machine_uint freed = 0;
		
		_caches.erase(std::remove_if(_caches.begin(), _caches.end(), [&] (Cacheable *cacheable) {
			
			if(freed >= weight)
				return false;
			
			if(cacheable->_lock.TryLock())
			{
				if(cacheable->_accessCount == 0 && cacheable->DiscardContent())
				{
					freed += cacheable->_weight;
					_evicted.insert(cacheable);
					
					cacheable->_lock.Unlock();
					return true;
				}
				
				cacheable->_lock.Unlock();
			}
			
			return false;
		}), _caches.end());
		
		_totalWeight -= freed;
		
	}
	
	void Cache::EvictCacheables(machine_uint weight)
	{
		_lock.Lock();
		__EvictCacheables(weight);
		_lock.Unlock();
	}
	
	
	void Cache::AgeCacheables()
	{
		for(auto i=_caches.begin(); i!=_caches.end(); i++)
		{
			Cacheable *cacheable = *i;
			
			if(cacheable->_accessCount > 0) // Currently accessed cacheables don't age
				continue;
			
			cacheable->_age += cacheable->_agingFactor;
		}
		
		std::sort(_caches.begin(), _caches.end(), [](Cacheable *cacheA, Cacheable *cacheB) {
			return cacheB->_age < cacheA->_age;
		});
		
		if(_weightLimit > 0 && _totalWeight > _weightLimit)
			__EvictCacheables(_totalWeight - _weightLimit);
	}
	
	
	
	void Cache::SetWeightLimit(machine_uint maxWeight)
	{
		_lock.Lock();
		
		_weightLimit = maxWeight;
		
		if(_weightLimit > 0 && _totalWeight > _weightLimit)
			__EvictCacheables(_totalWeight - _weightLimit);
		
		_lock.Unlock();
	}
	
	
	
	void Cache::AddCacheable(Cacheable *cacheable)
	{
		cacheable->_lock.Lock();
		_lock.Lock();
		
		RN_ASSERT0(cacheable->_owner == 0);
		UpdateCacheables();
		
		if(cacheable->IsDiscarded())
		{
			_evicted.insert(cacheable);
			_lock.Unlock();
			
			cacheable->_owner = this;
			cacheable->_weight.SynchronizePast();
			cacheable->_lock.Unlock();
			
			return;
		}
		
		cacheable->_weight.SynchronizePast();
		machine_uint weight = cacheable->_weight;
		
		if(_weightLimit > 0)
		{
			if(weight > _weightLimit)
			{
				printf("Added cacheable with cost of %i into cache with a maximal cost of %i!\n", (int)weight, (int)_weightLimit);
				
				_lock.Unlock();
				return;
			}
			
			if(_totalWeight + weight > _weightLimit)
				__EvictCacheables(weight);
		}
		
		
		AgeCacheables();
		
		cacheable->_age = 0;
		cacheable->_owner = this;
		
		_caches.push_back(cacheable);
		_totalWeight += weight;
		
		_lock.Unlock();
		cacheable->_lock.Unlock();
	}
	
	void Cache::RemoveCacheable(Cacheable *cacheable)
	{
		cacheable->_lock.Lock();
		_lock.Lock();
		
		RN_ASSERT0(cacheable->_owner == this);
		
		_dirtyCaches.erase(cacheable);
		
		auto iterator = _evicted.find(cacheable);
		if(iterator == _evicted.end())
		{
			for(auto i=_caches.begin(); i!=_caches.end(); i++)
			{
				if(cacheable == *i)
				{
					_caches.erase(i);
					break;
				}
			}
				   
			_totalWeight -= cacheable->_weight.AccessPast();
		}
		else
		{
			_evicted.erase(iterator);
		}
		
		cacheable->_owner = 0;
		UpdateCacheables();
		
		_lock.Unlock();
		cacheable->_lock.Unlock();
	}
	
	void Cache::ClearCache()
	{
		_lock.Lock();
		
		for(auto i=_caches.begin(); i!=_caches.end(); i++)
		{
			Cacheable *cacheable = *i;
			
			cacheable->_lock.Lock();
			cacheable->_owner = 0;
			cacheable->_lock.Unlock();
		}
		
		for(auto i=_evicted.begin(); i!=_evicted.end(); i++)
		{
			Cacheable *cacheable = *i;
			
			cacheable->_lock.Lock();
			cacheable->_owner = 0;
			cacheable->_lock.Unlock();
		}
		
		_caches.clear();
		_evicted.clear();
		_dirtyCaches.clear();
		
		_totalWeight = 0;
		
		_lock.Unlock();
	}
}
