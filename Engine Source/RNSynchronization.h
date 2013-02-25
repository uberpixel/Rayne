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
		
		
		operator T& ();
		operator T() const;
		
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
		
		T& AccessPast();
		T AccessPast() const;
		
		void SynchronizePast()
		{
			_past = _current;
		}
		
		bool IsSynchronWithPast() const
		{
			return (_current == _past);
		}
		
		template <typename R=T>
		R Diff() const
		{
			return ((R)_current) - ((R)_past);
		}
		
	private:
		T _current;
		T _past;
	};
	
	template <typename T>
	RN_INLINE  Past<T>::operator T& ()
	{
		return _current;
	}
	
	template <typename T>
	RN_INLINE  Past<T>::operator T() const
	{
		return _current;
	}
	
	template <typename T>
	RN_INLINE T& Past<T>::AccessPast()
	{
		return _past;
	}
	
	template <typename T>
	RN_INLINE T Past<T>::AccessPast() const
	{
		return _past;
	}
}

#endif /* __RAYNE_SYNCHRONIZATION_H__ */
