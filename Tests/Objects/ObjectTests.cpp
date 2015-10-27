//
//  ObjectTests.cpp
//  Rayne Unit Tests
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "__Bootstrap.h"

struct TestObjectResults
{
	TestObjectResults() :
		isDeallocated(false)
	{}

	bool hasReachedWillDealloc;
	bool isDeallocated;
};

class TestObject : public RN::Object
{
public:
	TestObject(TestObjectResults &results) :
		_results(results)
	{}

	~TestObject() override
	{
		_results.isDeallocated = true;
	}

	void Dealloc() override
	{
		_results.hasReachedWillDealloc = true;
	}

private:
	TestObjectResults &_results;

	RNDeclareMeta(TestObject)
};

RNDefineMeta(TestObject, RN::Object)

class TestObjectDes : public TestObject
{
public:
	TestObjectDes(TestObjectResults &results) :
		TestObject(results)
	{}

	RNDeclareMeta(TestObjectDes)
};

RNDefineMeta(TestObjectDes, TestObject)

// ---------------------
// MARK: -
// MARK: LifeCycleTest
// ---------------------

class ObjectLifeCycleTests : public KernelFixture
{};

TEST_F(ObjectLifeCycleTests, AllocationDeallocation)
{
	TestObjectResults results;

	TestObject *object = new TestObject(results);
	ASSERT_TRUE(object);

	object->Retain();

	EXPECT_FALSE(results.isDeallocated);
	EXPECT_FALSE(results.hasReachedWillDealloc);

	object->Release();

	EXPECT_FALSE(results.isDeallocated);
	EXPECT_FALSE(results.hasReachedWillDealloc);

	object->Release();

	EXPECT_TRUE(results.isDeallocated);
	EXPECT_TRUE(results.hasReachedWillDealloc);
}

TEST_F(ObjectLifeCycleTests, Autorelease)
{
	TestObjectResults results;

	TestObject *object = new TestObject(results);
	ASSERT_TRUE(object);

	{
		RN::AutoreleasePool pool;

		object->Autorelease();

		EXPECT_FALSE(results.isDeallocated);
		EXPECT_FALSE(results.hasReachedWillDealloc);
	}

	EXPECT_TRUE(results.isDeallocated);
	EXPECT_TRUE(results.hasReachedWillDealloc);
}

// ---------------------
// MARK: -
// MARK: Equality Tests
// ---------------------

class ObjectEqualityTests : public KernelFixture
{};

TEST_F(ObjectEqualityTests, Equality)
{
	TestObjectResults results;

	TestObject *object = new TestObject(results);
	ASSERT_TRUE(object);

	EXPECT_FALSE(object->IsEqual(nullptr));
	EXPECT_TRUE(object->IsEqual(object));

	TestObject *other = new TestObject(results);
	ASSERT_TRUE(object);

	EXPECT_FALSE(object->IsEqual(other));

	object->Release();
	other->Release();
}

TEST_F(ObjectEqualityTests, Hash)
{
	TestObjectResults results;

	TestObject *object = new TestObject(results);
	ASSERT_TRUE(object);

	EXPECT_EQ(object->GetHash(), object->GetHash()); // Subsequent calls to get hash must return the same value

	TestObject *other = new TestObject(results);
	ASSERT_TRUE(object);

	EXPECT_NE(object->GetHash(), other->GetHash());

	object->Release();
	other->Release();
}

TEST_F(ObjectEqualityTests, Class)
{
	TestObjectResults results;

	TestObject *object = new TestObject(results);
	ASSERT_TRUE(object);

	EXPECT_TRUE(object->IsKindOfClass(RN::Object::GetMetaClass()));
	EXPECT_TRUE(object->IsKindOfClass(TestObject::GetMetaClass()));

	EXPECT_FALSE(object->IsKindOfClass(RN::String::GetMetaClass()));
	EXPECT_FALSE(object->IsKindOfClass(TestObjectDes::GetMetaClass()));

	object->Release();
}
