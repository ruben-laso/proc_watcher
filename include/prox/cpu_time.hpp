#pragma once

#include <unistd.h> // for sysconf, _SC_NPROCESSORS_ONLN

#include <fstream> // for ifstream, basic_istream, operator>>, basic_ostream, getline

#include <fmt/core.h> // for format

namespace prox
{
	class CPU_time
	{
	public:
		constexpr static auto FILE_CPU_STAT = "/proc/stat";

	private:
		unsigned long long int user_time_   = 0;
		unsigned long long int nice_time_   = 0;
		unsigned long long int system_time_ = 0;
		unsigned long long int idle_time_   = 0;
		unsigned long long int io_wait_     = 0;
		unsigned long long int irq_         = 0;
		unsigned long long int soft_irq_    = 0;
		unsigned long long int steal_       = 0;
		unsigned long long int guest_       = 0;
		unsigned long long int guest_nice_  = 0;

		unsigned long long int idle_total_time_   = 0;
		unsigned long long int system_total_time_ = 0;
		unsigned long long int virt_total_time_   = 0;
		unsigned long long int total_time_        = 0;
		unsigned long long int total_period_      = 0;

		unsigned long long int last_total_time_ = 0;

		float period_ = 0.0;

		void scan_cpu_time()
		{
			static const auto N_CPUS = sysconf(_SC_NPROCESSORS_ONLN);

			std::ifstream file(FILE_CPU_STAT, std::ios::in);

			if (not file.is_open())
			{
				const auto error_str =
				    fmt::format("Could not open file {}. Error {} ({})", FILE_CPU_STAT, errno, strerror(errno));
				throw std::runtime_error(error_str);
			}

			if (std::cmp_less_equal(N_CPUS, 0))
			{
				const auto error_str = fmt::format("Invalid number of CPUs: {}", N_CPUS);
				throw std::runtime_error(error_str);
			}

			std::string cpu_str; // = "cpu "

			file >> cpu_str;
			file >> user_time_;
			file >> nice_time_;
			file >> system_time_;
			file >> idle_time_;
			file >> io_wait_;
			file >> irq_;
			file >> soft_irq_;
			file >> steal_;
			file >> guest_;
			file >> guest_nice_;

			// Guest time is already accounted in user time
			user_time_ = user_time_ - guest_;
			nice_time_ = nice_time_ - guest_nice_;

			idle_total_time_   = idle_time_ + io_wait_;
			system_total_time_ = system_time_ + irq_ + soft_irq_;
			virt_total_time_   = guest_ + guest_nice_;
			total_time_ = user_time_ + nice_time_ + system_total_time_ + idle_total_time_ + steal_ + virt_total_time_;

			total_period_ = (total_time_ > last_total_time_) ? (total_time_ - last_total_time_) : 1;

			last_total_time_ = total_time_;

			period_ = static_cast<float>(total_period_) / static_cast<float>(N_CPUS);
		}

	public:
		CPU_time() { update(); }

		[[nodiscard]] auto user_time() const { return user_time_; }

		[[nodiscard]] auto nice_time() const { return nice_time_; }

		[[nodiscard]] auto system_time() const { return system_time_; }

		[[nodiscard]] auto idle_time() const { return idle_time_; }

		[[nodiscard]] auto io_wait() const { return io_wait_; }

		[[nodiscard]] auto irq() const { return irq_; }

		[[nodiscard]] auto soft_irq() const { return soft_irq_; }

		[[nodiscard]] auto steal() const { return steal_; }

		[[nodiscard]] auto guest() const { return guest_; }

		[[nodiscard]] auto guest_nice() const { return guest_nice_; }

		[[nodiscard]] auto idle_total_time() const { return idle_total_time_; }

		[[nodiscard]] auto system_total_time() const { return system_total_time_; }

		[[nodiscard]] auto virt_total_time() const { return virt_total_time_; }

		[[nodiscard]] auto total_time() const { return total_time_; }

		[[nodiscard]] auto total_period() const { return total_period_; }

		[[nodiscard]] auto last_total_time() const { return last_total_time_; }

		[[nodiscard]] auto period() const { return period_; }

		void update() { scan_cpu_time(); }
	};

} // namespace prox