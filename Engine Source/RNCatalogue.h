//
//  RNCatalogue.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CATALOGUE_H__
#define __RAYNE_CATALOGUE_H__

#include "RNBase.h"

namespace RN
{
	class Object;
	class Serializer;
	
	class MetaClassBase
	{
	public:
		MetaClassBase *SuperClass() const { return _superClass; }
		const std::string& Name() const { return _name; }
		RNAPI std::string Fullname() const;
		
		virtual Object *Construct() { throw Exception(Exception::Type::GenericException, ""); }
		virtual Object *ConstructWithSerializer(Serializer *) { throw Exception(Exception::Type::GenericException, ""); }
		
		virtual bool SupportsConstruction() const { return false; }
		virtual bool SupportsSerialization() const { return false; }
		
		RNAPI bool InheritsFromClass(MetaClassBase *other) const;
	
	protected:
		RNAPI MetaClassBase() {}
		RNAPI MetaClassBase(MetaClassBase *parent, const std::string& name, const char *namespaceBlob);
		RNAPI ~MetaClassBase();
		
	private:
		MetaClassBase *_superClass;
		std::string _name;
		std::vector<std::string> _namespace;
	};
	
	template<class T>
	class MetaClassTraitCronstructable : public virtual MetaClassBase
	{
	public:
		T *Construct() override
		{
			return new T();
		}
		
		bool SupportsConstruction() const override { return true; }
	};
	
	template<class T>
	class MetaClassTraitSerializable : public virtual MetaClassBase
	{
	public:
		T *ConstructWithSerializer(Serializer *serializer) override
		{
			return new T(serializer);
		}
		
		bool SupportsSerialization() const override { return true; }
	};
	
	template<class T, template <typename Type> class... Traits>
	class ConcreteMetaClass : public virtual MetaClassBase, public Traits<T>...
	{};
	
	class Catalogue : public Singleton<Catalogue>
	{
	public:
		friend class MetaClassBase;
		
		RNAPI MetaClassBase *ClassWithName(const std::string& name) const;
		RNAPI void EnumerateClasses(const std::function<void (MetaClassBase *meta, bool *stop)>& enumerator);
		
	private:
		void AddMetaClass(MetaClassBase *meta);
		void RemoveMetaClass(MetaClassBase *meta);
		
		static void ParsePrettyFunction(const char *string, std::vector<std::string>& namespaces);
		
		std::unordered_map<std::string, MetaClassBase *> _metaClasses;
	};
}

#endif
