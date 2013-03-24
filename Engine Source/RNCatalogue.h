//
//  RNCatalogue.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CATALOGUE_H__
#define __RAYNE_CATALOGUE_H__

#include "RNBase.h"

namespace RN
{
	class Object;
	class MetaClass
	{
	public:
		MetaClass *SuperClass() const { return _superClass; }
		const std::string& Name() const { return _name; }
		
		virtual Object *Construct() = 0;
		bool InheritsFromClass(MetaClass *other) const;
	
	protected:
		MetaClass(MetaClass *parent, const std::string& name);
		~MetaClass();
		
	private:
		MetaClass *_superClass;
		std::string _name;
	};
	
	class Catalogue : public Singleton<Catalogue>
	{
	friend class MetaClass;
	public:
		MetaClass *ClassWithName(const std::string& name) const;
		
	private:
		void AddMetaClass(MetaClass *meta);
		void RemoveMetaClass(MetaClass *meta);
		
		std::unordered_map<std::string, MetaClass *> _metaClasses;
	};
}

#endif
