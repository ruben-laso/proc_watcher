#include "prox/process.hpp"

#include <gtest/gtest.h>

#include "mock_cpu_time.hpp"
#include "mock_process.hpp"

#include "utils.hpp"

TEST(ProcessTest, CreateThisProcess)
{
	auto cpu_time_ptr = prox::get_mock_cpu_time();

	// Generate a random process that doesn't exist
	const auto pid  = ::getpid();
	const auto path = std::filesystem::path("/proc") / std::to_string(pid);

	EXPECT_NO_THROW(prox::process process(pid, path, *cpu_time_ptr););
}

TEST(ProcessTest, CreateNonExistentProcess)
{
	auto cpu_time_ptr = prox::get_mock_cpu_time();

	// Generate a random process that doesn't exist
	const auto pid  = 123456789;
	const auto path = std::filesystem::path("/proc") / std::to_string(pid);

	EXPECT_THROW(prox::process process(pid, path, *cpu_time_ptr), std::runtime_error);
}

TEST(ProcessTest, CreateNonExistentProcessWithWrongPath)
{
	auto cpu_time_ptr = prox::get_mock_cpu_time();

	// Generate a random process that doesn't exist
	const auto pid  = 123456789;
	const auto path = std::filesystem::path("/proc") / "non_existent_file.txt";

	EXPECT_THROW(prox::process process(pid, path, *cpu_time_ptr), std::runtime_error);
}

TEST(ProcessTest, ParseInformationCorrectly)
{
	prox::process_stat mock_process;

	auto cpu_time_ptr = prox::get_mock_cpu_time();

	const std::set<pid_t> expected_children = { 123456780, 123456709 };
	const std::set<pid_t> expected_tasks    = {}; // Do not store self-task

	mock_process.children = expected_children;
	mock_process.tasks    = expected_tasks;

	prox::write_mock_process_stat(mock_process);

	// Build the process
	prox::process process(mock_process.pid, mock_process.path, *cpu_time_ptr);

	// Check the information
	EXPECT_EQ(process.pid(), mock_process.pid);
	EXPECT_STREQ(process.cmdline().c_str(), mock_process.name.c_str());
	EXPECT_EQ(process.state(), mock_process.state);
	EXPECT_EQ(process.ppid(), mock_process.ppid);
	EXPECT_EQ(process.pgrp(), mock_process.pgrp);
	EXPECT_EQ(process.session(), mock_process.session);
	EXPECT_EQ(process.tty_nr(), mock_process.tty);
	EXPECT_EQ(process.tpgid(), mock_process.tpgid);
	EXPECT_EQ(process.flags(), mock_process.flags);
	EXPECT_EQ(process.minflt(), mock_process.minflt);
	EXPECT_EQ(process.cminflt(), mock_process.cminflt);
	EXPECT_EQ(process.majflt(), mock_process.majflt);
	EXPECT_EQ(process.cmajflt(), mock_process.cmajflt);
	EXPECT_EQ(process.utime(), mock_process.utime);
	EXPECT_EQ(process.stime(), mock_process.stime);
	EXPECT_EQ(process.cutime(), mock_process.cutime);
	EXPECT_EQ(process.cstime(), mock_process.cstime);
	EXPECT_EQ(process.priority(), mock_process.priority);
	EXPECT_EQ(process.nice(), mock_process.nice);
	EXPECT_EQ(process.num_threads(), mock_process.num_threads);

	EXPECT_EQ(process.starttime(), mock_process.starttime);
	EXPECT_EQ(process.exit_signal(), mock_process.exit_signal);
	EXPECT_EQ(process.processor(), mock_process.processor);

	EXPECT_EQ(process.time(), mock_process.utime + mock_process.stime);

	EXPECT_TRUE(utils::equivalent_rngs(process.children(), expected_children));
	EXPECT_TRUE(utils::equivalent_rngs(process.tasks(), expected_tasks));

	EXPECT_TRUE(
	    utils::equivalent_rngs(process.children_and_tasks(), ranges::views::concat(expected_children, expected_tasks)));

	EXPECT_EQ(process.numa_node(), numa_node_of_cpu(mock_process.processor));
}

TEST(ProcessTest, ParseInformationTrailingWhitespacesCmdline)
{
	prox::process_stat mock_process;

	const auto original_name = mock_process.name;
	mock_process.name += "   ";

	prox::write_mock_process_stat(mock_process);

	auto cpu_time_ptr = prox::get_mock_cpu_time();

	// Build the process
	prox::process process(mock_process.pid, mock_process.path, *cpu_time_ptr);

	// Check the information
	EXPECT_STREQ(process.cmdline().c_str(), original_name.c_str());
}

TEST(ProcessTest, AddNewChild)
{
	prox::process_stat mock_process;
	prox::write_mock_process_stat(mock_process);

	auto cpu_time_ptr = prox::get_mock_cpu_time();

	prox::process process(mock_process.pid, mock_process.path, *cpu_time_ptr);

	const auto new_child_pid = 987654321;

	auto expected_children = process.children() | ranges::to<std::set>;
	expected_children.insert(new_child_pid);

	process.add_child(new_child_pid);

	EXPECT_TRUE(utils::equivalent_rngs(process.children(), expected_children));
}

TEST(ProcessTest, AddNewTask)
{
	prox::process_stat mock_process;
	prox::write_mock_process_stat(mock_process);

	auto cpu_time_ptr = prox::get_mock_cpu_time();

	prox::process process(mock_process.pid, mock_process.path, *cpu_time_ptr);

	const auto new_task_pid = 987654321;

	auto expected_tasks = process.tasks() | ranges::to<std::set>;
	expected_tasks.insert(new_task_pid);

	process.add_task(new_task_pid);

	EXPECT_TRUE(utils::equivalent_rngs(process.tasks(), expected_tasks));
}

TEST(ProcessTest, AddAlreadyExistingChildren)
{
	const auto expected_children = { 123456780, 123456709 };

	prox::process_stat mock_process;
	mock_process.children = expected_children;
	prox::write_mock_process_stat(mock_process);

	auto cpu_time_ptr = prox::get_mock_cpu_time();

	prox::process process(mock_process.pid, mock_process.path, *cpu_time_ptr);

	// Add repeated children
	for (const auto & child_pid : expected_children)
	{
		process.add_child(child_pid);
	}

	EXPECT_TRUE(utils::equivalent_rngs(process.children(), expected_children));
}

TEST(ProcessTest, AddAlreadyExistingTask)
{
	prox::process_stat mock_process;
	prox::write_mock_process_stat(mock_process);

	auto cpu_time_ptr = prox::get_mock_cpu_time();

	prox::process process(mock_process.pid, mock_process.path, *cpu_time_ptr);

	const auto task_pid = 123456789;

	auto expected_tasks = process.tasks();

	process.add_task(task_pid);

	EXPECT_TRUE(utils::equivalent_rngs(process.tasks(), expected_tasks));
}

TEST(ProcessTest, PinCurrentProcessCpu)
{
	const auto pid  = ::getpid();
	const auto path = std::filesystem::path("/proc") / std::to_string(pid);

	auto cpu_time_ptr = prox::get_mock_cpu_time();

	prox::process process(pid, path, *cpu_time_ptr);

	const auto expected_cpu = 0;

	process.pin_processor(expected_cpu);

	EXPECT_EQ(process.processor(), expected_cpu);
}

TEST(ProcessTest, PinCurrentProcessNode)
{
	const auto pid  = ::getpid();
	const auto path = std::filesystem::path("/proc") / std::to_string(pid);

	auto cpu_time_ptr = prox::get_mock_cpu_time();

	prox::process process(pid, path, *cpu_time_ptr);

	const auto expected_node = numa_node_of_cpu(0);

	process.pin_processor(expected_node);

	EXPECT_EQ(process.numa_node(), expected_node);
}

auto main() -> int
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}