//
//  RNScopeGuard.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCOPEGUARD_H__
#define __RAYNE_SCOPEGUARD_H__

#include "RNBase.h"
#include "RNFunction.h"

namespace RN
{
	class ScopeGuard
	{
	public:
		template<typename F>
		explicit ScopeGuard(F &&rollback) :
			_committed(false),
			_rollback(std::move(rollback))
		{}
		
		ScopeGuard(ScopeGuard &&other) :
			_committed(other._committed),
			_rollback(std::move(other._rollback))
		{}
		
		ScopeGuard &operator=(ScopeGuard &&other)
		{
			_rollback = std::move(other._rollback);
			_committed = other._committed;
			
			return *this;
		}
		
		~ScopeGuard()
		{
			if(!_committed)
				_rollback();
		}
		
		
		void Commit()
		{
			_committed = true;
		}
		
	private:
		bool _committed;
		Function _rollback;
	};
}

#endif /* __RAYNE_SCOPEGUARD_H__ */
