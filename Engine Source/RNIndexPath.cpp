//
//  RNIndexPath.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNIndexPath.h"

namespace RN
{
	RNDeclareMeta(IndexPath)
	
	IndexPath::IndexPath()
	{}
	
	void IndexPath::AddIndex(size_t index)
	{
		_indices.push_back(index);
	}
	
	size_t IndexPath::GetLength() const
	{
		return _indices.size();
	}
	
	size_t IndexPath::GetIndexAtPosition(size_t position) const
	{
		return _indices.at(position);
	}
}
