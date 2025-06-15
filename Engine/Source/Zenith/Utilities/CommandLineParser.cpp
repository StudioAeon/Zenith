#include "znpch.hpp"
#include "CommandLineParser.hpp"

#include <algorithm>

namespace Zenith {
	CommandLineParser::CommandLineParser(int argc, char** argv, bool allow_ms_style) {
		ParseArguments(argc, argv, allow_ms_style);
	}

	void CommandLineParser::ParseArguments(int argc, char** argv, bool allow_ms_style) {
		m_raw_args.clear();
		m_options.clear();
		m_errors.clear();
		m_cache_valid = false;

		for (int i = 1; i < argc; ++i) {
			std::string_view arg = argv[i];

			if (arg.empty()) {
				m_raw_args.emplace_back(arg);
				continue;
			}

			auto parsed_opt = ParseOption(arg, allow_ms_style);
			if (parsed_opt) {
				ParsedOption option;
				option.name = std::move(parsed_opt->first);
				option.is_ms_style = (arg[0] == '/');

				if (parsed_opt->second) {
					// Option has explicit value (via = or :)
					option.value = std::move(*parsed_opt->second);
					option.has_explicit_value = true;
				} else {
					if (i + 1 < argc && argv[i + 1][0] != '-' &&
						(!allow_ms_style || argv[i + 1][0] != '/')) {
						option.value = argv[++i];
						option.has_explicit_value = false;
					}
				}

				m_options.emplace_back(std::move(option));
			} else {
				m_raw_args.emplace_back(arg);
			}
		}
	}

	std::optional<std::pair<std::string, std::optional<std::string>>>
	CommandLineParser::ParseOption(std::string_view arg, bool allow_ms_style) const {
		if (arg.length() < 2) return std::nullopt;

		std::string name;
		std::optional<std::string> value;

		// MS-style option: /option or /option:value
		if (allow_ms_style && arg[0] == '/') {
			auto colon_pos = arg.find(':', 1);
			if (colon_pos != std::string_view::npos) {
				name = arg.substr(1, colon_pos - 1);
				value = arg.substr(colon_pos + 1);
			} else {
				name = arg.substr(1);
			}

			if (name.empty()) {
				return std::nullopt;
			}

			return std::make_pair(std::move(name), std::move(value));
		}

		// Unix-style options
		if (arg[0] != '-') return std::nullopt;

		// Long option: --option or --option=value
		if (arg.length() >= 3 && arg[1] == '-') {
			auto eq_pos = arg.find('=', 2);
			if (eq_pos != std::string_view::npos) {
				name = arg.substr(2, eq_pos - 2);
				value = arg.substr(eq_pos + 1);
			} else {
				name = arg.substr(2);
			}

			if (name.empty()) {
				return std::nullopt;
			}

			return std::make_pair(std::move(name), std::move(value));
		}

		// Short option: -o or -o=value (non-standard but supported)
		if (arg.length() >= 2) {
			auto eq_pos = arg.find('=', 1);
			if (eq_pos != std::string_view::npos) {
				name = arg.substr(1, eq_pos - 1);
				value = arg.substr(eq_pos + 1);
			} else {
				name = arg.substr(1);
			}

			if (name.empty()) {
				return std::nullopt;
			}

			return std::make_pair(std::move(name), std::move(value));
		}

		return std::nullopt;
	}

	void CommandLineParser::BuildIndexCache() const {
		if (m_cache_valid) return;

		m_option_index_cache.clear();
		for (size_t i = 0; i < m_options.size(); ++i) {
			m_option_index_cache[m_options[i].name].push_back(i);
		}
		m_cache_valid = true;
	}

	bool CommandLineParser::HasOption(std::string_view name) const noexcept {
		BuildIndexCache();
		return m_option_index_cache.find(std::string(name)) != m_option_index_cache.end();
	}

	std::string_view CommandLineParser::GetOptionValue(std::string_view name) const noexcept {
		BuildIndexCache();

		auto it = m_option_index_cache.find(std::string(name));
		if (it != m_option_index_cache.end() && !it->second.empty()) {
			const auto& option = m_options[it->second.back()]; // Get last occurrence
			return option.value ? std::string_view(*option.value) : std::string_view{};
		}

		return {};
	}

	std::string CommandLineParser::GetOptionValue(std::string_view name, const std::string& default_value) const {
		auto value = GetOptionValue(name);
		return value.empty() ? default_value : std::string(value);
	}

	std::vector<std::string_view> CommandLineParser::GetOptionValues(std::string_view name) const {
		BuildIndexCache();

		std::vector<std::string_view> result;
		auto it = m_option_index_cache.find(std::string(name));
		if (it != m_option_index_cache.end()) {
			result.reserve(it->second.size());
			for (size_t idx : it->second) {
				const auto& option = m_options[idx];
				if (option.value) {
					result.emplace_back(*option.value);
				}
			}
		}

		return result;
	}
}