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
	RNDefineMeta(Asset)
	
	Asset::Asset()
	{}
	Asset::~Asset()
	{
		_signal.Emit(this);
	}
	
	
	void Asset::Unfault(Deserializer *deserializer)
	{}
	
	
	void Asset::Serialize(Serializer *serializer)
	{
		MetaClassBase *meta = Class();
		
		serializer->EncodeString(meta->Fullname());
		serializer->EncodeString(_name);
	}
	
	Asset *Asset::Deserialize(Deserializer *deserializer)
	{
		std::string meta = deserializer->DecodeString();
		std::string name = deserializer->DecodeString();
		
		MetaClassBase *cmeta = Catalogue::GetSharedInstance()->GetClassWithName(meta);
		Asset *asset;
		
		if(name.empty())
		{
			asset = static_cast<Asset *>(cmeta->Construct());
		}
		else
		{
			asset = ResourceCoordinator::GetSharedInstance()->RequestResourceWithName(cmeta, RNSTR(name.c_str()), nullptr);
		}
		
		asset->Unfault(deserializer);
		return asset;
	}
	
	
	void Asset::SetName(const std::string &name)
	{
		_name = name;
	}
	
	const std::string &Asset::GetName() const
	{
		return _name;
	}
}
