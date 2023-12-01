#pragma once

#include <filesystem>

#include "mock_process.hpp"

namespace prox
{
	class Mock_proc_dir
	{
	public:
		const std::filesystem::path mock_proc_dir = std::filesystem::temp_directory_path() / "mock" / "proc";

		enum PIDs : pid_t
		{
			root   = 1,
			task1  = 2,
			task2  = 3,
			child1 = 4,
			child2 = 5
		};

		Mock_proc_dir()
		{
			// Build a mockup of the /proc directory
			prox::process_stat root_proc;
			root_proc.pid  = PIDs::root;
			root_proc.path = mock_proc_dir / std::to_string(root_proc.pid);
			root_proc.name = "root";

			prox::process_stat task1_proc;
			task1_proc.pid  = PIDs::task1;
			task1_proc.path = root_proc.path / "task" / std::to_string(task1_proc.pid);
			task1_proc.name = "task1";

			prox::process_stat task2_proc;
			task2_proc.pid  = PIDs::task2;
			task2_proc.path = root_proc.path / "task" / std::to_string(task2_proc.pid);
			task2_proc.name = "task2";

			root_proc.tasks = { root_proc.pid, task1_proc.pid, task2_proc.pid };

			prox::process_stat child1_proc;
			child1_proc.pid  = PIDs::child1;
			child1_proc.path = mock_proc_dir / std::to_string(child1_proc.pid);
			child1_proc.name = "child1";

			prox::process_stat child2_proc;
			child2_proc.pid  = PIDs::child2;
			child2_proc.path = mock_proc_dir / std::to_string(child2_proc.pid);
			child2_proc.name = "child2";

			root_proc.children = { child1_proc.pid, child2_proc.pid };

			prox::write_mock_process_stat(root_proc);
			prox::write_mock_process_stat(task1_proc);
			prox::write_mock_process_stat(task2_proc);
			prox::write_mock_process_stat(child1_proc);
			prox::write_mock_process_stat(child2_proc);
		}

		~Mock_proc_dir() { std::filesystem::remove_all(mock_proc_dir); }
	};
} // namespace prox