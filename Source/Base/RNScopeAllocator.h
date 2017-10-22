//
//  RNScopeAllocator.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RN_SCOPE_ALLOCATOR_H__
#define __RN_SCOPE_ALLOCATOR_H__

#include "RNBase.h"
#include "RNBumpAllocator.h"

namespace RN
{
	struct __ScopeAllocatorFinalizer;

	class ScopeAllocator
	{
	public:
		RNAPI ScopeAllocator();
		RNAPI ScopeAllocator(const ScopeAllocator &other);
		RNAPI ScopeAllocator(BumpAllocator &allocator);
		RNAPI ~ScopeAllocator();

		RNAPI ScopeAllocator &GetThreadAllocator();

		template<class T, typename ...Args>
		T *Alloc(Args &&...args)
		{
			void *mem;

			if(!std::is_trivially_destructible<T>::value)
			{
				auto des = &ScopeAllocator::Destructor<T>;
				mem = AllocWithDestructor(sizeof(T), alignof(T), des);
			}
			else
				mem = Alloc(sizeof(T), alignof(T));

			T *res = new(mem) T(args...);
			return res;
		}

		template<class T>
		T *AllocBytes(size_t size)
		{
			return static_cast<T *>(Alloc(size, 16));
		}

	private:
		template<class T>
		static void Destructor(void *ptr) { static_cast<T *>(ptr)->~T(); }

		RNAPI void *AllocWithDestructor(size_t size, size_t alignment, void (*fn)(void *ptr));
		RNAPI void *Alloc(size_t size, size_t alignment);

		BumpAllocator &_allocator;
		__ScopeAllocatorFinalizer *_finalizerChain;
		size_t _nonFinalizerSize;
		ScopeAllocator *_previous;
	};
}

#endif /* __RN_SCOPE_ALLOCATOR_H__ */
