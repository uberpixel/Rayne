//
//  RNCatalogue.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCatalogue.h"

namespace RN
{
	MetaClass::MetaClass(MetaClass *parent, const std::string& name) :
		_name(name)
	{
		_superClass = parent;
		Catalogue::SharedInstance()->AddMetaClass(this);
	}
	
	MetaClass::~MetaClass()
	{
		Catalogue::SharedInstance()->RemoveMetaClass(this);
	}
	
	bool MetaClass::InheritsFromClass(MetaClass *other) const
	{
		if(this == other)
			return true;
		
		if(!_superClass)
			return false;
		
		return _superClass->InheritsFromClass(other);
	}
	
	
	MetaClass *Catalogue::ClassWithName(const std::string& name) const
	{
		auto iterator = _metaClasses.find(name);
		return (iterator != _metaClasses.end()) ? iterator->second : 0;
	}
	
	void Catalogue::AddMetaClass(MetaClass *meta)
	{
		auto iterator = _metaClasses.find(meta->Name());
		if(iterator != _metaClasses.end())
			throw ErrorException(0, 0, 0);
		
		_metaClasses[meta->Name()] = meta;
	}
	
	void Catalogue::RemoveMetaClass(MetaClass *meta)
	{
		_metaClasses.erase(meta->Name());
	}
}
