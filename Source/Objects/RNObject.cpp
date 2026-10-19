//
//  RNObject.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNObject.h"
#include "../Debug/RNLogger.h"
#include "RNAutoreleasePool.h"
#include "RNData.h"
#include "RNObjectInternals.h"
#include "RNString.h"
#include <string>

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	#include <iomanip>
	#if RN_PLATFORM_WINDOWS
		#include <dbghelp.h>
		#pragma comment(lib, "dbghelp.lib")
	#elif RN_PLATFORM_POSIX
		#if RN_PLATFORM_ANDROID || RN_PLATFORM_IOS
			#include <dlfcn.h>
			#include <unwind.h>
		#else
			#include <execinfo.h>
		#endif
		#include <cstdlib>
		#include <cxxabi.h>
		#include <dlfcn.h>
	#endif
#endif

#if RN_ZOMBIE_ALLOCATION
	#define AssertZombieInteraction()                                                                                                                                                                 \
		do {                                                                                                                                                                                          \
			if(_isZombie)                                                                                                                                                                             \
			{                                                                                                                                                                                         \
				MetaClass *meta = GetClass();                                                                                                                                                         \
				RNError(RN_FUNCTION_SIGNATURE << " called on zombie object <" << meta->GetFullname() << ":" << reinterpret_cast<const void *>(this) << ">, this will result in memory corruptions!"); \
			}                                                                                                                                                                                         \
		} while(0)
#else
	#define AssertZombieInteraction() \
		(void)0
#endif

namespace RN
{
	Object::Object() :
#if RN_ZOMBIE_ALLOCATION
		_isZombie(false),
#endif
		_refCount(1)
#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		,
		_availableReferenceCount(1),
		_autoreleaseCounter(0),
		_isTracked(false)
#endif
	{}

	Object::~Object()
	{
		if(std::uncaught_exceptions() == 0)
			RN_ASSERT(_refCount.load(std::memory_order_relaxed) <= 1, "refCount must be <= 1 upon destructor call. Use object->Unlock(); instead of delete object;");

		for(auto &pair : _associatedObjects)
		{
			MemoryPolicy policy = std::get<1>(pair.second);

			switch(policy)
			{
				case MemoryPolicy::Retain:
				case MemoryPolicy::Copy:
				{
					Object *object = std::get<0>(pair.second);
					object->Release();
					break;
				}

				default:
					break;
			}
		}

		__DestroyWeakReferences(this);
	}

	void Object::InitialWakeUp(MetaClass *meta)
	{}

	void Object::Dealloc()
	{}

	const String *Object::GetDescription() const
	{
		return RNSTR("<" << GetClass()->GetFullname() << ":" << (void *)this << ">");
	}

	MetaClass *Object::GetClass() const
	{
		return Object::GetMetaClass();
	}
	MetaClass *Object::GetMetaClass()
	{
		static MetaType *meta = []() -> MetaType * {
			__InitWeakTables();
			return new MetaType();
		}();

		return meta;
	}

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	Object *Object::Retain_debug(const char *file, int line, const char *func)
#else
	Object *Object::Retain()
#endif
	{
		AssertZombieInteraction();

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		if(_isTracked)
		{
			WillChangeReferenceCount(_refCount.load(std::memory_order_relaxed) + 1);
			RefcountDebugGraph::GetSharedInstance().AddRefcountDebugInfo(this, RefcountDebugGraph::RefcountCallEvent::Retain, file, line, func, _refCount.load(std::memory_order_relaxed) + 1);
		}
#endif

		_refCount.fetch_add(1, std::memory_order_relaxed); // RMW pairs with relaxed memory ordering
#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		_availableReferenceCount.fetch_add(1, std::memory_order_relaxed);
#endif
		return this;
	}

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	const Object *Object::Retain_debug(const char *file, int line, const char *func) const
#else
	const Object *Object::Retain() const
#endif
	{
		AssertZombieInteraction();

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		if(_isTracked)
		{
			WillChangeReferenceCount(_refCount.load(std::memory_order_relaxed) + 1);
			RefcountDebugGraph::GetSharedInstance().AddRefcountDebugInfo(this, RefcountDebugGraph::RefcountCallEvent::Retain, file, line, func, _refCount.load(std::memory_order_relaxed) + 1);
		}
#endif

		_refCount.fetch_add(1, std::memory_order_relaxed); // RMW pairs with relaxed memory ordering
#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		_availableReferenceCount.fetch_add(1, std::memory_order_relaxed);
#endif
		return this;
	}

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	void Object::Release_debug(const char *file, int line, const char *func) const
#else
	void Object::Release() const
#endif
	{
#if RN_ZOMBIE_ALLOCATION
		if(_isZombie)
		{
			MetaClass *meta = GetClass();
			RNError("Release() called on zombie object <" << meta->GetFullname() << ":" << reinterpret_cast<const void *>(this) << ">, this will result in memory corruptions!");

			return;
		}
#endif

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		RN_ASSERT(_availableReferenceCount.fetch_sub(1, std::memory_order_relaxed) > 0, "Object is in too many autorelease pools and will be over released!");

		if(_isTracked)
		{
			WillChangeReferenceCount(_refCount.load(std::memory_order_relaxed) - 1);
			RefcountDebugGraph::GetSharedInstance().AddRefcountDebugInfo(this, RefcountDebugGraph::RefcountCallEvent::Release, file, line, func, _refCount.load(std::memory_order_relaxed) - 1);
		}
#endif

		// If this is the last reference this thread has, which it very well might be,
		// we need to flush all accesses done so far. Thus the release barrier
		if(_refCount.fetch_sub(1, std::memory_order_release) == 1)
		{
			// Catch up with all changes from all other threads that had access to the object
			std::atomic_thread_fence(std::memory_order_acquire);

#if RN_ZOMBIE_ALLOCATION
			_isZombie = true;
#else
			// Since this function is marked const, we need to do this
			Object *nonConstThis = const_cast<Object *>(this);

			nonConstThis->Dealloc();
			delete nonConstThis;
#endif
		}
	}

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	Object *Object::Autorelease_debug(const char *file, int line, const char *func)
#else
	Object *Object::Autorelease()
#endif
	{
		AssertZombieInteraction();

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		if(_isTracked)
		{
			WillChangeAutoreleaseCount(_autoreleaseCounter.load(std::memory_order_relaxed) + 1);
			RefcountDebugGraph::GetSharedInstance().AddRefcountDebugInfo(this, RefcountDebugGraph::RefcountCallEvent::Autorelease, file, line, func, _refCount.load(std::memory_order_relaxed));
		}
#endif

		AutoreleasePool *pool = AutoreleasePool::GetCurrentPool();
		if(!pool)
		{
			if(Logger::GetSharedInstance())
			{
				AutoreleasePool::PerformBlock([this]() {
					MetaClass *meta = GetClass();
					RNError("Autorelease() with no pool in place, <" << meta->GetFullname() << ":" << reinterpret_cast<void *>(this) << "> will leak!");
				});
			}

			return this;
		}

		pool->AddObject(this);
		return this;
	}

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	const Object *Object::Autorelease_debug(const char *file, int line, const char *func) const
#else
	const Object *Object::Autorelease() const
#endif
	{
		AssertZombieInteraction();

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		if(_isTracked)
		{
			WillChangeAutoreleaseCount(_autoreleaseCounter.load(std::memory_order_relaxed) + 1);
			RefcountDebugGraph::GetSharedInstance().AddRefcountDebugInfo(this, RefcountDebugGraph::RefcountCallEvent::Autorelease, file, line, func, _refCount.load(std::memory_order_relaxed));
		}
#endif

		AutoreleasePool *pool = AutoreleasePool::GetCurrentPool();
		if(!pool)
		{
			if(Logger::GetSharedInstance())
			{
				AutoreleasePool::PerformBlock([this]() {
					MetaClass *meta = GetClass();
					RNError("Autorelease() with no pool in place, <" << meta->GetFullname() << ":" << reinterpret_cast<const void *>(this) << "> will leak!");
				});
			}

			return this;
		}

		pool->AddObject(this);
		return this;
	}


	Object *Object::Copy() const
	{
		AssertZombieInteraction();

		RN_ASSERT(GetClass()->SupportsCopying(), "Only Objects that support the copy trait can be copied!\n");
		return GetClass()->ConstructWithCopy(const_cast<Object *>(this));
	}

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	void Object::StartReferenceTracking()
	{
		_isTracked = true;
	}

	void Object::StopReferenceTracking()
	{
		_isTracked = false;
	}

	void Object::WillChangeReferenceCount(size_t refCount) const
	{
	}

	void Object::WillChangeAutoreleaseCount(size_t autoreleaseCount) const
	{
	}
#endif

	void Object::Serialize(Serializer *serializer) const
	{
		throw InconsistencyException("Serialization not supported (or a subclass called Object::Serialize)");
	}


	bool Object::IsEqual(const Object *other) const
	{
		return (this == other);
	}

	size_t Object::GetHash() const
	{
		size_t hash = reinterpret_cast<size_t>(this);

		hash = ~hash + (hash << 15);
		hash = hash ^ (hash >> 12);
		hash = hash + (hash << 2);
		hash = hash ^ (hash >> 4);
		hash = hash * 2057;
		hash = hash ^ (hash >> 16);

		return hash;
	}


	bool Object::IsKindOfClass(const MetaClass *other) const
	{
		return GetClass()->InheritsFromClass(other);
	}


	void Object::__RemoveAssociatedObject(const void *key)
	{
		auto iterator = _associatedObjects.find((void *)key);
		if(iterator != _associatedObjects.end())
		{
			Object *object = std::get<0>(iterator->second);
			MemoryPolicy policy = std::get<1>(iterator->second);

			switch(policy)
			{
				case MemoryPolicy::Retain:
				case MemoryPolicy::Copy:
					object->Release();
					break;

				default:
					break;
			}

			_associatedObjects.erase(iterator);
		}
	}


	void Object::SetAssociatedObject(const void *key, Object *value, MemoryPolicy policy)
	{
		RN_ASSERT(value, "Value mustn't be NULL!");
		RN_ASSERT(key, "Key mustn't be NULL!");

		Object *object = nullptr;

		if(value)
		{
			switch(policy)
			{
				case MemoryPolicy::Assign:
					object = value;
					break;

				case MemoryPolicy::Retain:
					object = value->Retain();
					break;

				case MemoryPolicy::Copy:
					object = value->GetMetaClass()->ConstructWithCopy(value);
					break;
			}
		}

		Lock();
		__RemoveAssociatedObject(key);

		if(object)
		{
			std::tuple<Object *, MemoryPolicy> tuple = std::make_tuple(object, policy);
			_associatedObjects.emplace(const_cast<void *>(key), std::move(tuple));
		}

		Unlock();
	}

	Object *Object::GetAssociatedObject(const void *key)
	{
		Object *object = 0;

		Lock();

		auto iterator = _associatedObjects.find((void *)key);

		if(iterator != _associatedObjects.end())
			object = std::get<0>(iterator->second);

		Unlock();

		return object;
	}


	void Object::Lock()
	{
		_lock.Lock();
	}

	void Object::Unlock()
	{
		_lock.Unlock();
	}

	// ---------------------
	// MARK: -
	// MARK: KVO / KVC
	// ---------------------

	std::vector<ObservableProperty *> Object::GetPropertiesForClass(MetaClass *meta)
	{
		if(!meta)
			return _properties;

		std::vector<ObservableProperty *> result;

		for(ObservableProperty *property : _properties)
		{
			if(property->_opaque == meta)
				result.push_back(property);
		}

		return result;
	}

	void Object::AddObservable(ObservableProperty *property)
	{
		RN_ASSERT(property->_owner == nullptr, "ObservableProperty can only be added once to a receiver!");

		property->_owner = this;
		property->_opaque = GetClass();

		_properties.push_back(property);
	}

	void Object::AddObservables(const std::initializer_list<ObservableProperty *> properties)
	{
		_properties.reserve(_properties.size() + properties.size());

		for(ObservableProperty *property : properties)
			AddObservable(property);
	}

	void Object::MapCookie(void *cookie, ObservableProperty *property, Connection *connection) const
	{
		LockGuard<RecursiveLockable> lock(const_cast<RecursiveLockable &>(_lock));
		_cookies.emplace_back(std::make_tuple(cookie, property, connection));
	}

	void Object::UnmapCookie(void *cookie, ObservableProperty *property) const
	{
		LockGuard<RecursiveLockable> lock(const_cast<RecursiveLockable &>(_lock));

		for(auto iterator = _cookies.begin(); iterator != _cookies.end();)
		{
			auto &tuple = *iterator;

			if(cookie == std::get<0>(tuple) && property == std::get<1>(tuple))
			{
				std::get<2>(tuple)->Disconnect();

				if(property->_signal && property->_signal->GetCount() == 0)
				{
					delete property->_signal;
					property->_signal = nullptr;
				}

				iterator = _cookies.erase(iterator);
				continue;
			}

			iterator++;
		}
	}

	ObservableProperty *Object::GetPropertyForKey(const char *key) const
	{
		LockGuard<RecursiveLockable> lock(const_cast<RecursiveLockable &>(_lock));

		for(ObservableProperty *property : _properties)
		{
			if(strcmp(property->_name, key) == 0)
				return property;
		}

		return nullptr;
	}


	Object *Object::GetPrimitiveValueForKey(const char *key) const
	{
		for(ObservableProperty *property : _properties)
		{
			if(strcmp(key, property->_name) == 0)
				return property->GetValue();
		}

		return GetValueForUndefinedKey(key);
	}

	Object *Object::GetPrimitiveValueForKeyPath(const char *keyPath) const
	{
		char storage[33];
		const char *temp = strchr(keyPath, '.');

		if(!temp)
			return GetPrimitiveValueForKey(keyPath);

		temp++;

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
		strlcpy(storage, keyPath, temp - keyPath);
#else
		strncpy(storage, keyPath, temp - keyPath);
		storage[32] = '\0';
#endif

		Object *next = GetValueForKey(storage);
		if(!next)
			return GetValueForUndefinedKey(storage);

		return next->GetPrimitiveValueForKeyPath(temp);
	}


	void Object::WillChangeValueForKey(const char *key)
	{
		ObservableProperty *property = GetPropertyForKey(key);
		if(property)
		{
			property->WillChangeValue();
			return;
		}
	}

	void Object::DidChangeValueForKey(const char *key)
	{
		ObservableProperty *property = GetPropertyForKey(key);

		if(property)
		{
			property->DidChangeValue();
			return;
		}
	}


	void Object::SetValueForKeyPath(Object *value, const char *keyPath)
	{
		char storage[33];
		const char *temp = strchr(keyPath, '.');

		if(!temp)
		{
			SetValueForKey(value, keyPath);
			return;
		}

		temp++;

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
		strlcpy(storage, keyPath, temp - keyPath);
#else
		strncpy(storage, keyPath, temp - keyPath);
		storage[32] = '\0';
#endif

		Object *next = GetValueForKey(storage);
		if(!next)
			throw InconsistencyException(RNSTR("SetValueForKeyPath() with undefined key '" << storage << "' on " << this));

		next->SetValueForKeyPath(value, temp);
	}

	void Object::SetValueForKey(Object *value, const char *key)
	{
		ObservableProperty *property = GetPropertyForKey(key);
		property ? property->SetValue(value) : SetValueForUndefinedKey(value, key);
	}

	void Object::SetPrimitiveValueForKey(Object *value, const char *key)
	{
		for(ObservableProperty *property : _properties)
		{
			if(strcmp(key, property->_name) == 0)
				property->SetValue(value);
		}

		SetValueForUndefinedKey(value, key);
	}


	void Object::SetValueForUndefinedKey(Object *value, const char *key)
	{
		throw InconsistencyException(RNSTR("SetValueForKey() with undefined key '" << key << "'"));
	}

	Object *Object::GetValueForUndefinedKey(const char *key) const
	{
		throw InconsistencyException(RNSTR("GetValueForKey() with undefined key '" << key << "'"));
	}

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	std::string RefcountDebugGraph::CaptureStackTrace()
	{
		constexpr int kMaxFrames = 32;
		constexpr int kSkip = 2; // Skip CaptureStackTrace and its caller

		std::ostringstream out;

	#if RN_PLATFORM_WINDOWS
		void *frames[kMaxFrames] = {};
		USHORT count = CaptureStackBackTrace(kSkip, kMaxFrames, frames, nullptr);
		for(USHORT i = 0; i < count; ++i)
			out << frames[i] << "\n";

	#elif RN_PLATFORM_LINUX || RN_PLATFORM_MAC_OS
		void *frames[kMaxFrames] = {};
		int count = backtrace(frames, kMaxFrames);
		if(count <= kSkip)
			return "<no stack>";
		for(int i = kSkip; i < count; ++i)
			out << frames[i] << "\n";

	#elif RN_PLATFORM_ANDROID || RN_PLATFORM_IOS
		struct BacktraceState
		{
			void **current;
			void **end;
			int skip;
		};

		auto unwindCallback = [](_Unwind_Context *ctx, void *arg) -> _Unwind_Reason_Code {
			auto *state = static_cast<BacktraceState *>(arg);
			uintptr_t pc = _Unwind_GetIP(ctx);
			if(pc)
			{
				if(state->skip > 0)
				{
					--state->skip;
				}
				else if(state->current < state->end)
				{
					*state->current++ = reinterpret_cast<void *>(pc);
				}
			}
			return _URC_NO_REASON;
		};

		void *buffer[kMaxFrames];
		BacktraceState state = {buffer, buffer + kMaxFrames, kSkip};
		_Unwind_Backtrace(unwindCallback, &state);
		int captured = state.current - buffer;
		for(int i = 0; i < captured; ++i)
			out << buffer[i] << "\n";

	#else
		out << "<stack capture unsupported>\n";
	#endif

		return out.str();
	}

	void RefcountDebugGraph::DumpAll(const std::string &filePath, bool onlyStillLive)
	{
		std::lock_guard<std::mutex> lk(_mu);
		std::ofstream file(filePath + "/ref_trace.html");

		// escape HTML attributes
		auto escapeAttr = [](const std::string &s) {
			std::string r;
			r.reserve(s.size());
			for(char c : s)
			{
				switch(c)
				{
					case '&':
						r += "&amp;";
						break;
					case '"':
						r += "&quot;";
						break;
					case '\'':
						r += "&#39;";
						break;
					case '<':
						r += "&lt;";
						break;
					case '>':
						r += "&gt;";
						break;
					default:
						r += c;
						break;
				}
			}
			return r;
		};

		// 0) match retain→(autorelease)→release per-object, per-caller-class
		auto temp = _refcountDebugMap;
		if(onlyStillLive)
		{
			for(auto it = temp.begin(); it != temp.end();)
			{
				auto &events = it->second.events;
				if(events.empty() || events.back().afterRefCount == 0)
					it = temp.erase(it);
				else
					++it;
			}
		}

		std::unordered_map<const Object *, std::vector<char>> matchedMap;
		auto extractClass = [](const char *func) {
			std::string f(func);
			if(auto p = f.find('('); p != f.npos) f.resize(p);
			if(auto p = f.rfind("::"); p != f.npos) f.resize(p);
			if(auto p = f.rfind("::"); p != f.npos) f = f.substr(p + 2);
			return f;
		};

		for(auto &pr : temp)
		{
			auto &events = pr.second.events;
			std::vector<char> matched(events.size(), 0);
			std::vector<size_t> retains;
			std::vector<std::pair<size_t, size_t>> autoStack;

			for(size_t i = 0; i < events.size(); ++i)
			{
				auto &e = events[i];
				std::string cls = extractClass(e.func);
				switch(e.type)
				{
					case RefcountCallEvent::Retain:
						retains.push_back(i);
						break;
					case RefcountCallEvent::Autorelease:
						for(auto it = retains.rbegin(); it != retains.rend(); ++it)
						{
							if(extractClass(events[*it].func) == cls)
							{
								retains.erase(std::next(it).base());
								autoStack.emplace_back(*it, i);
								break;
							}
						}
						break;
					case RefcountCallEvent::Release:
						if(cls == "AutoreleasePool" && !autoStack.empty())
						{
							auto [ridx, ar] = autoStack.back();
							autoStack.pop_back();
							matched[ridx] = matched[ar] = matched[i] = 1;
						}
						else
						{
							for(auto it = retains.rbegin(); it != retains.rend(); ++it)
							{
								if(extractClass(events[*it].func) == cls)
								{
									matched[*it] = matched[i] = 1;
									retains.erase(std::next(it).base());
									break;
								}
							}
						}
						break;
				}
			}
			matchedMap.emplace(pr.first, std::move(matched));
		}

		// 1) group by class + full sequence, collecting object IDs
		using Seq = std::vector<std::string>;
		std::unordered_map<std::string, std::map<Seq, std::vector<std::string>>> byClass;
		struct Snip
		{
			std::string html;
		};
		std::unordered_map<std::string, Snip> snippets;
		std::unordered_map<std::string, std::string> eventClass, displayLabel;
		std::unordered_map<std::string, std::map<Seq, std::vector<char>>> matchedMask;

		for(auto const &pr : temp)
		{
			auto const &info = pr.second;
			auto const &events = info.events;
			auto const &mask = matchedMap.at(pr.first);

			Seq seq;
			seq.reserve(events.size());
			std::vector<char> seqM;
			seqM.reserve(events.size());
			for(size_t i = 0; i < events.size(); ++i)
			{
				auto &e = events[i];
				// file:line
				std::string path(e.file);
				if(auto p = path.find_last_of("/\\"); p != path.npos)
					path = path.substr(p + 1);
				// fn
				std::string fn(e.func);
				if(auto p = fn.find('('); p != fn.npos) fn.resize(p);
				if(auto c = fn.find_last_of(':'); c != fn.npos) fn = fn.substr(c + 1);

				std::ostringstream lbl;
				lbl << fn << "(" << e.afterRefCount << ")@" << path << ":" << e.line;
				std::string L = lbl.str();
				const char *T =
				(e.type == RefcountCallEvent::Retain)  ? "retain|" :
				(e.type == RefcountCallEvent::Release) ? "release|" :
														 "autorelease|";
				std::string key = std::string(T) + L;

				if(!eventClass.count(key))
				{
					eventClass[key] =
					(e.type == RefcountCallEvent::Retain)  ? "retain" :
					(e.type == RefcountCallEvent::Release) ? "release" :
															 "autorelease";
				}
				displayLabel.emplace(key, L);

				seq.push_back(key);
				seqM.push_back(mask[i]);

				if(!snippets.count(key))
				{
					std::istringstream in(e.stackTrace);
					std::string addr;
					std::ostringstream out;
	#if RN_PLATFORM_WINDOWS
					HANDLE p = GetCurrentProcess();
					SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
					SymInitialize(p, nullptr, TRUE);
					while(std::getline(in, addr))
					{
						uintptr_t v = std::stoull(addr, nullptr, 16);
						DWORD64 a = (DWORD64)v;
						alignas(SYMBOL_INFO) char buf[sizeof(SYMBOL_INFO) + 256];
						auto sym = reinterpret_cast<PSYMBOL_INFO>(buf);
						sym->SizeOfStruct = sizeof(SYMBOL_INFO);
						sym->MaxNameLen = 255;
						if(SymFromAddr(p, a, nullptr, sym))
							out << "  at " << sym->Name << " [0x" << std::hex << sym->Address << std::dec << "]\n";
						else
							out << "  at [0x" << std::hex << a << std::dec << "]\n";
					}
	#elif RN_PLATFORM_POSIX
					while(std::getline(in, addr))
					{
						uintptr_t v = std::stoull(addr, nullptr, 16);
						Dl_info di {};
						if(dladdr((void *)v, &di) && di.dli_sname)
						{
							int st = 0;
							char *d = abi::__cxa_demangle(di.dli_sname, nullptr, nullptr, &st);
							out << "  at " << (st == 0 ? d : di.dli_sname) << " [0x" << std::hex << v << std::dec << "]\n";
							if(d) std::free(d);
						}
						else
						{
							out << "  at [0x" << std::hex << v << std::dec << "]\n";
						}
					}
	#else
					out << "<stack unsupported>\n";
	#endif
					std::ostringstream ss;
					ss << "<pre><code>" << out.str() << "\n</code></pre>";
					snippets[key] = {ss.str()};
				}
			}

			byClass[info.className][seq].push_back(info.object);

			auto &mm = matchedMask[info.className][seq];
			if(mm.empty())
				mm = seqM;
			else
			{
				for(size_t i = 0, n = std::min(mm.size(), seqM.size()); i < n; ++i)
					mm[i] = mm[i] && seqM[i];
			}
		}

		// 2) assign IDs
		std::unordered_map<std::string, std::string> eventID;
		size_t eid = 0;
		for(auto const &c : byClass)
			for(auto const &s : c.second)
				for(auto const &k : s.first)
					if(!eventID.count(k))
						eventID[k] = "EV" + std::to_string(eid++);

		// 3) emit HTML+CSS+JS
		file << R"(<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>Ref Trace</title>
<style>
 body{font:14px sans-serif;margin:0;padding:0}
 #viewer{position:fixed;right:0;top:0;bottom:0;width:40%;padding:10px;
  background:#fff;border-left:2px solid #ccc;overflow:auto;display:none;
  z-index:2000;box-shadow:-2px 0 5px rgba(0,0,0,0.1)}
 #closeBtn{cursor:pointer;color:#c00;float:right;font-size:1.2em}
 #code{white-space:pre-wrap;font-family:monospace}
 .row{display: flex;align-items: center;padding: 8px;border-bottom: 1px solid #eee;width: max-content;min-width: 100%;background-clip: padding-box;box-sizing: border-box;}
 .row[data-final-count="0"] { /* dead rows hidden when .hide-dead */ }
 .row:not([data-final-count="0"]) {background-color: #abf7b1;}
 .node{position:relative;padding:4px 8px;margin-right:24px;
  background:#f5f5f5;border:1px solid #888;border-radius:4px;
  cursor:pointer;white-space:nowrap;font-size:0.9em}
 .node::after{content:'→';position:absolute;right:-16px;top:50%;
  transform:translateY(-50%);color:#555}
 .node:last-child::after{content:''}
 .node.class{background:#cce5ff;border-color:#339;font-weight:bold}
 .node.class .id{font-size:0.8em;color:#555;margin-left:6px}
 .node.retain{background:#e0ffe0;border-color:#080}
 .node.release{background:#ffe0e0;border-color:#800}
 .node.autorelease{background:#e0f0ff;border-color:#008}
 .node.event.matched{opacity:0.35}
 .hide-matched .row > .node.event.matched { display: none; }
 .hide-matched .row > .node.event.matched.first-visible { display: block; }
 .hide-dead .row[data-final-count="0"]{display:none}
 .node.event.selected{outline:2px solid #06c}
 .count{margin-left:4px;color:#c33;font-weight:bold}
 #toolbar {position:fixed;top:0;left:0;right:0;background:#fafafa;
  padding:8px 12px;border-bottom:1px solid #ddd;z-index:1000;}
 #toolbar button{padding:6px 10px;border:1px solid #888;
  background:#fff;border-radius:4px;cursor:pointer}
 .seq-group{background:#fff8e1;border-color:#caa74a}
 .hidden-by-collapse{display:none!important}
 #content{padding-top:48px;}
</style>
</head><body>
<div id="viewer"><span id="closeBtn">✖</span><div id="code"></div></div>
<div id="content"><h1>Reference‑Count Sequences</h1>
<div id="toolbar">
  <button id="btnPrev">◀ Prev</button>
  <button id="btnNext">Next ▶</button>
  <button id="btnToggleMatched">Hide matched</button>
  <button id="btnToggleAlive">Show only alive</button>
  <button id="btnToggleOrder">Show newest first</button>
  <button id="btnToggleCollapse">Collapse repeats</button>
</div>
)";

		// 4) render rows, tagging final count
		for(auto const &c : byClass)
		{
			for(auto const &p : c.second)
			{
				const auto &seq = p.first;
				const auto &objs = p.second;
				const auto &mask = matchedMask[c.first].at(seq);
				int cnt = (int)objs.size();
				// final count = last event’s afterRefCount
				const auto &lastKey = seq.back();
				int finalCount = std::stoi(displayLabel[lastKey].substr(
				displayLabel[lastKey].find('(') + 1));

				file << "<div class=\"row\" data-final-count=\"" << finalCount << "\">\n"
					 << "  <div class=\"node class\">" << c.first;
				if(cnt == 1)
				{
					file << "<span class=\"id\">@" << objs[0] << "</span>";
				}
				else
				{
					file << "<span class=\"count\">×" << cnt << "</span>";
				}
				file << "</div>\n";

				for(size_t i = 0; i < seq.size(); ++i)
				{
					const auto &k = seq[i];
					bool m = (i < mask.size() && mask[i]);
					file << "<div class=\"node event " << eventClass[k]
						 << (m ? " matched" : "")
						 << "\" id=\"" << eventID[k]
						 << "\" data-snippet=\"" << escapeAttr(snippets[k].html)
						 << "\">" << displayLabel[k] << "</div>\n";
				}
				file << "</div>\n\n";
			}
		}

		// 5) JS for popup, nav, toggles, collapse, first-visible
		file << R"(</div><script>
 // popup
 document.getElementById('closeBtn').onclick = ()=>document.getElementById('viewer').style.display='none';
 document.addEventListener('click', e => {
  const el = e.target.closest('.node.event');
  if (!el || !el.dataset.snippet) return;
  document.getElementById('code').innerHTML = el.dataset.snippet;
  document.getElementById('viewer').style.display = 'block';
 });

 // collect all events
 function allEvs() {
   return Array.from(document.querySelectorAll('.node.event'));
 }

 let nav = [], idx = 0;
 function rebuildNav() {
   nav = allEvs().filter(n => !n.classList.contains('matched'));
   idx = 0;
   if (nav.length) select(0);
 }

 function select(i) {
   if (nav[idx]) nav[idx].classList.remove('selected');
   idx = i;
   nav[idx].classList.add('selected');
   nav[idx].scrollIntoView({behavior:'smooth',block:'center'});
 }

 // show first/matched event marker
 function updateFirstVisible() {
   document.querySelectorAll('.row').forEach(row => {
	 const evs = Array.from(row.querySelectorAll('.node.event.matched'));
	 evs.forEach(n => n.classList.remove('first-visible'));
	 if (row.classList.contains('newest-first')) {
	   if (evs.length) evs[0].classList.add('first-visible');
	 } else {
	   if (evs.length) evs[evs.length-1].classList.add('first-visible');
	 }
   });
 }

 document.getElementById('btnNext').onclick = ()=>{ if(idx+1<nav.length) select(idx+1); };
 document.getElementById('btnPrev').onclick = ()=>{ if(idx>0) select(idx-1); };

 // toggles
 const btnM = document.getElementById('btnToggleMatched'),
	   btnA = document.getElementById('btnToggleAlive'),
	   btnO = document.getElementById('btnToggleOrder'),
	   btnC = document.getElementById('btnToggleCollapse');
 let collapse=false, oldest=true;

 btnM.onclick = ()=> {
   document.body.classList.toggle('hide-matched');
   btnM.textContent = document.body.classList.contains('hide-matched') ? 'Show matched' : 'Hide matched';
   if (collapse) collapseAll();
   updateFirstVisible();
   rebuildNav();
 };
 btnA.onclick = ()=> {
   document.body.classList.toggle('hide-dead');
   btnA.textContent = document.body.classList.contains('hide-dead') ? 'Show all' : 'Show only alive';
   rebuildNav();
 };
 btnO.onclick = ()=> {
   document.querySelectorAll('.row').forEach(r => {
	 if (collapse) clearRow(r);
	 const evs = Array.from(r.querySelectorAll('.node.event'));
	 for (let i = evs.length-1; i >= 0; --i) r.appendChild(evs[i]);
	 if (oldest) r.classList.add('newest-first');
	 else      r.classList.remove('newest-first');
   });
   oldest = !oldest;
   document.body.classList.toggle('newest-first');
   btnO.textContent = oldest ? 'Show newest first' : 'Show oldest first';
   if (collapse) collapseAll();
   updateFirstVisible();
   rebuildNav();
 };

 function clearRow(r) {
   r.querySelectorAll('.seq-group').forEach(g=>g.remove());
   r.querySelectorAll('.hidden-by-collapse').forEach(n=>n.classList.remove('hidden-by-collapse'));
 }
 function collapseRow(r) {
   const evs = Array.from(r.querySelectorAll('.node.event')).filter(n=>n.style.display!=='none');
   const keys = evs.map(n=>n.id+'|'+(n.classList.contains('matched')?'m':'u'));
   let i=0,n=keys.length;
   while (i<n) {
	 let bl=0,br=1;
	 for (let L=2; i+2*L<=n && L<8; ++L) {
	   let rep=1,j=i+L;
	   while (j+L<=n && keys.slice(i,i+L).join()===keys.slice(j,j+L).join()) { rep++; j+=L; }
	   if (rep>1 && L*rep>bl*br) { bl=L; br=rep; }
	 }
	 if (bl>1 && br>1) {
	   const first = evs[i], grp = document.createElement('div');
	   grp.className='node seq-group'; grp.setAttribute('data-expanded','0');
	   const labs = evs.slice(i,i+bl).map(x=>x.textContent);
	   const s = labs.length<=3 ? labs.join(' → ') : labs.slice(0,2).join(' → ')+' … '+labs[labs.length-1];
	   grp.textContent='['+s+'] ×'+br; grp._nodes=[];
	   for (let k=0;k<bl*br;++k) {
		 evs[i+k].classList.add('hidden-by-collapse');
		 grp._nodes.push(evs[i+k]);
	   }
	   r.insertBefore(grp, first);
	   i += bl*br;
	 } else i++;
   }
 }
 function collapseAll() {
   document.querySelectorAll('.row').forEach(r=>{ clearRow(r); collapseRow(r); });
 }
 btnC.onclick = ()=> {
   collapse = !collapse;
   if (collapse) { collapseAll(); btnC.textContent='Expand repeats'; }
   else          { document.querySelectorAll('.row').forEach(clearRow); btnC.textContent='Collapse repeats'; }
 };

 // initial setup
 updateFirstVisible();
 rebuildNav();
</script></body></html>)";
	}
#endif
} // namespace RN
