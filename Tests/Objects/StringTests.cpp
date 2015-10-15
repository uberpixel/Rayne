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

TEST_F(StringComparisonTests, RangeOfString)
{
	RN::String *test1 = RNCSTR("World x World");
	ASSERT_TRUE(test1);

	EXPECT_NE(kRNNotFound, test1->GetRangeOfString(RNCSTR("World")).origin);
	EXPECT_EQ(RN::Range(0, 5), test1->GetRangeOfString(RNCSTR("World")));
	EXPECT_EQ(RN::Range(6, 1), test1->GetRangeOfString(RNCSTR("x")));

	// Revers
	EXPECT_NE(RN::Range(0, 5), test1->GetRangeOfString(RNCSTR("World"), RN::String::ComparisonMode::Reverse));
	EXPECT_EQ(RN::Range(8, 5), test1->GetRangeOfString(RNCSTR("World"), RN::String::ComparisonMode::Reverse));

	// Case sensitivity
	EXPECT_EQ(kRNNotFound, test1->GetRangeOfString(RNCSTR("world")).origin);
	EXPECT_EQ(0, test1->GetRangeOfString(RNCSTR("woRld"), RN::String::ComparisonMode::CaseInsensitive).origin);
	EXPECT_EQ(8, test1->GetRangeOfString(RNCSTR("woRld"), RN::String::ComparisonMode::CaseInsensitive | RN::String::ComparisonMode::Reverse).origin);

	// Sub-Ranges
	RN::String *test2 = RNCSTR("SubSubSubSub");
	ASSERT_TRUE(test2);

	EXPECT_NE(kRNNotFound, test2->GetRangeOfString(RNCSTR("Sub")).origin);
	EXPECT_EQ(RN::Range(0, 3), test2->GetRangeOfString(RNCSTR("Sub")));
	EXPECT_EQ(RN::Range(3, 3), test2->GetRangeOfString(RNCSTR("Sub"), 0, RN::Range(3, 5)));
	EXPECT_EQ(RN::Range(3, 3), test2->GetRangeOfString(RNCSTR("Sub"), RN::String::ComparisonMode::Reverse, RN::Range(3, 5)));
	EXPECT_EQ(RN::Range(6, 3), test2->GetRangeOfString(RNCSTR("Sub"), RN::String::ComparisonMode::Reverse, RN::Range(3, 6)));
}

TEST_F(StringComparisonTests, Compare)
{
	RN::String *test1 = RNCSTR("World x World");
	ASSERT_TRUE(test1);

	EXPECT_EQ(RN::ComparisonResult::GreaterThan, test1->Compare(RNCSTR("World")));
	EXPECT_EQ(RN::ComparisonResult::GreaterThan, test1->Compare(RNCSTR("Hello")));
	EXPECT_EQ(RN::ComparisonResult::LessThan, test1->Compare(RNCSTR("X")));
	EXPECT_EQ(RN::ComparisonResult::EqualTo, test1->Compare(RNCSTR("X"), RN::String::ComparisonMode::CaseInsensitive, RN::Range(6, 1)));
	EXPECT_EQ(RN::ComparisonResult::GreaterThan, test1->Compare(RNCSTR("X"), 0, RN::Range(6, 1)));
	EXPECT_EQ(RN::ComparisonResult::EqualTo, test1->Compare(RNCSTR("World x World")));
}

TEST_F(StringComparisonTests, CompareNumerical)
{
	RN::String *test1 = RNCSTR("1234");
	RN::String *test2 = RNCSTR("999");
	RN::String *test3 = RNCSTR("World 467");
	RN::String *test4 = RNCSTR("World 467 World");
	ASSERT_TRUE(test1);
	ASSERT_TRUE(test2);
	ASSERT_TRUE(test3);
	ASSERT_TRUE(test4);

	EXPECT_EQ(RN::ComparisonResult::LessThan, test1->Compare(RNCSTR("1235"), RN::String::ComparisonMode::Numerically));
	EXPECT_EQ(RN::ComparisonResult::LessThan, test2->Compare(RNCSTR("1000"), RN::String::ComparisonMode::Numerically));
	EXPECT_EQ(RN::ComparisonResult::GreaterThan, test2->Compare(RNCSTR("980"), RN::String::ComparisonMode::Numerically));

	EXPECT_EQ(RN::ComparisonResult::GreaterThan, test3->Compare(RNCSTR("467"), RN::String::ComparisonMode::Numerically));
	EXPECT_EQ(RN::ComparisonResult::GreaterThan, test3->Compare(RNCSTR("World 466"), RN::String::ComparisonMode::Numerically));
	EXPECT_EQ(RN::ComparisonResult::LessThan, test3->Compare(RNCSTR("World 468"), RN::String::ComparisonMode::Numerically));
	EXPECT_EQ(RN::ComparisonResult::EqualTo, test3->Compare(RNCSTR("World 467"), RN::String::ComparisonMode::Numerically));

	EXPECT_EQ(RN::ComparisonResult::GreaterThan, test4->Compare(RNCSTR("467 World"), RN::String::ComparisonMode::Numerically));
	EXPECT_EQ(RN::ComparisonResult::GreaterThan, test4->Compare(RNCSTR("World 466 World"), RN::String::ComparisonMode::Numerically));
	EXPECT_EQ(RN::ComparisonResult::LessThan, test4->Compare(RNCSTR("World 468 World"), RN::String::ComparisonMode::Numerically));
	EXPECT_EQ(RN::ComparisonResult::EqualTo, test4->Compare(RNCSTR("World 467 World"), RN::String::ComparisonMode::Numerically));
}

TEST_F(StringComparisonTests, Prefix)
{
	RN::String *test1 = RNCSTR("Hello World --");
	ASSERT_TRUE(test1);

	EXPECT_TRUE(test1->HasPrefix(RNCSTR("H")));
	EXPECT_TRUE(test1->HasPrefix(RNCSTR("Hello")));
	EXPECT_FALSE(test1->HasPrefix(RNCSTR("ello")));
	EXPECT_FALSE(test1->HasPrefix(RNCSTR("x")));
}

TEST_F(StringComparisonTests, Suffix)
{
	RN::String *test1 = RNCSTR("Hello World --");
	ASSERT_TRUE(test1);

	EXPECT_TRUE(test1->HasSuffix(RNCSTR("d --")));
	EXPECT_TRUE(test1->HasSuffix(RNCSTR("World --")));
	EXPECT_FALSE(test1->HasSuffix(RNCSTR("ld ---")));
	EXPECT_FALSE(test1->HasSuffix(RNCSTR("x")));
}


