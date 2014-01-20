//
//  RNSyncPoint.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SYNCPOINT_H__
#define __RAYNE_SYNCPOINT_H__

#include "RNBase.h"

namespace RN
{
	class syncable;
	class __sync_state;

	class sync_point
	{
	public:
		friend class syncable;
		
		sync_point();
		sync_point(sync_point &&other);
		sync_point(const sync_point &other) = delete;
		~sync_point();
		
		sync_point &operator= (sync_point &&other);
		sync_point &operator= (const sync_point &other) = delete;
		
		void wait();
		bool signaled();
		
	private:
		sync_point(__sync_state *state);
		
		__sync_state *_shared;
	};
	
	class syncable
	{
	public:
		syncable();
		~syncable();
		
		void signal_exception(std::exception_ptr e);
		void signal();
		
		sync_point get_sync_point();
		
	public:
		__sync_state *_state;
	};
	
}

#endif /* __RAYNE_SYNCPOINT_H__ */
