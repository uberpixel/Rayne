//
//  RNMemoryPool.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MEMORYPOOL_H_
#define __RAYNE_MEMORYPOOL_H_

#include "../Threads/RNLockable.h"
#include <vector>
#include <list>
#include <bitset>

namespace RN
{
	class MemoryPool
	{
	public:
		MemoryPool();

		void *Allocate(size_t size);
		void Free(void *ptr);

	private:
		class SizePool;
		class Node
		{
		public:
			friend class SizePool;

			Node(size_t size, SizePool *pool);
			~Node();

			bool TryLock();

			void Lock();
			void Unlock();

			bool IsFull() const;
			SizePool *GetOwner() const;

			void *Allocate();

		private:
			SizePool *_owner;
			std::bitset<128> _usage;
			Lockable _lock;
			uint8_t *_data;
			size_t _size;
		};

		class SizePool
		{
		public:
			SizePool(size_t size);

			SizePool(SizePool &&other) :
				_size(other._size),
				_nodes(std::move(other._nodes))
			{}

			SizePool &operator =(SizePool &&other)
			{
				_size = other._size;
				_nodes = std::move(other._nodes);

				return *this;
			}

			void *Allocate();
			void Free(Node *node, void *ptr);

			size_t GetSize() const;

		private:
			size_t _size;
			std::list<Node> _nodes;
		};

		std::vector<SizePool> _pools;
	};
}


#endif /* __RAYNE_MEMORYPOOL_H_ */
