#pragma once

#include <algorithm>
#include <vector>
#include <ranges>

namespace Zenith::Utils {

	template<std::ranges::range Range, typename T>
	inline bool Contains(const Range& range, const T& item)
	{
		return std::ranges::find(range, item) != std::ranges::end(range);
	}

	template<std::ranges::range Range, typename T>
	inline bool AppendIfNotPresent(Range& range, const T& item)
	{
		if (Contains(range, item))
			return false;

		range.push_back(item);
		return true;
	}

	template<typename T, typename Predicate>
	inline bool RemoveIf(std::vector<T>& vec, Predicate pred)
	{
		auto it = std::ranges::find_if(vec, pred);
		if (it != vec.end())
		{
			vec.erase(it);
			return true;
		}
		return false;
	}

	template<std::ranges::range Range, typename T>
	inline bool Remove(Range& range, const T& item)
	{
		auto it = std::ranges::find(range, item);
		if (it == std::ranges::end(range))
			return false;

		range.erase(it);
		return true;
	}

}
