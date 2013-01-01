//
//  RNList.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNList.h"

namespace RN
{
	ListMember::ListMember()
	{
		_list = 0;
		_next = _prev = 0;
	}
	
	void ListMember::AddNext(ListMember *member)
	{
		RN::Assert(member && member->_list == 0);
		
		member->Retain();
		
		member->_next = _next;
		member->_prev = this;
		member->_list = _list;
		
		_next = member;
		
		if(_list->_tail == this)
			_list->_tail = member;
	}
	
	void ListMember::AddPrev(ListMember *member)
	{
		RN::Assert(member && member->_list == 0);
		
		member->Retain();
		
		member->_next = this;
		member->_prev = _prev;
		member->_list = _list;
		
		_prev = member;
		
		if(_list->_head == this)
			_list->_head = member;
	}
	
	
	
	List::List()
	{
		_head = 0;
		_tail = 0;
	}
	
	List::~List()
	{
		ListMember *member = _head;
		while(member)
		{
			ListMember *next = member->Next();
			
			member->_list = 0;
			member->_next = member->_prev = 0;
			
			member->Release();
			member = next;
		}
	}
	
	
	void List::AddFront(ListMember *member)
	{
		RN::Assert(member->_list == 0);
		
		member->_prev = 0;
		member->_next = _head;
		
		_head = member;
		if(!_tail)
			_tail = member;
		
		member->_list = this;
		member->Retain();
	}
	
	void List::AddBack(ListMember *member)
	{
		RN::Assert(member->_list == 0);
		
		member->_prev = _tail;
		member->_next = 0;
		
		_tail = member;
		if(!_head)
			_head = member;
		
		member->_list = this;
		member->Retain();		
	}
	
	void List::Remove(ListMember *member)
	{
		RN::Assert(member->_list == this);
		
		if(member->_next)
			member->_next->_prev = member->_prev;
		
		if(member->_prev)
			member->_prev->_next = member->_next;
		
		if(_head == member)
			_head = member->_next;
		
		if(_tail == member)
			_tail = member->_prev;
		
		member->_list = 0;
		member->Release();
	}
}
