#include <chrono>
#include <thread>

#include <csignal>

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <CLI/CLI.hpp>

#include <range/v3/all.hpp>

#include <prox/prox.hpp>

// Options structure
struct Options
{
	static constexpr auto DEFAULT_DEBUG     = false;
	static constexpr auto DEFAULT_PROFILE   = false;
	static constexpr auto DEFAULT_PARTIAL   = false;
	static constexpr auto DEFAULT_MIGRATION = false;

	static constexpr auto DEFAULT_TIME    = 30.0;
	static constexpr auto DEFAULT_DT      = 1.0;
	static constexpr auto DEFAULT_CPU_USE = -1.0;

	bool debug     = DEFAULT_DEBUG;
	bool profile   = DEFAULT_PROFILE;
	bool partial   = DEFAULT_PARTIAL;
	bool migration = DEFAULT_MIGRATION;

	float time    = DEFAULT_TIME;
	float dt      = DEFAULT_DT;
	float cpu_use = DEFAULT_CPU_USE;

	std::string child_process{};
};

Options options;

struct Global
{
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();

	prox::process_tree processes;

	pid_t child_pid = 0;
};

Global global;

void clean_end(const int signal, [[maybe_unused]] siginfo_t * const info, [[maybe_unused]] void * const context)
{
	if (std::cmp_equal(signal, SIGCHLD))
	{
		spdlog::info("Child process (PID {}) ended.", global.child_pid);
		global.child_pid = 0;
		exit(EXIT_SUCCESS);
	}
}

auto run_child(const std::string & command)
{
	if (const auto pid = fork(); pid == 0)
	{
		// Child process
		execlp(command.c_str(), command.c_str(), nullptr);
	}
	else if (pid > 0)
	{
		// Parent process
		global.child_pid = pid;
		spdlog::info("Child process (PID {}) started.", global.child_pid);
		// Register signal handler
		struct sigaction action
		{};
		action.sa_sigaction = clean_end;
		sigemptyset(&action.sa_mask);
		action.sa_flags = SA_SIGINFO;
		sigaction(SIGCHLD, &action, nullptr);
	}
	else
	{
		// Error
		const auto err = errno;
		const auto msg = fmt::format("Failed to fork child process: Error {} ({})", err, strerror(err));
		spdlog::error(msg);
		throw std::runtime_error(msg);
	}
}

void parse_options(CLI::App & app, const int argc, const char * argv[])
{
	app.add_flag("-d,--debug", options.debug, "Debug output");
	app.add_flag("-p,--profile", options.profile, "Profile children processes");
	app.add_flag("-m,--migration", options.migration, "Migrate child process to random CPU");

	app.add_flag("--partial", options.partial, "Partial update");

	app.add_option("-t,--time", options.time, "Time to run (seconds) the demo for");
	app.add_option("-s,--dt", options.dt, "Time step (seconds) for the demo");
	app.add_option("-c,--cpu", options.cpu_use, "Minimum CPU usage (0-100%) to show processes");

	app.add_option("-r,--run", options.child_process, "Child process (command) to run");

	app.parse(argc, argv);

	if (options.debug) { spdlog::set_level(spdlog::level::debug); }

	if (not options.child_process.empty()) { run_child(options.child_process); }

	if (options.debug)
	{
		spdlog::debug("Options:");
		spdlog::debug("\tDebug: {}", options.debug);
		spdlog::debug("\tProfile: {}", options.profile);
		spdlog::debug("\tPartial: {}", options.partial);
		spdlog::debug("\tTime: {}", options.time);
		spdlog::debug("\tTime step: {}", options.dt);
		spdlog::debug("\tCPU usage: {}", options.cpu_use);
		if (options.child_process.empty()) { spdlog::debug("\tChild process: None"); }
		else { spdlog::debug("\tChild process (PID {}): {}", global.child_pid, options.child_process); }
	}
}


template<typename T = double>
auto measure(auto && operation)
{
	const auto start = std::chrono::high_resolution_clock::now();
	operation();
	const auto end = std::chrono::high_resolution_clock::now();
	return std::chrono::duration<T>(end - start).count();
}

template<typename T>
auto format_seconds(const T & seconds)
{
	if (seconds > 100) { return fmt::format("{:.0f}s", seconds); }
	if (seconds > 10) { return fmt::format("{:.1f}s", seconds); }
	if (seconds > 1) { return fmt::format("{:.2f}s", seconds); }
	if (seconds > 1e-3) { return fmt::format("{:.0f}ms", seconds * 1e3); }
	if (seconds > 1e-6) { return fmt::format("{:.0f}us", seconds * 1e6); }
	return fmt::format("{:.0f}ns", seconds * 1e9);
}

auto keep_running()
{
	// If child process is running, keep running
	if (global.child_pid > 0) { return true; }

	// Else, keep running until time is up
	const auto now     = std::chrono::high_resolution_clock::now();
	const auto elapsed = std::chrono::duration<decltype(options.time)>(now - global.start_time).count();
	return elapsed < options.time;
}

void update_tree()
{
	const auto millis_global   = measure([&] { global.processes.update(); });
	const auto millis_per_proc = millis_global / static_cast<double>(global.processes.size());
	spdlog::info("{} update for {} processes took {} ({} per process)", options.partial ? "Partial" : "Full",
	             global.processes.size(), format_seconds(millis_global), format_seconds(millis_per_proc));

	spdlog::debug("Process tree with {} entries.", global.processes.size());

	// Print the process tree
	if (options.debug)
	{
		spdlog::info("Processes tree:");
		const auto millis_print_tree = measure([&] { std::cout << global.processes << '\n'; });
		spdlog::info("Print tree took {}", format_seconds(millis_print_tree));
	}
}

void most_CPU_consuming_procs()
{
	const auto high_cpu_use = [&](const auto & proc) {
		return proc.cpu_use() > options.cpu_use;
	};

	auto heavy_proc = global.processes | ranges::views::filter(high_cpu_use);

	spdlog::info("Most CPU consuming processes ({}%):", options.cpu_use);
	for (const auto & cpu_proc : heavy_proc)
	{
		const auto update_seconds_ago =
		    std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - cpu_proc.last_update()).count();
		spdlog::info("\tPID {}. Update {} ago. CPU {} at {:>.2f}%. \"{}\"", cpu_proc.pid(),
		             format_seconds(update_seconds_ago), cpu_proc.processor(), cpu_proc.cpu_use(), cpu_proc.cmdline());
	}
}

void print_children_info()
{
	// If the child process does NOT exist, return -> Nothing to do...
	if (std::cmp_equal(global.child_pid, 0))
	{
		spdlog::warn("No child process to profile.");
		return;
	}

	// Print information about the child process
	spdlog::info("Child process(es):");
	const auto & child_opt = global.processes.get(global.child_pid);

	if (not child_opt)
	{
		spdlog::error("\tPID {} not found in process tree.", global.child_pid);
		return;
	}

	for (const auto & child_pid : child_opt->get()->children_and_tasks())
	{
		const auto & proc_opt = global.processes.get(child_pid);

		if (not proc_opt)
		{
			spdlog::error("\tPID {} not found in process tree.", child_pid);
			continue;
		}

		const auto & proc = proc_opt->get();
		spdlog::info("\tPID {}. CPU {} at {:>.2f}%. \"{}\"", proc->pid(), proc->processor(), proc->cpu_use(),
		             proc->cmdline());
	}
}

void migrate_random_child()
{
	const auto child_proc_opt = global.processes.get(global.child_pid);

	if (not child_proc_opt)
	{
		spdlog::error("Child process (PID {}) not found in process tree.", global.child_pid);
		return;
	}

	const auto & child_proc    = child_proc_opt->get();
	const auto & children_pids = child_proc->children_and_tasks();

	// Select a random CPU
	static const auto n_cpus = std::thread::hardware_concurrency();

	const auto cpu = static_cast<int>(*(ranges::views::indices(n_cpus) | ranges::views::sample(1)).begin());
	const auto pid = children_pids.empty() ? global.child_pid : *(children_pids | ranges::views::sample(1)).begin();

	spdlog::info("Migrating child process (PID {}) to CPU {}", pid, cpu);

	// Migrate the child process to the selected CPU
	global.processes.pin_processor(pid, cpu);

	spdlog::info("Child process (PID {}) migrated to CPU {}", pid, cpu);
}

auto main(const int argc, const char * argv[]) -> int
{
	try
	{
		CLI::App app{ "Demo of prox" };

		try
		{
			parse_options(app, argc, argv);
		}
		catch (const CLI::ParseError & e)
		{
			return app.exit(e);
		}
		catch (...)
		{
			return EXIT_FAILURE;
		}

		spdlog::info("Demo of prox");

		if (options.partial) { global.processes = prox::process_tree{ getpid() }; }

		auto sleep_time = options.dt;

		while (keep_running())
		{
			using namespace std::literals::chrono_literals;
			std::this_thread::sleep_for(sleep_time * 1s);

			const auto loop_time = measure<float>([&] {
				update_tree();

				if (options.cpu_use > 0.0F) { most_CPU_consuming_procs(); }

				if (options.profile) { print_children_info(); }

				if (options.migration) { migrate_random_child(); }
			});

			sleep_time = options.dt - loop_time;
		}
	}
	catch (const std::exception & e)
	{
		spdlog::error("Exception: {}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
