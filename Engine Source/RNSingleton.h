//
//  RNSingleton.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SINGLETON_H__
#define __RAYNE_SINGLETON_H__

#include "RNDefines.h"

namespace RN
{
	template<class T>
	class ISingleton
	{
	public:
		RNAPI static T *GetSharedInstance() { throw ""; }
		
	protected:
		RNAPI virtual ~ISingleton() {};
		
	private:
		RNAPI virtual void MakeShared(bool override = false) = 0;
		RNAPI virtual void ResignShared() = 0;
	};
	
	template<class T>
	class INonConstructingSingleton
	{
	public:
		RNAPI static T *GetSharedInstance() { throw ""; }
		
	protected:
		RNAPI virtual ~INonConstructingSingleton() {};
		
	private:
		RNAPI virtual void MakeShared(bool override = false) = 0;
		RNAPI virtual void ResignShared() = 0;
	};
	
	template<class T, bool DefaultConstructor>
	struct __SingletonCreator
	{};
	
	template<class T>
	struct __SingletonCreator<T, true>
	{
		T *operator ()() { return new T(); }
	};
	
	template<class T>
	struct __SingletonCreator<T, false>
	{
		T *operator ()() { return nullptr; }
	};
	
#define RNDefineSingleton(T) \
	public: \
		RNAPI_DEFINEBASE static T *GetSharedInstance(); \
	private: \
		void MakeShared(bool override = false) final; \
		void ResignShared() final;
	
#define RNDeclareSingleton(T) \
		static T *__RN ## T ## SingletonInstance = nullptr; \
		static SpinLock __RN ## T ## SingletonLock; \
		T *T::GetSharedInstance() \
		{ \
			LockGuard<SpinLock> lock(__RN ## T ## SingletonLock); \
			if(!__RN ## T ## SingletonInstance && !std::is_base_of<INonConstructingSingleton<T>, T>::value) \
				__RN ## T ## SingletonInstance = __SingletonCreator<T, std::is_default_constructible<T>::value>{}(); \
			return __RN ## T ## SingletonInstance; \
		} \
		void T::MakeShared(bool override) \
		{ \
			LockGuard<SpinLock> lock(__RN ## T ## SingletonLock); \
			if(!__RN ## T ## SingletonInstance || override) \
				__RN ## T ## SingletonInstance = this; \
		} \
		void T::ResignShared() \
		{ \
			LockGuard<SpinLock> lock(__RN ## T ## SingletonLock); \
			if(__RN ## T ## SingletonInstance == this) \
				__RN ## T ## SingletonInstance = nullptr; \
		}
}

#endif /* __RAYNE_SINGLETON_H__ */
