//
//  RNFunction.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_FUNCTIONH_H__
#define __RAYNE_FUNCTIONH_H__

#include "RNBase.h"
#include "RNMemoryPool.h"

namespace RN
{
	RNAPI MemoryPool *__GetFunctionPool();

	class Function
	{
	public:
		Function() = default;
		
		template<typename F>
		Function(F &&f) :
			_implementation(new ImplementationType<F>(std::move(f)))
		{}
		
		Function(Function &&other) RN_NOEXCEPT :
			_implementation(std::move(other._implementation))
		{}
		
		Function &operator=(Function &&other) RN_NOEXCEPT
		{
			_implementation = std::move(other._implementation);
			return *this;
		}
		
		Function(const Function&) = delete;
		Function &operator= (const Function&) = delete;
		
		void operator() () { _implementation->Call(); }
		
	private:
		struct Base
		{
			virtual void Call() = 0;
			virtual ~Base() {}
		};
		
		template<typename F>
		struct ImplementationType : Base
		{
			ImplementationType(F &&f) :
				function(std::move(f))
			{}
			
			void Call()
			{
				function();
			}

			RN_INLINE void *operator new(size_t size) { return __GetFunctionPool()->Allocate(size); }
			RN_INLINE void operator delete(void *ptr) { if(ptr) __GetFunctionPool()->Free(ptr); }

			F function;
		};

		std::unique_ptr<Base> _implementation;
	};

	template<class Functor>
	Function MakeFunction(Functor &&functor)
	{
		return Function(std::forward<Functor>(functor));
	}
}

#endif
