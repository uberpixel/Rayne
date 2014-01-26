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
	namespace stl
	{
		class syncable;
		class __sync_state;

		class sync_point
		{
		public:
			friend class syncable;
			
			RNAPI sync_point();
			RNAPI sync_point(sync_point &&other);
			sync_point(const sync_point &other) = delete;
			RNAPI ~sync_point();
			
			RNAPI sync_point &operator= (sync_point &&other);
			sync_point &operator= (const sync_point &other) = delete;
			
			RNAPI void wait();
			RNAPI bool signaled();
			
		private:
			sync_point(__sync_state *state);
			
			__sync_state *_shared;
		};
		
		class syncable
		{
		public:
			RNAPI syncable();
			RNAPI syncable(syncable &&other);
			syncable(const syncable &other) = delete;
			RNAPI ~syncable();
			
			RNAPI syncable &operator= (syncable &&other);
			syncable &operator= (const syncable &other) = delete;
			
			RNAPI void signal_exception(std::exception_ptr e);
			RNAPI void signal();
			
			RNAPI sync_point get_sync_point();
			
		public:
			__sync_state *_state;
		};
	}
}

#endif /* __RAYNE_SYNCPOINT_H__ */
