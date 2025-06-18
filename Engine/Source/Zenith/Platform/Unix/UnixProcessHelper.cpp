#include "znpch.hpp"
#include "Zenith/Utilities/ProcessHelper.hpp"
#include "Zenith/Utilities/StringUtils.hpp"
#include "Zenith/Utilities/FileSystem.hpp"

#include <spawn.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

namespace Zenith {

	static std::unordered_map<UUID, pid_t> s_LinuxProcessStorage;

	UUID ProcessHelper::CreateProcess(const ProcessInfo& inProcessInfo)
	{
		std::filesystem::path workingDirectory = inProcessInfo.WorkingDirectory.empty() ? inProcessInfo.FilePath.parent_path() : inProcessInfo.WorkingDirectory;

		auto params = Utils::SplitString(inProcessInfo.CommandLine, ' ');

		std::string bin = inProcessInfo.FilePath.string();
		std::vector<char*> exec;
		exec.push_back(const_cast<char*>(bin.c_str()));

		for(auto& s : params)
		{
			exec.push_back(const_cast<char*>(s.c_str()));
		}
		exec.push_back(nullptr);

		pid_t pid;
		posix_spawnattr_t attr;

		int result = posix_spawnattr_init(&attr);
		if (result != 0)
		{
			ZN_CORE_ERROR("posix_spawnattr_init failed: {}", strerror(result));
			return UUID();
		}

		const char* ldEnv = std::getenv("LD_LIBRARY_PATH");
		std::string ld_lib_path = std::format("LD_LIBRARY_PATH={}", ldEnv ? ldEnv : "");

		char* env[] = { const_cast<char*>(ld_lib_path.c_str()), nullptr };

		std::filesystem::path old = std::filesystem::current_path();

		if (!std::filesystem::exists(workingDirectory))
		{
			ZN_CORE_ERROR("Working directory '{}' does not exist.", workingDirectory.string());
			posix_spawnattr_destroy(&attr);
			return UUID();
		}

		std::filesystem::current_path(workingDirectory);

		result = posix_spawnp(&pid, exec[0], nullptr, &attr, exec.data(), env);

		// Restore old path no matter what
		std::filesystem::current_path(old);

		posix_spawnattr_destroy(&attr);

		if (result != 0)
		{
			ZN_CORE_ERROR("posix_spawnp failed: {}", strerror(result));
			return UUID();
		}

		UUID processID;

		s_LinuxProcessStorage[processID] = pid;

		return processID;
	}

	void ProcessHelper::DestroyProcess(UUID inHandle, uint32_t inExitCode)
	{
		int result = kill(s_LinuxProcessStorage[inHandle], SIGTERM);
		if(result) ZN_CORE_VERIFY(false);
	}

}