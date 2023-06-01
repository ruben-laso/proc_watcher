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

		[[nodiscard]] inline auto children(const pid_t pid) -> decltype(std::declval<process>().children())
		{
			const auto proc_it = processes_.find(pid);

			// If the process is found, return a reference to its children
			if (proc_it not_eq processes_.end())
			{
				const auto & [pid_, proc_ptr] = *proc_it;

				return proc_ptr->children();
			}

			// Otherwise, try to create a new process
			try
			{
				const auto & process = insert(pid);
				return process->children();
			}
			catch (const std::exception & e)
			{
				// Couldn't create a new process
				return {};
			}
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