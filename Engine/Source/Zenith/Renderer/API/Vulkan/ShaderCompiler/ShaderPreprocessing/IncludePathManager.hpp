#pragma once

#include <filesystem>
#include <vector>
#include <algorithm>

namespace Zenith {
	namespace Utils {
		class IncludePathManager
		{
		public:
			void AddSearchPath(const std::filesystem::path& path)
			{
				if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
				{
					auto canonicalPath = std::filesystem::canonical(path);
					// Avoid duplicates
					if (std::find(m_SearchPaths.begin(), m_SearchPaths.end(), canonicalPath) == m_SearchPaths.end())
					{
						m_SearchPaths.push_back(canonicalPath);
					}
				}
			}

			std::filesystem::path FindIncludeFile(const std::string& filename) const
			{
				std::filesystem::path filePath(filename);
				if (filePath.is_absolute() && std::filesystem::exists(filePath))
				{
					return std::filesystem::canonical(filePath);
				}

				if (std::filesystem::exists(filename))
				{
					return std::filesystem::canonical(filename);
				}

				for (const auto& searchPath : m_SearchPaths)
				{
					auto fullPath = searchPath / filename;
					if (std::filesystem::exists(fullPath))
					{
						return std::filesystem::canonical(fullPath);
					}
				}

				return {};
			}

			std::filesystem::path FindRelativeIncludeFile(const std::filesystem::path& requestingFile, const std::string& filename) const
			{
				auto requestingDir = std::filesystem::path(requestingFile).parent_path();
				auto relativePath = requestingDir / filename;

				if (std::filesystem::exists(relativePath))
				{
					return std::filesystem::canonical(relativePath);
				}

				return FindIncludeFile(filename);
			}

			const std::vector<std::filesystem::path>& GetSearchPaths() const
			{
				return m_SearchPaths;
			}

			void Clear()
			{
				m_SearchPaths.clear();
			}

			bool HasSearchPath(const std::filesystem::path& path) const
			{
				auto canonicalPath = std::filesystem::canonical(path);
				return std::find(m_SearchPaths.begin(), m_SearchPaths.end(), canonicalPath) != m_SearchPaths.end();
			}

		private:
			std::vector<std::filesystem::path> m_SearchPaths;
		};

	}
}