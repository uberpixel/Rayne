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
	
	class MetaClass
	{
	public:
		MetaClass *SuperClass() const { return _superClass; }
		const std::string& Name() const { return _name; }
		RNAPI std::string Fullname() const;
		
		virtual Object *Construct() { throw ErrorException(0); }
		virtual Object *ConstructWithSerializer(Serializer *) { throw ErrorException(0); }
		
		RNAPI bool InheritsFromClass(MetaClass *other) const;
	
	protected:
		RNAPI MetaClass() {}
		RNAPI MetaClass(MetaClass *parent, const std::string& name, const char *namespaceBlob);
		RNAPI ~MetaClass();
		
	private:
		MetaClass *_superClass;
		std::string _name;
		std::vector<std::string> _namespace;
	};
	
	template<class T>
	class MetaClassTraitCreatable : public virtual MetaClass
	{
	public:
		T *Construct() override
		{
			return new T();
		}
	};
	
	template<class T>
	class MetaClassTraitSerializable : public virtual MetaClass
	{
	public:
		T *ConstructWithSerializer(Serializer *serializer) override
		{
			return new T(serializer);
		}
	};
	
	template<class T, template <typename Type> class... Traits>
	class ConcreteMetaClass : public virtual MetaClass, public Traits<T>...
	{};
	
	class Catalogue : public Singleton<Catalogue>
	{
	friend class MetaClass;
	public:
		RNAPI MetaClass *ClassWithName(const std::string& name) const;
		RNAPI void EnumerateClasses(const std::function<void (MetaClass *meta, bool *stop)>& enumerator);
		
	private:
		void AddMetaClass(MetaClass *meta);
		void RemoveMetaClass(MetaClass *meta);
		
		static void ParsePrettyFunction(const char *string, std::vector<std::string>& namespaces);
		
		std::unordered_map<std::string, MetaClass *> _metaClasses;
	};
}

#endif
