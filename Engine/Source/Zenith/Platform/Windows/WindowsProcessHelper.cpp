#include "znpch.hpp"
#include "Zenith/Utilities/ProcessHelper.hpp"

#include <codecvt>

namespace Zenith {

	static std::unordered_map<UUID, PROCESS_INFORMATION> s_WindowsProcessStorage;

	UUID ProcessHelper::CreateProcess(const ProcessInfo& inProcessInfo)
	{
		std::filesystem::path workingDirectory = inProcessInfo.WorkingDirectory.empty() ? inProcessInfo.FilePath.parent_path() : inProcessInfo.WorkingDirectory;

		std::wstring commandLine = inProcessInfo.IncludeFilePathInCommands ? inProcessInfo.FilePath.wstring() : L"";

		if (!inProcessInfo.CommandLine.empty())
		{
			if (commandLine.size() > 0)
			{
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wstringConverter;
				commandLine += L" " + wstringConverter.from_bytes(inProcessInfo.CommandLine);
			}
			else
			{
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wstringConverter;
				commandLine = wstringConverter.from_bytes(inProcessInfo.CommandLine);
			}
		}

		PROCESS_INFORMATION processInformation;
		ZeroMemory(&processInformation, sizeof(PROCESS_INFORMATION));

		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
		startupInfo.cb = sizeof(STARTUPINFO);

		DWORD creationFlags = NORMAL_PRIORITY_CLASS;

		if (inProcessInfo.Detached)
			creationFlags |= DETACHED_PROCESS;

		BOOL success = ::CreateProcess(
			reinterpret_cast<LPCSTR>(inProcessInfo.FilePath.c_str()), reinterpret_cast<LPSTR>(commandLine.data()),
			NULL, NULL, FALSE, creationFlags, NULL,
			reinterpret_cast<LPCSTR>(workingDirectory.c_str()), &startupInfo, &processInformation);

		if (!success)
		{
			CloseHandle(processInformation.hThread);
			CloseHandle(processInformation.hProcess);
			return UUID::null();
		}

		UUID processID = UUID();

		if (inProcessInfo.Detached)
		{
			CloseHandle(processInformation.hThread);
			CloseHandle(processInformation.hProcess);
		}
		else
		{
			s_WindowsProcessStorage[processID] = processInformation;
		}

		return processID;
	}

	void ProcessHelper::DestroyProcess(UUID inHandle, uint32_t inExitCode)
	{
		ZN_CORE_VERIFY(s_WindowsProcessStorage.find(inHandle) != s_WindowsProcessStorage.end(), "Trying to destroy untracked process!");
		const auto& processInformation = s_WindowsProcessStorage[inHandle];
		TerminateProcess(processInformation.hProcess, inExitCode);
		CloseHandle(processInformation.hThread);
		CloseHandle(processInformation.hProcess);
		s_WindowsProcessStorage.erase(inHandle);
	}

}