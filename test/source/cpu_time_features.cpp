#include "prox/cpu_time.hpp"

#include <gtest/gtest.h>

TEST(CPUTimeTest, InitialValues)
{
	prox::CPU_time cpu_time;
	EXPECT_EQ(cpu_time.user_time(), 0);
	EXPECT_EQ(cpu_time.nice_time(), 0);
	EXPECT_EQ(cpu_time.system_time(), 0);
	EXPECT_EQ(cpu_time.idle_time(), 0);
	EXPECT_EQ(cpu_time.io_wait(), 0);
	EXPECT_EQ(cpu_time.irq(), 0);
	EXPECT_EQ(cpu_time.soft_irq(), 0);
	EXPECT_EQ(cpu_time.steal(), 0);
	EXPECT_EQ(cpu_time.guest(), 0);
	EXPECT_EQ(cpu_time.guest_nice(), 0);
	EXPECT_EQ(cpu_time.idle_total_time(), 0);
	EXPECT_EQ(cpu_time.system_total_time(), 0);
	EXPECT_EQ(cpu_time.virt_total_time(), 0);
	EXPECT_EQ(cpu_time.total_time(), 0);
	EXPECT_EQ(cpu_time.total_period(), 0);
	EXPECT_EQ(cpu_time.last_total_time(), 0);
	EXPECT_EQ(cpu_time.period(), 0);
}

TEST(CPUTimeTest, Update)
{
	// Mockup a /proc/stat file
	std::stringstream file;
	file
	    //       user nice system idle iowait irq softirq steal guest guest_nice
	    << "cpu  1816560 4773 518338 23132813 31707 0 49840 0 0 0\n"
	       "cpu0 157226 402 45232 1925414 2650 0 820 0 0 0\n"
	       "cpu1 172879 424 45130 1902881 2681 0 1087 0 0 0\n"
	       "cpu2 158965 386 45468 1925392 2664 0 421 0 0 0\n"
	       "cpu3 165289 408 46038 1913614 2583 0 586 0 0 0\n"
	       "cpu4 158222 361 43278 1928246 2661 0 276 0 0 0\n"
	       "cpu5 160689 352 44207 1925083 2519 0 289 0 0 0\n"
	       "cpu6 144070 428 38490 1942058 2656 0 2966 0 0 0\n"
	       "cpu7 111912 346 37076 1969334 2643 0 2500 0 0 0\n"
	       "cpu8 152636 600 42796 1933042 2929 0 248 0 0 0\n"
	       "cpu9 138498 325 42556 1903559 2533 0 31574 0 0 0\n"
	       "cpu10 149564 312 44519 1934349 2497 0 479 0 0 0\n"
	       "cpu11 146605 425 43545 1929834 2684 0 8589 0 0 0\n"
	       "intr 85874642 12 1334 0 0 0 0 0 0 0 318969 0 0 225 0 0 0 3 4737226 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 112242 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 5 0 0 0 0 5 0 0 68 765 0 2104354 0 0 0 10688324 0 0 0 0 0 0 10 19344 19301 0 0 0 0 0 0 0 0 0 0 0 0 0 0 34 71547 69654 74260 61883 63570 56577 55816 61200 69096 58268 62497 63539 0 8 0 0 0 0 0 0 0 0 0 0 0 0 2 9018160 1259 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
	       "ctxt 251528755\n"
	       "btime 1701244738\n"
	       "processes 139934\n"
	       "procs_running 2\n"
	       "procs_blocked 1\n"
	       "softirq 95197389 21120365 3474468 3649606 6070268 26548 0 20664647 24432043 2 15759442\n";

	std::filesystem::path path = std::filesystem::temp_directory_path() / "cpu_time_test.txt";

	std::ofstream out(path);
	out << file.str();
	out.close();

	const auto N_CPUS = sysconf(_SC_NPROCESSORS_ONLN);

	const unsigned long long int user_time   = 1816560;
	const unsigned long long int nice_time   = 4773;
	const unsigned long long int system_time = 518338;
	const unsigned long long int idle_time   = 23132813;
	const unsigned long long int io_wait     = 31707;
	const unsigned long long int irq         = 0;
	const unsigned long long int soft_irq    = 49840;
	const unsigned long long int steal       = 0;
	const unsigned long long int guest       = 0;
	const unsigned long long int guest_nice  = 0;

	const unsigned long long int last_total_time = 0;

	// Guest time is already accounted in user time
	const auto real_user_time = user_time - guest;
	const auto real_nice_time = nice_time - guest_nice;

	const auto idle_total_time   = idle_time + io_wait;
	const auto system_total_time = system_time + irq + soft_irq;
	const auto virt_total_time   = guest + guest_nice;
	const auto total_time =
	    real_user_time + real_nice_time + system_total_time + idle_total_time + steal + virt_total_time;

	const auto total_period = total_time - last_total_time;

	const auto period = static_cast<float>(total_period) / static_cast<float>(N_CPUS);

	const auto expected_last_total_time = total_time;

	prox::CPU_time cpu_time;

	cpu_time.update(path);
	EXPECT_EQ(cpu_time.user_time(), real_user_time);
	EXPECT_EQ(cpu_time.nice_time(), real_nice_time);
	EXPECT_EQ(cpu_time.system_time(), system_time);
	EXPECT_EQ(cpu_time.idle_time(), idle_time);
	EXPECT_EQ(cpu_time.io_wait(), io_wait);
	EXPECT_EQ(cpu_time.irq(), irq);
	EXPECT_EQ(cpu_time.soft_irq(), soft_irq);
	EXPECT_EQ(cpu_time.steal(), steal);
	EXPECT_EQ(cpu_time.guest(), guest);
	EXPECT_EQ(cpu_time.guest_nice(), guest_nice);


	EXPECT_EQ(cpu_time.idle_total_time(), idle_total_time);
	EXPECT_EQ(cpu_time.system_total_time(), system_total_time);
	EXPECT_EQ(cpu_time.virt_total_time(), virt_total_time);
	EXPECT_EQ(cpu_time.total_time(), total_time);
	EXPECT_EQ(cpu_time.total_period(), total_period);
	EXPECT_FLOAT_EQ(cpu_time.period(), period);

	EXPECT_EQ(cpu_time.last_total_time(), expected_last_total_time);
}

TEST(CPUTimeTest, UpdateWithNoFile)
{
	prox::CPU_time cpu_time;
	EXPECT_THROW(cpu_time.update("non_existent_file.txt"), std::runtime_error);
}

TEST(CPUTimeTest, UpdateWithWrongFile)
{
	std::filesystem::path path = std::filesystem::temp_directory_path() / "cpu_time_test.txt";
	std::ofstream         out(path);
	out << "wrong file";
	out.close();

	prox::CPU_time cpu_time;
	EXPECT_THROW(cpu_time.update(path), std::runtime_error);
}

TEST(CPUTimeTest, UpdateWithWrongFileMissingFields)
{
	std::filesystem::path path = std::filesystem::temp_directory_path() / "cpu_time_test.txt";
	std::ofstream         out(path);
	out << "cpu  1816560 4773 518338 23132813";
	out.close();

	prox::CPU_time cpu_time;
	EXPECT_THROW(cpu_time.update(path), std::runtime_error);
}

TEST(CPUTimeTest, UpdateWithWrongUnexpectedFields)
{
	std::filesystem::path path = std::filesystem::temp_directory_path() / "cpu_time_test.txt";
	std::ofstream         out(path);
	out << "cpu  1816560 4773 518338 23132813 31707 0 49840 0 0 0 1 2 3 4 5 6 7 8 9";
	out.close();

	prox::CPU_time cpu_time;
	EXPECT_THROW(cpu_time.update(path), std::runtime_error);
}

auto main() -> int
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}