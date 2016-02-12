//
//  NumberTests.cpp
//  Rayne Unit Tests
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Shared/Bootstrap.h"

class NumberTests : public KernelFixture
{
};

TEST_F(NumberTests, TypeTests)
{
	ASSERT_EQ(RN::Number::Type::Float32, RN::Number::WithFloat(0.0f)->GetType());
	ASSERT_EQ(RN::Number::Type::Float64, RN::Number::WithDouble(0.0f)->GetType());

	ASSERT_EQ(RN::Number::Type::Boolean, RN::Number::WithBool(false)->GetType());

	ASSERT_EQ(RN::Number::Type::Int8, RN::Number::WithInt8(0)->GetType());
	ASSERT_EQ(RN::Number::Type::Int16, RN::Number::WithInt16(0)->GetType());
	ASSERT_EQ(RN::Number::Type::Int32, RN::Number::WithInt32(0)->GetType());
	ASSERT_EQ(RN::Number::Type::Int64, RN::Number::WithInt64(0)->GetType());

	ASSERT_EQ(RN::Number::Type::Uint8, RN::Number::WithUint8(0)->GetType());
	ASSERT_EQ(RN::Number::Type::Uint16, RN::Number::WithUint16(0)->GetType());
	ASSERT_EQ(RN::Number::Type::Uint32, RN::Number::WithUint32(0)->GetType());
	ASSERT_EQ(RN::Number::Type::Uint64, RN::Number::WithUint64(0)->GetType());
}

TEST_F(NumberTests, EqualityTests)
{
	RN::Number *floatNumber1 = RN::Number::WithFloat(32.0f);
	RN::Number *floatNumber2 = RN::Number::WithFloat(64.0f);

	RN::Number *doubleNumber1 = RN::Number::WithDouble(32.0f);
	RN::Number *doubleNumber2 = RN::Number::WithDouble(64.0f);

	ASSERT_FALSE(floatNumber1->IsEqual(floatNumber2));
	ASSERT_TRUE(floatNumber1->IsEqual(doubleNumber1));

	ASSERT_TRUE(doubleNumber1->IsEqual(floatNumber1));
	ASSERT_FALSE(doubleNumber2->IsEqual(floatNumber1));
}
