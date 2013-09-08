//
//  RNKVO.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KVO_H__
#define __RAYNE_KVO_H__

#include "RNBase.h"

#define kRNObservableNewValueKey RNCSTR("kRNObservableNewValueKey")

namespace RN
{
	namespace KVO
	{
		template<class T>
		struct hash
		{};
		
		template<class T>
		struct equal_to
		{};
		
		template<>
		struct hash<const char *>
		{
			size_t operator()(const char *string) const
			{
				size_t hash = 0;
				std::hash<char> hasher;
				
				while(*string != '\0')
				{
					hash = static_cast<size_t>(hasher(*string)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
					string ++;
				}
				
				return hash;
			}
		};
		
		template<>
		struct equal_to<const char *>
		{
			bool operator()(const char *string1, const char *string2) const
			{
				return (strcmp(string1, string2) == 0);
			}
		};
	}

	
	
	class Object;
	class Dictionary;
	class ObservableContainer;
	
	enum class ObservableType
	{
		Int32,
		Float
	};
	
	class ObservableBase
	{
	public:
		friend class ObservableContainer;
		virtual ~ObservableBase();
	
		std::string GetName() const { return _name; }
		ObservableType GetType() const { return _type; }
		
		virtual void SetValue(Object *value) = 0;
		virtual Object *GetValue() const = 0;
		
		void SetWritable(bool writable) { _writable = writable; }
		bool IsWritable() const { return _writable; }
		
	protected:
		ObservableBase(const char *name, ObservableType type);
		
		void WillChangeValue();
		void DidChangeValue();
		
	private:
		const char *_name;
		ObservableContainer *_observable;
		ObservableType _type;
		
		bool _writable;
	};
	
	template<class T>
	class ObservableVariable : public ObservableBase
	{
	public:
	protected:
		ObservableVariable(T *value, const char *name, ObservableType type) :
			ObservableBase(name, type),
			_value(value)
		{
			RN_ASSERT(value != nullptr, "Value mustn't be NULL!");
		}
		
	protected:
		T *_value;
	};
	
	
	
	
	class ObservableContainer
	{
	public:
		typedef std::function<void (ObservableContainer *, const std::string&, Dictionary *)> CallbackType;
		
		friend class ObservableBase;
		virtual ~ObservableContainer();
		
		Object *GetValueForKey(const std::string& key) const;
		Object *GetValueForKeyPath(const std::string& key) const;
		
		void SetValueForKey(const std::string& key, Object *value);
		
		template<class Function, class Receiver>
		void AddObserver(const std::string& key, Function&& function, Receiver receiver, void *cookie)
		{
			AddObserver(key, std::bind(function, receiver, std::placeholders::_1), cookie);
		}
		
		void AddObserver(const std::string& key, CallbackType callback, void *cookie);
		void RemoveObserver(const std::string& key, void *cookie);
		
		const std::vector<ObservableBase *>& GetObservables() const { return _createdObservers; }
		
	protected:
		ObservableContainer();
		
		void AddObservable(ObservableBase *core);
		ObservableBase *AddObservable(void *ptr, const char *name, ObservableType type);
		
		void WillChangeValueForkey(const std::string& key);
		void DidChangeValueForKey(const std::string& key);
		
	private:
		struct Observer
		{
		public:
			Observer(const std::string& tkey, CallbackType tcallback, void *tcookie)
			{
				key = tkey;
				callback = tcallback;
				cookie = tcookie;
			}
			
			CallbackType callback;
			std::string key;
			void *cookie;
		};
		
		void WillChangeValueForVariable(ObservableBase *core);
		void DidChangeValueForVariable(ObservableBase *core);
		
		SpinLock _lock;
		
		ObservableBase *GetObservableForKey(const std::string& key) const;
		std::vector<Observer *>& GetObserversForKey(const std::string& key);
		
		std::unordered_map<const char *, ObservableBase *, KVO::hash<const char *>, KVO::equal_to<const char *>> _variables;
		std::vector<ObservableBase *> _createdObservers;
		
		std::vector<Observer> _observer;
		std::unordered_map<std::string, std::vector<Observer *>> _observerMap;
	};
}

#endif /* __RAYNE_KVO_H__ */
