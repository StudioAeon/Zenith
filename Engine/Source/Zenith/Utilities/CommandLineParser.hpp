#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>

namespace Zenith {
	/**
	 * Command line parser that supports:
	 * - Unix-style short options: -f, -f value, -f=value
	 * - Unix-style long options: --flag, --flag value, --flag=value
	 * - MS-style options (Windows): /flag, /flag:value
	 * - Raw positional arguments
	 *
	 * Note: Currently assumes named options consume the next argument if no
	 * explicit value is provided (no support for parameterless flags mixed
	 * with positional args in arbitrary order).
	 */
	class CommandLineParser {
	public:
		struct ParsedOption {
			std::string name;
			std::optional<std::string> value;
			bool is_ms_style = false;
			bool has_explicit_value = false;
		};

#ifdef ZN_PLATFORM_WINDOWS
		explicit CommandLineParser(int argc, char** argv, bool allow_ms_style = true);
#else
		explicit CommandLineParser(int argc, char** argv, bool allow_ms_style = false);
#endif

		[[nodiscard]] const std::vector<std::string>& GetRawArgs() const noexcept { return m_raw_args; }

		[[nodiscard]] const std::vector<ParsedOption>& GetOptions() const noexcept { return m_options; }

		[[nodiscard]] bool HasOption(std::string_view name) const noexcept;

		[[nodiscard]] std::string_view GetOptionValue(std::string_view name) const noexcept;

		[[nodiscard]] std::string GetOptionValue(std::string_view name, const std::string& default_value) const;

		[[nodiscard]] std::vector<std::string_view> GetOptionValues(std::string_view name) const;

		[[nodiscard]] bool HasErrors() const noexcept { return !m_errors.empty(); }
		[[nodiscard]] const std::vector<std::string>& GetErrors() const noexcept { return m_errors; }

	private:
		std::vector<std::string> m_raw_args;
		std::vector<ParsedOption> m_options;
		std::vector<std::string> m_errors;

		mutable std::unordered_map<std::string, std::vector<size_t>> m_option_index_cache;
		mutable bool m_cache_valid = false;

		void BuildIndexCache() const;
		void ParseArguments(int argc, char** argv, bool allow_ms_style);
		std::optional<std::pair<std::string, std::optional<std::string>>> ParseOption(
			std::string_view arg, bool allow_ms_style) const;
	};
}