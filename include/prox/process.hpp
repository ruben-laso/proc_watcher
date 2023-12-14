#pragma once

#include <cerrno>     // for errno, EFAULT, EINVAL, EPERM, ESRCH
#include <cmath>      // for isnormal
#include <cstring>    // for strerror
#include <numa.h>     // for numa_allocate_cpumask, numa_free_cpumask, numa_node_to_cpus, numa_sched_setaffinity
#include <sched.h>    // for sched_setaffinity, cpu_set_t, sched_getaffinity, CPU_SET, CPU_ZERO
#include <sys/stat.h> // for stat
#include <unistd.h>   // for sysconf, _SC_NPROCESSORS_ONLN

#include <algorithm>   // for clamp
#include <chrono>      // for time_point, system_clock, chrono_literals
#include <exception>   // for exception
#include <filesystem>  // for path, directory_iterator, exists, is_directory, is_regular_file, directory_entry
#include <fstream>     // for ifstream, basic_istream, operator>>, basic_ostream, getline
#include <optional>    // for optional
#include <set>         // for set
#include <stdexcept>   // for runtime_error
#include <string>      // for string, to_string, getline
#include <string_view> // for string_view
#include <utility>     // for
#include <vector>      // for vector

#include <range/v3/all.hpp> // for views::split, views::to, views::concat

#include "stat.hpp" // for stat

namespace prox
{
	template<typename CPU_time_provider>
	class process
	{
		// From htop: supposed to be in linux/sched.h
		constexpr static auto PF_KTHREAD = 0x00200000;

	public:
		constexpr static std::string_view DEFAULT_PROC = "/proc";

	private:
		std::reference_wrapper<const CPU_time_provider> cpu_time_;

		std::vector<pid_t> children_{}; // The children of this process.
		std::vector<pid_t> tasks_{};    // The tasks (LWP) of this process.

		pid_t pid_{}; // The process ID.

		std::filesystem::path path_{}; // The path to the process folder.

		pid_t effective_ppid_{}; // The parent process ID.
		                         // PPID is the same for "main" threads and its "LWP" threads.
		                         // This keeps track of the "main" thread when this process is a "LWP".
		                         // Otherwise, it is the same as ppid_.

		bool migratable_ = false; // True if the process is migratable.
		bool lwp_        = false; // Is a Lightweight Process (or a thread).
		                          // True if this process has the same command line as its parent
		                          // or "ps" command is empty.
		bool task_       = false; // Is a task of the effective parent. Path contains "task" somewhere.

		uid_t st_uid_{}; // User ID the process belongs to.

		stat stat_{}; // Struct with the information from the stat file.

		std::optional<int> pinned_processor_{}; // CPU number pinned on. There might be a delay between pinning
		                                        // a process and the migration is performed.
		std::optional<int> pinned_numa_node_{}; // NUMA node of pinned_processor_ field. There might be a delay between
		                                        // pinning a process and the migration is performed.

		unsigned long long last_times_{}; // (utime + stime). Updated when the process is updated.
		float              cpu_use_{};    // Portion of CPU time used (between 0 and 1).

		std::string cmdline_{}; // The command line of this process.

		std::chrono::time_point<std::chrono::high_resolution_clock> last_update_{}; // Last time the process was updated.

		[[nodiscard]] static auto uid() -> uid_t
		{
			static const auto UID = getuid();
			return UID;
		}

		static auto set_affinity_error(const pid_t pid) -> std::string
		{
			switch (errno)
			{
				case EFAULT:
					return "Error setting affinity: A supplied memory address was invalid.";
				case EINVAL:
					return "Error setting affinity: The affinity bitmask mask contains no processors that are physically "
					       "on the system, or cpusetsize is smaller than the size of the affinity mask used by the kernel.";
				case EPERM: // permission denied
				{
					std::stringstream msg;
					msg << "Error setting affinity: The calling process does not have appropriate privileges for the "
					       "requested action on pid "
					    << pid << ".";
					return msg.str();
				}
				case ESRCH: // When this happens, it's practically unavoidable
				{
					std::stringstream msg;
					msg << "Error setting affinity: The process whose ID is " << pid << " could not be found.";
					return msg.str();
				}
				default:
					return "Error setting affinity: Unknown error";
			}
		}

		void read_stat_file()
		{
			// Update the process info from the stat file
			const auto pid_str        = std::to_string(pid_);
			const auto stat_file_path = task_ ? path_ / "stat" : path_ / "task" / pid_str / "stat";

			update_stat_file(stat_file_path, stat_);
		}

		void update_cpu_use()
		{
			const auto period = cpu_time_.get().period();

			const auto time = stat_.utime + stat_.stime;

			cpu_use_ = static_cast<float>(time - last_times_) / period * 100.0F;

			if (not std::isnormal(cpu_use_)) { cpu_use_ = 0.0; }

			// Parent processes gather all children CPU usage => cpu_use_ >> 100
			cpu_use_ = std::clamp(cpu_use_, static_cast<float>(0.0), static_cast<float>(100.0));

			last_times_ = time;

			last_update_ = std::chrono::high_resolution_clock::now();
		}

		void update_st_uid()
		{
			struct ::stat sstat;

			int ret = ::stat(path_.c_str(), &sstat);

			if (std::cmp_equal(ret, -1))
			{
				std::stringstream msg;
				msg << "Error retrieving st_uid from " << path_ << ": " << strerror(errno);
				throw std::runtime_error(msg.str());
			}

			st_uid_ = sstat.st_uid;
		}

		void update_list_of_tasks()
		{
			// A task cannot have tasks
			if (task_) { return; }

			tasks_.clear();

			// Tasks is a directory with subfolders named after the thread IDs
			for (const auto & entry : std::filesystem::directory_iterator(path_ / "task"))
			{
				if (not entry.is_directory()) { continue; }

				const auto tid = std::strtol(entry.path().filename().c_str(), nullptr, 10);

				if (std::cmp_equal(tid, pid_)) { continue; }

				tasks_.emplace_back(tid);
			}
		}

		void update_list_of_children()
		{
			children_ = {};

			const auto children_path = task_ ? path_ / "children" : path_ / "task" / std::to_string(pid_) / "children";

			// Children is a file with a list of PIDs
			std::ifstream file(children_path, std::ios::in);

			if (not file.is_open())
			{
				std::stringstream msg;
				msg << "Error opening file " << children_path << ": " << strerror(errno);
				throw std::runtime_error(msg.str());
			}

			for (pid_t child_pid = 0; file >> child_pid;)
			{
				children_.emplace_back(child_pid);
			}
		}

		[[nodiscard]] auto is_migratable() const -> bool
		{
			// Bad PID
			if (std::cmp_less(pid_, 1)) { return false; }
			// Root can do anything
			if (std::cmp_equal(uid(), 0)) { return true; }

			return std::cmp_equal(st_uid_, uid());
		}

		[[nodiscard]] auto obtain_cmdline()
		{
			try
			{
				std::ifstream cmdline_file(path_ / "cmdline");

				std::ostringstream cmdline_stream;

				ranges::copy(ranges::istream_view<std::string>(cmdline_file),
				             ranges::ostream_iterator<std::string>(cmdline_stream, " "));

				// Remove trailing whitespace(s)
				auto cmdline = cmdline_stream.str();

				static constexpr auto whitespaces{ " \t\f\v\n\r" };

				const auto it = cmdline.find_last_not_of(whitespaces);
				if (it not_eq std::string::npos) { cmdline.erase(it + 1); }

				return cmdline;
			}
			catch (std::exception & e)
			{
				std::stringstream msg;
				msg << "Error retrieving cmdline from PID " << pid_ << ": " << e.what();
				throw std::runtime_error(msg.str());
			}
			catch (...)
			{
				std::stringstream msg;
				msg << "Error retrieving cmdline from PID " << pid_ << ": Unknown error";
				throw std::runtime_error(msg.str());
			}
		}

		[[nodiscard]] auto is_userland_lwp() const { return std::cmp_not_equal(pid_, stat_.pgrp); }

		[[nodiscard]] auto is_kernel_lwp() const { return static_cast<bool>(stat_.flags & PF_KTHREAD); }

	public:
		process() = delete;

		explicit process(const pid_t pid, const CPU_time_provider & cpu_time) :
		    cpu_time_(cpu_time),
		    pid_(pid),
		    path_(std::filesystem::path{ DEFAULT_PROC } / std::to_string(pid)),
		    migratable_(is_migratable()),
		    // First guess to know if it is a LWP
		    lwp_(not std::filesystem::exists(std::filesystem::path{ DEFAULT_PROC } / std::to_string(pid))),
		    task_(path_.string().find("task") != std::string::npos),
		    cmdline_(obtain_cmdline())
		{
			update();
			effective_ppid_ = stat_.ppid;

			// Update lwp after parsing the stat file
			lwp_ = is_userland_lwp() or is_kernel_lwp();
		}

		process(const pid_t pid, std::filesystem::path path, const CPU_time_provider & cpu_time) :
		    cpu_time_(cpu_time),
		    pid_(pid),
		    path_(std::move(path)),
		    migratable_(is_migratable()),
		    // First guess to know if it is a LWP
		    lwp_(not std::filesystem::exists(std::filesystem::path{ DEFAULT_PROC } / std::to_string(pid))),
		    task_(path_.string().find("task") != std::string::npos),
		    cmdline_(obtain_cmdline())
		{
			update();

			lwp_ = is_userland_lwp() or is_kernel_lwp();

			if (task_)
			{
				// Find the parent PID of the LWP -> /proc/<pid>/task/<tid>
				const auto parent_path = path_.parent_path().parent_path();

				effective_ppid_ = static_cast<pid_t>(std::strtol(parent_path.filename().c_str(), nullptr, 10));
			}
			else { effective_ppid_ = stat_.ppid; }
		}

		[[nodiscard]] auto cmdline() const { return cmdline_; }

		[[nodiscard]] auto pid() const -> pid_t { return pid_; }

		[[nodiscard]] auto ppid() const -> pid_t { return stat_.ppid; }

		[[nodiscard]] auto effective_ppid() const -> pid_t { return effective_ppid_; }

		[[nodiscard]] auto processor() const
		{
			return pinned_processor_.has_value() ? pinned_processor_.value() : stat_.processor;
		}

		[[nodiscard]] auto numa_node() const
		{
			return pinned_numa_node_.has_value() ? pinned_numa_node_.value() : numa_node_of_cpu(stat_.processor);
		}

		[[nodiscard]] auto cpu_use() const { return cpu_use_; }

		[[nodiscard]] auto path() const { return path_; }

		[[nodiscard]] auto lwp() const { return lwp_; }

		[[nodiscard]] auto migratable() const { return migratable_; }

		[[nodiscard]] auto running() const { return stat_.state == 'R'; }

		[[nodiscard]] auto stat_info() const -> const auto & { return stat_; }

		[[nodiscard]] auto last_update() const { return last_update_; }

		void update()
		{
			// Update the values from the stat file
			read_stat_file();
			// Update the CPU usage
			update_cpu_use();
			// Update the st_uid
			update_st_uid();
			// Update the list of tasks
			update_list_of_tasks();
			// Update the list of children
			update_list_of_children();
		}

		[[nodiscard]] auto children() const { return children_ | ranges::to<std::set<pid_t>>(); }

		[[nodiscard]] auto add_child(const pid_t pid)
		{
			// If the child is already in the children/tasks list, do nothing
			if (ranges::contains(children_, pid) or ranges::contains(tasks_, pid) or std::cmp_equal(pid, pid_))
			{
				return;
			}
			children_.emplace_back(pid);
			ranges::actions::sort(children_);
		}

		[[nodiscard]] auto tasks() const { return tasks_ | ranges::to<std::set<pid_t>>(); }

		[[nodiscard]] auto add_task(const pid_t pid)
		{
			// If the child is already in the children/tasks list, do nothing
			if (ranges::contains(children_, pid) or ranges::contains(tasks_, pid) or std::cmp_equal(pid, pid_))
			{
				return;
			}
			tasks_.emplace_back(pid);
			ranges::actions::sort(tasks_);
		}

		[[nodiscard]] auto children_and_tasks() const { return ranges::views::concat(children_, tasks_); }

		void pin_processor(const int processor)
		{
			if (pinned_processor_.has_value() and std::cmp_equal(pinned_processor_.value(), processor)) { return; }

			cpu_set_t affinity;

			CPU_ZERO(&affinity);
			CPU_SET(static_cast<size_t>(processor), &affinity);

			if (__glibc_unlikely(sched_setaffinity(pid_, sizeof(cpu_set_t), &affinity)))
			{
				throw std::runtime_error(set_affinity_error(pid_));
			}

			pinned_processor_ = processor;
		}

		void pin_processor()
		{
			if (pinned_processor_.has_value()) { return; }

			pin_processor(stat_.processor);
		}

		void pin_numa_node(const int numa_node)
		{
			if (pinned_numa_node_.has_value() and std::cmp_equal(pinned_numa_node_.value(), numa_node)) { return; }

			std::unique_ptr<bitmask, decltype(&numa_free_cpumask)> cpus(numa_allocate_cpumask(), numa_free_cpumask);

			if (__glibc_unlikely(std::cmp_equal(numa_node_to_cpus(numa_node, cpus.get()), -1)))
			{
				std::stringstream msg;
				msg << "Error retrieving cpus from node " << numa_node << ": " << strerror(errno);
				throw std::runtime_error(msg.str());
			}

			if (__glibc_unlikely(numa_sched_setaffinity(pid_, cpus.get())))
			{
				throw std::runtime_error(set_affinity_error(pid_));
			}

			pinned_numa_node_ = numa_node;
		}

		void pin_numa_node()
		{
			if (pinned_numa_node_.has_value()) { return; }

			pin_numa_node(numa_node());
		}

		void unpin()
		{
			if (not pinned_processor_.has_value() and not pinned_numa_node_.has_value()) { return; }

			cpu_set_t affinity;
			sched_getaffinity(0, sizeof(cpu_set_t),
			                  &affinity); // Gets this process' affinity (supposed to be the default)

			pinned_processor_ = std::nullopt;
			pinned_numa_node_ = std::nullopt;

			if (__glibc_unlikely(sched_setaffinity(pid_, sizeof(cpu_set_t), &affinity)))
			{
				throw std::runtime_error(set_affinity_error(pid_));
			}
		}

		friend auto operator<<(std::ostream & os, const process & p) -> std::ostream &
		{
			const auto to_string_wide = [](const auto & value, const auto width) -> std::string {
				std::stringstream ss;
				ss << std::setw(width) << std::right << value;
				return ss.str();
			};

			const auto to_string_decimal = [](const auto & value, const int precision) {
				using type_t = decltype(value);

				const auto integer_part = std::floor(value);
				const auto decimal_part =
				    (value - integer_part) * std::pow(static_cast<type_t>(10), static_cast<type_t>(precision));

				std::stringstream ss;
				ss << static_cast<int64_t>(integer_part) << "." << std::setw(precision) << std::setfill('0')
				   << std::right << decimal_part;
				return ss.str();
			};

			os << "PID " << to_string_wide(p.pid(), 6) << " ";
			os << "PPID " << to_string_wide(p.ppid(), 6) << " ";
			os << "NODE " << to_string_wide(p.numa_node(), 2) << " ";
			os << "CPU " << to_string_wide(p.processor(), 3) << " ";
			os << "(" << to_string_decimal(p.cpu_use(), 1) << "%) ";
			os << "LWP " << to_string_wide(p.lwp_, 5) << " ";
			os << "CMDLINE " << p.cmdline();

			return os;
		}
	};
} // namespace prox