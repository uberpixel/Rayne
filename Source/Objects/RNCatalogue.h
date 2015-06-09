//
//  RNCatalogue.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CATALOGUE_H__
#define __RAYNE_CATALOGUE_H__

#include "../Base/RNBase.h"

namespace RN
{
	class Object;
	class Serializer;
	class Deserializer;
	
	class MetaClass
	{
	public:
		RNAPI MetaClass *GetSuperClass() const { return _superClass; }
		RNAPI std::string GetName() const { return _name; }
		RNAPI std::string GetFullname() const;
		
		virtual Object *Construct() { throw InconsistencyException("Construct() called but not provided"); }
		virtual Object *ConstructWithDeserializer(Deserializer *deserializer) { throw InconsistencyException("ConstructWithDeserializer() called but not provided"); }
		virtual Object *ConstructWithCopy(Object *) { throw InconsistencyException("ConstructWithCopy() called but not provided");  }
		
		virtual bool SupportsConstruction() const { return false; }
		virtual bool SupportsSerialization() const { return false; }
		virtual bool SupportsCopying() const { return false; }
		
		RNAPI bool InheritsFromClass(const MetaClass *other) const;
	
	protected:
		RNAPI MetaClass() {}
		RNAPI MetaClass(MetaClass *parent, const std::string &name, const char *namespaceBlob);
		RNAPI ~MetaClass();
		
	private:
		MetaClass *_superClass;
		std::string _name;
		std::vector<std::string> _namespace;
	};
	
	template<class T>
	class __MetaClassTraitNull0 : public virtual MetaClass
	{};
	
	template<class T>
	class __MetaClassTraitNull1 : public virtual MetaClass
	{};
	
	template<class T>
	class __MetaClassTraitNull2 : public virtual MetaClass
	{};

	template<class T>
	class MetaClassTraitCronstructable : public virtual MetaClass
	{
	public:
		T *Construct() override
		{
			return new T();
		}
		
		bool SupportsConstruction() const override { return true; }
	};
	
	template<class T>
	class MetaClassTraitSerializable : public virtual MetaClass
	{
	public:
		T *ConstructWithDeserializer(Deserializer *deserializer) override
		{
			return new T(deserializer);
		}
		
		bool SupportsSerialization() const override { return true; }
	};
	
	template<class T>
	class MetaClassTraitCopyable : public virtual MetaClass
	{
	public:
		T *ConstructWithCopy(Object *source) override
		{
			return new T(static_cast<T *>(source));
		}
		
		bool SupportsCopying() const override { return true; }
	};
	
	
	template<class T, class... Traits>
	class __ConcreteMetaClass : public virtual MetaClass, public Traits...
	{};
	
	class Catalogue
	{
	public:
		friend class MetaClass;

		static Catalogue *GetSharedInstance();
		
		RNAPI MetaClass *GetClassWithName(const std::string &name) const;
		RNAPI void EnumerateClasses(const std::function<void (MetaClass *meta, bool &stop)>& enumerator);
		
	private:
		void AddMetaClass(MetaClass *meta);
		void RemoveMetaClass(MetaClass *meta);
		
		static void ParsePrettyFunction(const char *string, std::vector<std::string>& namespaces);
		
		std::unordered_map<std::string, MetaClass *> _metaClasses;
	};
}

#endif
