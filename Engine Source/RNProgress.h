//
//  RNProgress.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PROGRESS_H__
#define __RAYNE_PROGRESS_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNString.h"

namespace RN
{
	class Progress : public Object
	{
	public:
		RNAPI Progress(size_t units);
		RNAPI ~Progress();
		
		RNAPI Progress *IntermediateProgressAcountingFor(size_t units);
		
		RNAPI void SetDescription(String *description);
		RNAPI void SetTotalUnits(size_t units);
		RNAPI void SetCompletedUnits(size_t units);
		RNAPI void IncrementCompletedUnits(size_t units);
		
		RNAPI void MakeActive();
		RNAPI void ResignActive();
		
		RNAPI static Progress *GetActiveProgress();
		
		RNAPI String *GetDescription();
		RNAPI size_t GetTotalUnits() const { return _totalUnits.load(); }
		RNAPI size_t GetCompletedUnits() const { return _completedUnits.load(); }
		RNAPI double GetFractionCompleted();
		RNAPI bool IsComplete() const { return _completed; }
		
		RNAPI Signal<void (double)> &GetProgressSignal() const { return const_cast<Signal<void (double)> &>(_progressSignal); }
		
	private:
		void AttemptCompletion();
		void ChildCompleted(Progress *child, size_t units);
		
		Array _children;
		
		Progress *_previous;
		Progress *_parent;
		
		Signal<void (double)> _progressSignal;
		
		std::atomic<size_t> _completedUnits;
		std::atomic<size_t> _totalUnits;
		
		String *_description;
		size_t _accountingUnits;
		size_t _accountedUnits;
		
		bool _completed;
		
		RNDeclareMeta(Progress)
	};
}

#endif /* __RAYNE_PROGRESS_H__ */
