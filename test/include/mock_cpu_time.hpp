#pragma once

#include <gmock/gmock.h>

#include <filesystem>

namespace prox
{
	// Build a mockup of the cpu_time class (it doesn't have any virtual methods)
	class Mock_cpu_time
	{
		using ull = unsigned long long int;

	public:
		MOCK_METHOD(ull, user_time, (), (const));
		MOCK_METHOD(ull, nice_time, (), (const));
		MOCK_METHOD(ull, system_time, (), (const));
		MOCK_METHOD(ull, idle_time, (), (const));
		MOCK_METHOD(ull, io_wait, (), (const));
		MOCK_METHOD(ull, irq, (), (const));
		MOCK_METHOD(ull, soft_irq, (), (const));
		MOCK_METHOD(ull, steal, (), (const));
		MOCK_METHOD(ull, guest, (), (const));
		MOCK_METHOD(ull, guest_nice, (), (const));
		MOCK_METHOD(ull, idle_total_time, (), (const));
		MOCK_METHOD(ull, system_total_time, (), (const));
		MOCK_METHOD(ull, virt_total_time, (), (const));
		MOCK_METHOD(ull, total_time, (), (const));
		MOCK_METHOD(ull, total_period, (), (const));
		MOCK_METHOD(ull, last_total_time, (), (const));
		MOCK_METHOD(float, period, (), (const));
		MOCK_METHOD(void, update, (const std::filesystem::path &), ());
	};

	/**
 * @brief Get a mockup of the cpu_time class with 0s for all values
 *
 * @return std::unique_ptr<Mock_cpu_time>
 */
	auto get_empty_mock_cpu_time() -> std::unique_ptr<Mock_cpu_time>
	{
		auto mock_cpu_time_ptr = std::make_unique<Mock_cpu_time>();

		auto & mock_cpu_time = *mock_cpu_time_ptr;

		ON_CALL(mock_cpu_time, user_time()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, nice_time()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, system_time()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, idle_time()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, io_wait()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, irq()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, soft_irq()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, steal()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, guest()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, guest_nice()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, idle_total_time()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, system_total_time()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, virt_total_time()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, total_time()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, total_period()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, last_total_time()).WillByDefault(testing::Return(0));
		ON_CALL(mock_cpu_time, period()).WillByDefault(testing::Return(0.0F));
		ON_CALL(mock_cpu_time, update(testing::_)).WillByDefault(testing::Return());

		return mock_cpu_time_ptr;
	}

	/**
 * @brief Get a mockup of the cpu_time class with some values
 *
 * @return std::unique_ptr<Mock_cpu_time>
 */
	auto get_mock_cpu_time() -> std::unique_ptr<Mock_cpu_time>
	{
		auto mock_cpu_time_ptr = std::make_unique<Mock_cpu_time>();

		auto & mock_cpu_time = *mock_cpu_time_ptr;

		const auto N_CPUS = 12;

		const unsigned long long int user_time   = 1816560;
		const unsigned long long int nice_time   = 4773;
		const unsigned long long int system_time = 518338;
		const unsigned long long int idle_time   = 23132813;
		const unsigned long long int io_wait     = 31707;
		const unsigned long long int irq         = 0;
		const unsigned long long int soft_irq    = 49840;
		const unsigned long long int steal       = 0;
		const unsigned long long int guest       = 0;
		const unsigned long long int guest_nice  = 0;

		const unsigned long long int last_total_time = 0;

		// Guest time is already accounted in user time
		const auto real_user_time = user_time - guest;
		const auto real_nice_time = nice_time - guest_nice;

		const auto idle_total_time   = idle_time + io_wait;
		const auto system_total_time = system_time + irq + soft_irq;
		const auto virt_total_time   = guest + guest_nice;
		const auto total_time =
		    real_user_time + real_nice_time + system_total_time + idle_total_time + steal + virt_total_time;

		const auto total_period = total_time - last_total_time;

		const auto period = static_cast<float>(total_period) / static_cast<float>(N_CPUS);

		ON_CALL(mock_cpu_time, user_time()).WillByDefault(testing::Return(real_user_time));
		ON_CALL(mock_cpu_time, nice_time()).WillByDefault(testing::Return(real_nice_time));
		ON_CALL(mock_cpu_time, system_time()).WillByDefault(testing::Return(system_time));
		ON_CALL(mock_cpu_time, idle_time()).WillByDefault(testing::Return(idle_time));
		ON_CALL(mock_cpu_time, io_wait()).WillByDefault(testing::Return(io_wait));
		ON_CALL(mock_cpu_time, irq()).WillByDefault(testing::Return(irq));
		ON_CALL(mock_cpu_time, soft_irq()).WillByDefault(testing::Return(soft_irq));
		ON_CALL(mock_cpu_time, steal()).WillByDefault(testing::Return(steal));
		ON_CALL(mock_cpu_time, guest()).WillByDefault(testing::Return(guest));
		ON_CALL(mock_cpu_time, guest_nice()).WillByDefault(testing::Return(guest_nice));
		ON_CALL(mock_cpu_time, idle_total_time()).WillByDefault(testing::Return(idle_total_time));
		ON_CALL(mock_cpu_time, system_total_time()).WillByDefault(testing::Return(system_total_time));
		ON_CALL(mock_cpu_time, virt_total_time()).WillByDefault(testing::Return(virt_total_time));
		ON_CALL(mock_cpu_time, total_time()).WillByDefault(testing::Return(total_time));
		ON_CALL(mock_cpu_time, total_period()).WillByDefault(testing::Return(total_period));
		ON_CALL(mock_cpu_time, last_total_time()).WillByDefault(testing::Return(last_total_time));
		ON_CALL(mock_cpu_time, period()).WillByDefault(testing::Return(period));
		ON_CALL(mock_cpu_time, update(testing::_)).WillByDefault(testing::Return());

		return mock_cpu_time_ptr;
	}
} // namespace prox