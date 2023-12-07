#include <prox/stat.hpp>

#include <gtest/gtest.h>

TEST(prox, stat_exists)
{
	EXPECT_TRUE(true);
}

auto main() -> int
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}