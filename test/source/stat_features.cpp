#include "prox/stat.hpp"

#include <gtest/gtest.h>

#include "mock_process.hpp"

TEST(prox, stat_exists)
{
	prox::process_stat mock_process;
	prox::write_mock_process_stat(mock_process);

	const auto stat_path = mock_process.path / "task" / std::to_string(mock_process.pid) / "stat";

	prox::stat stat = prox::read_stat_file(stat_path);

	EXPECT_EQ(stat.pid, mock_process.pid);
	EXPECT_EQ(stat.comm, mock_process.name);
	EXPECT_EQ(stat.state, mock_process.state);
	EXPECT_EQ(stat.ppid, mock_process.ppid);
	EXPECT_EQ(stat.pgrp, mock_process.pgrp);
	EXPECT_EQ(stat.session, mock_process.session);
	EXPECT_EQ(stat.tty_nr, mock_process.tty_nr);
	EXPECT_EQ(stat.tpgid, mock_process.tpgid);
	EXPECT_EQ(stat.flags, mock_process.flags);
	EXPECT_EQ(stat.minflt, mock_process.minflt);
	EXPECT_EQ(stat.cminflt, mock_process.cminflt);
	EXPECT_EQ(stat.majflt, mock_process.majflt);
	EXPECT_EQ(stat.cmajflt, mock_process.cmajflt);
	EXPECT_EQ(stat.utime, mock_process.utime);
	EXPECT_EQ(stat.stime, mock_process.stime);
	EXPECT_EQ(stat.cutime, mock_process.cutime);
	EXPECT_EQ(stat.cstime, mock_process.cstime);
	EXPECT_EQ(stat.priority, mock_process.priority);
	EXPECT_EQ(stat.nice, mock_process.nice);
	EXPECT_EQ(stat.num_threads, mock_process.num_threads);
	EXPECT_EQ(stat.itrealvalue, mock_process.itrealvalue);
	EXPECT_EQ(stat.starttime, mock_process.starttime);
	EXPECT_EQ(stat.vsize, mock_process.vsize);
	EXPECT_EQ(stat.rss, mock_process.rss);
	EXPECT_EQ(stat.rsslim, mock_process.rsslim);
	EXPECT_EQ(stat.startcode, mock_process.startcode);
	EXPECT_EQ(stat.endcode, mock_process.endcode);
	EXPECT_EQ(stat.startstack, mock_process.startstack);
	EXPECT_EQ(stat.kstkesp, mock_process.kstkesp);
	EXPECT_EQ(stat.kstkeip, mock_process.kstkeip);
	EXPECT_EQ(stat.signal, mock_process.signal);
	EXPECT_EQ(stat.blocked, mock_process.blocked);
	EXPECT_EQ(stat.sigignore, mock_process.sigignore);
	EXPECT_EQ(stat.sigcatch, mock_process.sigcatch);
	EXPECT_EQ(stat.wchan, mock_process.wchan);
	EXPECT_EQ(stat.nswap, mock_process.nswap);
	EXPECT_EQ(stat.cnswap, mock_process.cnswap);
	EXPECT_EQ(stat.exit_signal, mock_process.exit_signal);
	EXPECT_EQ(stat.processor, mock_process.processor);
	EXPECT_EQ(stat.rt_priority, mock_process.rt_priority);
	EXPECT_EQ(stat.policy, mock_process.policy);
	EXPECT_EQ(stat.delayacct_blkio_ticks, mock_process.delayacct_blkio_ticks);
	EXPECT_EQ(stat.guest_time, mock_process.guest_time);
	EXPECT_EQ(stat.cguest_time, mock_process.cguest_time);
	EXPECT_EQ(stat.start_data, mock_process.start_data);
	EXPECT_EQ(stat.end_data, mock_process.end_data);
	EXPECT_EQ(stat.start_brk, mock_process.start_brk);
	EXPECT_EQ(stat.arg_start, mock_process.arg_start);
	EXPECT_EQ(stat.arg_end, mock_process.arg_end);
	EXPECT_EQ(stat.env_start, mock_process.env_start);
	EXPECT_EQ(stat.env_end, mock_process.env_end);
	EXPECT_EQ(stat.exit_code, mock_process.exit_code);
}

auto main(int argc, char ** argv) -> int
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}