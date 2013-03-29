//
//  RNCache.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CACHE_H__
#define __RAYNE_CACHE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNSpinLock.h"
#include "RNSynchronization.h"

namespace RN
{
	class Cache;
	class Cacheable
	{
	friend class Cache;
	public:
		virtual bool BeginContentAccess();
		virtual void EndContentAccess();
		
		virtual bool DiscardContent() = 0;
		virtual bool IsDiscarded() = 0;
		
		machine_uint Cost() const { return _cost; }
		machine_uint Weight() const { return _weight; }
		
		Cache *Cache() const { return _owner; }
		
	protected:
		Cacheable();
		Cacheable(machine_uint cost, machine_uint weight);
		virtual ~Cacheable();
		
		void SetCost(machine_uint cost);
		void SetWeight(machine_uint weight);
		
		void BecameDirty();
		
		class Cache *_owner;
		SpinLock _lock;
		
	private:
		void UpdateAgingFactor();
		
		machine_uint _cost; // The cost to recreate the data after it was discarded
		Past<machine_uint> _weight; // The cost to keep the data alive in the cache
		
		machine_uint _accessCount;
		machine_uint _age;
		machine_uint _agingFactor;
	};
	
	class AsyncCacheable : public Cacheable
	{
	public:		
		virtual bool BeginContentAccess();
		virtual void EndContentAccess();
		
		virtual bool DiscardContent();
		virtual bool IsDiscarded();
		
		virtual void RecreateContent() = 0;
		virtual void DisposeContent() = 0;
		
	protected:
		AsyncCacheable(bool hasContent=false);
		AsyncCacheable(machine_uint cost, machine_uint weight, bool hasContent=false);
		
	private:
		SpinLock _contentLock;
		bool _isCreatingContent;
		bool _hasContent;
	};
	
	class Cache : public Object
	{
	friend class Cacheable;
	public:
		Cache();
		virtual ~Cache();
		
		void AddCacheable(Cacheable *cacheable);
		void RemoveCacheable(Cacheable *cacheable);
		void ClearCache();
		
		void SetWeightLimit(machine_uint maxWeight);
		void EvictCacheables(machine_uint weight);
		
		machine_uint WeightLimit() const { return _weightLimit; }
		machine_uint TotalWeight() const { return _totalWeight; }
		
	private:
		void __EvictCacheables(machine_uint weight);
		void AgeCacheables();
		void CleanCacheables();
		
		void CacheableBecameDirty(Cacheable *cacheable);
		void UpdateCacheable(Cacheable *cacheable);
		void UpdateCacheables();
		
		SpinLock _lock;
		SpinLock _cacheLock;
		
		machine_uint _weightLimit;
		machine_uint _totalWeight;
		
		std::vector<Cacheable *> _caches;
		std::unordered_set<Cacheable *> _dirtyCaches;
		std::unordered_set<Cacheable *> _evicted;
	};
}

#endif /* __RAYNE_CACHE_H__ */
