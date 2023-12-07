#include "prox/cpu_time.hpp"

#include <gtest/gtest.h>

TEST(prox, cpu_time_exists)
{
	EXPECT_TRUE(true);
}

auto main() -> int
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}