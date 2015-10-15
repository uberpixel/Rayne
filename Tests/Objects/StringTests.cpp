//
//  StringTests.cpp
//  Rayne Unit Tests
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "__Bootstrap.h"


// ---------------------
// MARK: -
// MARK: Comparison
// ---------------------

class StringComparisonTests : public KernelFixture
{};

TEST_F(StringComparisonTests, Prefix)
{
	RN::String *test1 = RNCSTR("Hello World --");
	ASSERT_TRUE(test1);

	ASSERT_TRUE(test1->HasPrefix(RNCSTR("H")));
	ASSERT_TRUE(test1->HasPrefix(RNCSTR("Hello")));
	ASSERT_FALSE(test1->HasPrefix(RNCSTR("ello")));
	ASSERT_FALSE(test1->HasPrefix(RNCSTR("x")));
}

TEST_F(StringComparisonTests, Suffix)
{
	RN::String *test1 = RNCSTR("Hello World --");
	ASSERT_TRUE(test1);

	ASSERT_TRUE(test1->HasSuffix(RNCSTR("d --")));
	ASSERT_TRUE(test1->HasSuffix(RNCSTR("World --")));
	ASSERT_FALSE(test1->HasSuffix(RNCSTR("ld ---")));
	ASSERT_FALSE(test1->HasSuffix(RNCSTR("x")));
}


