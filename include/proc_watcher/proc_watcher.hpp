#pragma once

#include <iostream>

#include <filesystem>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <range/v3/all.hpp>

#include "process.hpp"

namespace proc_watcher
{
	// taken from https://stackoverflow.com/a/478960
	template<size_t buff_length = 128>
	inline auto exec_cmd(std::string_view cmd, const bool truncate_final_newlines = true) -> std::string
	{
		std::array<char, buff_length> buffer{};

		std::string result;

		const std::unique_ptr<std::FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);

		if (pipe == nullptr) { throw std::runtime_error("Could not execute command " + std::string(cmd)); }

		while (fgets(buffer.data(), buffer.size(), pipe.get()) not_eq nullptr)
		{
			result += buffer.data();
		}

		if (truncate_final_newlines)
		{
			// Remove last '\n' (if there is one)
			while (result[result.length() - 1] == '\n')
			{
				result.pop_back();
			}
		}

		return result;
	}

	class process_tree
	{
		static constexpr const char * TREE_STR_HORZ = "\xe2\x94\x80"; // TREE_STR_HORZ ─
		static constexpr const char * TREE_STR_VERT = "\xe2\x94\x82"; // TREE_STR_VERT │
		static constexpr const char * TREE_STR_RTEE = "\xe2\x94\x9c"; // TREE_STR_RTEE ├
		static constexpr const char * TREE_STR_BEND = "\xe2\x94\x94"; // TREE_STR_BEND └
		static constexpr const char * TREE_STR_TEND = "\xe2\x94\x8c"; // TREE_STR_TEND ┌
		static constexpr const char * TREE_STR_OPEN = "+";            // TREE_STR_OPEN +
		static constexpr const char * TREE_STR_SHUT = "\xe2\x94\x80"; // TREE_STR_SHUT ─

		static constexpr pid_t ROOT = 1;

		std::map<pid_t, std::shared_ptr<process>> processes_ = {};

		std::shared_ptr<process> root_;

		void insert(const std::shared_ptr<process> & proc_, std::shared_ptr<process> parent_proc_ = nullptr)
		{
			// Check if the process is already in the tree
			const auto find_it = processes_.find(proc_->pid());

			if (find_it not_eq processes_.end())
			{
				// If the process is already in the tree, nothing to do...
				return;
			}

			// Add the process to the tree
			const auto & [it, inserted] = processes_.try_emplace(proc_->pid(), proc_);

			auto & [pid, proc] = *it;

			// Add their tasks and children as well (if not already in the tree)
			const auto & tasks = proc->tasks();

			for (const auto & task : tasks)
			{
				const auto task_path = proc->path() / std::to_string(proc->pid()) / "task";
				std::ignore = processes_.try_emplace(task, std::make_shared<process>(task, task_path.string()));
			}

			const auto & children = proc->children();

			for (const auto & child : children)
			{
				std::ignore = processes_.try_emplace(child, std::make_shared<process>(child));
			}
		}

		inline void print_level(std::ostream & os, const process & p, const size_t level = 0) const
		{
			static constexpr size_t TAB_SIZE = 3;

			for (const auto i : ranges::views::indices(size_t(), level * TAB_SIZE))
			{
				os << ((i % TAB_SIZE == 0 and i > 0) ? TREE_STR_VERT : " ");
			}

			if (level not_eq 0) { os << TREE_STR_RTEE << TREE_STR_HORZ << " "; }

			os << p << '\n';

			const auto & proc_children = p.children_and_tasks();
			for (const auto & child : proc_children)
			{
				const auto & child_proc = get(child);
				if (not child_proc.has_value()) { continue; }
				print_level(os, *child_proc.value(), level + 1);
			}
		}

	public:
		process_tree() : root_(std::make_shared<process>(ROOT)) { processes_.emplace(ROOT, root_); }

		explicit process_tree(const pid_t pid) : root_(std::make_shared<process>(pid))
		{
			processes_.emplace(pid, root_);
		}

		[[nodiscard]] inline auto begin() const
		{
			auto proc_view = processes_ | ranges::views::values | ranges::views::indirect;
			return proc_view.begin();
			// return processes_.begin();
		}

		[[nodiscard]] inline auto end() const
		{
			auto proc_view = processes_ | ranges::views::values | ranges::views::indirect;
			return proc_view.end();
			// return processes_.end();
		}

		[[nodiscard]] inline auto size() const { return processes_.size(); }

		[[nodiscard]] inline auto root() const -> pid_t { return root_->pid(); }

		[[nodiscard]] inline auto processes() const
		{
			return processes_ | ranges::views::values | ranges::views::indirect;
		}

		inline auto insert(const pid_t pid, const std::string_view path = "/proc") -> std::shared_ptr<process>
		{
			// Try to find it within the process tree
			const auto proc_it = processes_.find(pid);

			// If the process is found, nothing to do...
			if (proc_it not_eq processes_.end()) { return proc_it->second; }

			// Otherwise, try to create a new process
			auto proc_ptr = std::make_shared<process>(pid, path);
			insert(proc_ptr);
			return proc_ptr;
		}

		[[nodiscard]] inline auto get(const pid_t pid) -> std::optional<std::shared_ptr<process>>
		{
			const auto & proc_it = processes_.find(pid);

			// If the process is found, return a reference to it
			if (proc_it not_eq processes_.end()) { return { proc_it->second }; }

			// Otherwise, try to create a new process
			try
			{
				return { insert(pid) };
			}
			catch (const std::exception & e)
			{
				// Couldn't create a new process
				return {};
			}
		}

		[[nodiscard]] inline auto get(const pid_t pid) const -> std::optional<std::shared_ptr<process>>
		{
			const auto & proc_it = processes_.find(pid);

			// If the process is found, return a reference to it
			if (proc_it not_eq processes_.end()) { return { proc_it->second }; }

			// Otherwise, return an empty optional
			return {};
		}

		[[nodiscard]] inline auto children(const pid_t pid)
		{
			const auto opt_proc = get(pid);

			if (not opt_proc.has_value()) { return opt_proc.value()->children(); }

			return std::set<pid_t>{};
		}

		[[nodiscard]] inline auto tasks(const pid_t pid)
		{
			const auto opt_proc = get(pid);

			if (not opt_proc.has_value()) { return opt_proc.value()->tasks(); }

			return std::set<pid_t>{};
		}

		[[nodiscard]] inline auto children_and_tasks(const pid_t pid)
		{
			const auto opt_proc = get(pid);

			if (not opt_proc.has_value())
			{
				return opt_proc.value()->children_and_tasks() | ranges::to<std::set<pid_t>>;
			}

			return std::set<pid_t>{};
		}

		[[nodiscard]] inline auto all_children_of(const pid_t pid_) const -> std::set<pid_t>
		{
			std::set<pid_t> children = {};

			std::queue<pid_t> q = {};
			q.push(pid_);

			while (not q.empty())
			{
				const auto & p = q.front();
				q.pop();

				const auto opt_proc_ptr = get(p);

				if (not opt_proc_ptr.has_value()) { continue; }

				const auto & proc = *opt_proc_ptr.value();

				const auto & p_tasks = proc.tasks();

				for (const auto & task : p_tasks)
				{
					q.push(task);
					children.insert(task);
				}

				const auto & p_children = proc.children();

				for (const auto & child : p_children)
				{
					q.push(child);
					children.insert(child);
				}
			}

			return children;
		}

		[[nodiscard]] inline auto alive(const pid_t pid) { return processes_.contains(pid); }

		template<typename T>
		inline auto do_or_throw(const pid_t pid, T && func)
		{
			const auto & proc_it = processes_.find(pid);

			// If the process is not found, throw an exception
			if (proc_it == processes_.end()) { throw std::runtime_error("Process not found"); }

			return func(*proc_it->second);
		}

		[[nodiscard]] inline auto state(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.state(); });
		}

		[[nodiscard]] inline auto running(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.running(); });
		}

		[[nodiscard]] inline auto ppid(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.ppid(); });
		}

		[[nodiscard]] inline auto priority(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.priority(); });
		}

		[[nodiscard]] inline auto nice(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.nice(); });
		}

		[[nodiscard]] inline auto processor(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.processor(); });
		}

		[[nodiscard]] inline auto numa_node(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.numa_node(); });
		}

		[[nodiscard]] inline auto cpu_use(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.cpu_use(); });
		}

		[[nodiscard]] inline auto cmdline(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.cmdline(); });
		}

		[[nodiscard]] inline auto migratable(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.migratable(); });
		}

		[[nodiscard]] inline auto lwp(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.lwp(); });
		}

		[[nodiscard]] inline auto pin_processor(const pid_t pid, const int cpu)
		{
			return do_or_throw(pid, [cpu](auto & proc) { return proc.pin_processor(cpu); });
		}

		[[nodiscard]] inline auto pin_processor(const pid_t pid)
		{
			return do_or_throw(pid, [](auto & proc) { return proc.pin_processor(); });
		}

		[[nodiscard]] inline auto pin_numa_node(const pid_t pid, const int numa_node)
		{
			return do_or_throw(pid, [numa_node](auto & proc) { return proc.pin_numa_node(numa_node); });
		}

		[[nodiscard]] inline auto pin_numa_node(const pid_t pid)
		{
			return do_or_throw(pid, [](auto & proc) { return proc.pin_numa_node(); });
		}

		[[nodiscard]] inline auto unpin(const pid_t pid)
		{
			return do_or_throw(pid, [](auto & proc) { return proc.unpin(); });
		}

		[[nodiscard]] inline auto unpin()
		{
			for (auto & [pid, proc] : processes_)
			{
				proc->unpin();
			}
		}

		[[nodiscard]] static inline auto memory_usage(const pid_t pid)
		{
			static constexpr auto MB_TO_B = 1024 * 1024;

			std::vector<float> mem_usage(numa_max_node() + 1, {});

			// Get amount of memory allocated (in MBs)
			const auto command = "(NUMASTAT_WIDTH=1000 numastat -p " + std::to_string(pid) + " 2> /dev/null) " +
			                     " | tail -n 1 | grep -P -o '[0-9]+.[0-9]+'";
			const auto output = exec_cmd(command);

			std::istringstream is(output);

			for (auto & mem : mem_usage)
			{
				float amount = {};
				is >> amount;

				mem = amount * MB_TO_B;
			}

			return mem_usage;
		}

		void erase(const pid_t pid)
		{
			const auto proc_it = processes_.find(pid);
			if (proc_it == processes_.end())
			{
				// Process not found, nothing to do
				return;
			}

			// Get the process
			processes_.erase(proc_it);
		}

		inline void update(const pid_t root = ROOT)
		{
			namespace fs = std::filesystem;

			// Pair for PID and the /proc path
			using pid_path_t = std::pair<pid_t, std::optional<fs::path>>;

			// Get all the children of the root PID to know which processes had died since the last update
			std::set<pid_t> all_children_of_pid = all_children_of(root);

			// Set of updated PIDs to avoid updating the same process twice
			std::set<pid_t> updated_pids;

			// Set of PIDs to remove from the process tree (processes that could not be parsed)
			std::set<pid_t> to_remove;

			// Queue of PIDs to update
			std::queue<pid_path_t> to_update;
			to_update.emplace(root, std::nullopt);

			while (not to_update.empty())
			{
				const auto [pid, path_opt] = to_update.front();
				to_update.pop();

				// Check if the PID is already updated
				if (updated_pids.find(pid) not_eq updated_pids.end()) { continue; }

				// Find the process
				auto proc_it = processes_.find(pid);

				// If the process is not found, try to create a new one
				auto path = path_opt.value_or("/proc");

				std::shared_ptr<process> proc_ptr;

				try
				{
					// Insert the process or update it
					if (proc_it == processes_.end()) { proc_ptr = insert(pid, path.string()); }
					else
					{
						proc_ptr = proc_it->second;
						// Update the process
						proc_ptr->update();
					}

					// Add the PID to the updated PIDs
					updated_pids.emplace(pid);
				}
				catch ([[maybe_unused]] const std::exception & e)
				{
					// Couldn't create a new process
					// Remove the process from the tree
					to_remove.emplace(pid);
					continue;
				}

				// Update its tasks
				const auto & tasks = proc_ptr->tasks();

				for (const auto & task : tasks)
				{
					// Check if
					to_update.emplace(task, path / std::to_string(pid) / "task");
				}

				// Add the children to the queue
				const auto & children = proc_ptr->children();

				for (const auto & child : children)
				{
					to_update.emplace(child, std::nullopt);
				}
			}

			// Check not updated processes
			ranges::set_difference(all_children_of_pid, updated_pids, std::inserter(to_remove, to_remove.end()));

			// Remove the processes that couldn't be updated
			// or that are not in the tree anymore
			for (const auto & pid : to_remove)
			{
				erase(pid);
			}
		}

		friend auto operator<<(std::ostream & os, const process_tree & p) -> std::ostream &
		{
			os << "Process tree with " << p.processes_.size() << " entries." << '\n';
			const auto & root_proc = *p.root_;
			p.print_level(os, root_proc);

			return os;
		}
	};
} // namespace proc_watcher