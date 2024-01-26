#pragma once

#include <iostream>

#include <filesystem>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <range/v3/all.hpp>

#include "cpu_time.hpp"
#include "process.hpp"

namespace prox
{
	static const auto write_into_bool_vector = [](std::vector<bool> & vec, const auto & pos, const auto & value) {
		if (std::cmp_less(pos, 0)) { throw std::runtime_error("Cannot write into a negative position"); }
		const auto pos_ = static_cast<std::size_t>(pos);
		if (std::cmp_less_equal(vec.size(), pos)) { vec.resize(pos_ + 1, false); }
		vec.at(pos_) = value;
	};

	static const auto read_from_bool_vector = [](const std::vector<bool> & vec, const auto & pos) -> bool {
		if (std::cmp_less(pos, 0)) { throw std::runtime_error("Cannot read from a negative position"); }
		const auto pos_ = static_cast<std::size_t>(pos);
		if (std::cmp_less_equal(vec.size(), pos)) { return false; }
		return vec.at(pos_);
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
		using proc_t     = process<CPU_time>;
		using proc_ptr_t = std::shared_ptr<proc_t>;

		template<typename... Args>
		[[nodiscard]] static auto make_proc_ptr(Args &&... args)
		{
			return std::make_shared<proc_t>(args...);
		}

		static constexpr const char * TREE_STR_HORZ = "\xe2\x94\x80"; // TREE_STR_HORZ ─
		static constexpr const char * TREE_STR_VERT = "\xe2\x94\x82"; // TREE_STR_VERT │
		static constexpr const char * TREE_STR_RTEE = "\xe2\x94\x9c"; // TREE_STR_RTEE ├
		static constexpr const char * TREE_STR_BEND = "\xe2\x94\x94"; // TREE_STR_BEND └
		static constexpr const char * TREE_STR_TEND = "\xe2\x94\x8c"; // TREE_STR_TEND ┌
		static constexpr const char * TREE_STR_OPEN = "+";            // TREE_STR_OPEN +
		static constexpr const char * TREE_STR_SHUT = "\xe2\x94\x80"; // TREE_STR_SHUT ─

		static constexpr pid_t DEFAULT_ROOT      = 1;
		static constexpr auto  DEFAULT_PROC_PATH = "/proc";

		pid_t root_ = DEFAULT_ROOT;

		std::filesystem::path proc_path_ = DEFAULT_PROC_PATH;

		CPU_time cpu_time_ = {};

		std::map<pid_t, proc_ptr_t> processes_ = {};

		void insert(const proc_ptr_t & proc_)
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
				const auto task_path = proc->path() / "task" / std::to_string(task);
				std::ignore          = processes_.try_emplace(task, make_proc_ptr(task, task_path.string(), cpu_time_));
			}

			for (const auto & child : proc->children())
			{
				std::ignore = processes_.try_emplace(child, make_proc_ptr(child, cpu_time_));
			}
		}

		void notify_parents()
		{
			// Make sure that all processes know their children/tasks
			auto to_notify = [&]() {
				std::queue<pid_t> q = {};
				for (const auto pid : processes_ | ranges::views::keys)
				{
					q.push(pid);
				}
				return q;
			}();

			while (not to_notify.empty())
			{
				const auto pid = to_notify.front();
				to_notify.pop();

				// The root process has no parent
				if (pid == root_) { continue; }

				// Get the process
				const auto proc_opt = get(pid);
				if (not proc_opt.has_value()) { continue; }
				auto & proc = *proc_opt.value();

				// Notify to the parent
				const auto ppid = proc.effective_ppid();

				// Get the parent process
				auto parent_opt = get(ppid);
				if (not parent_opt.has_value()) { continue; }
				auto & parent = *parent_opt.value();

				// Add the process to the parent
				if (proc.lwp()) { parent.add_task(proc.pid()); }
				else { parent.add_child(proc.pid()); }

				to_notify.push(ppid);
			}
		}

		template<typename Bool_map>
		void tree_update(const pid_t root, Bool_map & updated_pids)
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
				if (read_from_bool_vector(updated_pids, pid)) { continue; }

				// Find the process
				auto proc_it = processes_.find(pid);

				// If the process is not found, try to create a new one
				auto path = path_opt.value_or(proc_path_ / std::to_string(pid));

				proc_ptr_t proc_ptr;

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
					to_update.emplace(task, path / "task" / std::to_string(task));
				}

				// Add the children to the queue
				for (const auto & child : proc_ptr->children())
				{
					to_update.emplace(child, std::nullopt);
				}
			}
		}

		void tree_update(const pid_t root)
		{
			namespace fs = std::filesystem;

			cpu_time_.update();

			const auto max_pid = processes_.empty() ? 99'999 : ranges::max(processes_ | ranges::views::keys);

			// old_pids = all children and tasks of "root"
			std::vector<bool> old_pids(static_cast<std::size_t>(max_pid + 1), false);
			ranges::for_each(processes_ | ranges::views::keys,
			                 [&](const auto & pid) { write_into_bool_vector(old_pids, pid, true); });

			// Set of updated PIDs to avoid updating the same process twice
			std::vector<bool> updated_pids(static_cast<size_t>(max_pid + 1), false);

			// Update in "tree-mode"
			tree_update(root, updated_pids);

			// Make sure all processes know their children/tasks
			notify_parents();

			// Remove the processes that couldn't be updated or that are not in the tree anymore
			const auto condition_to_remove = [&](const auto & pid) {
				return read_from_bool_vector(old_pids, pid) and not read_from_bool_vector(updated_pids, pid);
			};

			const auto to_remove = processes_ | ranges::views::keys | ranges::views::filter(condition_to_remove) |
			                       ranges::to<std::vector<pid_t>>;

			ranges::for_each(to_remove, [&](const auto & pid) { erase(pid); });
		}

		void full_update()
		{
			namespace fs = std::filesystem;

			cpu_time_.update();

			const auto max_pid = processes_.empty() ? 99'999 : ranges::max(processes_ | ranges::views::keys);

			std::vector<bool> old_pids(static_cast<std::size_t>(max_pid + 1), false);
			ranges::for_each(processes_ | ranges::views::keys,
			                 [&](const auto & pid) { write_into_bool_vector(old_pids, pid, true); });

			// Set of updated PIDs to avoid updating the same process twice
			std::vector<bool> updated_pids(static_cast<size_t>(max_pid + 1), false);

			for (const auto & entry : fs::directory_iterator(proc_path_))
			{
				// Check if the entry is a directory
				if (not fs::is_directory(entry)) { continue; }

				const auto & path = entry.path().filename().string();

				// Check if the entry is a number (checking the first character is enough)
				if (std::isdigit(path[0]) == 0) { continue; }

				const pid_t pid = std::stoi(path);

				// Check if the PID is already updated
				if (read_from_bool_vector(updated_pids, pid)) { continue; }

				// Update in "tree-mode"
				tree_update(pid, updated_pids);
			}

			// Make sure that all processes know their children/tasks
			notify_parents();

			// Remove the processes that couldn't be updated or that are not in the tree anymore
			const auto condition_to_remove = [&](const auto & pid) {
				return read_from_bool_vector(old_pids, pid) and not read_from_bool_vector(updated_pids, pid);
			};

			const auto to_remove = processes_ | ranges::views::keys | ranges::views::filter(condition_to_remove) |
			                       ranges::to<std::vector<pid_t>>;

			ranges::for_each(to_remove, [&](const auto & pid) { erase(pid); });
		}

		void print_level(std::ostream & os, const proc_t & p, const size_t level = 0) const
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
		process_tree()
		{
			// Check that the proc path exists
			if (not std::filesystem::exists(proc_path_) or not std::filesystem::is_directory(proc_path_))
			{
				throw std::runtime_error("The proc path \"" + proc_path_.string() + "\" is not valid");
			}

			update();

			// Check that the root process exists
			if (not processes_.contains(root_))
			{
				throw std::runtime_error("The root process \"" + std::to_string(root_) + "\" is not valid");
			}

			// Check that the tree is not empty
			if (processes_.empty()) { throw std::runtime_error("The process tree is empty"); }
		}

		process_tree(const pid_t root) : process_tree(root, DEFAULT_PROC_PATH) {}

		process_tree(const pid_t root, std::filesystem::path proc_path) : root_(root), proc_path_(std::move(proc_path))
		{
			// Check that the proc path exists
			if (not std::filesystem::exists(proc_path_) or not std::filesystem::is_directory(proc_path_))
			{
				throw std::runtime_error("The proc path \"" + proc_path_.string() + "\" is not valid");
			}

			update();

			// Check that the root process exists
			if (not processes_.contains(root_))
			{
				throw std::runtime_error("The root process \"" + std::to_string(root_) + "\" is not valid");
			}

			// Check that the tree is not empty
			if (processes_.empty()) { throw std::runtime_error("The process tree is empty"); }
		}

		auto find(const pid_t pid) -> auto &
		{
			const auto & proc_it = processes_.find(pid);

			// If the process is not found, throw an exception
			if (proc_it == processes_.end()) { throw std::runtime_error("Process not found"); }

			return *proc_it->second;
		}

		auto find(const pid_t pid) const -> const auto &
		{
			const auto & proc_it = processes_.find(pid);

			// If the process is not found, throw an exception
			if (proc_it == processes_.end()) { throw std::runtime_error("Process not found"); }

			return *proc_it->second;
		}

		[[nodiscard]] auto root() const -> pid_t { return root_; }

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

		[[nodiscard]] auto processes() const { return processes_ | ranges::views::values | ranges::views::indirect; }

		auto insert(const pid_t pid, const std::filesystem::path & path) -> proc_ptr_t
		{
			// Try to find it within the process tree. If the process is found, nothing to do...
			if (const auto proc_it = processes_.find(pid); proc_it not_eq processes_.end()) { return proc_it->second; }

			// Otherwise, try to create a new process
			auto proc_ptr = make_proc_ptr(pid, path, cpu_time_);
			insert(proc_ptr);
			return proc_ptr;
		}

		auto insert(const pid_t pid) -> proc_ptr_t { return insert(pid, proc_path_ / std::to_string(pid)); }

		[[nodiscard]] auto get(const pid_t pid) -> std::optional<proc_ptr_t>
		{
			// If the process is found, return a reference to it
			const auto & proc_it = processes_.find(pid);
			if (proc_it not_eq processes_.end()) { return { proc_it->second }; }

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

		[[nodiscard]] auto get(const pid_t pid) const -> std::optional<proc_ptr_t>
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
				const auto p = q.front();
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

		[[nodiscard]] auto stat(const pid_t pid) -> const auto & { return find(pid).stat_info(); }

		[[nodiscard]] auto running(const pid_t pid) { return find(pid).running(); }

		[[nodiscard]] auto ppid(const pid_t pid) { return find(pid).ppid(); }

		[[nodiscard]] auto processor(const pid_t pid) { return find(pid).processor(); }

		[[nodiscard]] auto numa_node(const pid_t pid) { return find(pid).numa_node(); }

		[[nodiscard]] auto cpu_use(const pid_t pid) { return find(pid).cpu_use(); }

		[[nodiscard]] auto cmdline(const pid_t pid) { return find(pid).cmdline(); }

		[[nodiscard]] auto migratable(const pid_t pid) { return find(pid).migratable(); }

		[[nodiscard]] auto lwp(const pid_t pid) { return find(pid).lwp(); }

		[[nodiscard]] auto pin_processor(const pid_t pid, const int cpu) { return find(pid).pin_processor(cpu); }

		[[nodiscard]] auto pin_processor(const pid_t pid) { return find(pid).pin_processor(); }

		[[nodiscard]] auto pin_numa_node(const pid_t pid, const int numa_node)
		{
			return find(pid).pin_numa_node(numa_node);
		}

		[[nodiscard]] auto pin_numa_node(const pid_t pid) { return find(pid).pin_numa_node(); }

		[[nodiscard]] auto unpin(const pid_t pid) { return find(pid).unpin(); }

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

			std::vector<float> mem_usage(static_cast<std::size_t>(numa_max_node() + 1), {});

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

		void update()
		{
			if (std::cmp_equal(root_, DEFAULT_ROOT)) { full_update(); }
			else { tree_update(root_); }
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
} // namespace prox