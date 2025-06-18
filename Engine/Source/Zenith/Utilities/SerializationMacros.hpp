#pragma once

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define ZN_SERIALIZE_PROPERTY(propName, propVal, outputJson) outputJson[#propName] = propVal;

#define ZN_SERIALIZE_PROPERTY_ASSET(propName, propVal, outputJson) \
	outputJson[#propName] = (propVal ? static_cast<uint64_t>(propVal->Handle) : 0);

#define ZN_DESERIALIZE_PROPERTY(propertyName, destination, inputJson, defaultValue)  \
if (inputJson.contains(#propertyName))                                               \
{                                                                                    \
	try                                                                             \
	{                                                                               \
		destination = inputJson.at(#propertyName).get<decltype(defaultValue)>();  \
	}                                                                               \
	catch (const std::exception& e)                                                \
	{                                                                               \
		ZN_CONSOLE_LOG_ERROR(e.what());                                           \
		destination = defaultValue;                                               \
	}                                                                               \
}                                                                                    \
else                                                                                 \
{                                                                                    \
	destination = defaultValue;                                                     \
}

#define ZN_DESERIALIZE_PROPERTY_ASSET(propName, destination, inputJson, assetClass) \
{                                                                                   \
	uint64_t assetHandle = inputJson.contains(#propName)                            \
		? inputJson.at(#propName).get<uint64_t>()                                   \
		: 0;                                                                        \
	if (AssetManager::IsAssetHandleValid(assetHandle))                              \
	{                                                                               \
		destination = AssetManager::GetAsset<assetClass>(assetHandle);             \
	}                                                                               \
	else                                                                            \
	{                                                                               \
		ZN_CORE_ERROR_TAG("AssetManager", "Tried to load invalid asset {0}.", #assetClass); \
	}                                                                               \
}
