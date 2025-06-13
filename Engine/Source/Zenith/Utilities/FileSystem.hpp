#pragma once

#include "Zenith/Core/Buffer.hpp"

#ifdef CreateDirectory
#undef CreateDirectory
#undef DeleteFile
#undef MoveFile
#undef CopyFile
#undef SetEnvironmentVariable
#undef GetEnvironmentVariable
#endif

#include <functional>
#include <filesystem>

namespace Zenith {

	enum class FileStatus
	{
		Success = 0, Invalid, Locked, OtherError
	};

	class FileSystem
	{
	public:
		static std::filesystem::path GetWorkingDirectory();
		static void SetWorkingDirectory(std::filesystem::path path);
		static bool CreateDirectory(const std::filesystem::path& directory);
		static bool CreateDirectory(const std::string& directory);
		static bool Exists(const std::filesystem::path& filepath);
		static bool Exists(const std::string& filepath);
		static bool DeleteFile(const std::filesystem::path& filepath);
		static bool MoveFile(const std::filesystem::path& filepath, const std::filesystem::path& dest);
		static bool CopyFile(const std::filesystem::path& filepath, const std::filesystem::path& dest);
		static bool IsDirectory(const std::filesystem::path& filepath);

		static FileStatus TryOpenFile(const std::filesystem::path& filepath);
		static FileStatus TryOpenFileAndWait(const std::filesystem::path& filepath, uint64_t waitms = 100);

		static bool IsNewer(const std::filesystem::path& fileA, const std::filesystem::path& fileB);
		static bool Move(const std::filesystem::path& oldFilepath, const std::filesystem::path& newFilepath);
		static bool Copy(const std::filesystem::path& oldFilepath, const std::filesystem::path& newFilepath);
		static bool Rename(const std::filesystem::path& oldFilepath, const std::filesystem::path& newFilepath);
		static bool RenameFilename(const std::filesystem::path& oldFilepath, const std::string& newName);

		static bool ShowFileInExplorer(const std::filesystem::path& path);
		static bool OpenDirectoryInExplorer(const std::filesystem::path& path);
		static bool OpenExternally(const std::filesystem::path& path);

		static bool WriteBytes(const std::filesystem::path& filepath, const Buffer& buffer);
		static Buffer ReadBytes(const std::filesystem::path& filepath);

		static std::filesystem::path GetUniqueFileName(const std::filesystem::path& filepath);
		static uint64_t GetLastWriteTime(const std::filesystem::path& filepath);

		struct FileDialogFilterItem
		{
			const char* Name;
			const char* Spec;
		};

		static std::filesystem::path OpenFileDialog(const std::initializer_list<FileDialogFilterItem> filters = {});
		static std::filesystem::path OpenFolderDialog(const char* initialFolder = "");
		static std::filesystem::path SaveFileDialog(const std::initializer_list<FileDialogFilterItem> filters = {});

		static std::filesystem::path GetPersistentStoragePath();

		static bool HasConfigValue(const std::string& key);
		static bool SetConfigValue(const std::string& key, const std::string& value);
		static std::string GetConfigValue(const std::string& key);
	};
}
