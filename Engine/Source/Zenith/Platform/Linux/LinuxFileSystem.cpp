#include "znpch.hpp"
#include "Zenith/Utilities/FileSystem.hpp"

#include "Zenith/Core/Application.hpp"

#include <sys/inotify.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <nlohmann/json.hpp>

namespace Zenith {

	static std::filesystem::path s_PersistentStoragePath;

	FileStatus FileSystem::TryOpenFile(const std::filesystem::path& filepath)
	{
		int res = access(filepath.c_str(), F_OK);

		if (!res) return FileStatus::Success;

		switch (errno)
		{
			default: return FileStatus::OtherError;

			case ENOENT: [[fallthrough]];
			case ENOTDIR: return FileStatus::Invalid;

			case EPERM: [[fallthrough]];
			case EACCES: return FileStatus::Locked;
		}
	}

	bool FileSystem::WriteBytes(const std::filesystem::path& filepath, const Buffer& buffer)
	{
		std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);

		if (!stream)
		{
			stream.close();
			return false;
		}

		stream.write((char*)buffer.Data, buffer.Size);
		stream.close();

		return true;
	}

	Buffer FileSystem::ReadBytes(const std::filesystem::path& filepath)
	{
		Buffer buffer;

		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);
		ZN_CORE_ASSERT(stream);

		auto end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		auto size = end - stream.tellg();
		ZN_CORE_ASSERT(size != 0);

		buffer.Allocate((uint32_t)size);
		stream.read((char*)buffer.Data, buffer.Size);
		stream.close();

		return buffer;
	}

	std::filesystem::path FileSystem::GetPersistentStoragePath()
	{
		if (!s_PersistentStoragePath.empty())
			return s_PersistentStoragePath;

		const char* configHome = std::getenv("XDG_CONFIG_HOME");
		if (configHome)
			s_PersistentStoragePath = configHome;
		else
		{
			const char* home = std::getenv("HOME");
			ZN_CORE_ASSERT(home, "HOME environment variable not set");
			s_PersistentStoragePath = std::string(home) + "/.config";
		}

		s_PersistentStoragePath /= "Zenith-Editor";

		if (!std::filesystem::exists(s_PersistentStoragePath))
			std::filesystem::create_directories(s_PersistentStoragePath);

		return s_PersistentStoragePath;
	}

	bool FileSystem::HasConfigValue(const std::string& key)
	{
		auto configPath = GetPersistentStoragePath() / "zenith.conf";
		if (!std::filesystem::exists(configPath))
			return false;

		try {
			std::ifstream file(configPath);
			if (!file.is_open())
				return false;

			nlohmann::json config;
			file >> config;

			return config.contains(key);
		}
		catch (const std::exception& e) {
			ZN_CORE_ERROR("Failed to read config: {}", e.what());
			return false;
		}
	}

	bool FileSystem::SetConfigValue(const std::string& key, const std::string& value)
	{
		auto configPath = GetPersistentStoragePath() / "zenith.conf";

		nlohmann::json config;

		if (std::filesystem::exists(configPath))
		{
			try {
				std::ifstream file(configPath);
				if (file.is_open())
					file >> config;
			}
			catch (const std::exception& e) {
				ZN_CORE_WARN("Failed to read existing config, creating new: {}", e.what());
				config = nlohmann::json::object();
			}
		}

		config[key] = value;

		try {
			std::ofstream file(configPath);
			if (!file.is_open())
				return false;

			file << config.dump(4);
			return true;
		}
		catch (const std::exception& e) {
			ZN_CORE_ERROR("Failed to write config: {}", e.what());
			return false;
		}
	}

	std::string FileSystem::GetConfigValue(const std::string& key)
	{
		auto configPath = GetPersistentStoragePath() / "zenith.conf";
		if (!std::filesystem::exists(configPath))
			return {};

		try {
			std::ifstream file(configPath);
			if (!file.is_open())
				return {};

			nlohmann::json config;
			file >> config;

			if (config.contains(key) && config[key].is_string()) {
				return config[key].get<std::string>();
			}
		}
		catch (const std::exception& e) {
			ZN_CORE_ERROR("Failed to read config value '{}': {}", key, e.what());
		}

		return {};
	}

}
