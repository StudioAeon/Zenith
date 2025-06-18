#pragma once

#include <nlohmann/json.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

using json = nlohmann::json;

namespace Zenith {

	// ============================= glm::ivecN =============================

	inline void to_json(json& j, const glm::ivec2& v) { j = { v.x, v.y }; }
	inline void to_json(json& j, const glm::ivec3& v) { j = { v.x, v.y, v.z }; }
	inline void to_json(json& j, const glm::ivec4& v) { j = { v.x, v.y, v.z, v.w }; }

	inline void from_json(const json& j, glm::ivec2& v) { v = { j[0], j[1] }; }
	inline void from_json(const json& j, glm::ivec3& v) { v = { j[0], j[1], j[2] }; }
	inline void from_json(const json& j, glm::ivec4& v) { v = { j[0], j[1], j[2], j[3] }; }

	// ============================= glm::bvecN =============================

	inline void to_json(json& j, const glm::bvec2& v) { j = { v.x, v.y }; }
	inline void to_json(json& j, const glm::bvec3& v) { j = { v.x, v.y, v.z }; }
	inline void to_json(json& j, const glm::bvec4& v) { j = { v.x, v.y, v.z, v.w }; }

	inline void from_json(const json& j, glm::bvec2& v) { v = { j[0], j[1] }; }
	inline void from_json(const json& j, glm::bvec3& v) { v = { j[0], j[1], j[2] }; }
	inline void from_json(const json& j, glm::bvec4& v) { v = { j[0], j[1], j[2], j[3] }; }

	// ============================= glm::vecN =============================

	inline void to_json(json& j, const glm::vec2& v) { j = { v.x, v.y }; }
	inline void to_json(json& j, const glm::vec3& v) { j = { v.x, v.y, v.z }; }
	inline void to_json(json& j, const glm::vec4& v) { j = { v.x, v.y, v.z, v.w }; }

	inline void from_json(const json& j, glm::vec2& v) { v = { j[0], j[1] }; }
	inline void from_json(const json& j, glm::vec3& v) { v = { j[0], j[1], j[2] }; }
	inline void from_json(const json& j, glm::vec4& v) { v = { j[0], j[1], j[2], j[3] }; }

	// ============================= glm::quat =============================

	inline void to_json(json& j, const glm::quat& q) { j = { q.w, q.x, q.y, q.z }; }
	inline void from_json(const json& j, glm::quat& q) { q = { j[0], j[1], j[2], j[3] }; }
}
