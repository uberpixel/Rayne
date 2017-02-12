//
//  RNMemoryPool.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMemoryPool.h"
#include "../Math/RNAlgorithm.h"

namespace RN
{
	/**
	 * Each MemoryPool has a collection of  SizePool objects, one for each power of two
	 * in the range of [8, 512] (8, 16, 32, 64, 128, 256, 512). Each SizePool then further contains
	 * a list of Node objects which contain 128 slots each. A Node keeps track of allocations using a bitmap
	 * of all free memory regions.
	 **/

	MemoryPool::MemoryPool()
	{
		// Create the size pools
		_pools.emplace_back(8);
		_pools.emplace_back(16);
		_pools.emplace_back(32);
		_pools.emplace_back(64);
		_pools.emplace_back(128);
		_pools.emplace_back(256);
		_pools.emplace_back(512);
	}

	void *MemoryPool::Allocate(size_t size)
	{
		size = NextPowerOfTwo(static_cast<uint64>(size + sizeof(void *)));
		size = std::max(size, static_cast<size_t>(8));

		RN_ASSERT(size <= 512, "MemoryPool serves a maximum of 512 bytes!");

		for(SizePool &pool : _pools)
		{
			if(pool.GetSize() == size)
				return pool.Allocate();
		}

		abort(); // WTF?!
	}

	void MemoryPool::Free(void *ptr)
	{
		uintptr_t *value = reinterpret_cast<uintptr_t *>(ptr) - 1;
		Node *node = reinterpret_cast<Node *>(*value);

		SizePool *pool = node->GetOwner();
		pool->Free(node, value);
	}


	MemoryPool::SizePool::SizePool(size_t size) :
		_size(size)
	{}

	size_t MemoryPool::SizePool::GetSize() const
	{
		return _size;
	}

	void *MemoryPool::SizePool::Allocate()
	{
		for(Node &node : _nodes)
		{
			if(node.TryLock())
			{
				if(!node.IsFull())
				{
					uintptr_t *result = reinterpret_cast<uintptr_t *>(node.Allocate());
					node.Unlock();

					*result = reinterpret_cast<uintptr_t>(&node);
					return result + 1;
				}

				node.Unlock();
			}
		}

		_nodes.emplace_back(_size, this);
		return Allocate();
	}

	void MemoryPool::SizePool::Free(Node *node, void *ptr)
	{
		size_t slot = reinterpret_cast<size_t>(ptr);
		slot -= reinterpret_cast<size_t>(node->_data);
		slot /= _size;

		node->Lock();
		node->_usage.reset(slot);
		node->Unlock();
	}




	MemoryPool::Node::Node(size_t size, SizePool *pool) :
		_owner(pool),
		_data(new uint8[size * 128]),
		_size(size)
	{}

	MemoryPool::Node::~Node()
	{
		delete[] _data;
	}


	bool MemoryPool::Node::IsFull() const
	{
		return (_usage.all());
	}

	bool MemoryPool::Node::TryLock()
	{
		return _lock.TryLock();
	}

	void MemoryPool::Node::Lock()
	{
		_lock.Lock();
	}

	void MemoryPool::Node::Unlock()
	{
		_lock.Unlock();
	}

	MemoryPool::SizePool *MemoryPool::Node::GetOwner() const
	{
		return _owner;
	}

	void *MemoryPool::Node::Allocate()
	{
		for(size_t i = 0; i < 128; i ++)
		{
			if(!_usage.test(i))
			{
				_usage.set(i);
				return reinterpret_cast<void *>(_data + (i * _size));
			}
		}

		abort();
	}
}
