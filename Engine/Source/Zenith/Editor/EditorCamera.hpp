#pragma once

#include <glm/detail/type_quat.hpp>

#include "Zenith/Renderer/Camera.hpp"
#include "Zenith/Core/TimeStep.hpp"
#include "Zenith/Events/KeyEvent.hpp"
#include "Zenith/Events/MouseEvent.hpp"

#include "Zenith/Core/Ref.hpp"

namespace Zenith {

	enum class CameraMode
	{
		NONE, FLYCAM, ARCBALL
	};

	class EditorCamera : public Camera, public RefCounted
	{
	public:
		EditorCamera(const float degFov, const float width, const float height, const float nearP, const float farP);
		void Init();

		void Focus(const glm::vec3& focusPoint);
		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		bool IsActive() const { return m_IsActive; }
		void SetActive(bool active) { m_IsActive = active; }

		CameraMode GetCurrentMode() const { return m_CameraMode; }

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		const glm::vec3& GetFocalPoint() const { return m_FocalPoint; }

		inline void SetViewportBounds(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
		{
			if (m_ViewportLeft == left && m_ViewportTop == top && m_ViewportRight == right && m_ViewportBottom == bottom)
				return;

			if ((right - left) != (m_ViewportRight - m_ViewportLeft) || (bottom - top) != (m_ViewportBottom - m_ViewportTop))
			{
				float width = (float)(right - left);
				float height = (float)(bottom - top);
				if (width > 0.0f && height > 0.0f)
				{
					SetPerspectiveProjectionMatrix(m_VerticalFOV, width, height, m_NearClip, m_FarClip);
				}
			}

			m_ViewportLeft = left;
			m_ViewportTop = top;
			m_ViewportRight = right;
			m_ViewportBottom = bottom;
		}

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 GetViewProjection() const { return GetProjectionMatrix() * m_ViewMatrix; }
		glm::mat4 GetUnReversedViewProjection() const { return GetUnReversedProjectionMatrix() * m_ViewMatrix; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;

		const glm::vec3& GetPosition() const { return m_Position; }

		glm::quat GetOrientation() const;

		[[nodiscard]] float GetVerticalFOV() const { return m_VerticalFOV; }
		[[nodiscard]] float GetAspectRatio() const { return m_AspectRatio; }
		[[nodiscard]] float GetNearClip() const { return m_NearClip; }
		[[nodiscard]] float GetFarClip() const { return m_FarClip; }
		[[nodiscard]] float GetPitch() const { return m_Pitch; }
		[[nodiscard]] float GetYaw() const { return m_Yaw; }
		[[nodiscard]] float GetCameraSpeed() const;

		bool IsMouseInputEnabled() const { return m_MouseInputEnabled; }
		void SetMouseInputEnabled(bool enabled) { m_MouseInputEnabled = enabled; }

	private:
		void UpdateCameraView();

		bool OnMouseScroll(MouseScrolledEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	private:
		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position, m_Direction, m_FocalPoint;

		float m_VerticalFOV, m_AspectRatio, m_NearClip, m_FarClip;

		bool m_IsActive = false;
		bool m_MouseInputEnabled = true;
		bool m_Panning, m_Rotating;
		glm::vec2 m_InitialMousePosition {};
		glm::vec3 m_InitialFocalPoint, m_InitialRotation;

		float m_Distance;
		float m_NormalSpeed{ 0.002f };

		float m_Pitch, m_Yaw;
		float m_PitchDelta{}, m_YawDelta{};
		glm::vec3 m_PositionDelta{};
		glm::vec3 m_RightDirection{};

		CameraMode m_CameraMode{ CameraMode::ARCBALL };

		float m_MinFocusDistance{ 100.0f };

		uint32_t m_ViewportLeft = 0;
		uint32_t m_ViewportTop = 0;
		uint32_t m_ViewportRight = 1920;
		uint32_t m_ViewportBottom = 1080;

		constexpr static float MIN_SPEED{ 0.0005f }, MAX_SPEED{ 2.0f };
		friend class EditorLayer;
	};

}