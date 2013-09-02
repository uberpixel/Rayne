//
//  RNIndexSet.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNIndexSet.h"

namespace RN
{
	RNDeclareMeta(IndexSet)
	
	IndexSet::IndexSet()
	{}
	
	IndexSet::IndexSet(IndexSet *other)
	{
		_sortedIndices = other->_sortedIndices;
		_indices = other->_indices;
	}
	
	
	void IndexSet::AddIndex(size_t index)
	{
		if(_indices.find(index) == _indices.end())
		{
			_indices.insert(index);
			_sortedIndices.push_back(index);
			
			std::sort(_sortedIndices.begin(), _sortedIndices.end());
		}
	}
	void IndexSet::RemoveIndex(size_t index)
	{
		if(_indices.find(index) != _indices.end())
		{
			_indices.erase(index);
			_sortedIndices.erase(std::remove(_sortedIndices.begin(), _sortedIndices.end(), index), _sortedIndices.end());
		}
	}
	
	
	bool IndexSet::ContainsIndex(size_t index)
	{
		return (_indices.find(index) != _indices.end());
	}
	
	
	size_t IndexSet::GetFirstIndex() const
	{
		if(_sortedIndices.size() > 0)
			return _sortedIndices.at(0);
		
		return k::NotFound;
	}
	
	size_t IndexSet::GetLastIndex() const
	{
		if(_sortedIndices.size() > 0)
			return _sortedIndices.at(_sortedIndices.size() - 1);
		
		return k::NotFound;
	}
	
	size_t IndexSet::GetCount() const
	{
		return _sortedIndices.size();
	}
	
	size_t IndexSet::GetIndex(size_t index) const
	{
		return _sortedIndices.at(index);
	}
}
