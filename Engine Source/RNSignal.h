//
//  RNSignal.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection..
//

#ifndef __RAYNE_SIGNAL_H__
#define __RAYNE_SIGNAL_H__

#include "RNBase.h"

namespace RN
{
	template<class>
	class Signal {};
	class Connection;
	
	class __SignalBase
	{
	public:
		friend class Connection;
		
	private:
		virtual void RemoveSlot(Tag tag) = 0;
	};
	
	class Connection
	{
	public:
		Connection(__SignalBase *signal, Tag tag) :
			_signal(signal),
			_tag(tag)
		{}
		
		bool IsConnected() const
		{
			return (_signal.load() != nullptr);
		}
		
		void Disconnect()
		{
			__SignalBase *signal = nullptr;
			
			signal = _signal.exchange(signal);
			
			if(signal)
			{
				signal->RemoveSlot(_tag);
				signal = nullptr;
			}
		}
		
	private:
		std::atomic<__SignalBase *> _signal;
		Tag _tag;
	};
	
	class ScopeConnection
	{
	public:
		ScopeConnection(Connection *connection) :
			_connection(connection)
		{}
		
		~ScopeConnection()
		{
			_connection->Disconnect();
		}
		
		Connection *GetConnection() const
		{
			return _connection;
		}
		
	private:
		Connection *_connection;
	};
	
	template<class R, class ...Sig>
	class Signal<R (Sig...)> : public __SignalBase
	{
	public:
		Signal() :
			_tags(0)
		{}
		
		template <typename... SigCompatible>
		void Emit(SigCompatible&&... args)
		{
			LockGuard<SpinLock> lock(_lock);
			
			std::vector<Slot> slots;
			std::swap(slots, _slots);
			
			lock.Unlock();
			
			for(Slot& slot : slots)
				slot.callback(std::forward<SigCompatible>(args)...);
			
			lock.Lock();
			std::swap(slots, _slots);
			
			if(!slots.empty())
				_slots.insert(_slots.end(), std::make_move_iterator(slots.begin()), std::make_move_iterator(slots.end()));
		}
		
		template<class F>
		Connection *Connect(F&& f)
		{
			Tag tag = _tags ++;
			Connection *connection = new Connection(this, tag);
			
			LockGuard<SpinLock> lock(_lock);
			_slots.emplace_back(Slot(tag, connection, std::move(f)));
			
			return connection;
		}
		
		size_t GetCount() const
		{
			return _slots.size();
		}
		
	private:
		struct Slot
		{
			template<class F>
			Slot(Tag ttag, Connection *tconnection, F&& f) :
				tag(ttag),
				callback(std::move(f)),
				connection(tconnection)
			{}
			
			~Slot()
			{
				if(connection)
					connection->Disconnect();
			}
			
			Slot(Slot&& other)
			{
				Move(std::move(other));
			}
			
			Slot& operator= (Slot&& other)
			{
				Move(std::move(other));
				return *this;
			}
			
			Tag tag;
			std::function<R (Sig...)> callback;
			std::unique_ptr<Connection> connection;
			
		private:
			void Move(Slot&& other)
			{
				tag = std::move(other.tag);
				callback = std::move(other.callback);
				connection = std::move(other.connection);
			}
		};
		
		void RemoveSlot(Tag tag) override
		{
			LockGuard<SpinLock> lock(_lock);
		
			auto iterator = std::find_if(_slots.begin(), _slots.end(), [&](const Slot& slot) { return slot.tag == tag; });
			if(iterator != _slots.end())
			{
				iterator->connection.reset();
				_slots.erase(iterator);
			}
		}
		
		SpinLock _lock;
		std::atomic<Tag> _tags;
		std::vector<Slot> _slots;
	};
}

#endif /* __RAYNE_SIGNAL_H__ */
