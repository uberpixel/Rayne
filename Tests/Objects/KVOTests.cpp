//
//  KVOTests.cpp
//  Rayne Unit Tests
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Shared/Bootstrap.h"

#define RNObservable(name) _##name(#name)
#define RNObservableInit(name, value) _##name(#name, value)

using namespace RN::numeric;

class KVOTestObject : public RN::Object
{
public:
	KVOTestObject() :
		_testFloat("testFloat", &KVOTestObject::GetTestFloat, &KVOTestObject::SetTestFloat),
		_testBool("testBool", &KVOTestObject::GetTestBool, &KVOTestObject::SetTestBool),
		_testValue("testValue", &KVOTestObject::GetTestValue, &KVOTestObject::SetTestValue)
	{
		AddObservables({ &_testFloat, &_testBool, &_testValue });
	}

	void SetTestFloat(float value)
	{
		_testFloat.WillChangeValue();
		_testFloat = value;
		_testFloat.DidChangeValue();
	}

	float GetTestFloat() const
	{
		return _testFloat;
	}

	void SetTestBool(bool value)
	{
		_testBool.WillChangeValue();
		_testBool = value;
		_testBool.DidChangeValue();
	}

	bool GetTestBool() const
	{
		return _testBool;
	}


	void SetTestValue(const RN::Vector3 &value)
	{
		_testValue.WillChangeValue();
		_testValue = value;
		_testValue.DidChangeValue();
	}
	const RN::Vector3 &GetTestValue() const
	{
		return _testValue;
	}


private:
	RN::ObservableScalar<float, KVOTestObject> _testFloat;
	RN::ObservableScalar<bool, KVOTestObject> _testBool;
	RN::ObservableValue<RN::Vector3, KVOTestObject> _testValue;

	RNDeclareMeta(KVOTestObject)
};


class KVOTestObjectContainer : public RN::Object
{
public:
	KVOTestObjectContainer() :
		_testObject("testObject", RN::Object::MemoryPolicy::Retain, &KVOTestObjectContainer::GetTestObject, &KVOTestObjectContainer::SetTestObject)
	{
		AddObservables({ &_testObject });
	}

	void SetTestObject(KVOTestObject *object)
	{
		_testObject.WillChangeValue();
		_testObject = object;
		_testObject.DidChangeValue();
	}

	KVOTestObject *GetTestObject() const
	{
		return _testObject;
	}

private:
	RN::ObservableObject<KVOTestObject *, KVOTestObjectContainer> _testObject;

	RNDeclareMeta(KVOTestObjectContainer)
};

RNDefineMeta(KVOTestObject, RN::Object)
RNDefineMeta(KVOTestObjectContainer, RN::Object)


class KVOTests : public KernelFixture
{};

TEST_F(KVOTests, ScalarValues)
{
	KVOTestObject *object = new KVOTestObject();

	object->SetTestFloat(0.0f);

	ASSERT_FLOAT_EQ(0.0f, object->GetTestFloat());

	object->SetValueForKey(RN::Number::WithFloat(42), "testFloat");

	ASSERT_FLOAT_EQ(42.0f, object->GetTestFloat());

	object->SetTestFloat(128.0f);

	ASSERT_FLOAT_EQ(128.0f, object->GetTestFloat());
	ASSERT_FLOAT_EQ(128.0f, object->GetValueForKey<RN::Number>("testFloat")->GetFloatValue());

	object->Release();
}

TEST_F(KVOTests, ObjectValues)
{
	KVOTestObjectContainer *container = new KVOTestObjectContainer();
	KVOTestObject *object = new KVOTestObject();

	container->SetValueForKey(object, "testObject");
	ASSERT_EQ(object, container->GetTestObject());

	container->SetTestObject(nullptr);
	ASSERT_EQ(nullptr, container->GetTestObject());

	container->SetTestObject(object);
	ASSERT_EQ(object, container->GetTestObject());

	container->SetValueForKey(nullptr, "testObject");
	ASSERT_EQ(nullptr, container->GetTestObject());

	object->Release();
	container->Release();
}

TEST_F(KVOTests, KeyPaths)
{
	KVOTestObjectContainer *container = new KVOTestObjectContainer();
	KVOTestObject *object = new KVOTestObject();

	container->SetTestObject(object);
	container->SetValueForKeyPath(RN::Number::WithFloat(64), "testObject.testFloat");

	ASSERT_FLOAT_EQ(64.0f, object->GetTestFloat());

	object->SetTestFloat(512.0f);

	ASSERT_FLOAT_EQ(512.0f, container->GetValueForKeyPath<RN::Number>("testObject.testFloat")->GetFloatValue());

	object->Release();
	container->Release();
}

TEST_F(KVOTests, Observer)
{
	KVOTestObject *object = new KVOTestObject();

	char *_token;
	void *token = &_token;

	float newValues[5];
	float oldValues[5];
	uint32 index = 0;

	object->SetTestFloat(0.0f); // Start with a known value

	object->AddObserver("testFloat", [&](RN::Object *value, const char *key, const RN::Dictionary *changes) {

		RN::Number *oldValue = changes->GetObjectForKey<RN::Number>(kRNObservableOldValueKey);
		RN::Number *newValue = changes->GetObjectForKey<RN::Number>(kRNObservableNewValueKey);

		oldValues[index] = oldValue->GetFloatValue();
		newValues[index] = newValue->GetFloatValue();

		index ++;

	}, token);

	object->SetTestFloat(128);
	object->SetValueForKey(RN::Number::WithFloat(16), "testFloat");
	object->SetTestFloat(1024.0f);

	ASSERT_FLOAT_EQ(128, newValues[0]);
	ASSERT_FLOAT_EQ(16, newValues[1]);
	ASSERT_FLOAT_EQ(1024, newValues[2]);

	ASSERT_FLOAT_EQ(0, oldValues[0]);
	ASSERT_FLOAT_EQ(128, oldValues[1]);
	ASSERT_FLOAT_EQ(16, oldValues[2]);

	// Make sure removing an observer works as well
	uint32 capturedIndex = index;

	object->RemoveObserver("testFloat", token);

	object->SetTestFloat(128.0f);
	object->SetValueForKey(RN::Number::WithFloat(32.0f), "testFloat");

	ASSERT_EQ(capturedIndex, index);

	object->Release();
}

TEST_F(KVOTests, InvalidKeys)
{
	KVOTestObjectContainer *container = new KVOTestObjectContainer();
	KVOTestObject *object = new KVOTestObject();

	ASSERT_THROW(container->SetValueForKey(object, "testObj"), RN::InconsistencyException);

	container->SetTestObject(object);

	ASSERT_THROW(container->SetValueForKeyPath(object, "testObject.test"), RN::InconsistencyException);
	ASSERT_THROW(container->SetValueForKeyPath(object, "test"), RN::InconsistencyException);

	ASSERT_THROW(container->GetValueForKeyPath("test"), RN::InconsistencyException);
	ASSERT_THROW(container->GetValueForKeyPath("testObject.x"), RN::InconsistencyException);

	object->Release();
	container->Release();
}
