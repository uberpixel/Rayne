//
//  RNProgress.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNProgress.h"
#include "RNThread.h"

#define kRNProgressThreadKey "kRNProgressThreadKey"

namespace RN
{
	RNDefineMeta(Progress, Object)
	
	Progress::Progress(size_t units) :
		_totalUnits(units),
		_completedUnits(0),
		_accountingUnits(0),
		_accountedUnits(0),
		_description(new String()),
		_completed(false),
		_parent(nullptr),
		_previous(nullptr)
	{}
	
	Progress::~Progress()
	{
		if(GetActiveProgress() == this)
			ResignActive();
		
		SafeRelease(_description);
	}
	
	
	Progress *Progress::GetActiveProgress()
	{
		Thread *thread = Thread::GetCurrentThread();
		return static_cast<Progress *>(thread->GetAssociatedObject(kRNProgressThreadKey));
	}
	
	Progress *Progress::IntermediateProgressAcountingFor(size_t units)
	{
		Progress *progress = new Progress(0);
		progress->_accountingUnits = units;
		progress->_parent = this;
		
		LockGuard<Object *> lock(this);
		_children.AddObject(progress);
		progress->Release();
		
		_accountedUnits += units;
		_progressSignal.Emit(GetFractionCompleted());
		
		progress->MakeActive();
		return progress;
	}
	
	
	void Progress::MakeActive()
	{
		_previous = SafeRetain(GetActiveProgress());
		
		Thread *thread = Thread::GetCurrentThread();
		thread->SetAssociatedObject(kRNProgressThreadKey, this, MemoryPolicy::Assign);
	}
	
	void Progress::ResignActive()
	{
		RN_ASSERT(GetActiveProgress() == this, "Can't resign a non active progress!");
		
		SetCompletedUnits(_totalUnits.load());
		
		if(_previous)
		{
			_previous->MakeActive();
			_previous->Release();
			
			_previous = nullptr;
		}
	}
	
	
	
	void Progress::SetDescription(String *description)
	{
		LockGuard<Object *> lock(this);
		RN_ASSERT(!_completed, "Can't update an already completed progress!");
		
		_description->Release();
		_description = description->Copy();
	}
	
	
	void Progress::SetTotalUnits(size_t units)
	{
		LockGuard<Object *> lock(this);
		RN_ASSERT(!_completed, "Can't update an already completed progress!");
		
		_totalUnits.store(units);
		_progressSignal.Emit(GetFractionCompleted());
		
		AttemptCompletion();
	}
	void Progress::SetCompletedUnits(size_t units)
	{
		LockGuard<Object *> lock(this);
		if(_completed)
			return;
		
		_completedUnits.store(units);
		_progressSignal.Emit(GetFractionCompleted());
		
		AttemptCompletion();
	}
	void Progress::IncrementCompletedUnits(size_t units)
	{
		LockGuard<Object *> lock(this);
		if(_completed)
			return;
		
		_completedUnits.fetch_add(units);
		_progressSignal.Emit(GetFractionCompleted());
		
		AttemptCompletion();
	}
	
	
	
	void Progress::ChildCompleted(Progress *child, size_t units)
	{
		LockGuard<Object *> lock(this);
		
		_accountedUnits -= units;
		IncrementCompletedUnits(units);
	}
	void Progress::AttemptCompletion()
	{
		if(!_completed)
		{
			if(_completedUnits.load() >= _totalUnits.load())
			{
				_completed = true;
				
				if(_parent)
					_parent->ChildCompleted(this, _accountingUnits);
			}
		}
		
		if(_parent)
			_parent->_progressSignal.Emit(_parent->GetFractionCompleted());
	}
	
	
	String *Progress::GetDescription()
	{
		LockGuard<Object *> lock(this);
		String *description = _description->Copy();
		return description->Autorelease();
	}
	
	double Progress::GetFractionCompleted()
	{
		LockGuard<Object *> lock(this);
		
		if(_completed || _totalUnits.load() == 0)
			return 1.0;
		
		double base = (_completedUnits.load() / static_cast<double>(_totalUnits.load()));
		
		_children.Enumerate<Progress>([&](Progress *child, size_t index, bool &stop) {
			
			if(child->_completed)
				return;
			
			double percentage = child->_accountingUnits / static_cast<double>(_totalUnits.load());
			base += child->GetFractionCompleted() * percentage;
			
		});
		
		return base;
	}
}
