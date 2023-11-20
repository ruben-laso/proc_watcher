#pragma once

#include <cerrno>   // for errno, EFAULT, EINVAL, EPERM, ESRCH
#include <cmath>    // for isnormal
#include <cstring>  // for strerror
#include <numa.h>   // for numa_allocate_cpumask, numa_free_cpumask, numa_node_to_cpus, numa_sched_setaffinity
#include <sched.h>  // for sched_setaffinity, cpu_set_t, sched_getaffinity, CPU_SET, CPU_ZERO
#include <unistd.h> // for sysconf, _SC_NPROCESSORS_ONLN

#include <algorithm>   // for clamp
#include <chrono>      // for time_point, system_clock, chrono_literals
#include <exception>   // for exception
#include <filesystem>  // for path, directory_iterator, exists, is_directory, is_regular_file, directory_entry
#include <fstream>     // for ifstream, basic_istream, operator>>, basic_ostream, getline
#include <iostream>    // for operator<<, basic_ostream, ostream
#include <optional>    // for optional
#include <set>         // for set
#include <stdexcept>   // for runtime_error
#include <string>      // for string, to_string, getline
#include <string_view> // for string_view
#include <utility>     // for

#include <fmt/core.h> // for format

#include <range/v3/all.hpp> // for views::split, views::to, views::concat

namespace proc_watcher
{
	class process
	{
	public:
		constexpr static const std::string_view DEFAULT_PROC = "/proc";
		constexpr static const std::string_view DEF_CPU_STAT = "/proc/stat";

		constexpr static const char RUNNING_CHAR  = 'R';
		constexpr static const char SLEEPING_CHAR = 'S';
		constexpr static const char WAITING_CHAR  = 'W';
		constexpr static const char ZOMBIE_CHAR   = 'Z';
		constexpr static const char STOPPED_CHAR  = 'T';

	private:
		std::set<pid_t> children_{}; // The children of this process.
		std::set<pid_t> tasks_{};    // The tasks (LWP) of this process.

		pid_t pid_{}; // The process ID.

		pid_t effective_ppid_{}; // The parent process ID.
		                         // PPID is the same for "main" threads and its "LWP" threads.
		                         // This keeps track of the "main" thread when this process is a "LWP".
		                         // Otherwise, it is the same as ppid_.

		bool migratable_ = false; // True if the process is migratable.
		bool lwp_        = false; // Is a Lightweight Process (or a thread).
		                          // True if this process has the same command line as its parent
		                          // or "ps" command is empty.

		char state_{}; // State of the process.

		pid_t        ppid_{};    // The PID of the parent of this process.
		unsigned int pgrp_{};    // The process group ID of the process.
		unsigned int session_{}; // The session ID of the process.
		unsigned int tty_nr_{};  // The controlling terminal of the process.
		int          tpgid_{};   // The ID of the foreground process group of the controlling terminal of the process.

		unsigned long int flags_{}; // The kernel flags word of the process.

		unsigned long int minflt_{};  // The number of minor faults the process has made.
		unsigned long int cminflt_{}; // The number of minor faults that the process's waited-for children have made.
		unsigned long int majflt_{};  // The number of major faults the process has made.
		unsigned long int cmajflt_{}; // The number of major faults that the process's waited-for children have made.

		unsigned long long utime_{}; // Amount of time that this process has been scheduled in user mode.
		unsigned long long stime_{}; // Amount of time that this process has been scheduled in kernel mode.
		unsigned long long
		    cutime_{}; // Amount of time that this process's waited-for children have been scheduled in user mode.
		unsigned long long
		    cstime_{}; // mount of time that this process's waited-for children have been scheduled in kernel mode.

		unsigned long long time_{}; // Amount of time that this process has been scheduled in user and kernel mode.

		long int
		    priority_{}; // For processes running a real-time scheduling policy, this is the negated scheduling priority, minus one.
		long int nice_{}; // The nice value, a value in the range 19 (low priority) to -20 (high priority).

		long int num_threads_{}; // Number of threads in this process.

		unsigned long long starttime_{}; // The time the process started after system boot.

		uid_t st_uid_{};    // User ID the process belongs to.
		int   processor_{}; // CPU number last executed on.

		int exit_signal_{}; // The thread's exit status in the form reported by wait_pid.

		std::optional<int> pinned_processor_{}; // CPU number pinned on. There might be a delay between pinning
		                                        // a process and the migration is performed.
		std::optional<int> pinned_numa_node_{}; // NUMA node of pinned_processor_ field. There might be a delay between
		                                        // pinning a process and the migration is performed.

		unsigned long long last_times_{}; // (utime + stime). Updated when the process is updated.
		float              cpu_use_{};    // Portion of CPU time used (between 0 and 1).

		unsigned long long int last_total_time_{}; // Last total time of the CPU. Used for the calculation of cpu_use_.

		std::filesystem::path path_{};          // The path to the process folder.
		std::filesystem::path children_path_{}; // The path to the children file.
		std::filesystem::path tasks_path_{};    // The path to the tasks file.

		std::string cmdline_{}; // The command line of this process.

		[[nodiscard]] static inline auto uid() -> uid_t
		{
			static const auto UID = getuid();
			return UID;
		}

		static auto set_affinity_error(const pid_t pid)
		{
			switch (errno)
			{
				case EFAULT:
					return fmt::format("Error setting affinity: A supplied memory address was invalid.");
				case EINVAL:
					return fmt::format(
					    "Error setting affinity: The affinity bitmask mask contains no processors that are physically "
					    "on the system, or cpusetsize is smaller than the size of the affinity mask used by the kernel.");
				case EPERM:
					return fmt::format(
					    "Error setting affinity: The calling process does not have appropriate privileges for PID {}.",
					    pid);
				case ESRCH: // When this happens, it's practically unavoidable
					return fmt::format("Error setting affinity: The process whose ID is {} could not be found", pid);
				default:
					return fmt::format("Error setting affinity: Unknown error");
			}
		}

		void manual_read_stat_file()
		{
			const auto pid_str = std::to_string(pid_);
			const auto stat_file_path =
			    lwp_ ? path_ / pid_str / "stat" :                                         // LWP
			           std::filesystem::path("/proc") / pid_str / "task" / pid_str / "stat"; // Main thread
			std::ifstream stat_file{ stat_file_path };

			if (not stat_file.is_open() or not stat_file.good())
			{
				const auto error =
				    fmt::format("Error opening stat file for PID {} ({})", pid_, stat_file_path.string());
				throw std::runtime_error(error);
			}

			pid_t pid = 0;
			stat_file >> pid;
			if (std::cmp_not_equal(pid, pid_))
			{
				// get back to the beginning of the file for future reads
				stat_file.clear();
				stat_file.seekg(0, std::ifstream::beg);

				const auto error = fmt::format("PID mismatch: expected {} but got {}", pid_, pid);
				throw std::runtime_error(error);
			}

			std::string name;
			std::getline(stat_file, name, ' '); // Skip space before name
			std::getline(stat_file, name, ' '); // get the name in the format -> "(name)"

			stat_file >> state_;
			stat_file >> ppid_;
			stat_file >> pgrp_;
			stat_file >> session_;
			stat_file >> tty_nr_;
			stat_file >> tpgid_;
			stat_file >> flags_;
			stat_file >> minflt_;
			stat_file >> cminflt_;
			stat_file >> majflt_;
			stat_file >> cmajflt_;
			stat_file >> utime_;
			stat_file >> stime_;
			stat_file >> cutime_;
			stat_file >> cstime_;
			stat_file >> priority_;
			stat_file >> nice_;
			stat_file >> num_threads_;

			// skip (21) itrealvalue
			int it_real_value = 0;
			stat_file >> it_real_value;

			stat_file >> starttime_;

			// skip from (23) vsize to (37) cnswap
			constexpr size_t skip_fields = 37 - 23;
			for (size_t i = 0; i <= skip_fields; ++i)
			{
				unsigned long skip = 0;
				stat_file >> skip;
			}

			stat_file >> exit_signal_;

			time_ = utime_ + stime_;

			stat_file >> processor_;
		}

		void read_stat_file()
		{
			// Update the process info from the stat file
			manual_read_stat_file();

			const auto time = utime_ + stime_;

			const auto period = scan_cpu_time();

			cpu_use_ = static_cast<float>(time - last_times_) / period * 100.0F;
			if (not std::isnormal(cpu_use_)) { cpu_use_ = 0.0; }

			// Parent processes gather all children CPU usage => cpu_use_ >> 100
			cpu_use_ = std::clamp(cpu_use_, static_cast<float>(0.0), static_cast<float>(100.0));

			last_times_ = time;
		}

		auto scan_cpu_time(const std::string_view filename = DEF_CPU_STAT) -> float
		{
			static const auto N_CPUS = sysconf(_SC_NPROCESSORS_ONLN);

			std::ifstream file(filename.data(), std::ios::in);

			if (not file.is_open())
			{
				const auto error_str =
				    fmt::format("Could not open file {}. Error {} ({})", filename, errno, strerror(errno));
				throw std::runtime_error(error_str);
			}

			if (__glibc_unlikely(std::cmp_less_equal(N_CPUS, 0)))
			{
				const auto error_str = fmt::format("Invalid number of CPUs: {}", N_CPUS);
				throw std::runtime_error(error_str);
			}

			std::string cpu_str; // = "cpu "

			unsigned long long int user_time   = 0;
			unsigned long long int nice_time   = 0;
			unsigned long long int system_time = 0;
			unsigned long long int idle_time   = 0;
			unsigned long long int io_wait     = 0;
			unsigned long long int irq         = 0;
			unsigned long long int soft_irq    = 0;
			unsigned long long int steal       = 0;
			unsigned long long int guest       = 0;
			unsigned long long int guest_nice  = 0;

			file >> cpu_str;
			file >> user_time;
			file >> nice_time;
			file >> system_time;
			file >> idle_time;
			file >> io_wait;
			file >> irq;
			file >> soft_irq;
			file >> steal;
			file >> guest;
			file >> guest_nice;

			// Guest time is already accounted in user time
			user_time = user_time - guest;
			nice_time = nice_time - guest_nice;

			const auto idle_all_time   = idle_time + io_wait;
			const auto system_all_time = system_time + irq + soft_irq;
			const auto virt_all_time   = guest + guest_nice;
			const auto total_time = user_time + nice_time + system_all_time + idle_all_time + steal + virt_all_time;

			const auto total_period = (total_time > last_total_time_) ? (total_time - last_total_time_) : 1;

			last_total_time_ = total_time;

			const auto period = static_cast<float>(total_period) / static_cast<float>(N_CPUS);

			return period;
		}

		[[nodiscard]] inline auto update_list_of_tasks()
		{
			tasks_ = {};

			if (not std::filesystem::exists(tasks_path_)) { return; }

			// Tasks is a directory with subfolders named after the thread IDs
			for (const auto & entry : std::filesystem::directory_iterator(tasks_path_))
			{
				try
				{
					const auto thread_id = entry.path().filename().string();
					tasks_.emplace(std::stoul(thread_id));
				}
				catch (const std::exception & e)
				{
					throw std::runtime_error(
					    fmt::format("Error reading thread ID from {}: {}", entry.path().string(), e.what()));
				}
			}

			// Remove the main thread from the list
			tasks_.erase(pid_);
		}

		[[nodiscard]] inline auto update_list_of_children()
		{
			children_ = {};

			if (not std::filesystem::exists(children_path_)) { return; }

			// Children is a file with a list of PIDs
			std::ifstream file(children_path_, std::ios::in);

			if (not file.is_open())
			{
				const auto error_str = fmt::format("Could not open file {}. Error {} ({})", children_path_.string(),
				                                   errno, strerror(errno));
				throw std::runtime_error(error_str);
			}

			for (pid_t child_pid = 0; file >> child_pid;)
			{
				children_.emplace(child_pid);
			}
		}

		[[nodiscard]] inline auto is_migratable() const -> bool
		{
			// Bad PID
			if (std::cmp_less(pid_, 1)) { return false; }
			// Root can do anything
			if (std::cmp_equal(uid(), 0)) { return true; }

			return std::cmp_equal(st_uid_, uid());
		}

		[[nodiscard]] inline auto tasks_folder() { return path_ / std::to_string(pid_) / "task"; }

		[[nodiscard]] inline auto children_folder()
		{
			static const std::string CHILDREN_FILE("children");
			// Straightforward if the process is a LWP
			if (lwp_) { return path_ / CHILDREN_FILE; }
			// Otherwise, we need to go through the tasks folder
			return tasks_folder() / std::to_string(pid_) / CHILDREN_FILE;
		}

		[[nodiscard]] inline auto obtain_cmdline()
		{
			try
			{
				std::ifstream cmdline_file(path_ / std::to_string(pid_) / "cmdline");

				std::ostringstream cmdline_stream;

				ranges::copy(ranges::istream_view<std::string>(cmdline_file),
				             ranges::ostream_iterator<std::string>(cmdline_stream, " "));

				// Remove trailing whitespace(s)
				auto cmdline = cmdline_stream.str();

				static constexpr auto whitespaces{ " \t\f\v\n\r" };

				cmdline.erase(cmdline.find_last_not_of(whitespaces));

				return cmdline;
			}
			catch (std::exception & e)
			{
				const auto error = fmt::format("Could not retrieve cmdline from PID {}: {}", pid_, e.what());
				throw std::runtime_error(error);
			}
			catch (...)
			{
				const auto error = fmt::format("Could not retrieve cmdline from PID {}", pid_);
				throw std::runtime_error(error);
			}
		}

	public:
		process() = delete;

		explicit process(const pid_t pid) :
		    pid_(pid),
		    lwp_(not std::filesystem::exists(fmt::format("/proc/{}", pid))),
		    path_("/proc"),
		    tasks_path_(tasks_folder()),
		    children_path_(children_folder()),
		    cmdline_(obtain_cmdline()),
		    migratable_(is_migratable())
		{
			update();
			effective_ppid_ = ppid_;
		}

		process(const pid_t pid, const std::string_view dirname, std::optional<pid_t> ppid_opt = std::nullopt) :
		    pid_(pid),
		    lwp_(not std::filesystem::exists(fmt::format("/proc/{}", pid))),
		    path_(dirname),
		    tasks_path_(tasks_folder()),
		    children_path_(children_folder()),
		    cmdline_(obtain_cmdline()),
		    migratable_(is_migratable())
		{
			update();

			if (not lwp_) { effective_ppid_ = ppid_; }
			else if (ppid_opt.has_value()) { effective_ppid_ = ppid_opt.value(); }
			else
			{
				// Find the parent PID of the LWP -> /proc/<pid>/task/<tid>
				const auto split = ranges::views::split(dirname, "/") | ranges::to<std::vector<std::string>>();

				if (split.size() < 2) { throw std::runtime_error("Could not retrieve PPID of LWP"); }

				effective_ppid_ = std::stoi(split[2]);
			}
		}

		[[nodiscard]] inline auto cmdline() const { return cmdline_; }

		[[nodiscard]] inline auto pid() const -> pid_t { return pid_; }

		[[nodiscard]] inline auto ppid() const -> pid_t { return ppid_; }

		[[nodiscard]] inline auto effective_ppid() const -> pid_t { return effective_ppid_; }

		[[nodiscard]] inline auto processor() const
		{
			return pinned_processor_.has_value() ? pinned_processor_.value() : processor_;
		}

		[[nodiscard]] inline auto numa_node() const
		{
			return pinned_numa_node_.has_value() ? pinned_numa_node_.value() : numa_node_of_cpu(processor_);
		}

		[[nodiscard]] inline auto cpu_use() const { return cpu_use_; }

		[[nodiscard]] inline auto path() const { return path_; }

		[[nodiscard]] inline auto lwp() const { return lwp_; }

		[[nodiscard]] inline auto migratable() const { return migratable_; }

		[[nodiscard]] inline auto state() const { return state_; }

		[[nodiscard]] inline auto running() const { return state_ == RUNNING_CHAR; }

		[[nodiscard]] inline auto pgrp() const { return pgrp_; }

		[[nodiscard]] inline auto session() const { return session_; }

		[[nodiscard]] inline auto tty_nr() const { return tty_nr_; }

		[[nodiscard]] inline auto tpgid() const { return tpgid_; }

		[[nodiscard]] inline auto flags() const { return flags_; }

		[[nodiscard]] inline auto minflt() const { return minflt_; }

		[[nodiscard]] inline auto cminflt() const { return cminflt_; }

		[[nodiscard]] inline auto majflt() const { return majflt_; }

		[[nodiscard]] inline auto cmajflt() const { return cmajflt_; }

		[[nodiscard]] inline auto utime() const { return utime_; }

		[[nodiscard]] inline auto stime() const { return stime_; }

		[[nodiscard]] inline auto cutime() const { return cutime_; }

		[[nodiscard]] inline auto cstime() const { return cstime_; }

		[[nodiscard]] inline auto time() const { return time_; }

		[[nodiscard]] inline auto priority() const { return priority_; }

		[[nodiscard]] inline auto nice() const { return nice_; }

		[[nodiscard]] inline auto num_threads() const { return num_threads_; }

		[[nodiscard]] inline auto starttime() const { return starttime_; }

		[[nodiscard]] inline auto st_uid() const { return st_uid_; }

		[[nodiscard]] inline auto exit_signal() const { return exit_signal_; }

		inline void update()
		{
			// Update the values from the stat file
			read_stat_file();
			// Update the list of tasks
			update_list_of_tasks();
			// Update the list of children
			update_list_of_children();
		}

		[[nodiscard]] inline auto children() const { return children_; }

		[[nodiscard]] inline auto tasks() const { return tasks_; }

		[[nodiscard]] inline auto children_and_tasks() const { return ranges::views::concat(children_, tasks_); }

		inline void pin_processor(const int processor)
		{
			if (pinned_processor_.has_value() and std::cmp_equal(pinned_processor_.value(), processor)) { return; }

			cpu_set_t affinity;

			CPU_ZERO(&affinity);
			CPU_SET(processor, &affinity);

			if (__glibc_unlikely(sched_setaffinity(pid_, sizeof(cpu_set_t), &affinity)))
			{
				throw std::runtime_error(set_affinity_error(pid_));
			}

			pinned_processor_ = processor;
		}

		inline void pin_processor()
		{
			if (pinned_processor_.has_value()) { return; }

			pin_processor(processor_);
		}

		inline void pin_numa_node(const int numa_node)
		{
			if (pinned_numa_node_.has_value() and std::cmp_equal(pinned_numa_node_.value(), numa_node)) { return; }

			bitmask * cpus = numa_allocate_cpumask();

			if (__glibc_unlikely(std::cmp_equal(numa_node_to_cpus(numa_node, cpus), -1)))
			{
				throw std::runtime_error(fmt::format("Could not retrieve CPUs of NUMA node {}", numa_node));
			}

			if (__glibc_unlikely(numa_sched_setaffinity(pid_, cpus)))
			{
				throw std::runtime_error(set_affinity_error(pid_));
			}

			numa_free_cpumask(cpus);

			pinned_numa_node_ = numa_node;
		}

		inline void pin_numa_node()
		{
			if (pinned_numa_node_.has_value()) { return; }

			pin_numa_node(numa_node());
		}

		inline void unpin()
		{
			if (not pinned_processor_.has_value() and not pinned_numa_node_.has_value()) { return; }

			cpu_set_t affinity;
			sched_getaffinity(0, sizeof(cpu_set_t),
			                  &affinity); // Gets this process' affinity (supposed to be the default)

			if (__glibc_unlikely(sched_setaffinity(pid_, sizeof(cpu_set_t), &affinity)))
			{
				throw std::runtime_error(set_affinity_error(pid_));
			}
		}

		friend auto operator<<(std::ostream & os, const process & p) -> std::ostream &
		{
			os << fmt::format("PID: {:>6}, PPID: {:>6}, NODE: {:>2}, CPU: {:>3} ({:.1f}%), LWP: {:>5}, CMDLINE: {}",
			                  p.pid_, p.ppid_, p.numa_node(), p.processor(), p.cpu_use(), p.lwp_, p.cmdline());

			return os;
		}
	};
} // namespace proc_watcher