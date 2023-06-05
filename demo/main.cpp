#include <chrono>
#include <thread>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <proc_watcher/proc_watcher.hpp>
#include <proc_watcher/process.hpp>

template<typename F>
auto measure(F && f)
{
	const auto start = std::chrono::high_resolution_clock::now();
	f();
	const auto end = std::chrono::high_resolution_clock::now();
	return std::chrono::duration<double>(end - start).count();
}

auto main() -> int
{
	spdlog::info("Demo of proc_watcher");

	const auto pid = getpid();

#ifndef NDEBUG
	spdlog::set_level(spdlog::level::debug);
#endif

	proc_watcher::process_tree processes;

	proc_watcher::process process(pid);

	while (true)
	{
		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(1s);

		const auto millis_global = measure([&] { processes.update(); }) * 1e3;
		spdlog::info("Global update for {} processes took {:.5f}ms", processes.size(), millis_global);

		const auto millis_local = measure([&] { processes.update(pid); }) * 1e3;
		spdlog::info("Update from this process (PID {}) took {:.5f}ms", pid, millis_local);

		spdlog::debug("Process tree with {} entries.", processes.size());

		// spdlog::info("Processes tree:");
		// const auto millis_print_tree = measure([&] { std::cout << processes << '\n'; }) * 1e3;
		// spdlog::info("Print tree took {:.5f}ms", millis_print_tree);

		const auto millis_proc = measure([&] { process.update(); }) * 1e3;
		spdlog::info("Update this process (PID {}) took {:.5f}ms", pid, millis_proc);

		spdlog::info("This process info: {}", process);

		// Print the most CPU consuming processes
		const auto & cpu_proc =
		    *ranges::max_element(processes, std::less{}, [&](const auto & proc) { return proc.cpu_use(); });

		spdlog::info("Most CPU consuming PID {}: {:>.2f}%", cpu_proc.pid(), cpu_proc.cpu_use());
	}

	return EXIT_SUCCESS;
}
