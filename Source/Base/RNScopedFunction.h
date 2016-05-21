//
//  RNScopedFunction.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCOPEDFUNCTION_H_
#define __RAYNE_SCOPEDFUNCTION_H_

#include "RNBase.h"

namespace RN
{
	template<class F>
	class __ScopedFunction;

	template<class R, class... Args>
	class __ScopedFunction<R (Args...)>
	{
	public:
		__ScopedFunction() :
			_implementation(nullptr),
			_arg(nullptr)
		{}

		__ScopedFunction(R (*implementation)(void *, Args...), void *arg = nullptr) :
			_implementation(implementation),
			_arg(arg)
		{}

		template<class... PassedArgs>
		R operator()(PassedArgs &&... args) const
		{
			return _implementation(_arg, std::forward<PassedArgs>(args)...);
		}

	private:
		R (*_implementation)(void *, Args...);
		void *_arg;
	};

	template<class F, class Functor>
	class ScopedFunction;

	template<class R, class... Args, class Functor>
	class ScopedFunction<R (Args...), Functor> : public __ScopedFunction<R (Args...)>
	{
	public:
		template<class PassedFunctor>
		ScopedFunction(PassedFunctor &&functor) :
			__ScopedFunction<R (Args...)>(Callback, this),
			_functor(std::forward<PassedFunctor>(functor))
		{}

	private:
		static R Callback(void *argument, Args... arguments)
		{
			return static_cast<ScopedFunction *>(argument)->_functor(arguments...);
		}

		Functor _functor;
	};

	template<class FunctionType, class Functor>
	ScopedFunction<FunctionType, Functor> MakeScopedFunction(Functor &&functor)
	{
		return ScopedFunction<FunctionType, Functor>(std::forward<Functor>(functor));
	}
}

#endif /* __RAYNE_SCOPEDFUNCTION_H_ */
