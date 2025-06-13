#include "znpch.hpp"
#include "Zenith/Utilities/FileSystem.hpp"

#include "Zenith/Core/Application.hpp"

#include <Windows.h>
#include <Shlobj.h>
#include <comdef.h>

#include <filesystem>
#include <thread>

constexpr const char* ZENITH_REGISTRY_PATH = "Software\\ZenithEngine";

namespace Zenith {

	static std::filesystem::path s_PersistentStoragePath;

	FileStatus FileSystem::TryOpenFile(const std::filesystem::path& filepath)
	{
		HANDLE fileHandle = CreateFileW(
			filepath.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);

		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			DWORD errorCode = GetLastError();
			if (errorCode == ERROR_FILE_NOT_FOUND || errorCode == ERROR_PATH_NOT_FOUND)
				return FileStatus::Invalid;
			if (errorCode == ERROR_SHARING_VIOLATION)
				return FileStatus::Locked;

			return FileStatus::OtherError;
		}

		CloseHandle(fileHandle);
		return FileStatus::Success;
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

		PWSTR roamingFilePath;
		HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &roamingFilePath);
		ZN_CORE_VERIFY(result == S_OK);

		s_PersistentStoragePath = roamingFilePath;
		s_PersistentStoragePath /= "Zenith-Editor";

		// Free the allocated string
		CoTaskMemFree(roamingFilePath);

		if (!std::filesystem::exists(s_PersistentStoragePath))
			std::filesystem::create_directories(s_PersistentStoragePath);

		return s_PersistentStoragePath;
	}

	bool FileSystem::HasConfigValue(const std::string& key)
	{
		HKEY hKey;
		LSTATUS status = RegOpenKeyExA(HKEY_CURRENT_USER, ZENITH_REGISTRY_PATH, 0, KEY_READ, &hKey);
		if (status != ERROR_SUCCESS)
			return false;

		status = RegQueryValueExA(hKey, key.c_str(), nullptr, nullptr, nullptr, nullptr);
		RegCloseKey(hKey);
		return status == ERROR_SUCCESS;
	}

	bool FileSystem::SetConfigValue(const std::string& key, const std::string& value)
	{
		HKEY hKey;
		DWORD disp;
		LSTATUS status = RegCreateKeyExA(HKEY_CURRENT_USER, ZENITH_REGISTRY_PATH, 0, nullptr,
																		 REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disp);
		if (status != ERROR_SUCCESS)
			return false;

		status = RegSetValueExA(hKey, key.c_str(), 0, REG_SZ,
			reinterpret_cast<const BYTE*>(value.c_str()),
			static_cast<DWORD>(value.size() + 1));

		RegCloseKey(hKey);
		return status == ERROR_SUCCESS;
	}

	std::string FileSystem::GetConfigValue(const std::string& key)
	{
		HKEY hKey;
		LSTATUS status = RegOpenKeyExA(HKEY_CURRENT_USER, ZENITH_REGISTRY_PATH, 0, KEY_READ, &hKey);
		if (status != ERROR_SUCCESS)
			return {};

		char buffer[512];
		DWORD bufferSize = sizeof(buffer);
		DWORD type = 0;

		status = RegQueryValueExA(hKey, key.c_str(), nullptr, &type,
			reinterpret_cast<LPBYTE>(buffer), &bufferSize);

		RegCloseKey(hKey);

		if (status == ERROR_SUCCESS && type == REG_SZ)
			return std::string(buffer, bufferSize - 1);

		return {};
	}

}
