//
//  RNExtensionPoint.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EXTENSIONPOINT_H_
#define __RAYNE_EXTENSIONPOINT_H_

#include "../Base/RNBase.h"
#include "../Debug/RNLogger.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNString.h"

namespace RN
{
	class __ExtensionPointBase
	{
	public:
		friend class Kernel;

		~__ExtensionPointBase()
		{
			RemoveExtensionPoint(this);
		}

		const MetaClass *GetMetaClass() const { return _meta; }
		const std::string &GetName() const { return _name; }

	protected:
		struct Descriptor
		{
			friend class __ExtensionPointBase;

			Descriptor(Object *object, int32 tpriority) :
				priority(tpriority),
				_object(SafeRetain(object))
			{}

			Descriptor(const Descriptor &source) :
				priority(source.priority),
				_object(SafeRetain(source._object))
			{}
			Descriptor &operator = (const Descriptor &source)
			{
				SafeRelease(_object);

				_object = SafeRetain(source._object);
				priority = source.priority;

				return *this;
			}

			~Descriptor()
			{
				SafeRelease(_object);
			}

			int32 priority;

		private:
			void SetObject(Object *object)
			{
				SafeRelease(_object);
				_object = SafeRetain(object);
			}

			Object *_object;
		};

		__ExtensionPointBase(const std::string &name, MetaClass *meta) :
			_name(name),
			_meta(meta)
		{
			InstallExtensionPoint(this);
		}

		Array *AllObjects() const
		{
			Array *array = new Array(_descriptors.size());

			for(auto iterator : _descriptors)
				array->AddObject(iterator._object);

			return array->Autorelease();
		}

		void AddSortDescriptor(Descriptor descriptor)
		{
			_descriptors.push_back(descriptor);
			Sort();
		}

		void Sort()
		{
			std::stable_sort(_descriptors.begin(), _descriptors.end(), [](const Descriptor &descriptorA, const Descriptor &descriptorB) -> bool {
				return (descriptorA.priority < descriptorB.priority);
			});
		}

		RNAPI static __ExtensionPointBase *GetExtensionPoint(const std::string &name);

	private:
		RNAPI static void InstallExtensionPoint(__ExtensionPointBase *point);
		RNAPI static void RemoveExtensionPoint(__ExtensionPointBase *point);

		RNAPI static void InitializeExtensionPoints();
		RNAPI static void TeardownExtensionPoints();

		MetaClass *_meta;
		std::string _name;
		std::vector<Descriptor> _descriptors;
	};

	template<class T>
	class ExtensionPoint : public __ExtensionPointBase
	{
	public:
		static_assert(std::is_base_of<Object, T>::value, "T must inherit from Object!");

		ExtensionPoint(const std::string &name) :
			__ExtensionPointBase(name, T::GetMetaClass())
		{}

		void AddExtension(T *extension, int32 priority)
		{
			RN_ASSERT(extension, "Extension mustn't be NULL");
			RN_ASSERT(extension->IsKindOfClass(T::GetMetaClass()), "AddExtension() called with an invalid class");

			AddSortDescriptor(Descriptor(extension, priority));
		}

		Array *GetExtensions() const { return AllObjects(); }

		static ExtensionPoint<T> *GetExtensionPoint(const std::string &name)
		{
			__ExtensionPointBase *base = __ExtensionPointBase::GetExtensionPoint(name);
			if(!base)
				return nullptr;

			if(base->GetMetaClass() == T::GetMetaClass())
			{
				RNWarning("Requested ExtensionPoint<" << T::GetMetaClass() << ">(" << name << "), but it's actually ExtensionPoint<" << base->GetMetaClass() << ">");
				return nullptr;
			}

			return static_cast<ExtensionPoint<T> *>(base);
		}
	};
}


#endif /* __RAYNE_EXTENSIONPOINT_H_ */
