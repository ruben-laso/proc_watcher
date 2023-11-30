#include "prox/process.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utils.hpp"

// Build a mockup of the cpu_time class (it doesn't have any virtual methods)
class Mock_cpu_time
{
	using ull = unsigned long long int;

public:
	MOCK_METHOD(ull, user_time, (), (const));
	MOCK_METHOD(ull, nice_time, (), (const));
	MOCK_METHOD(ull, system_time, (), (const));
	MOCK_METHOD(ull, idle_time, (), (const));
	MOCK_METHOD(ull, io_wait, (), (const));
	MOCK_METHOD(ull, irq, (), (const));
	MOCK_METHOD(ull, soft_irq, (), (const));
	MOCK_METHOD(ull, steal, (), (const));
	MOCK_METHOD(ull, guest, (), (const));
	MOCK_METHOD(ull, guest_nice, (), (const));
	MOCK_METHOD(ull, idle_total_time, (), (const));
	MOCK_METHOD(ull, system_total_time, (), (const));
	MOCK_METHOD(ull, virt_total_time, (), (const));
	MOCK_METHOD(ull, total_time, (), (const));
	MOCK_METHOD(ull, total_period, (), (const));
	MOCK_METHOD(ull, last_total_time, (), (const));
	MOCK_METHOD(float, period, (), (const));
	MOCK_METHOD(void, update, (const std::filesystem::path &), ());
};

struct process_stat
{
	using lint  = long int;
	using ulint = unsigned long int;
	using ull   = unsigned long long int;

	pid_t                 pid  = 123456789;
	std::filesystem::path path = std::filesystem::temp_directory_path() / std::to_string(pid);

	std::string name        = "my-mock-pid";
	char        state       = 'S';
	pid_t       ppid        = 4456;
	uint        pgrp        = 4487;
	uint        session     = 4487;
	uint        tty         = 34816;
	int         tpgid       = 13349;
	ulint       flags       = 4194304;
	ulint       minflt      = 48695;
	ulint       cminflt     = 385441;
	ulint       majflt      = 77;
	ulint       cmajflt     = 353;
	ull         utime       = 142;
	ull         stime       = 88;
	ull         cutime      = 486;
	ull         cstime      = 406;
	lint        priority    = 20;
	lint        nice        = 0;
	lint        num_threads = 1;
	int         itrealvalue = 0;
	ull         starttime   = 29218;

	// Next values are not parsed
	ull vsize      = 23072768;
	ull rss        = 3432;
	ull rsslim     = 184467440737095;
	ull startcode  = 94317919137792;
	ull endcode    = 94317919912838;
	ull startstack = 140733960279152;
	ull kstkesp    = 0;
	ull kstkeip    = 0;
	ull signal     = 0;
	ull blocked    = 2;
	ull sigignore  = 3686400;
	ull sigcatch   = 134295555;
	ull wchan      = 1;
	ull nswap      = 0;
	ull cnswap     = 0;
	// End of non-parsed values

	int exit_signal           = 17;
	int processor             = 6;
	int rt_priority           = 0;
	int policy                = 0;
	int delayacct_blkio_ticks = 0;
	int guest_time            = 0;
	int cguest_time           = 0;
	ull start_data            = 94317920029408;
	ull end_data              = 94317920058604;
	ull start_brk             = 94317926449152;
	ull arg_start             = 140733960280484;
	ull arg_end               = 140733960280488;
	ull env_start             = 140733960280488;
	ull env_end               = 140733960282091;
	int exit_code             = 0;
};

static void generate_mock_process(process_stat & process)
{
	std::stringstream file_content;

	// Make the directory (if it doesn't exist)
	std::filesystem::create_directories(process.path);

	if (not std::filesystem::exists(process.path))
	{
		throw std::runtime_error("The directory " + process.path.string() + " could not be created");
	}

	// Generate the cmdline file
	file_content << process.name << std::endl;

	const auto    cmdline_path = process.path / "cmdline";
	std::ofstream out(cmdline_path);
	out << file_content.str();
	out.close();

	// Check that the file was written correctly
	if (not std::filesystem::exists(cmdline_path))
	{
		throw std::runtime_error("The cmdline file was not created correctly in \"" + cmdline_path.string() + "\"");
	}

	// Generate the stat file
	file_content.str("");
	file_content
	    << process.pid << " (" << process.name << ") " << process.state << " " << process.ppid << " " << process.pgrp
	    << " " << process.session << " " << process.tty << " " << process.tpgid << " " << process.flags << " "
	    << process.minflt << " " << process.cminflt << " " << process.majflt << " " << process.cmajflt << " "
	    << process.utime << " " << process.stime << " " << process.cutime << " " << process.cstime << " "
	    << process.priority << " " << process.nice << " " << process.num_threads << " " << process.itrealvalue << " "
	    << process.starttime << " " << process.vsize << " " << process.rss << " " << process.rsslim << " "
	    << process.startcode << " " << process.endcode << " " << process.startstack << " " << process.kstkesp << " "
	    << process.kstkeip << " " << process.signal << " " << process.blocked << " " << process.sigignore << " "
	    << process.sigcatch << " " << process.wchan << " " << process.nswap << " " << process.cnswap << " "
	    << process.exit_signal << " " << process.processor << " " << process.rt_priority << " " << process.policy << " "
	    << process.delayacct_blkio_ticks << " " << process.guest_time << " " << process.cguest_time << " "
	    << process.start_data << " " << process.end_data << " " << process.start_brk << " " << process.arg_start << " "
	    << process.arg_end << " " << process.env_start << " " << process.env_end << " " << process.exit_code
	    << std::endl;

	// Write the "stat" file
	const auto folder_path = process.path / "task" / std::to_string(process.pid);
	std::filesystem::create_directories(folder_path);
	const auto stat_path = process.path / "task" / std::to_string(process.pid) / "stat";

	out.open(stat_path);
	out << file_content.str();
	out.close();

	// Check that the file was written correctly
	if (not std::filesystem::exists(stat_path))
	{
		throw std::runtime_error("The stat file was not created correctly in \"" + stat_path.string() + "\"");
	}

	// Generate the "children" file
	file_content.str("");
	file_content << "123456780 123456709" << std::endl;

	const auto children_path = process.path / "task" / std::to_string(process.pid) / "children";

	out.open(children_path);
	out << file_content.str();
	out.close();

	// Check that the file was written correctly
	if (!std::filesystem::exists(children_path))
	{
		throw std::runtime_error("The children file was not created correctly in \"" + children_path.string() + "\"");
	}
}

TEST(ProcessTest, CreateThisProcess)
{
	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	// Generate a random process that doesn't exist
	const auto pid  = ::getpid();
	const auto path = std::filesystem::path("/proc") / std::to_string(pid);

	EXPECT_NO_THROW(prox::process process(pid, path, cpu_time););
}

TEST(ProcessTest, CreateNonExistentProcess)
{
	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	// Generate a random process that doesn't exist
	const auto pid  = 123456789;
	const auto path = std::filesystem::path("/proc") / std::to_string(pid);

	EXPECT_THROW(prox::process process(pid, path, cpu_time), std::runtime_error);
}

TEST(ProcessTest, CreateNonExistentProcessWithWrongPath)
{
	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	// Generate a random process that doesn't exist
	const auto pid  = 123456789;
	const auto path = std::filesystem::path("/proc") / "non_existent_file.txt";

	EXPECT_THROW(prox::process process(pid, path, cpu_time), std::runtime_error);
}

TEST(ProcessTest, ParseInformationCorrectly)
{
	process_stat mock_process;
	generate_mock_process(mock_process);

	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	// Build the process
	prox::process process(mock_process.pid, mock_process.path, cpu_time);

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

	const std::set<pid_t> expected_children = { 123456780, 123456709 };
	EXPECT_TRUE(utils::equivalent_rngs(process.children(), expected_children));

	const std::set<pid_t> expected_tasks = {}; // Do not store self-task
	EXPECT_TRUE(utils::equivalent_rngs(process.tasks(), expected_tasks));

	EXPECT_TRUE(
	    utils::equivalent_rngs(process.children_and_tasks(), ranges::views::concat(expected_children, expected_tasks)));

	EXPECT_EQ(process.numa_node(), numa_node_of_cpu(mock_process.processor));
}

TEST(ProcessTest, ParseInformationTrailingWhitespacesCmdline)
{
	process_stat mock_process;
	const auto   original_name = mock_process.name;
	mock_process.name += "   ";
	generate_mock_process(mock_process);

	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	// Build the process
	prox::process process(mock_process.pid, mock_process.path, cpu_time);

	// Check the information
	EXPECT_STREQ(process.cmdline().c_str(), original_name.c_str());
}

TEST(ProcessTest, AddNewChild)
{
	process_stat mock_process;
	generate_mock_process(mock_process);

	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	prox::process process(mock_process.pid, mock_process.path, cpu_time);

	const auto new_child_pid = 987654321;

	auto expected_children = process.children() | ranges::to<std::set>;
	expected_children.insert(new_child_pid);

	process.add_child(new_child_pid);

	EXPECT_TRUE(utils::equivalent_rngs(process.children(), expected_children));
}

TEST(ProcessTest, AddNewTask)
{
	process_stat mock_process;
	generate_mock_process(mock_process);

	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	prox::process process(mock_process.pid, mock_process.path, cpu_time);

	const auto new_task_pid = 987654321;

	auto expected_tasks = process.tasks() | ranges::to<std::set>;
	expected_tasks.insert(new_task_pid);

	process.add_task(new_task_pid);

	EXPECT_TRUE(utils::equivalent_rngs(process.tasks(), expected_tasks));
}

TEST(ProcessTest, AddAlreadyExistingChildren)
{
	process_stat mock_process;
	generate_mock_process(mock_process);

	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	prox::process process(mock_process.pid, mock_process.path, cpu_time);

	const auto child_pid = 123456780;

	auto expected_children = process.children();

	process.add_child(child_pid);

	EXPECT_TRUE(utils::equivalent_rngs(process.children(), expected_children));
}

TEST(ProcessTest, AddAlreadyExistingTask)
{
	process_stat mock_process;
	generate_mock_process(mock_process);

	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	prox::process process(mock_process.pid, mock_process.path, cpu_time);

	const auto task_pid = 123456789;

	auto expected_tasks = process.tasks();

	process.add_task(task_pid);

	EXPECT_TRUE(utils::equivalent_rngs(process.tasks(), expected_tasks));
}

TEST(ProcessTest, PinCurrentProcessCpu)
{
	const auto pid  = ::getpid();
	const auto path = std::filesystem::path("/proc") / std::to_string(pid);

	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	prox::process process(pid, path, cpu_time);

	const auto expected_cpu = 0;

	process.pin_processor(expected_cpu);

	EXPECT_EQ(process.processor(), expected_cpu);
}

TEST(ProcessTest, PinCurrentProcessNode)
{
	const auto pid  = ::getpid();
	const auto path = std::filesystem::path("/proc") / std::to_string(pid);

	Mock_cpu_time cpu_time;
	ON_CALL(cpu_time, period()).WillByDefault(testing::Return(100));

	prox::process process(pid, path, cpu_time);

	const auto expected_node = numa_node_of_cpu(0);

	process.pin_processor(expected_node);

	EXPECT_EQ(process.numa_node(), expected_node);
}

auto main() -> int
{
	::testing::InitGoogleTest();
	return RUN_ALL_TESTS();
}