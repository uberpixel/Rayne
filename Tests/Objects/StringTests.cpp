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
// MARK: Mutation
// ---------------------

class StringMutationTests : public KernelFixture
{};

TEST_F(StringMutationTests, Append)
{
	RN::String *test1 = RNSTR("Hello");
	ASSERT_TRUE(test1);

	EXPECT_STREQ("Hello", test1->GetUTF8String());

	test1->Append(RNCSTR(" World"));
	EXPECT_STREQ("Hello World", test1->GetUTF8String());

	test1->Append(" Test1");
	EXPECT_STREQ("Hello World Test1", test1->GetUTF8String());

	test1->Append(RNSTR(" Test2"));
	EXPECT_STREQ("Hello World Test1 Test2", test1->GetUTF8String());
}

TEST_F(StringMutationTests, Insert)
{
	RN::String *test1 = RNSTR("Hello x World");
	ASSERT_TRUE(test1);

	test1->Insert(RNCSTR("X"), 6);
	test1->Insert(RNCSTR("X"), 8);

	EXPECT_STREQ("Hello XxX World", test1->GetUTF8String());

	test1->Insert(RNCSTR("Foo "), 0);
	EXPECT_STREQ("Foo Hello XxX World", test1->GetUTF8String());

	test1->Insert(RNCSTR(" Foo"), test1->GetLength());
	EXPECT_STREQ("Foo Hello XxX World Foo", test1->GetUTF8String());
}

TEST_F(StringMutationTests, DeleteCharacters)
{
	RN::String *test1 = RNSTR("Hello x World");
	ASSERT_TRUE(test1);

	EXPECT_STREQ("Hello x World", test1->GetUTF8String());

	test1->DeleteCharacters(RN::Range(6, 2));
	EXPECT_STREQ("Hello World", test1->GetUTF8String());

	test1->DeleteCharacters(RN::Range(0, 2));
	EXPECT_STREQ("llo World", test1->GetUTF8String());

	test1->DeleteCharacters(RN::Range(test1->GetLength() - 3, 3));
	EXPECT_STREQ("llo Wo", test1->GetUTF8String());
}

TEST_F(StringMutationTests, ReplaceCharacters)
{
	RN::String *test1 = RNSTR("Hello x World");
	ASSERT_TRUE(test1);

	test1->ReplaceCharacters(RNCSTR("Lovely"), RN::Range(6, 1));
	EXPECT_STREQ("Hello Lovely World", test1->GetUTF8String());

	test1->ReplaceCharacters(RNCSTR("Hi"), RN::Range(0, 5));
	EXPECT_STREQ("Hi Lovely World", test1->GetUTF8String());

	test1->ReplaceCharacters(RNCSTR("Trouble"), RN::Range(10, 5));
	EXPECT_STREQ("Hi Lovely Trouble", test1->GetUTF8String());
}

// ---------------------
// MARK: -
// MARK: Comparison
// ---------------------

class StringComparisonTests : public KernelFixture
{};

TEST_F(StringComparisonTests, Equality)
{
	EXPECT_TRUE(RNCSTR("Hello")->IsEqual(RNSTR("Hello")));
	EXPECT_TRUE(RNCSTR("World")->IsEqual(RNSTR("World")));

	RN::String *test1 = RNCSTR("Hello");
	ASSERT_TRUE(test1);

	test1->Append(RNCSTR(" World"));

	EXPECT_TRUE(test1->IsEqual(RNCSTR("Hello World")));
	EXPECT_FALSE(test1->IsEqual(RNCSTR("Hello")));

	EXPECT_FALSE(test1->IsEqual(RN::Number::WithInt32(0x32)));
}

TEST_F(StringComparisonTests, Hash)
{
	RN::String *test1 = RNSTR("Hello");
	ASSERT_TRUE(test1);

	EXPECT_EQ(RNCSTR("Hello")->GetHash(), test1->GetHash());

	test1->Append(" World");
	EXPECT_EQ(RNCSTR("Hello World")->GetHash(), test1->GetHash());

	test1->Append(" Test1");
	EXPECT_EQ(RNCSTR("Hello World Test1")->GetHash(), test1->GetHash());
}

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

	RN::String *test2 = RNCSTR("World");
	ASSERT_TRUE(test2);

	EXPECT_EQ(RN::ComparisonResult::LessThan, test2->Compare(RNCSTR("World x World")));
	EXPECT_EQ(RN::ComparisonResult::GreaterThan, test2->Compare(RNCSTR("Hello x World")));
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

TEST_F(StringComparisonTests, ComponentsSeparatedByString)
{
	RN::String *test1 = RNCSTR("Test, Bar, Foo, X");
	ASSERT_TRUE(test1);

	{
		RN::Array *components = test1->GetComponentsSeparatedByString(RNCSTR(", "));

		ASSERT_EQ(4, components->GetCount());

		EXPECT_TRUE(components->GetObjectAtIndex<RN::String>(0)->IsEqual(RNCSTR("Test")));
		EXPECT_TRUE(components->GetObjectAtIndex<RN::String>(1)->IsEqual(RNCSTR("Bar")));
		EXPECT_TRUE(components->GetObjectAtIndex<RN::String>(2)->IsEqual(RNCSTR("Foo")));
		EXPECT_TRUE(components->GetObjectAtIndex<RN::String>(3)->IsEqual(RNCSTR("X")));
	}

	RN::String *test2 = RNCSTR("Test Bar");
	ASSERT_TRUE(test2);

	{
		RN::Array *components = test2->GetComponentsSeparatedByString(RNCSTR(", "));
		EXPECT_EQ(1, components->GetCount());
	}
}

// ---------------------
// MARK: -
// MARK: Paths
// ---------------------

class StringPathTests : public KernelFixture
{};

TEST_F(StringPathTests, DeletePathExtension)
{
	RN::String *string;

	ASSERT_TRUE((string = RNCSTR("/usr/home/test.txt")));
	string->DeletePathExtension();
	EXPECT_STREQ("/usr/home/test", string->GetUTF8String());

	ASSERT_TRUE((string = RNCSTR("/usr/home/test.")));
	string->DeletePathExtension();
	EXPECT_STREQ("/usr/home/test", string->GetUTF8String());

	ASSERT_TRUE((string = RNCSTR("/usr/home/test")));
	string->DeletePathExtension();
	EXPECT_STREQ("/usr/home/test", string->GetUTF8String());
}

TEST_F(StringPathTests, StringByDeletingPathExtension)
{
	RN::String *string;

	ASSERT_TRUE((string = RNSTR("/usr/home/test.txt")));

	// StringByDeletingPathExtension() uses DeletePathExtension(), so just make sure the original string
	// says the same

	EXPECT_STREQ("/usr/home/test", string->StringByDeletingPathExtension()->GetUTF8String());
	EXPECT_STREQ("/usr/home/test.txt", string->GetUTF8String());
}

TEST_F(StringPathTests, DeletePathComponent)
{
	RN::String *string;

	ASSERT_TRUE((string = RNCSTR("/usr/home/test.txt")));
	string->DeleteLastPathComponent();
	EXPECT_STREQ("/usr/home", string->GetUTF8String());

	string->DeleteLastPathComponent();
	EXPECT_STREQ("/usr", string->GetUTF8String());

	string->DeleteLastPathComponent();
	EXPECT_STREQ("/", string->GetUTF8String());

	string->DeleteLastPathComponent();
	EXPECT_STREQ("/", string->GetUTF8String());

	ASSERT_TRUE((string = RNCSTR("/usr/home/test.")));
	string->DeleteLastPathComponent();
	EXPECT_STREQ("/usr/home", string->GetUTF8String());

	ASSERT_TRUE((string = RNCSTR("/usr/home/test")));
	string->DeleteLastPathComponent();
	EXPECT_STREQ("/usr/home", string->GetUTF8String());
}

TEST_F(StringPathTests, AppendPathComponent)
{
	RN::String *string;

	ASSERT_TRUE((string = RNCSTR("/usr/home/test.txt")));
	string->AppendPathComponent(RNCSTR("Bar"));
	EXPECT_STREQ("/usr/home/test.txt/Bar", string->GetUTF8String());

	string->AppendPathComponent(RNCSTR("/Foo"));
	EXPECT_STREQ("/usr/home/test.txt/Bar/Foo", string->GetUTF8String());
}

TEST_F(StringPathTests, AppendPathExtension)
{
	RN::String *string;

	ASSERT_TRUE((string = RNCSTR("/usr/home/test")));
	string->AppendPathExtension(RNCSTR("Bar"));
	EXPECT_STREQ("/usr/home/test.Bar", string->GetUTF8String());

	string->AppendPathExtension(RNCSTR("Foo"));
	EXPECT_STREQ("/usr/home/test.Bar.Foo", string->GetUTF8String());
}

TEST_F(StringPathTests, GetPathComponents)
{
	RN::String *string;
	RN::Array *components;

	ASSERT_TRUE((string = RNCSTR("/usr/home/test")));

	components = string->GetPathComponents();
	ASSERT_EQ(components->GetCount(), 3);

	EXPECT_STREQ("usr", components->GetObjectAtIndex<RN::String>(0)->GetUTF8String());
	EXPECT_STREQ("home", components->GetObjectAtIndex<RN::String>(1)->GetUTF8String());
	EXPECT_STREQ("test", components->GetObjectAtIndex<RN::String>(2)->GetUTF8String());

	ASSERT_TRUE((string = RNCSTR("/usr/home.txt")));

	components = string->GetPathComponents();
	ASSERT_EQ(components->GetCount(), 2);

	EXPECT_STREQ("usr", components->GetObjectAtIndex<RN::String>(0)->GetUTF8String());
	EXPECT_STREQ("home.txt", components->GetObjectAtIndex<RN::String>(1)->GetUTF8String());

	ASSERT_TRUE((string = RNCSTR("usr")));

	components = string->GetPathComponents();
	ASSERT_EQ(components->GetCount(), 1);

	EXPECT_STREQ("usr", components->GetObjectAtIndex<RN::String>(0)->GetUTF8String());
}

TEST_F(StringPathTests, GetPathExtension)
{
	RN::String *string;

	ASSERT_TRUE((string = RNCSTR("/usr/home/test")));
	ASSERT_FALSE(string->GetPathExtension());

	ASSERT_TRUE((string = RNCSTR("/usr/home/test.txt")));
	ASSERT_TRUE(string->GetPathExtension());
	EXPECT_STREQ("txt", string->GetPathExtension()->GetUTF8String());

	ASSERT_TRUE((string = RNCSTR("/usr/home/test.")));
	ASSERT_TRUE(string->GetPathExtension());
	EXPECT_STREQ("", string->GetPathExtension()->GetUTF8String());
}

TEST_F(StringPathTests, GetLastPathComponent)
{
	RN::String *string;

	ASSERT_TRUE((string = RNCSTR("/usr/home/test")));
	EXPECT_STREQ("test", string->GetLastPathComponent()->GetUTF8String());

	ASSERT_TRUE((string = RNCSTR("/usr/home/test.txt")));
	EXPECT_STREQ("test.txt", string->GetLastPathComponent()->GetUTF8String());

	ASSERT_TRUE((string = RNCSTR("/usr/home/")));
	EXPECT_STREQ("home", string->GetLastPathComponent()->GetUTF8String());
}
