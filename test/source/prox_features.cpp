#include <gtest/gtest.h>

#include "utils.hpp"

#include "mock_proc_dir.hpp"
#include "mock_process.hpp"

#include "prox/prox.hpp"

TEST(ProcessTree, InitialiseWithProc)
{
	EXPECT_NO_THROW(prox::process_tree process_tree{});
}

TEST(ProcessTree, InitialiseMockProc)
{
	prox::Mock_proc_dir mock{};

	EXPECT_NO_THROW(prox::process_tree(1, mock.mock_proc_dir));
}

TEST(ProcessTree, InitialiseWithNonExistentProc)
{
	EXPECT_THROW(prox::process_tree(1, "/proc/does/not/exist"), std::runtime_error);
}

TEST(ProcessTree, InitialiseWithNonExistentPid)
{
	EXPECT_THROW(prox::process_tree(-1, "/proc"), std::runtime_error);
}

TEST(ProcessTree, InitialiseWithNonExistentPidAndProc)
{
	EXPECT_THROW(prox::process_tree(0, "/proc/does/not/exist"), std::runtime_error);
}

TEST(ProcessTree, InitialiseWithNonExistentPidAndNonExistentProc)
{
	EXPECT_THROW(prox::process_tree(-1, "/proc/does/not/exist"), std::runtime_error);
}

TEST(ProcessTree, ProcessExist)
{
	prox::Mock_proc_dir mock{};

	prox::process_tree process_tree{ prox::Mock_proc_dir::root, mock.mock_proc_dir };

	EXPECT_NO_THROW(std::ignore = process_tree.get(prox::Mock_proc_dir::PIDs::root));
	EXPECT_NO_THROW(std::ignore = process_tree.get(prox::Mock_proc_dir::PIDs::task1));
	EXPECT_NO_THROW(std::ignore = process_tree.get(prox::Mock_proc_dir::PIDs::task2));
	EXPECT_NO_THROW(std::ignore = process_tree.get(prox::Mock_proc_dir::PIDs::child1));
	EXPECT_NO_THROW(std::ignore = process_tree.get(prox::Mock_proc_dir::PIDs::child2));
}

TEST(ProcessTree, ProcessDoesNotExist)
{
	prox::Mock_proc_dir mock{};

	prox::process_tree process_tree{ 1, mock.mock_proc_dir };

	EXPECT_EQ(process_tree.get(-1), std::nullopt);
}

TEST(ProcessTree, ProcessInformation)
{
	prox::Mock_proc_dir mock{};

	prox::process_tree process_tree{ 1, mock.mock_proc_dir };

	const auto root_pid = prox::Mock_proc_dir::PIDs::root;

	const auto & root_opt = process_tree.get(root_pid);

	EXPECT_TRUE(root_opt.has_value());

	const auto & root = root_opt.value();

	EXPECT_EQ(root->pid(), root_pid);
	EXPECT_STREQ(root->cmdline().c_str(), "root");
	EXPECT_STREQ(root->path().c_str(), (mock.mock_proc_dir / std::to_string(root_pid)).c_str());

	std::set<pid_t> expected_children{ prox::Mock_proc_dir::PIDs::child1, prox::Mock_proc_dir::PIDs::child2 };
	const auto      children = process_tree.get(prox::Mock_proc_dir::PIDs::root)->get()->children();

	fmt::print("[{}] =? [{}]", fmt::join(children, " "), fmt::join(expected_children, " "));

	EXPECT_TRUE(utils::equivalent_rngs(children, expected_children));

	std::set<pid_t> expected_tasks{ prox::Mock_proc_dir::PIDs::task1, prox::Mock_proc_dir::PIDs::task2 };
	const auto      tasks = process_tree.get(prox::Mock_proc_dir::PIDs::root)->get()->tasks();

	fmt::print("[{}] =? [{}]", fmt::join(tasks, " "), fmt::join(expected_tasks, " "));

	EXPECT_TRUE(utils::equivalent_rngs(tasks, expected_tasks));
}

auto main() -> int
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}