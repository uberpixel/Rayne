//
//  RNExpected.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EXPECT_H__
#define __RAYNE_EXPECT_H__

#include "RNBase.h"

namespace RN
{
	template<class T>
	struct Expected;
	
	template<class T>
	struct Expected
	{
	public:
		Expected() :
			_acknowledged(false)
		{}
		
		Expected(const T &value) :
			_result(value),
			_acknowledged(false)
		{}
		
		template<class E>
		Expected(const E &e) :
			_exception(std::make_exception_ptr(e)),
			_acknowledged(false)
		{}
		
		Expected(Expected &&other) :
			_exception(std::move(other._exception)),
			_result(std::move(other._result)),
			_acknowledged(other._acknowledged.load())
		{
			other._acknowledged = true;
		}
		
		Expected &operator =(Expected &&other)
		{
			_exception = std::move(other._exception);
			_result    = std::move(other._result);
			_acknowledged = other._acknowledged.load();
			
			other._acknowledged = true;
			return *this;
		}
		
		~Expected()
		{
			assert(_acknowledged);
		}
		
		operator T() const { return Get(); }
		
		bool IsValid() const { _acknowledged = true; return !_exception; }
		T Get() const { if(!IsValid()) { std::rethrow_exception(_exception); } return _result; }
		void Suppress() { _acknowledged = true; }
		
	private:
		T _result;
		std::exception_ptr _exception;
		mutable std::atomic<bool> _acknowledged;
	};
	
	template<>
	struct Expected<void>
	{
	public:
		Expected() :
			_acknowledged(false)
		{}
		
		template<class E>
		Expected(const E &e) :
			_exception(std::make_exception_ptr(e)),
			_acknowledged(false)
		{}
		
		Expected(Expected &&other) :
			_exception(std::move(other._exception)),
			_acknowledged(other._acknowledged.load())
		{
			other._acknowledged = true;
		}
		
		Expected &operator =(Expected &&other)
		{
			_exception = std::move(other._exception);
			_acknowledged = other._acknowledged.load();
			
			other._acknowledged = true;
			return *this;
		}
		
		~Expected()
		{
			assert(_acknowledged);
		}
		
		bool IsValid() const { _acknowledged = true; return !_exception; }
		void Get() const { if(!IsValid()) { std::rethrow_exception(_exception); } }
		void Suppress() { _acknowledged = true; }
		
	private:
		std::exception_ptr _exception;
		mutable std::atomic<bool> _acknowledged;
	};
}

#endif /* __RAYNE_EXPECT_H__ */
