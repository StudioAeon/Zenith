#pragma once

#include <filesystem>

namespace Zenith::Utils {

	//============================================================
	// String Utilities
	//============================================================
	namespace String {
		bool EqualsIgnoreCase(const std::string_view a, const std::string_view b);
		std::string& ToLower(std::string& string);
		std::string ToLowerCopy(std::string_view string);
		void Erase(std::string& str, const char* chars);
		void Erase(std::string& str, const std::string& chars);
		std::string SubStr(const std::string& string, size_t offset, size_t count = std::string::npos);
		std::string TrimWhitespace(const std::string& str);
		std::string RemoveWhitespace(const std::string& str);
		std::string GetCurrentTimeString(bool includeDate = false, bool useDashes = false);
		int32_t CompareCase(std::string_view a, std::string_view b);
	}

	std::string_view GetFilename(const std::string_view filepath);
	std::string GetExtension(const std::string& filename);
	std::string RemoveExtension(const std::string& filename);

	std::vector<std::string> SplitStringAndKeepDelims(std::string str); // Keeps delimiters except spaces
	std::vector<std::string> SplitString(const std::string_view string, const std::string_view& delimiters);
	std::vector<std::string> SplitString(const std::string_view string, const char delimiter);
	std::string SplitAtUpperCase(std::string_view string, std::string_view delimiter = " ", bool ifLowerCaseOnTheRight = true);
	std::string ToLower(std::string_view string);
	std::string ToUpper(std::string_view string);
	std::string BytesToString(uint64_t bytes);

	//============================================================
	// File I/O
	//============================================================
	int SkipBOM(std::istream& in);
	std::string ReadFileAndSkipBOM(const std::filesystem::path& filepath);

	//============================================================
	// Time and Duration
	//============================================================
	template<class...Durations, class DurationIn>
	std::tuple<Durations...> BreakDownDuration(DurationIn d)
	{
		std::tuple<Durations...> retval;
		using discard = int[];
		(void)discard
		{
			0, (void((
				std::get<Durations>(retval) = std::chrono::duration_cast<Durations>(d),
				d -= std::chrono::duration_cast<DurationIn>(std::get<Durations>(retval))
				)), 0)...
		};
		return retval;
	}

	std::string DurationToString(std::chrono::duration<double> duration);

	//============================================================
	// String Manipulation & Naming
	//============================================================
	template <typename IsAlreadyUsedFn>
	std::string AddSuffixToMakeUnique(const std::string& name, IsAlreadyUsedFn&& isUsed)
	{
		std::string nameToUse = name;
		int suffix = 1;

		while (isUsed(nameToUse))
			nameToUse = name + "_" + std::to_string(++suffix);

		return nameToUse;
	}

	std::string TemplateToParenthesis(std::string_view name);          // Get<float> -> Get (Float)
	std::string CreateUserFriendlyTypeName(std::string_view name);    // e.g., SoundGraph::Get<float> -> Get (Float)

	//============================================================
	// Constexpr Utilities
	//============================================================
	constexpr bool StartsWith(std::string_view t, std::string_view s)
	{
		return t.size() >= s.size() && t.substr(0, s.size()) == s;
	}

	constexpr bool EndsWith(std::string_view t, std::string_view s)
	{
		return t.size() >= s.size() && t.substr(t.size() - s.size()) == s;
	}

	constexpr size_t GetNumberOfTokens(std::string_view source, std::string_view delimiter)
	{
		size_t count = 1;
		for (size_t i = 0; i + delimiter.size() <= source.size(); ++i)
			if (source.substr(i, delimiter.size()) == delimiter)
				++count;
		return count;
	}

	template<size_t N>
	constexpr std::array<std::string_view, N> SplitString(std::string_view source, std::string_view delimiter)
	{
		std::array<std::string_view, N> tokens{};
		size_t tokenIndex = 0;
		size_t start = 0, end;

		while ((end = source.find(delimiter, start)) != std::string_view::npos && tokenIndex < N - 1)
		{
			tokens[tokenIndex++] = source.substr(start, end - start);
			start = end + delimiter.size();
		}
		tokens[tokenIndex] = source.substr(start);
		return tokens;
	}

	constexpr std::string_view RemoveNamespace(std::string_view name)
	{
		const auto pos = name.find_last_of(':');
		return (pos == std::string_view::npos) ? name : name.substr(pos + 1);
	}

	constexpr std::string_view RemoveOuterNamespace(std::string_view name)
	{
		const auto first = name.find_first_of(':');
		if (first == std::string_view::npos)
			return name;
		return (first < name.size() - 1 && name[first + 1] == ':') ? name.substr(first + 2) : name.substr(first + 1);
	}

	template<size_t N>
	constexpr std::array<std::string_view, N> RemoveNamespace(std::array<std::string_view, N> memberList)
	{
		for (std::string_view& fullName : memberList)
			fullName = RemoveNamespace(fullName);
		return memberList;
	}

	constexpr std::string_view RemovePrefixAndSuffix(std::string_view name)
	{
		if (StartsWith(name, "in_"))
			name.remove_prefix(3);
		else if (StartsWith(name, "out_"))
			name.remove_prefix(4);

		if (EndsWith(name, "_Raw"))
			name.remove_suffix(4);

		return name;
	}

}
