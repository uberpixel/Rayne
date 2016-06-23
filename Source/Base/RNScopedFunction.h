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
	class ScopedFunction;

	template<class R, class... Args>
	class ScopedFunction<R (Args...)>
	{
	public:
		ScopedFunction(R (*imp)(void *arg, Args...) = nullptr, void *arg = nullptr) :
			_implementation(imp),
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

	template<class T, class F>
	class __ScopedFunctionImplementation;

	template<class R, class... Args, class F>
	class __ScopedFunctionImplementation<R (Args...), F>  : public ScopedFunction<R (Args...)>
	{
	public:
		template<class PassedFunctor>
		__ScopedFunctionImplementation(PassedFunctor &&functor) :
			ScopedFunction<R (Args...)>(ForwardCall, this),
			_functor(functor)
		{}

		__ScopedFunctionImplementation(const __ScopedFunctionImplementation &other) :
			ScopedFunction<R (Args...)>(ForwardCall, this),
			_functor(other._functor)
		{}

		__ScopedFunctionImplementation(__ScopedFunctionImplementation &&other) :
			ScopedFunction<R (Args...)>(ForwardCall, this),
			_functor(other._functor)
		{}


		__ScopedFunctionImplementation &operator =(const __ScopedFunctionImplementation &other)
		{
			_functor = other._functor;
		}
		__ScopedFunctionImplementation &operator =(__ScopedFunctionImplementation &&other)
		{
			_functor = other._functor;
		}

	private:
		static R ForwardCall(void *argument, Args... arguments)
		{
			return static_cast<__ScopedFunctionImplementation *>(argument)->_functor(arguments...);
		}

		F _functor;
	};


	template<class FunctionType, class Functor>
	__ScopedFunctionImplementation<FunctionType, Functor> MakeScopedFunction(const Functor &functor)
	{
		return __ScopedFunctionImplementation<FunctionType, Functor>(functor);
	}
	template<class FunctionType, class Functor>
	__ScopedFunctionImplementation<FunctionType, Functor> MakeScopedFunction(Functor &&functor)
	{
		return __ScopedFunctionImplementation<FunctionType, Functor>(std::move<Functor>(functor));
	}
}

#endif /* __RAYNE_SCOPEDFUNCTION_H_ */
