#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>

namespace prox
{
	struct process_stat
	{
		using lint  = long int;
		using ulint = unsigned long int;
		using ull   = unsigned long long int;

		pid_t                 pid  = 123456789;
		std::filesystem::path path = std::filesystem::temp_directory_path() / std::to_string(pid);

		std::string name        = "my-mock-pid";
		char        state       = 'S';
		pid_t       ppid        = 4456;
		uint        pgrp        = 4487;
		uint        session     = 4487;
		uint        tty         = 34816;
		int         tpgid       = 13349;
		ulint       flags       = 4194304;
		ulint       minflt      = 48695;
		ulint       cminflt     = 385441;
		ulint       majflt      = 77;
		ulint       cmajflt     = 353;
		ull         utime       = 142;
		ull         stime       = 88;
		ull         cutime      = 486;
		ull         cstime      = 406;
		lint        priority    = 20;
		lint        nice        = 0;
		lint        num_threads = 1;
		int         itrealvalue = 0;
		ull         starttime   = 29218;

		// Next values are not parsed
		ull vsize      = 23072768;
		ull rss        = 3432;
		ull rsslim     = 184467440737095;
		ull startcode  = 94317919137792;
		ull endcode    = 94317919912838;
		ull startstack = 140733960279152;
		ull kstkesp    = 0;
		ull kstkeip    = 0;
		ull signal     = 0;
		ull blocked    = 2;
		ull sigignore  = 3686400;
		ull sigcatch   = 134295555;
		ull wchan      = 1;
		ull nswap      = 0;
		ull cnswap     = 0;
		// End of non-parsed values

		int exit_signal           = 17;
		int processor             = 6;
		int rt_priority           = 0;
		int policy                = 0;
		int delayacct_blkio_ticks = 0;
		int guest_time            = 0;
		int cguest_time           = 0;
		ull start_data            = 94317920029408;
		ull end_data              = 94317920058604;
		ull start_brk             = 94317926449152;
		ull arg_start             = 140733960280484;
		ull arg_end               = 140733960280488;
		ull env_start             = 140733960280488;
		ull env_end               = 140733960282091;
		int exit_code             = 0;

		std::set<pid_t> children = {};

		std::set<pid_t> tasks = {};
	};

	static void write_mock_process_stat(process_stat & process)
	{
		std::stringstream file_content;

		// Make the directory (if it doesn't exist)
		std::filesystem::create_directories(process.path);

		if (not std::filesystem::exists(process.path))
		{
			throw std::runtime_error("The directory " + process.path.string() + " could not be created");
		}

		// Generate the cmdline file
		file_content << process.name << std::endl;

		const auto    cmdline_path = process.path / "cmdline";
		std::ofstream out(cmdline_path);
		out << file_content.str();
		out.close();

		// Check that the file was written correctly
		if (not std::filesystem::exists(cmdline_path))
		{
			throw std::runtime_error("The cmdline file was not created correctly in \"" + cmdline_path.string() + "\"");
		}

		// Generate the stat file
		file_content.str("");
		file_content
		    << process.pid << " (" << process.name << ") " << process.state << " " << process.ppid << " "
		    << process.pgrp << " " << process.session << " " << process.tty << " " << process.tpgid << " "
		    << process.flags << " " << process.minflt << " " << process.cminflt << " " << process.majflt << " "
		    << process.cmajflt << " " << process.utime << " " << process.stime << " " << process.cutime << " "
		    << process.cstime << " " << process.priority << " " << process.nice << " " << process.num_threads << " "
		    << process.itrealvalue << " " << process.starttime << " " << process.vsize << " " << process.rss << " "
		    << process.rsslim << " " << process.startcode << " " << process.endcode << " " << process.startstack << " "
		    << process.kstkesp << " " << process.kstkeip << " " << process.signal << " " << process.blocked << " "
		    << process.sigignore << " " << process.sigcatch << " " << process.wchan << " " << process.nswap << " "
		    << process.cnswap << " " << process.exit_signal << " " << process.processor << " " << process.rt_priority
		    << " " << process.policy << " " << process.delayacct_blkio_ticks << " " << process.guest_time << " "
		    << process.cguest_time << " " << process.start_data << " " << process.end_data << " " << process.start_brk
		    << " " << process.arg_start << " " << process.arg_end << " " << process.env_start << " " << process.env_end
		    << " " << process.exit_code << std::endl;

		// Write the "stat" file
		const auto folder_path = process.path / "task" / std::to_string(process.pid);
		std::filesystem::create_directories(folder_path);
		const auto stat_path = process.path / "task" / std::to_string(process.pid) / "stat";

		out.open(stat_path);
		out << file_content.str();
		out.close();

		// Check that the file was written correctly
		if (not std::filesystem::exists(stat_path))
		{
			throw std::runtime_error("The stat file was not created correctly in \"" + stat_path.string() + "\"");
		}

		// Generate the "children" file
		file_content.str("");
		for (const auto & child : process.children)
		{
			file_content << child << " ";
		}

		const auto children_path = process.path / "task" / std::to_string(process.pid) / "children";

		out.open(children_path);
		out << file_content.str();
		out.close();

		// Check that the file was written correctly
		if (!std::filesystem::exists(children_path))
		{
			throw std::runtime_error("The children file was not created correctly in \"" + children_path.string() +
			                         "\"");
		}
	}
} // namespace prox