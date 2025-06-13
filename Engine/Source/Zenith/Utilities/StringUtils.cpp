#include "znpch.hpp"
#include "StringUtils.hpp"

#include <filesystem>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <regex>

namespace Zenith::Utils {

	namespace String {

		bool EqualsIgnoreCase(const std::string_view a, const std::string_view b)
		{
			if (a.size() != b.size()) return false;
			return std::equal(a.begin(), a.end(), b.begin(), b.end(),
				[](const char a, const char b) { return std::tolower(a) == std::tolower(b); });
		}

		std::string& ToLower(std::string& str)
		{
			std::transform(str.begin(), str.end(), str.begin(),
				[](const unsigned char c) { return std::tolower(c); });
			return str;
		}

		std::string ToLowerCopy(const std::string_view str)
		{
			std::string result(str);
			ToLower(result);
			return result;
		}

		void Erase(std::string& str, const char* chars)
		{
			for (size_t i = 0; i < strlen(chars); ++i)
				str.erase(std::remove(str.begin(), str.end(), chars[i]), str.end());
		}

		void Erase(std::string& str, const std::string& chars)
		{
			Erase(str, chars.c_str());
		}

		std::string SubStr(const std::string& str, size_t offset, size_t count)
		{
			if (offset == std::string::npos) return str;
			if (offset >= str.length()) return str;
			return str.substr(offset, count);
		}

		const std::string WHITESPACE = " \n\r\t\f\v";

		std::string TrimWhitespace(const std::string& str)
		{
			size_t start = str.find_first_not_of(WHITESPACE);
			std::string trimmed = (start == std::string::npos) ? "" : str.substr(start);

			size_t end = trimmed.find_last_not_of(WHITESPACE);
			return (end == std::string::npos) ? "" : trimmed.substr(0, end + 1);
		}

		std::string RemoveWhitespace(const std::string& str)
		{
			std::string result = str;
			Erase(result, WHITESPACE);
			return result;
		}

		std::string GetCurrentTimeString(bool includeDate, bool useDashes)
		{
			time_t currentTime = time(NULL);
			std::stringstream timeString;
			tm* timeBuffer = localtime(&currentTime);
			timeString << std::put_time(timeBuffer, includeDate ? "%Y:%m:%d:%T" : "%T");
			std::string str = timeString.str();

			if (useDashes) std::replace(str.begin(), str.end(), ':', '-');
			return str;
		}

		int32_t CompareCase(std::string_view a, std::string_view b)
		{
#ifdef ZN_PLATFORM_WINDOWS
			return _stricmp(a.data(), b.data());
#else
			return strcasecmp(a.data(), b.data());
#endif
		}

	}

	std::string_view GetFilename(const std::string_view filepath)
	{
		const std::vector<std::string> parts = SplitString(filepath, "/\\");
		if (parts.empty()) return "";
		return parts.back();
	}

	std::string GetExtension(const std::string& filename)
	{
		std::vector<std::string> parts = SplitString(filename, '.');
		if (parts.size() > 1) return parts.back();
		return "";
	}

	std::string RemoveExtension(const std::string& filename)
	{
		return filename.substr(0, filename.find_last_of('.'));
	}

	std::vector<std::string> SplitStringAndKeepDelims(std::string str)
	{
		const static std::regex re(R"((^\W|^\w+)|(\w+)|[:()])", std::regex_constants::optimize);

		std::regex_iterator<std::string::iterator> rit(str.begin(), str.end(), re);
		std::regex_iterator<std::string::iterator> rend;
		std::vector<std::string> result;

		while (rit != rend) { result.emplace_back(rit->str()); ++rit; }
		return result;
	}

	std::vector<std::string> SplitString(const std::string_view str, const std::string_view& delimiters)
	{
		size_t first = 0;
		std::vector<std::string> result;

		while (first <= str.size())
		{
			const auto second = str.find_first_of(delimiters, first);

			if (first != second) result.emplace_back(str.substr(first, second - first));
			if (second == std::string_view::npos) break;
			first = second + 1;
		}

		return result;
	}

	std::vector<std::string> SplitString(const std::string_view str, const char delimiter)
	{
		return SplitString(str, std::string(1, delimiter));
	}

	std::string SplitAtUpperCase(std::string_view str, std::string_view delimiter, bool ifLowerCaseOnTheRight)
	{
		std::string result(str);
		for (int i = static_cast<int>(str.size()) - 1; i > 0; --i)
		{
			const auto rightIsLower = [&] { return i < static_cast<int>(str.size()) && std::islower(result[i + 1]); };
			if (std::isupper(result[i]) && (!ifLowerCaseOnTheRight || rightIsLower()))
				result.insert(i, delimiter);
		}

		return result;
	}

	std::string ToLower(const std::string_view str)
	{
		std::string result;
		for (const auto& character : str) result += std::tolower(character);
		return result;
	}

	std::string ToUpper(const std::string_view str)
	{
		std::string result;
		for (const auto& character : str) result += std::toupper(character);
		return result;
	}

	std::string BytesToString(uint64_t bytes)
	{
		constexpr uint64_t GB = 1024 * 1024 * 1024;
		constexpr uint64_t MB = 1024 * 1024;
		constexpr uint64_t KB = 1024;

		char buffer[33]{}; // 32 characters + 1 for null terminator

		if (bytes >= GB)
			snprintf(buffer, sizeof(buffer), "%.2f GB", static_cast<float>(bytes) / static_cast<float>(GB));
		else if (bytes >= MB)
			snprintf(buffer, sizeof(buffer), "%.2f MB", static_cast<float>(bytes) / static_cast<float>(MB));
		else if (bytes >= KB)
			snprintf(buffer, sizeof(buffer), "%.2f KB", static_cast<float>(bytes) / static_cast<float>(KB));
		else
			snprintf(buffer, sizeof(buffer), "%.2f bytes", static_cast<float>(bytes));

		return std::string(buffer);
	}

	std::string DurationToString(std::chrono::duration<double> duration)
	{
		const auto durations = BreakDownDuration<std::chrono::minutes, std::chrono::seconds, std::chrono::milliseconds>(duration);

		std::stringstream durSs;
		durSs << std::setfill('0') << std::setw(1) << std::get<0>(durations).count() << ':'
			<< std::setfill('0') << std::setw(2) << std::get<1>(durations).count() << '.'
			<< std::setfill('0') << std::setw(3) << std::get<2>(durations).count();
		return durSs.str();
	}

	std::string TemplateToParenthesis(std::string_view name)
	{
		std::string str(name);
		const auto open = str.find('<');
		const auto close = str.find('>');

		if (open == std::string::npos || close == std::string::npos) return str;
		if (open > 1 && str[open - 1] != ' ') str.insert(open, " ");
		if (open + 2 < str.size()) str[open + 2] = std::toupper(str[open + 2]);

		std::replace(str.begin(), str.end(), '<', '(');
		std::replace(str.begin(), str.end(), '>', ')');
		return str;
	}

	std::string CreateUserFriendlyTypeName(std::string_view name)
	{
		return TemplateToParenthesis(SplitAtUpperCase(RemoveNamespace(name)));
	}

	int SkipBOM(std::istream& in)
	{
		char test[4] = { 0 };
		in.seekg(0, std::ios::beg);
		in.read(test, 3);
		if (strcmp(test, "\xEF\xBB\xBF") == 0)
		{
			in.seekg(3, std::ios::beg);
			return 3;
		}
		in.seekg(0, std::ios::beg);
		return 0;
	}

	std::string ReadFileAndSkipBOM(const std::filesystem::path& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			auto fileSize = in.tellg();
			const int skippedChars = SkipBOM(in);

			fileSize -= skippedChars - 1;
			result.resize(fileSize);
			in.read(result.data() + 1, fileSize);
			result[0] = '\t';
		}
		in.close();
		return result;
	}

}
