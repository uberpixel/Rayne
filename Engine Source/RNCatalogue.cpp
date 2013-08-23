//
//  RNCatalogue.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCatalogue.h"

namespace RN
{
	MetaClassBase::MetaClassBase(MetaClassBase *parent, const std::string& name, const char *namespaceBlob) :
		_name(name),
		_superClass(parent)
	{
		Catalogue::ParsePrettyFunction(namespaceBlob, _namespace);
		
		_namespace.pop_back();
		_namespace.pop_back();
		
		Catalogue::GetSharedInstance()->AddMetaClass(this);
	}
	
	MetaClassBase::~MetaClassBase()
	{
		Catalogue::GetSharedInstance()->RemoveMetaClass(this);
	}
	
	bool MetaClassBase::InheritsFromClass(MetaClassBase *other) const
	{
		if(this == other)
			return true;
		
		if(!_superClass)
			return false;
		
		return _superClass->InheritsFromClass(other);
	}
	
	std::string MetaClassBase::Fullname() const
	{
		std::string name;
		
		for(auto i=_namespace.begin(); i!=_namespace.end(); i++)
		{
			name += *i;
			name += "::";
		}
		
		name += _name;
		return name;
	}
	
	
	
	MetaClassBase *Catalogue::GetClassWithName(const std::string& name) const
	{
		auto iterator = _metaClasses.find(name);
		if(iterator != _metaClasses.end())
			return iterator->second;
		
		return 0;
	}
	
	void Catalogue::EnumerateClasses(const std::function<void (MetaClassBase *meta, bool *stop)>& enumerator)
	{
		bool stop = false;
		
		for(auto i=_metaClasses.begin(); i!=_metaClasses.end(); i++)
		{
			enumerator(i->second, &stop);
			if(stop)
				break;
		}
	}
	
	
	void Catalogue::AddMetaClass(MetaClassBase *meta)
	{
		auto iterator = _metaClasses.find(meta->Fullname());
		if(iterator != _metaClasses.end())
			throw Exception(Exception::Type::InvalidArgumentException, "A MetaClass of the same name already exists!");
		
		_metaClasses.insert(std::unordered_map<std::string, MetaClassBase *>::value_type(meta->Fullname(), meta));
	}
	
	void Catalogue::RemoveMetaClass(MetaClassBase *meta)
	{
		_metaClasses.erase(meta->Fullname());
	}
	
	void Catalogue::ParsePrettyFunction(const char *string, std::vector<std::string>& namespaces)
	{
		const char *namespaceEnd = string;
		const char *namespaceBegin = 0;
		
		const char *signature = strpbrk(string, "(");
		
		while(1)
		{
			const char *temp = strstr(namespaceEnd, "::");
			if(!temp || (signature && temp >= signature))
				break;
			
			namespaceEnd = temp + 2;
		}
		
		namespaceEnd -= 2;
		namespaceBegin = namespaceEnd;
		
		while(namespaceBegin > string)
		{
			if(isalnum(*(namespaceBegin - 1)) || *(namespaceBegin - 1) == ':')
			{
				namespaceBegin --;
				continue;
			}
			
			break;
		}
		
		while(namespaceBegin < namespaceEnd)
		{
			const char *temp = strstr(namespaceBegin, "::");
			namespaces.emplace_back(std::string(namespaceBegin, temp - namespaceBegin));
			
			namespaceBegin = temp + 2;
		}
	}
}
