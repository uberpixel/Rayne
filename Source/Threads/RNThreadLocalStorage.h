//
//  RNThreadLocalStorage.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREADLOCALSTORAGE_H__
#define __RAYNE_THREADLOCALSTORAGE_H__

#include "../Base/RNBase.h"
#include "RNThread.h"

namespace RN
{
#if RN_PLATFORM_POSIX
	template<class T>
	class ThreadLocalStorage;

	template<class T>
	class ThreadLocalStorage
	{
	public:
		ThreadLocalStorage()
		{
			pthread_key_create(&_key, &CleanBuffer);
		}
		~ThreadLocalStorage()
		{
			pthread_key_delete(_key);
		}


		void SetValue(const T &value)
		{
			T *buffer = GetBuffer();
			*buffer = value;
		}
		T GetValue() const
		{
			T *buffer = GetBuffer();
			return *buffer;
		}

	private:
		static void CleanBuffer(void *buffer)
		{
			T *object = reinterpret_cast<T *>(buffer);
			delete object;
		}

		T *GetBuffer() const
		{
			void *buffer = pthread_getspecific(_key);
			if(!buffer)
			{
				buffer = new T();
				pthread_setspecific(_key, buffer);
			}

			return reinterpret_cast<T *>(buffer);
		}

		pthread_key_t _key;
	};

	template<class T>
	class ThreadLocalStorage<T *>
	{
	public:
		ThreadLocalStorage()
		{
			pthread_key_create(&_key, nullptr);
		}
		~ThreadLocalStorage()
		{
			pthread_key_delete(_key);
		}


		void SetValue(T *value)
		{
			pthread_setspecific(_key, value);
		}
		T *GetValue() const
		{
			return reinterpret_cast<T *>(pthread_getspecific(_key));
		}

	private:
		pthread_key_t _key;
	};
#endif

#if RN_PLATFORM_WINDOWS
	template<class T>
	class ThreadLocalStorage;

	template<class T>
	class ThreadLocalStorage
	{
	public:
		ThreadLocalStorage()
		{
			_key = ::TlsAlloc();
		}
		~ThreadLocalStorage()
		{
			::TlsFree(_key);
		}


		void SetValue(const T &value)
		{
			T *buffer = GetBuffer();
			*buffer = value;
		}
		T GetValue() const
		{
			T *buffer = GetBuffer();
			return *buffer;
		}

	private:
		static void CleanBuffer(void *buffer)
		{
			T *object = reinterpret_cast<T *>(buffer);
			delete object;
		}

		T *GetBuffer() const
		{
			void *buffer = ::TlsGetValue(_key);
			if(!buffer)
			{
				buffer = new T();
				::TlsSetValue(_key, buffer);

				Thread *thread = Thread::GetCurrentThread();
				thread->ExecuteOnExit([](void *context) {
					CleanBuffer(context);
				}, buffer);
			}

			return reinterpret_cast<T *>(buffer);
		}

		DWORD _key;
	};

	template<class T>
	class ThreadLocalStorage<T *>
	{
	public:
		ThreadLocalStorage()
		{
			_key = ::TlsAlloc();
		}
		~ThreadLocalStorage()
		{
			::TlsFree(_key);
		}


		void SetValue(T *value)
		{
			::TlsSetValue(_key, value);
		}
		T *GetValue() const
		{
			return reinterpret_cast<T *>(::TlsGetValue(_key));
		}

	private:
		DWORD _key;
	};
#endif
}

#endif /* __RAYNE_THREADLOCALSTORAGE_H__ */
