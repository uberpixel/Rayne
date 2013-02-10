//
//  RNSynchronization.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SYNCHRONIZATION_H__
#define __RAYNE_SYNCHRONIZATION_H__

#include "RNBase.h"

namespace RN
{
	template <typename T>
	class Past
	{
	public:
		Past()
		{}
		
		Past(const T& t) :
			_current(t),
			_past(t)
		{}
		
		bool operator == (const T& other) const
		{
			return (_current == other);
		}
		
		bool operator != (const T& other) const
		{
			return (_current != other);
		}
		
		
		operator T& ()
		{
			return _current;
		}
		
		operator T() const
		{
			return _current;
		}
		
		
		const T* operator-> () const
		{
			return &_current;
		}
		
		T* operator-> ()
		{
			return &_current;
		}
		
		
		T& operator= (const T& other)
		{
			_current = other;
			return _current;
		}
		
		T& operator+= (const T& other)
		{
			_current += other;
			return _current;
		}
		
		T& operator-= (const T& other)
		{
			_current -= other;
			return _current;
		}
		
		T& operator*= (const T& other)
		{
			_current *= other;
			return _current;
		}
		
		T& operator/= (const T& other)
		{
			_current /= other;
			return _current;
		}
		
		
		T operator+ (const T& other) const
		{
			return _current + other;
		}
		
		T operator- (const T& other) const
		{
			return _current - other;
		}
		
		T operator* (const T& other) const
		{
			return _current * other;
		}
		
		T operator/ (const T& other) const
		{
			return _current / other;
		}
		
		
		T& AccessPast()
		{
			return _past;
		}
		
		T AccessPast() const
		{
			return _past;
		}
		
		void SynchronizePast()
		{
			_past = _current;
		}
		
	private:
		T _current;
		T _past;
	};
}

#endif /* __RAYNE_SYNCHRONIZATION_H__ */
