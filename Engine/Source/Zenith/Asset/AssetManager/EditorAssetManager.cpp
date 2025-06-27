#include "znpch.hpp"
#include "EditorAssetManager.hpp"

#include "Zenith/Asset/AssetExtensions.hpp"
#include "Zenith/Asset/AssetManager.hpp"
#include "Zenith/Core/Application.hpp"
#include "Zenith/Events/EditorEvent.hpp"
#include "Zenith/Core/Timer.hpp"
#include "Zenith/Debug/Profiler.hpp"
#include "Zenith/Project/Project.hpp"
#include "Zenith/Utilities/StringUtils.hpp"

#include <nlohmann/json.hpp>

namespace Zenith {

	static AssetMetadata s_NullMetadata;

	EditorAssetManager::EditorAssetManager()
	{
#if ASYNC_ASSETS
		m_AssetThread = Ref<EditorAssetSystem>::Create();
#endif

		AssetImporter::Init();

		LoadAssetRegistry();
		ReloadAssets();
	}

	EditorAssetManager::~EditorAssetManager()
	{
		// TODO: shutdown explicitly?
		Shutdown();
	}

	void EditorAssetManager::Shutdown()
	{
#if ASYNC_ASSETS
		m_AssetThread->StopAndWait();
#endif
		WriteRegistryToFile();
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle assetHandle)
	{
		if (!IsAssetHandleValid(assetHandle))
			return AssetType::None;

		if (IsMemoryAsset(assetHandle))
			return GetAsset(assetHandle)->GetAssetType();

		const auto& metadata = GetMetadata(assetHandle);
		return metadata.Type;
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle assetHandle)
	{
		ZN_PROFILE_FUNC();
		ZN_SCOPE_PERF("AssetManager::GetAsset");

		Ref<Asset> asset = GetAssetIncludingInvalid(assetHandle);
		return asset && asset->IsValid() ? asset : nullptr;
	}

	AsyncAssetResult<Asset> EditorAssetManager::GetAssetAsync(AssetHandle assetHandle)
	{
#if ASYNC_ASSETS
		ZN_PROFILE_FUNC();
		ZN_SCOPE_PERF("AssetManager::GetAssetAsync");

		if (auto asset = GetMemoryAsset(assetHandle); asset)
			return { asset, true };

		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return { nullptr }; // TODO: return special error asset

		Ref<Asset> asset = nullptr;
		if (metadata.IsDataLoaded)
		{
			ZN_CORE_VERIFY(m_LoadedAssets.contains(assetHandle));
			return { m_LoadedAssets.at(assetHandle), true };
		}

		// Queue load (if not already) and return placeholder
		if (metadata.Status != AssetStatus::Loading)
		{
			auto metadataLoad = metadata;
			metadataLoad.Status = AssetStatus::Loading;
			SetMetadata(assetHandle, metadataLoad);
			m_AssetThread->QueueAssetLoad(metadata);
		}

		return AssetManager::GetPlaceholderAsset(metadata.Type);
#else
		return { GetAsset(assetHandle), true };
#endif
	}

	void EditorAssetManager::AddMemoryOnlyAsset(Ref<Asset> asset)
	{
		// Memory-only assets are not added to m_AssetRegistry (because that would require full thread synchronization for access to registry, we would like to avoid that)
		std::scoped_lock lock(m_MemoryAssetsMutex);
		m_MemoryAssets[asset->Handle] = asset;
	}

	std::unordered_set<AssetHandle> EditorAssetManager::GetAllAssetsWithType(AssetType type)
	{
		std::unordered_set<AssetHandle> result;

		// loop over memory only assets
		// This needs a lock because asset thread can create memory only assets
		{
			std::shared_lock lock(m_MemoryAssetsMutex);
			for (const auto& [handle, asset] : m_MemoryAssets)
			{
				if (asset->GetAssetType() == type)
					result.insert(handle);
			}
		}

		{
			std::shared_lock lock(m_AssetRegistryMutex);
			for (const auto& [handle, metadata] : m_AssetRegistry)
			{
				if (metadata.Type == type)
					result.insert(handle);
			}
		}
		return result;
	}

	std::unordered_map<AssetHandle, Ref<Asset>> EditorAssetManager::GetMemoryAssets()
	{
		std::shared_lock lock(m_MemoryAssetsMutex);
		return m_MemoryAssets;
	}

	AssetMetadata EditorAssetManager::GetMetadata(AssetHandle handle)
	{
		std::shared_lock lock(m_AssetRegistryMutex);

		if (m_AssetRegistry.Contains(handle))
			return m_AssetRegistry.Get(handle);

		return s_NullMetadata;
	}

	void EditorAssetManager::SetMetadata(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::unique_lock lock(m_AssetRegistryMutex);
		m_AssetRegistry.Set(handle, metadata);
	}


	AssetHandle EditorAssetManager::GetAssetHandleFromFilePath(const std::filesystem::path& filepath)
	{
		const auto relativePath = GetRelativePath(filepath);
		std::shared_lock lock(m_AssetRegistryMutex);
		for (auto& [handle, metadata] : m_AssetRegistry)
		{
			if (metadata.FilePath == relativePath)
			{
				return metadata.Handle;
			}
		}
		return AssetHandle();
	}


	AssetType EditorAssetManager::GetAssetTypeFromExtension(const std::string& extension)
	{
		std::string ext = Utils::String::ToLowerCopy(extension);
		if (s_AssetExtensionMap.find(ext) == s_AssetExtensionMap.end())
			return AssetType::None;

		return s_AssetExtensionMap.at(ext.c_str());
	}

	std::string EditorAssetManager::GetDefaultExtensionForAssetType(AssetType type)
	{
		for (const auto& [ext, assetType] : s_AssetExtensionMap)
		{
			if (assetType == type)
				return ext;
		}
		return "";
	}

	AssetType EditorAssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(const AssetMetadata& metadata)
	{
		return Project::GetActiveAssetDirectory() / metadata.FilePath;
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(AssetHandle handle)
	{
		return GetFileSystemPathString(GetMetadata(handle));
	}

	std::string EditorAssetManager::GetFileSystemPathString(const AssetMetadata& metadata)
	{
		return GetFileSystemPath(metadata).string();
	}

	std::filesystem::path EditorAssetManager::GetRelativePath(const std::filesystem::path& filepath)
	{
		std::filesystem::path relativePath = filepath.lexically_normal();
		std::string temp = filepath.string();
		if (temp.find(Project::GetActiveAssetDirectory().string()) != std::string::npos)
		{
			relativePath = std::filesystem::relative(filepath, Project::GetActiveAssetDirectory());
			if (relativePath.empty())
			{
				relativePath = filepath.lexically_normal();
			}
		}
		return relativePath;
	}

	bool EditorAssetManager::FileExists(AssetMetadata& metadata) const
	{
		return FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / metadata.FilePath);
	}

	bool EditorAssetManager::ReloadData(AssetHandle assetHandle)
	{
		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
		{
			ZN_CORE_ERROR("Trying to reload invalid asset");
			return false;
		}

		Ref<Asset> asset = GetAsset(assetHandle);

		ZN_CORE_INFO_TAG("AssetManager", "RELOADING ASSET - {}", metadata.FilePath.string());
		metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
		if (metadata.IsDataLoaded)
		{
			auto absolutePath = GetFileSystemPath(metadata);
			metadata.FileLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
			m_LoadedAssets[assetHandle] = asset;
			SetMetadata(assetHandle, metadata);
			ZN_CORE_INFO_TAG("AssetManager", "Finished reloading asset {}", metadata.FilePath.string());
			UpdateDependents(assetHandle);
			// TODO: Consider using a callback system or event dispatcher injection
			// auto event = std::make_unique<AssetReloadedEvent>(assetHandle);
			// Application::Get().GetEventBus().Dispatch(*event);
		}
		else
		{
			ZN_CORE_ERROR_TAG("AssetManager", "Failed to reload asset {}", metadata.FilePath.string());
		}

		return metadata.IsDataLoaded;
	}

	void EditorAssetManager::ReloadDataAsync(AssetHandle assetHandle)
	{
#if ASYNC_ASSETS
		// Queue load (if not already)
		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
		{
			ZN_CORE_ERROR("Trying to reload invalid asset");
			return;
		}

		if (metadata.Status != AssetStatus::Loading)
		{
			m_AssetThread->QueueAssetLoad(metadata);
			metadata.Status = AssetStatus::Loading;
			SetMetadata(assetHandle, metadata);
		}
#else
		ReloadData(assetHandle);
#endif
	}

	// Returns true if asset was reloaded
	bool EditorAssetManager::EnsureCurrent(AssetHandle assetHandle)
	{
		const auto& metadata = GetMetadata(assetHandle);
		auto absolutePath = GetFileSystemPath(metadata);

		if (!FileSystem::Exists(absolutePath))
			return false;

		uint64_t actualLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
		uint64_t recordedLastWriteTime = metadata.FileLastWriteTime;

		if (actualLastWriteTime == recordedLastWriteTime)
			return false;

		return ReloadData(assetHandle);
	}

	bool EditorAssetManager::EnsureAllLoadedCurrent()
	{
		ZN_PROFILE_FUNC();

		bool loaded = false;
		for (const auto& [handle, asset] : m_LoadedAssets)
		{
			loaded |= EnsureCurrent(handle);
		}
		return loaded;
	}

	Ref<Zenith::Asset> EditorAssetManager::GetMemoryAsset(AssetHandle handle)
	{
		std::shared_lock lock(m_MemoryAssetsMutex);
		if (auto it = m_MemoryAssets.find(handle); it != m_MemoryAssets.end())
			return it->second;

		return nullptr;
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return m_LoadedAssets.contains(handle);
	}

	bool EditorAssetManager::IsAssetValid(AssetHandle handle)
	{
		ZN_PROFILE_FUNC();
		ZN_SCOPE_PERF("AssetManager::IsAssetValid");

		auto asset = GetAssetIncludingInvalid(handle);
		return asset && asset->IsValid();
	}

	bool EditorAssetManager::IsAssetMissing(AssetHandle handle)
	{
		ZN_PROFILE_FUNC();
		ZN_SCOPE_PERF("AssetManager::IsAssetMissing");

		if(GetMemoryAsset(handle)) 
			return false;

		auto metadata = GetMetadata(handle);
		return !FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / metadata.FilePath);
	}

	bool EditorAssetManager::IsMemoryAsset(AssetHandle handle)
	{
		std::scoped_lock lock(m_MemoryAssetsMutex);
		return m_MemoryAssets.contains(handle);
	}

	bool EditorAssetManager::IsPhysicalAsset(AssetHandle handle)
	{
		return !IsMemoryAsset(handle);
	}

	void EditorAssetManager::RemoveAsset(AssetHandle handle)
	{
		{
			std::scoped_lock lock(m_MemoryAssetsMutex);
			if (m_MemoryAssets.contains(handle))
				m_MemoryAssets.erase(handle);
		}

		if (m_LoadedAssets.contains(handle))
			m_LoadedAssets.erase(handle);

		{
			std::scoped_lock lock(m_AssetRegistryMutex);
			if (m_AssetRegistry.Contains(handle))
				m_AssetRegistry.Remove(handle);
		}
	}

	// handle is dependent on dependency
	void EditorAssetManager::RegisterDependency(AssetHandle dependency, AssetHandle handle)
	{
		std::scoped_lock lock(m_AssetDependenciesMutex);

		if (dependency != 0)
		{
			ZN_CORE_ASSERT(handle != 0);
			m_AssetDependents[dependency].insert(handle);
			m_AssetDependencies[handle].insert(dependency);
			return;
		}

		// otherwise just make sure there is an entry in m_AssetDependencies for handle
		if (m_AssetDependencies.find(handle) == m_AssetDependencies.end())
		{
			m_AssetDependencies[handle] = {};
		}
	}

	// handle is no longer dependent on dependency
	void EditorAssetManager::DeregisterDependency(AssetHandle dependency, AssetHandle handle)
	{
		std::scoped_lock lock(m_AssetDependenciesMutex);
		if (dependency != 0)
		{
			m_AssetDependents[dependency].erase(handle);
			m_AssetDependencies[handle].erase(dependency);
		}
	}

	void EditorAssetManager::DeregisterDependencies(AssetHandle handle)
	{
		std::scoped_lock lock(m_AssetDependenciesMutex);
		if (auto it = m_AssetDependencies.find(handle); it != m_AssetDependencies.end())
		{
			for (AssetHandle dependency : it->second)
			{
				m_AssetDependents[dependency].erase(handle);
			}
			m_AssetDependencies.erase(it);
		}
	}

	std::unordered_set<Zenith::AssetHandle> EditorAssetManager::GetDependencies(AssetHandle handle)
	{
		bool registered = false;
		std::unordered_set<Zenith::AssetHandle> result;
		{
			std::shared_lock lock(m_AssetDependenciesMutex);
			if (auto it = m_AssetDependencies.find(handle); it != m_AssetDependencies.end())
			{
				registered = true;
				result = it->second;
			}
		}

		if (!registered)
		{
			if (auto metadata = GetMetadata(handle); metadata.IsValid())
			{
				AssetImporter::RegisterDependencies(metadata);
				{
					std::shared_lock lock(m_AssetDependenciesMutex);
					if (auto it = m_AssetDependencies.find(handle); it != m_AssetDependencies.end())
					{
						result = it->second;
					}
				}
			}
			else
			{
				m_AssetDependencies[handle] = {};
			}
			registered = true;

		}
		ZN_CORE_ASSERT(registered || (GetMetadata(handle).Handle == 0), "asset dependencies are not registered!");

		return result;
	}

	void EditorAssetManager::UpdateDependents(AssetHandle handle)
	{
		std::unordered_set<AssetHandle> dependents;
		{
			std::shared_lock lock(m_AssetDependenciesMutex);
			if (auto it = m_AssetDependents.find(handle); it != m_AssetDependents.end())
				dependents = it->second;
		}
		for (AssetHandle dependent : dependents)
		{
			if(IsAssetLoaded(dependent)) {
				Ref<Asset> asset = GetAsset(dependent);
				if (asset)
				{
					asset->OnDependencyUpdated(handle);
				}
			}
		}
	}

	void EditorAssetManager::SyncWithAssetThread()
	{
		std::vector<EditorAssetLoadResponse> freshAssets;

		m_AssetThread->RetrieveReadyAssets(freshAssets);
		for (auto& alr : freshAssets)
		{
			ZN_CORE_ASSERT(alr.Asset->Handle == alr.Metadata.Handle, "AssetHandle mismatch in AssetLoadResponse");
			m_LoadedAssets[alr.Metadata.Handle] = alr.Asset;
			alr.Metadata.Status = AssetStatus::Ready;
			alr.Metadata.IsDataLoaded = true;
			SetMetadata(alr.Metadata.Handle, alr.Metadata);
		}

		m_AssetThread->UpdateLoadedAssetList(m_LoadedAssets);

		// Update dependencies after syncing everything
		for (const auto& alr : freshAssets)
		{
			UpdateDependents(alr.Metadata.Handle);
		}
	}

	AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& filepath)
	{
		std::filesystem::path path = GetRelativePath(filepath);

		if (auto handle = GetAssetHandleFromFilePath(path); handle)
		{
			return handle;
		}

		AssetType type = GetAssetTypeFromPath(path);
		if (type == AssetType::None)
		{
			return AssetHandle::null();
		}

		AssetMetadata metadata;
		metadata.Handle = AssetHandle();
		metadata.FilePath = path;
		metadata.Type = type;

		auto absolutePath = GetFileSystemPath(metadata);
		metadata.FileLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
		SetMetadata(metadata.Handle, metadata);

		return metadata.Handle;
	}

	Ref<Asset> EditorAssetManager::GetAssetIncludingInvalid(AssetHandle assetHandle)
	{
		if (auto asset = GetMemoryAsset(assetHandle); asset)
			return asset;

		Ref<Asset> asset = nullptr;
		auto metadata = GetMetadata(assetHandle);
		if (metadata.IsValid())
		{
			if (metadata.IsDataLoaded)
			{
				asset = m_LoadedAssets[assetHandle];
			}
			else
			{
				if (Application::IsMainThread())
				{
					// If we're main thread, we can just try loading the asset as normal
					ZN_CORE_INFO_TAG("AssetManager", "LOADING ASSET - {}", metadata.FilePath.string());
					if (AssetImporter::TryLoadData(metadata, asset))
					{
						auto metadataLoaded = metadata;
						metadataLoaded.IsDataLoaded = true;
						auto absolutePath = GetFileSystemPath(metadata);
						metadataLoaded.FileLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
						m_LoadedAssets[assetHandle] = asset;
						SetMetadata(assetHandle, metadataLoaded);
						ZN_CORE_INFO_TAG("AssetManager", "Finished loading asset {}", metadata.FilePath.string());
					}
					else
					{
						ZN_CORE_ERROR_TAG("AssetManager", "Failed to load asset {}", metadata.FilePath.string());
					}
				}
				else
				{
					// Not main thread -> ask AssetThread for the asset
					// If the asset needs to be loaded, this will load the asset.
					// The load will happen on this thread (which is probably asset thread, but occasionally might be audio thread).
					// The asset will get synced into main thread at next asset sync point.
					asset = m_AssetThread->GetAsset(metadata);
				}
			}
		}
		return asset;
	}

	void EditorAssetManager::LoadAssetRegistry()
	{
		ZN_CORE_INFO("[AssetManager] Loading Asset Registry");

		const auto& assetRegistryPath = Project::GetAssetRegistryPath();
		if (!FileSystem::Exists(assetRegistryPath))
		{
			ZN_CORE_INFO("[AssetManager] Asset Registry file does not exist, will be created on first save");
			return;
		}

		std::ifstream stream(assetRegistryPath);
		if (!stream.is_open())
		{
			ZN_CORE_ERROR("[AssetManager] Failed to open Asset Registry file: {}", assetRegistryPath.string());
			return;
		}

		// Check if file is empty
		stream.seekg(0, std::ios::end);
		std::streampos fileSize = stream.tellg();
		stream.seekg(0, std::ios::beg);

		if (fileSize == 0)
		{
			ZN_CORE_INFO("[AssetManager] Asset Registry file is empty, will be populated on asset scan");
			return;
		}

		nlohmann::json data;
		try
		{
			stream >> data;
		}
		catch (const nlohmann::json::exception& e)
		{
			ZN_CORE_ERROR("[AssetManager] Failed to parse Asset Registry JSON: {}", e.what());
			ZN_CORE_WARN("[AssetManager] Asset Registry may be corrupted, will be regenerated");
			return;
		}

		if (!data.contains("Assets"))
		{
			ZN_CORE_WARN("[AssetManager] Asset Registry missing 'Assets' node, treating as empty");
			return;
		}

		if (!data["Assets"].is_array())
		{
			ZN_CORE_ERROR("[AssetManager] Asset Registry 'Assets' is not an array, regenerating registry");
			return;
		}

		size_t loadedCount = 0;
		for (const auto& entry : data["Assets"])
		{
			if (!entry.contains("FilePath") || !entry.contains("Handle") || !entry.contains("Type"))
			{
				ZN_CORE_WARN("[AssetManager] Skipping malformed asset entry in registry");
				continue;
			}

			try
			{
				std::string filepath = entry["FilePath"].get<std::string>();

				AssetMetadata metadata;
				metadata.Handle = AssetHandle(entry["Handle"].get<uint64_t>());
				metadata.FilePath = filepath;
				metadata.Type = (AssetType)Utils::AssetTypeFromString(entry["Type"].get<std::string>());

				if (metadata.Type == AssetType::None)
				{
					ZN_CORE_WARN("[AssetManager] Unknown asset type in registry: {}", entry["Type"].get<std::string>());
					continue;
				}

				if (metadata.Type != GetAssetTypeFromPath(filepath))
				{
					ZN_CORE_WARN_TAG("AssetManager", "Mismatch between stored AssetType and extension type when reading asset registry: {}", metadata.FilePath.string());
					continue;
				}

				SetMetadata(metadata.Handle, metadata);
				loadedCount++;
			}
			catch (const std::exception& e)
			{
				ZN_CORE_WARN("[AssetManager] Failed to parse asset entry: {}", e.what());
				continue;
			}
		}

		ZN_CORE_INFO("[AssetManager] Loaded {} asset entries from registry", loadedCount);
	}

	void EditorAssetManager::ProcessDirectory(const std::filesystem::path& directoryPath)
	{
		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
				ProcessDirectory(entry.path());
			else
				ImportAsset(entry.path());
		}
	}

	void EditorAssetManager::ReloadAssets()
	{
		ProcessDirectory(Project::GetActiveAssetDirectory().string());
		WriteRegistryToFile();
	}

	void EditorAssetManager::WriteRegistryToFile()
	{
		struct AssetRegistryEntry
		{
			std::string FilePath;
			AssetType Type;
		};
		std::map<UUID, AssetRegistryEntry> sortedMap;

		for (auto& [filepath, metadata] : m_AssetRegistry)
		{
			if (!FileSystem::Exists(GetFileSystemPath(metadata)))
			{
				ZN_CORE_TRACE("[AssetManager] Skipping missing asset: {}", metadata.FilePath.string());
				continue;
			}

			std::string pathToSerialize = metadata.FilePath.string();
			// Normalize path separators for cross-platform compatibility
			std::replace(pathToSerialize.begin(), pathToSerialize.end(), '\\', '/');

			if (pathToSerialize.empty())
			{
				ZN_CORE_WARN("[AssetManager] Skipping asset with empty path, handle: {}", static_cast<uint64_t>(metadata.Handle));
				continue;
			}

			sortedMap[metadata.Handle] = { pathToSerialize, metadata.Type };
		}

		ZN_CORE_INFO("[AssetManager] Serializing asset registry with {} entries", sortedMap.size());

		nlohmann::json jsonData;
		jsonData["Assets"] = nlohmann::json::array();

		for (const auto& [handle, entry] : sortedMap)
		{
			try
			{
				nlohmann::json assetEntry;
				assetEntry["Handle"] = static_cast<uint64_t>(handle);
				assetEntry["FilePath"] = entry.FilePath;
				assetEntry["Type"] = Utils::AssetTypeToString(entry.Type);

				if (assetEntry["FilePath"].get<std::string>().empty())
				{
					ZN_CORE_WARN("[AssetManager] Skipping asset entry with empty file path");
					continue;
				}

				jsonData["Assets"].push_back(assetEntry);
			}
			catch (const std::exception& e)
			{
				ZN_CORE_ERROR("[AssetManager] Failed to serialize asset entry {}: {}", static_cast<uint64_t>(handle), e.what());
				continue;
			}
		}

		const std::string& assetRegistryPath = Project::GetAssetRegistryPath().string();

		try
		{
			std::string jsonString = jsonData.dump(2);
			auto testParse = nlohmann::json::parse(jsonString);

			std::ofstream fout(assetRegistryPath);
			if (!fout.is_open())
			{
				ZN_CORE_ERROR("[AssetManager] Failed to open asset registry file for writing: {}", assetRegistryPath);
				return;
			}

			fout << jsonString;
			fout.close();

			if (fout.fail())
			{
				ZN_CORE_ERROR("[AssetManager] Failed to write asset registry file: {}", assetRegistryPath);
				return;
			}

			ZN_CORE_INFO("[AssetManager] Asset registry saved successfully with {} entries", jsonData["Assets"].size());
		}
		catch (const std::exception& e)
		{
			ZN_CORE_ERROR("[AssetManager] Failed to write asset registry: {}", e.what());

			try
			{
				nlohmann::json fallbackData;
				fallbackData["Assets"] = nlohmann::json::array();

				std::ofstream fallbackOut(assetRegistryPath);
				if (fallbackOut.is_open())
				{
					fallbackOut << fallbackData.dump(2);
					ZN_CORE_WARN("[AssetManager] Created empty asset registry as fallback");
				}
			}
			catch (...)
			{
				ZN_CORE_ERROR("[AssetManager] Failed to create fallback asset registry");
			}
		}
	}

	void EditorAssetManager::OnAssetRenamed(AssetHandle assetHandle, const std::filesystem::path& newFilePath)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return;

		metadata.FilePath = GetRelativePath(newFilePath);
		SetMetadata(assetHandle, metadata);
		WriteRegistryToFile();
	}

	void EditorAssetManager::OnAssetDeleted(AssetHandle assetHandle)
	{
		RemoveAsset(assetHandle);
		WriteRegistryToFile();
	}

}