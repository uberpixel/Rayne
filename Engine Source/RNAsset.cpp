//
//  RNAsset.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAsset.h"
#include "RNPathManager.h"
#include "RNResourceCoordinator.h"
#include "RNSerialization.h"

namespace RN
{
	RNDefineMeta(Asset, Object)
	
	Asset::Asset() :
		_settings(nullptr)
	{}
	Asset::~Asset()
	{
		_signal.Emit(this);
		SafeRelease(_settings);
	}
	
	
	void Asset::Unfault(Deserializer *deserializer)
	{
		_dirty = deserializer->DecodeBool();
	}
	
	
	void Asset::Serialize(Serializer *serializer)
	{
		MetaClassBase *meta = Class();
		
		serializer->EncodeString(meta->Fullname());
		serializer->EncodeString(_name);
		serializer->EncodeObject(_settings);
		
		serializer->EncodeBool(_dirty);
		serializer->EncodeBool(_dirty);
	}
	
	Asset *Asset::Deserialize(Deserializer *deserializer)
	{
		std::string meta = deserializer->DecodeString();
		std::string name = deserializer->DecodeString();
		
		Dictionary *settings = static_cast<Dictionary *>(deserializer->DecodeObject());
		bool dirty = deserializer->DecodeBool();
		
		MetaClassBase *cmeta = Catalogue::GetSharedInstance()->GetClassWithName(meta);
		Asset *asset;
		
		if(name.empty() || dirty)
		{
			asset = static_cast<Asset *>(cmeta->Construct());
		}
		else
		{
			asset = ResourceCoordinator::GetSharedInstance()->RequestResourceWithName(cmeta, RNSTR(name.c_str()), settings);
		}
		
		asset->Unfault(deserializer);
		return asset;
	}
	
	
	void Asset::MarkChanged()
	{
		_dirty = true;
	}
	
	void Asset::MarkUnchanged()
	{
		_dirty = false;
	}
	
	
	void Asset::WakeUpFromResourceCoordinator(const std::string &name, Dictionary *settings)
	{
		_dirty = false;
		_name  = name;
		
		SafeRelease(_settings);
		
		if(settings)
			settings = settings->Copy();
	}
	
	const std::string &Asset::GetName() const
	{
		return _name;
	}
}
