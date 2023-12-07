#include "prox/prox.hpp"

#include <gtest/gtest.h>

TEST(prox, exists)
{
	EXPECT_TRUE(true);
}

auto main() -> int
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}