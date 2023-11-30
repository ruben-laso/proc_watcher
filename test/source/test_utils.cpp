#include <set>

#include <gtest/gtest.h>

#include "utils.hpp"

TEST(utils, EquivalentSets)
{
	std::set<int> a{ 1, 2, 3, 4, 5 };
	std::set<int> b{ 5, 4, 3, 2, 1 };

	EXPECT_TRUE(utils::equivalent_sets(a, b));
}

TEST(utils, NonEquivalentSets)
{
	std::set<int> a{ 1, 2, 3, 4, 5 };
	std::set<int> b{ 5, 4, 3, 2, 1, 6 };

	EXPECT_FALSE(utils::equivalent_sets(a, b));
}

TEST(utils, EquivalentEmptySets)
{
	std::set<int> a{};
	std::set<int> b{};

	EXPECT_TRUE(utils::equivalent_sets(a, b));
}

TEST(utils, NonEquivalentEmptySets)
{
	std::set<int> a{};
	std::set<int> b{ 1 };

	EXPECT_FALSE(utils::equivalent_sets(a, b));
}

TEST(utils, EquivalentSetsWithDifferentTypes)
{
	std::set<int>    a{ 1, 2, 3, 4, 5 };
	std::set<double> b{ 5, 4, 3, 2, 1 };

	EXPECT_TRUE(utils::equivalent_sets(a, b));
}

TEST(utils, NonEquivalentSetsWithDifferentTypes)
{
	std::set<int>    a{ 1, 2, 3, 4, 5 };
	std::set<double> b{ 5, 4, 3, 2, 1, 6 };

	EXPECT_FALSE(utils::equivalent_sets(a, b));
}

TEST(utils, EquivalentSetsWithDifferentTypesAndDifferentValues)
{
	std::set<int>      a{ 1, 2, 3, 4, 5 };
	std::map<int, int> b{ { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 } };

	EXPECT_TRUE(utils::equivalent_sets(a, b | ranges::views::keys));
}


TEST(utils, EquivalentSetsWithDifferentContainersSameValues)
{
	std::set<int>    a{ 1, 2, 3, 4, 5 };
	std::vector<int> b{ 1, 2, 3, 4, 5 };

	EXPECT_TRUE(utils::equivalent_rngs(a, b));
}

TEST(utils, EquivalentSetsWithDifferentContainersSameValuesUnsorted)
{
	std::set<int>    a{ 1, 2, 3, 4, 5 };
	std::vector<int> b{ 5, 4, 3, 2, 1 };

	EXPECT_TRUE(utils::equivalent_rngs(a, b));
}

TEST(utils, NonEquivalentSetsWithDifferentContainersDifferentSizes)
{
	std::set<int>    a{ 1, 2, 3, 4, 5 };
	std::vector<int> b{ 5, 4, 3, 2, 1, 6 };

	EXPECT_FALSE(utils::equivalent_rngs(a, b));
}

TEST(utils, NonEquivalentSetsWithDifferentContainersDifferentValues)
{
	std::set<int>    a{ 1, 2, 3, 4, 5 };
	std::vector<int> b{ 5, 4, 3, 2, 1, 6 };

	EXPECT_FALSE(utils::equivalent_sets(a, b));
}

TEST(utils, NonEquivalentSetsWithDifferentContainersSameValuesDifferentTypes2)
{
	std::set<int>       a{ 1, 2, 3, 4, 5 };
	std::vector<double> b{ 5, 4, 3, 2, 1 };

	EXPECT_FALSE(utils::equivalent_sets(a, b));
}

TEST(utils, EquivalentSetsWithDifferentContainers)
{
	std::set<int>      a{ 1, 2, 3, 4, 5 };
	std::multiset<int> b{ 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };

	EXPECT_FALSE(utils::equivalent_rngs(a, b));
}

auto main() -> int
{
	testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}