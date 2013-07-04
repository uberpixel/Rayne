//
//  RNFunction.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		template<typename F>
		explicit Function(F&& f) :
			_implementation(new ImplementationType<F>(std::move(f)))
		{}
		Function(const Function& other) :
			_implementation(other._implementation)
		{}
		Function(Function&& other) :
			_implementation(std::move(other._implementation))
		{}
		Function() = default;
		
		template<typename F>
		Function& operator= (F&& f)
		{
			_implementation = std::shared_ptr<Base>(new ImplementationType<F>(std::move(f)));
			return *this;
		}
		Function& operator= (const Function& other)
		{
			_implementation = other._implementation;
			return *this;
		}
		Function& operator= (Function&& other)
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
			
			void Call() override { function(); }
			
			F function;
		};
		
		std::shared_ptr<Base> _implementation;
	};

}

#endif
