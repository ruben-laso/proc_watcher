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
			prox::process_stat root;
			root.pid  = PIDs::root;
			root.path = mock_proc_dir / std::to_string(root.pid);
			root.name = "root";

			prox::process_stat task1;
			task1.pid  = PIDs::task1;
			task1.path = root.path / "task" / std::to_string(task1.pid);
			task1.name = "task1";

			prox::process_stat task2;
			task2.pid  = PIDs::task2;
			task2.path = root.path / "task" / std::to_string(task2.pid);
			task2.name = "task2";

			root.tasks = { root.pid, task1.pid, task2.pid };

			prox::process_stat child1;
			child1.pid  = PIDs::child1;
			child1.path = mock_proc_dir / std::to_string(child1.pid);
			child1.name = "child1";

			prox::process_stat child2;
			child2.pid  = PIDs::child2;
			child2.path = mock_proc_dir / std::to_string(child2.pid);
			child2.name = "child2";

			root.children = { child1.pid, child2.pid };

			prox::write_mock_process_stat(root);
			prox::write_mock_process_stat(task1);
			prox::write_mock_process_stat(task2);
			prox::write_mock_process_stat(child1);
			prox::write_mock_process_stat(child2);
		}

		~Mock_proc_dir() { std::filesystem::remove_all(mock_proc_dir); }
	};
} // namespace prox