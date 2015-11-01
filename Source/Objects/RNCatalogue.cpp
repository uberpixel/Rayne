//
//  RNCatalogue.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCatalogue.h"
#include "RNString.h"

namespace RN
{
	MetaClass::MetaClass(MetaClass *parent, const std::string &name, const char *namespaceBlob) :
		_superClass(parent),
		_name(name),
		_module(nullptr)
	{
		Catalogue::ParsePrettyFunction(namespaceBlob, _namespace);
		
		_namespace.pop_back();
		
		if(!parent)
			_namespace.pop_back();
		
		Catalogue::GetSharedInstance()->AddMetaClass(this);
	}
	
	MetaClass::~MetaClass()
	{
		Catalogue::GetSharedInstance()->RemoveMetaClass(this);
	}
	
	bool MetaClass::InheritsFromClass(const MetaClass *other) const
	{
		if(this == other)
			return true;
		
		if(!_superClass)
			return false;
		
		return _superClass->InheritsFromClass(other);
	}
	
	std::string MetaClass::GetFullname() const
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
	
	
	
	static Catalogue *__sharedInstance = nullptr;
	
	Catalogue *Catalogue::GetSharedInstance()
	{
		if(RN_EXPECT_FALSE(!__sharedInstance))
			__sharedInstance = new Catalogue();
		
		return __sharedInstance;
	}
	
	MetaClass *Catalogue::GetClassWithName(const std::string &name) const
	{
		auto iterator = _metaClasses.find(name);
		if(iterator != _metaClasses.end())
			return iterator->second;
		
		return 0;
	}
	
	void Catalogue::EnumerateClasses(const std::function<void (MetaClass *meta, bool &stop)>& enumerator)
	{
		bool stop = false;
		
		for(auto i=_metaClasses.begin(); i!=_metaClasses.end(); i++)
		{
			enumerator(i->second, stop);
			if(stop)
				break;
		}
	}


	void Catalogue::PushModule(Module *module)
	{
		_modules.push_back(module);
	}
	void Catalogue::PopModule()
	{
		_modules.pop_back();
	}

	
	void Catalogue::AddMetaClass(MetaClass *meta)
	{
		auto iterator = _metaClasses.find(meta->GetFullname());
		if(iterator != _metaClasses.end())
			throw InvalidArgumentException(RNSTR("A MetaClass of the same name '" << meta->GetFullname() << "' already exists!"));

		if(!_modules.empty())
			meta->_module = _modules.back();

		_metaClasses.insert(std::unordered_map<std::string, MetaClass *>::value_type(meta->GetFullname(), meta));
	}
	
	void Catalogue::RemoveMetaClass(MetaClass *meta)
	{
		_metaClasses.erase(meta->GetFullname());
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
