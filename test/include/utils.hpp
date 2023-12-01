#pragma once

#include <range/v3/all.hpp>

namespace utils
{
	// Assumes that both ranges are sorted
	auto equivalent_sets(const auto & rng_a, const auto & rng_b, auto cmp) -> bool
	{
		const auto diff = ranges::views::set_symmetric_difference(rng_a, rng_b, cmp);

		return ranges::empty(diff);
	}

	// Assumes that both ranges are sorted
	template<typename RNG_T, typename RNG_U>
	auto equivalent_sets(const RNG_T & rng_a, const RNG_U & rng_b) -> bool
	{
		using T = typename RNG_T::value_type;
		return equivalent_sets(rng_a, rng_b, std::less<T>{});
	}

	auto equivalent_rngs(const auto & rng_a, const auto & rng_b, auto cmp) -> bool
	{
		auto rng_a_view = rng_a | ranges::views::all;
		auto rng_b_view = rng_b | ranges::views::all;

		auto sorted_rng_a_view = rng_a_view | ranges::to<std::vector> | ranges::actions::sort;
		auto sorted_rng_b_view = rng_b_view | ranges::to<std::vector> | ranges::actions::sort;

		return equivalent_sets(sorted_rng_a_view, sorted_rng_b_view, cmp);
	}

	auto equivalent_rngs(const auto & rng_a, const auto & rng_b) -> bool
	{
		return equivalent_rngs(rng_a, rng_b, std::less<>{});
	}

} // namespace utils
