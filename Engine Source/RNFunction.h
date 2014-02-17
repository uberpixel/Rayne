//
//  RNFunction.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_FUNCTIONH_H__
#define __RAYNE_FUNCTIONH_H__

#include "RNBase.h"

namespace RN
{
	class Function
	{
	public:
		Function() = default;
		
		template<typename F>
		Function(F&& f) :
			_implementation(new ImplementationType<F>(std::move(f)))
		{}
		
		Function(Function&& other) :
			_implementation(std::move(other._implementation))
		{}
		
		Function& operator=(Function&& other)
		{
			_implementation = std::move(other._implementation);
			return *this;
		}
		
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
			ImplementationType(F&& f) :
				function(std::move(f))
			{}
			
			void Call()
			{
				function();
			}
			
			F function;
		};
		
		std::unique_ptr<Base> _implementation;
	};
}

#endif
