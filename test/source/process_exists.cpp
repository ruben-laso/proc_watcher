#include "prox/process.hpp"

#include <gtest/gtest.h>

TEST(prox, process_exists)
{
	EXPECT_TRUE(true);
}

auto main() -> int
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}