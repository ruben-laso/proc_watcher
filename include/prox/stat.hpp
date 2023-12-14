#pragma once

#include <sys/types.h>

#include <cstring>

#include <filesystem>
#include <fstream>
#include <string>

namespace prox
{
	struct stat
	{
	public:
		using uint  = unsigned int;
		using lint  = long int;
		using luint = long unsigned int;

		pid_t pid{}; // The process ID.

		std::string comm{}; // The filename of the executable, in parentheses.

		char state{}; // State of the process.

		pid_t ppid{};    // The PID of the parent of this process.
		gid_t pgrp{};    // The process group ID of the process.
		int   session{}; // The session ID of the process.
		int   tty_nr{};  // The controlling terminal of the process.
		int   tpgid{};   // The ID of the foreground process group of the controlling terminal of the process.
		uint  flags{};   // The kernel flags word of the process.
		luint minflt{};  // The number of minor faults the process has made.
		luint cminflt{}; // The number of minor faults that the process's waited-for children have made.
		luint majflt{};  // The number of major faults the process has made.
		luint cmajflt{}; // The number of major faults that the process's waited-for children have made.
		luint utime{};   // Amount of time that this process has been scheduled in user mode.
		luint stime{};   // Amount of time that this process has been scheduled in kernel mode.
		lint  cutime{};  // Amount of time that this process's waited-for children have been scheduled in user mode.
		lint  cstime{};  // Amount of time that this process's waited-for children have been scheduled in kernel mode.
		lint priority{}; // For processes running a real-time scheduling policy, this is the negated scheduling priority, minus one.
		lint nice{};        // The nice value, a value in the range 19 (low priority) to -20 (high priority).
		lint num_threads{}; // Number of threads in this process.
		lint itrealvalue{}; // The time in jiffies before the next SIGALRM is sent to the process due to an interval timer.
		luint starttime{}; // The time the process started after system boot.
		luint vsize{};     // Virtual memory size in bytes.
		lint  rss{};       // Resident Set Size: number of pages the process has in real memory.
		luint rsslim{}; // Current soft limit in bytes on the rss of the process; see the description of RLIMIT_RSS in getpriority(2).
		luint startcode{};  // The address above which program text can run.
		luint endcode{};    // The address below which program text can run.
		luint startstack{}; // The address of the start (i.e., bottom) of the stack.
		luint kstkesp{}; // The current value of ESP (stack pointer), as found in the kernel stack page for the process.
		luint kstkeip{}; // The current EIP (instruction pointer).
		luint signal{};  // The bitmap of pending signals, displayed as a decimal number.
		luint blocked{}; // The bitmap of blocked signals, displayed as a decimal number.
		luint sigignore{};   // The bitmap of ignored signals, displayed as a decimal number.
		luint sigcatch{};    // The bitmap of caught signals, displayed as a decimal number.
		luint wchan{};       // This is the "channel" in which the process is waiting.
		luint nswap{};       // Number of pages swapped (not maintained).
		luint cnswap{};      // Cumulative nswap for child processes (not maintained).
		int   exit_signal{}; // The signal sent to its parent when it dies.
		int   processor{};   // CPU number last executed on.
		uint rt_priority{}; // Real-time scheduling priority, a number in the range 1 to 99 for processes scheduled under a real-time policy, or 0, for non-real-time processes (see sched_setscheduler(2)).
		uint  policy{};     // Scheduling policy (see sched_setscheduler(2)).
		luint delayacct_blkio_ticks{}; // Aggregated block I/O delays, measured in clock ticks (centiseconds).
		luint guest_time{}; // Guest time of the process (time spent running a virtual CPU for a guest operating system), measured in clock ticks (centiseconds).
		lint  cguest_time{}; // Guest time of the process's children, measured in clock ticks (centiseconds).
		luint start_data{};  // Address above which program initialized and uninitialized (BSS) data are placed.
		luint end_data{};    // Address below which program initialized and uninitialized (BSS) data are placed.
		luint start_brk{};   // Address above which program heap can be expanded with brk(2).
		luint arg_start{};   // Address above which program command-line arguments (argv) are placed.
		luint arg_end{};     // Address below program command-line arguments (argv) are placed.
		luint env_start{};   // Address above which program environment is placed.
		luint env_end{};     // Address below which program environment is placed.
		lint  exit_code{};   // The thread's exit status in the form reported by waitpid(2).
	};

	static void update_stat_file(const std::filesystem::path & stat_file, prox::stat & stat)
	{
		std::ifstream file(stat_file);

		if (not file.is_open())
		{
			std::stringstream msg;
			msg << "Error opening file " << stat_file << ": " << strerror(errno);
			throw std::runtime_error(msg.str());
		}

		file >> stat.pid;

		std::getline(file, stat.comm, ' '); // Skip space before name
		std::getline(file, stat.comm, ' '); // get the name in the format -> "(name)"
		// Remove the parentheses
		if (stat.comm.starts_with("(")) { stat.comm.erase(0, 1); }
		if (stat.comm.ends_with(")")) { stat.comm.erase(stat.comm.size() - 1); }

		file >> stat.state;
		file >> stat.ppid;
		file >> stat.pgrp;
		file >> stat.session;
		file >> stat.tty_nr;
		file >> stat.tpgid;
		file >> stat.flags;
		file >> stat.minflt;
		file >> stat.cminflt;
		file >> stat.majflt;
		file >> stat.cmajflt;
		file >> stat.utime;
		file >> stat.stime;
		file >> stat.cutime;
		file >> stat.cstime;
		file >> stat.priority;
		file >> stat.nice;
		file >> stat.num_threads;
		file >> stat.itrealvalue;
		file >> stat.starttime;
		file >> stat.vsize;
		file >> stat.rss;
		file >> stat.rsslim;
		file >> stat.startcode;
		file >> stat.endcode;
		file >> stat.startstack;
		file >> stat.kstkesp;
		file >> stat.kstkeip;
		file >> stat.signal;
		file >> stat.blocked;
		file >> stat.sigignore;
		file >> stat.sigcatch;
		file >> stat.wchan;
		file >> stat.nswap;
		file >> stat.cnswap;
		file >> stat.exit_signal;
		file >> stat.processor;
		file >> stat.rt_priority;
		file >> stat.policy;
		file >> stat.delayacct_blkio_ticks;
		file >> stat.guest_time;
		file >> stat.cguest_time;
		file >> stat.start_data;
		file >> stat.end_data;
		file >> stat.start_brk;
		file >> stat.arg_start;
		file >> stat.arg_end;
		file >> stat.env_start;
		file >> stat.env_end;
		file >> stat.exit_code;
	}

	static inline auto read_stat_file(const std::filesystem::path & stat_file)
	{
		prox::stat stat;
		update_stat_file(stat_file, stat);
		return stat;
	}
} // namespace prox