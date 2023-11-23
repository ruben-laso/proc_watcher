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

#include "cpu_time.hpp"
#include "process.hpp"

namespace proc_watcher
{
	static const auto write_into_bool_vector = [](std::vector<bool> & map, const auto & key, const auto & value) {
		if (map.size() <= key) { map.resize(key + 1, false); }
		map[key] = value;
	};

	// taken from https://stackoverflow.com/a/478960
	template<size_t buff_length = 128>
	auto exec_cmd(const std::string_view cmd, const bool truncate_final_newlines = true) -> std::string
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

		static constexpr pid_t DEFAULT_ROOT = 1;

		CPU_time cpu_time_ = {};

		pid_t root_ = DEFAULT_ROOT;

		std::map<pid_t, std::shared_ptr<process>> processes_ = {};

		void insert(const std::shared_ptr<process> & proc_)
		{
			// Check if the process is already in the tree
			if (const auto find_it = processes_.find(proc_->pid()); find_it not_eq processes_.end())
			{
				// If the process is already in the tree, nothing to do...
				return;
			}

			// Add the process to the tree
			const auto & [it, inserted] = processes_.try_emplace(proc_->pid(), proc_);

			auto & [pid, proc] = *it;

			// Add their tasks and children as well (if not already in the tree)
			for (const auto & task : proc->tasks())
			{
				const auto task_path = proc->path() / std::to_string(proc->pid()) / "task";
				std::ignore = processes_.try_emplace(task, std::make_shared<process>(task, task_path.string(), cpu_time_));
			}

			for (const auto & child : proc->children())
			{
				std::ignore = processes_.try_emplace(child, std::make_shared<process>(child, cpu_time_));
			}
		}

		void print_level(std::ostream & os, const process & p, const size_t level = 0) const
		{
			static constexpr size_t TAB_SIZE = 3;

			for (const auto i : ranges::views::indices(size_t(), level * TAB_SIZE))
			{
				os << ((i % TAB_SIZE == 0 and i > 0) ? TREE_STR_VERT : " ");
			}

			if (level not_eq 0) { os << TREE_STR_RTEE << TREE_STR_HORZ << " "; }

			os << p << '\n';

			for (const auto & child : p.children_and_tasks())
			{
				const auto & child_proc = get(child);
				if (not child_proc.has_value()) { continue; }
				print_level(os, *child_proc.value(), level + 1);
			}
		}

	public:
		process_tree() { insert(root_); }

		explicit process_tree(const pid_t pid, const std::string_view path = "/proc") : root_(pid)
		{
			insert(root_, path);
		}

		[[nodiscard]] auto begin() const
		{
			auto proc_view = processes_ | ranges::views::values | ranges::views::indirect;
			return proc_view.begin();
		}

		[[nodiscard]] auto end() const
		{
			auto proc_view = processes_ | ranges::views::values | ranges::views::indirect;
			return proc_view.end();
		}

		[[nodiscard]] auto size() const { return processes_.size(); }

		[[nodiscard]] auto root() const -> pid_t { return root_; }

		[[nodiscard]] auto processes() const { return processes_ | ranges::views::values | ranges::views::indirect; }

		auto insert(const pid_t pid, const std::string_view path = "/proc") -> std::shared_ptr<process>
		{
			// Try to find it within the process tree. If the process is found, nothing to do...
			if (const auto proc_it = processes_.find(pid); proc_it not_eq processes_.end()) { return proc_it->second; }

			// Otherwise, try to create a new process
			auto proc_ptr = std::make_shared<process>(pid, path, cpu_time_);
			insert(proc_ptr);
			return proc_ptr;
		}

		[[nodiscard]] auto get(const pid_t pid) -> std::optional<std::shared_ptr<process>>
		{
			// If the process is found, return a reference to it
			if (const auto & proc_it = processes_.find(pid); proc_it not_eq processes_.end())
			{
				return { proc_it->second };
			}

			// Otherwise, try to create a new process
			try
			{
				return { insert(pid) };
			}
			catch (...)
			{
				// Couldn't create a new process
				return {};
			}
		}

		[[nodiscard]] auto get(const pid_t pid) const -> std::optional<std::shared_ptr<process>>
		{
			// If the process is found, return a reference to it
			if (const auto & proc_it = processes_.find(pid); proc_it not_eq processes_.end())
			{
				return { proc_it->second };
			}

			// Otherwise, return an empty optional
			return {};
		}

		[[nodiscard]] auto children(const pid_t pid)
		{
			if (const auto opt_proc = get(pid); not opt_proc.has_value()) { return opt_proc.value()->children(); }

			return std::set<pid_t>{};
		}

		[[nodiscard]] auto tasks(const pid_t pid)
		{
			if (const auto opt_proc = get(pid); not opt_proc.has_value()) { return opt_proc.value()->tasks(); }

			return std::set<pid_t>{};
		}

		[[nodiscard]] auto children_and_tasks(const pid_t pid)
		{
			if (const auto opt_proc = get(pid); not opt_proc.has_value())
			{
				return opt_proc.value()->children_and_tasks() | ranges::to<std::set<pid_t>>;
			}

			return std::set<pid_t>{};
		}

		[[nodiscard]] auto all_children_of(const pid_t pid_) const -> std::set<pid_t>
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

				for (const auto & task : proc.tasks())
				{
					q.push(task);
					children.insert(task);
				}

				for (const auto & child : proc.children())
				{
					q.push(child);
					children.insert(child);
				}
			}

			return children;
		}

		[[nodiscard]] auto alive(const pid_t pid) const { return processes_.contains(pid); }

		template<typename T>
		auto do_or_throw(const pid_t pid, T && func)
		{
			const auto & proc_it = processes_.find(pid);

			// If the process is not found, throw an exception
			if (proc_it == processes_.end()) { throw std::runtime_error("Process not found"); }

			return func(*proc_it->second);
		}

		[[nodiscard]] auto state(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.state(); });
		}

		[[nodiscard]] auto running(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.running(); });
		}

		[[nodiscard]] auto ppid(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.ppid(); });
		}

		[[nodiscard]] auto priority(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.priority(); });
		}

		[[nodiscard]] auto nice(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.nice(); });
		}

		[[nodiscard]] auto processor(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.processor(); });
		}

		[[nodiscard]] auto numa_node(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.numa_node(); });
		}

		[[nodiscard]] auto cpu_use(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.cpu_use(); });
		}

		[[nodiscard]] auto cmdline(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.cmdline(); });
		}

		[[nodiscard]] auto migratable(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.migratable(); });
		}

		[[nodiscard]] auto lwp(const pid_t pid)
		{
			return do_or_throw(pid, [](const auto & proc) { return proc.lwp(); });
		}

		[[nodiscard]] auto pin_processor(const pid_t pid, const int cpu)
		{
			return do_or_throw(pid, [cpu](auto & proc) { return proc.pin_processor(cpu); });
		}

		[[nodiscard]] auto pin_processor(const pid_t pid)
		{
			return do_or_throw(pid, [](auto & proc) { return proc.pin_processor(); });
		}

		[[nodiscard]] auto pin_numa_node(const pid_t pid, const int numa_node)
		{
			return do_or_throw(pid, [numa_node](auto & proc) { return proc.pin_numa_node(numa_node); });
		}

		[[nodiscard]] auto pin_numa_node(const pid_t pid)
		{
			return do_or_throw(pid, [](auto & proc) { return proc.pin_numa_node(); });
		}

		[[nodiscard]] auto unpin(const pid_t pid)
		{
			return do_or_throw(pid, [](auto & proc) { return proc.unpin(); });
		}

		[[nodiscard]] auto unpin()
		{
			for (const auto & proc : ranges::views::values(processes_))
			{
				proc->unpin();
			}
		}

		[[nodiscard]] static auto memory_usage(const pid_t pid)
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

		template<typename Bool_map>
		void update(const pid_t root, Bool_map & updated_pids)
		{
			namespace fs = std::filesystem;

			// Pair for PID and the /proc path
			using pid_path_t = std::pair<pid_t, std::optional<fs::path>>;

			// Queue of PIDs to update
			std::queue<pid_path_t> to_update;
			to_update.emplace(root, std::nullopt);

			while (not to_update.empty())
			{
				const auto [pid, path_opt] = to_update.front();
				to_update.pop();

				// Check if the PID is already updated
				if (updated_pids[pid]) { continue; }

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
					write_into_bool_vector(updated_pids, pid, true);
				}
				catch (...)
				{
					continue;
				}

				// Update its tasks
				for (const auto & task : proc_ptr->tasks())
				{
					to_update.emplace(task, path / std::to_string(pid) / "task");
				}

				// Add the children to the queue
				for (const auto & child : proc_ptr->children())
				{
					to_update.emplace(child, std::nullopt);
				}
			}
		}

		void update()
		{
			namespace fs = std::filesystem;

			cpu_time_.update();

			const auto max_pid = ranges::max(processes_ | ranges::views::keys);

			std::vector<bool> old_pids(max_pid + 1, false);
			ranges::for_each(processes_ | ranges::views::keys, [&](const auto & pid) { old_pids[pid] = true; });

			// Set of updated PIDs to avoid updating the same process twice
			std::vector<bool> updated_pids(max_pid + 1, false);

			for (const auto & entry : fs::directory_iterator("/proc"))
			{
				const auto & path     = entry.path();
				const auto & path_str = path.filename().string();

				// Check if the entry is a directory
				if (not fs::is_directory(path)) { continue; }
				// Check if the entry is a number (checking the first character is enough)
				if (std::isdigit(path_str[0]) == 0) { continue; }

				const pid_t pid = std::stoi(path_str);

				// Check if the PID is already updated
				if (updated_pids[pid]) { continue; }

				// Update in "tree-mode"
				update(pid, updated_pids);
			}

			// Make sure that all processes know their children/tasks
			for (const auto & proc : ranges::views::values(processes_))
			{
				if (proc->pid() == root_) { continue; }

				const auto ppid = proc->ppid();

				// Get the parent process
				auto parent_opt = get(ppid);
				if (not parent_opt.has_value()) { continue; }
				auto & parent = *parent_opt.value();

				// Add the process to the parent
				if (proc->lwp()) { parent.add_task(proc->pid()); }
				else { parent.add_child(proc->pid()); }
			}

			// Set of PIDs to remove from the process tree (processes that could not be parsed)
			std::vector<pid_t> to_remove;

			// Remove the processes that couldn't be updated or that are not in the tree anymore
			ranges::for_each(processes_ | ranges::views::keys, [&](const auto & pid) {
				if (old_pids[pid] and not updated_pids[pid]) { to_remove.emplace_back(pid); }
			});

			ranges::for_each(to_remove, [&](const auto & pid) { erase(pid); });
		}

		friend auto operator<<(std::ostream & os, const process_tree & p) -> std::ostream &
		{
			os << "Process tree with " << p.processes_.size() << " entries." << '\n';
			const auto & root_opt = p.get(p.root());

			if (not root_opt.has_value()) { return os; }

			p.print_level(os, *root_opt.value());

			return os;
		}
	};
} // namespace proc_watcher