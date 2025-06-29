#include "znpch.hpp"
#include "EditorCamera.hpp"

#include "Zenith/Core/Input.hpp"

#include <SDL3/SDL.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Zenith/Core/Application.hpp"

namespace Zenith {

	EditorCamera::EditorCamera(const float degFov, const float width, const float height, const float nearP, const float farP)
		: Camera(glm::perspectiveFov(glm::radians(degFov), width, height, farP, nearP), glm::perspectiveFov(glm::radians(degFov), width, height, nearP, farP)), m_FocalPoint(0.0f), m_VerticalFOV(glm::radians(degFov)), m_NearClip(nearP), m_FarClip(farP)
	{
		Init();
	}

	void EditorCamera::Init()
	{
		constexpr glm::vec3 position = { -5, 5, 5 };
		m_Distance = glm::distance(position, m_FocalPoint);

		m_Yaw = 3.0f * glm::pi<float>() / 4.0f;
		m_Pitch = glm::pi<float>() / 4.0f;

		m_Position = CalculatePosition();
		const glm::quat orientation = GetOrientation();
		m_Direction = glm::eulerAngles(orientation) * (180.0f / glm::pi<float>());
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
	}

	void EditorCamera::OnUpdate(const Timestep ts)
	{
		if (!m_IsActive)
		{
			if (m_CursorCaptured)
			{
				RestoreCursor();
			}

			if (!m_MouseInputEnabled)
				m_MouseInputEnabled = true;
			return;
		}

		// Get relative mouse motion for camera controls
		glm::vec2 delta = GetMouseDelta();

		if (Input::IsMouseButtonDown(MouseButton::Right) && !Input::IsKeyDown(KeyCode::LeftAlt))
		{
			if (!m_CursorCaptured)
			{
				CaptureCursor();
			}

			m_CameraMode = CameraMode::FLYCAM;

			const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
			const float speed = GetCameraSpeed();

			glm::vec3 horizontalForward = GetHorizontalForwardDirection();
			glm::vec3 horizontalRight = GetHorizontalRightDirection();

			if (Input::IsKeyDown(KeyCode::Q))
				m_PositionDelta -= ts.GetMilliseconds() * speed * glm::vec3{ 0.f, yawSign, 0.f };
			if (Input::IsKeyDown(KeyCode::E))
				m_PositionDelta += ts.GetMilliseconds() * speed * glm::vec3{ 0.f, yawSign, 0.f };
			if (Input::IsKeyDown(KeyCode::S))
				m_PositionDelta -= ts.GetMilliseconds() * speed * horizontalForward;
			if (Input::IsKeyDown(KeyCode::W))
				m_PositionDelta += ts.GetMilliseconds() * speed * horizontalForward;
			if (Input::IsKeyDown(KeyCode::A))
				m_PositionDelta -= ts.GetMilliseconds() * speed * horizontalRight;
			if (Input::IsKeyDown(KeyCode::D))
				m_PositionDelta += ts.GetMilliseconds() * speed * horizontalRight;

			constexpr float maxRate{ 0.12f };
			m_YawDelta += glm::clamp(yawSign * delta.x * RotationSpeed(), -maxRate, maxRate);
			m_PitchDelta += glm::clamp(delta.y * RotationSpeed(), -maxRate, maxRate);

			m_RightDirection = glm::cross(m_Direction, glm::vec3{ 0.f, yawSign, 0.f });

			m_Direction = glm::rotate(glm::normalize(glm::cross(glm::angleAxis(-m_PitchDelta, m_RightDirection),
				glm::angleAxis(-m_YawDelta, glm::vec3{ 0.f, yawSign, 0.f }))), m_Direction);

			const float distance = glm::distance(m_FocalPoint, m_Position);
			m_FocalPoint = m_Position + GetForwardDirection() * distance;
			m_Distance = distance;
		}
		else if (Input::IsKeyDown(KeyCode::LeftAlt))
		{
			m_CameraMode = CameraMode::ARCBALL;

			if (Input::IsMouseButtonDown(MouseButton::Middle))
			{
				if (!m_CursorCaptured)
				{
					CaptureCursor();
				}
				MousePan(delta);
			}
			else if (Input::IsMouseButtonDown(MouseButton::Left))
			{
				if (!m_CursorCaptured)
				{
					CaptureCursor();
				}
				MouseRotate(delta);
			}
			else if (Input::IsMouseButtonDown(MouseButton::Right))
			{
				if (!m_CursorCaptured)
				{
					CaptureCursor();
				}
				MouseZoom((delta.x + delta.y) * 2.0f);
			}
			else
			{
				if (m_CursorCaptured)
				{
					RestoreCursor();
				}
			}
		}
		else
		{
			if (m_CursorCaptured)
			{
				RestoreCursor();
			}
		}

		m_Position += m_PositionDelta;
		m_Yaw += m_YawDelta;
		m_Pitch += m_PitchDelta;

		if (m_CameraMode == CameraMode::ARCBALL)
			m_Position = CalculatePosition();

		UpdateCameraView();
	}

	glm::vec2 EditorCamera::GetMouseDelta()
	{
		if (m_CursorCaptured)
		{
			// Use consistent scaling with movement speed
			float mouseSpeed = m_MouseSensitivity * 0.002f; // Base sensitivity

			// Apply same modifiers as WASD movement
			if (Input::IsKeyDown(KeyCode::LeftControl))
				mouseSpeed /= 2.0f; // Slower precision mode
			if (Input::IsKeyDown(KeyCode::LeftShift))
				mouseSpeed *= 2.0f; // Faster mode

			return Input::GetRelativeMouseMotion() * mouseSpeed;
		}
		else
		{
			const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
			const glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.002f;
			m_InitialMousePosition = mouse;
			return delta;
		}
	}

	void EditorCamera::CaptureCursor()
	{
		// Store current cursor position before capturing
		auto [x, y] = Input::GetMousePosition();
		m_CursorPositionBeforeCapture = { x, y };

		// Lock cursor for camera controls
		Input::SetCursorMode(CursorMode::Locked);
		m_MouseInputEnabled = false;
		m_CursorCaptured = true;

		// Clear any accumulated relative motion
		Input::GetRelativeMouseMotion();
	}

	void EditorCamera::RestoreCursor()
	{
		// Restore cursor to original position
		Input::SetCursorMode(CursorMode::Normal);
		Input::SetMousePosition(m_CursorPositionBeforeCapture.x, m_CursorPositionBeforeCapture.y);

		m_MouseInputEnabled = true;
		m_CursorCaptured = false;

		// Update initial mouse position for next time
		m_InitialMousePosition = m_CursorPositionBeforeCapture;
	}

	float EditorCamera::GetCameraSpeed() const
	{
		float speed = m_NormalSpeed;
		if (Input::IsKeyDown(KeyCode::LeftControl))
			speed /= glm::min(2 - glm::log(m_NormalSpeed), 2.5f);
		if (Input::IsKeyDown(KeyCode::LeftShift))
			speed *= glm::min(2 - glm::log(m_NormalSpeed), 2.5f);

		return glm::clamp(speed, MIN_SPEED, MAX_SPEED);
	}

	void EditorCamera::UpdateCameraView()
	{
		const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

		// Normalize angles to prevent flipping
		NormalizeAngles();

		const glm::vec3 lookAt = m_Position + GetForwardDirection();
		m_Direction = glm::normalize(lookAt - m_Position);
		m_Distance = glm::distance(m_Position, m_FocalPoint);

		m_Distance = glm::clamp(m_Distance, MIN_DISTANCE, MAX_DISTANCE);

		m_ViewMatrix = glm::lookAt(m_Position, lookAt, glm::vec3{ 0.f, yawSign, 0.f });

		// Adaptive damping - reduce damping near pitch extremes to prevent shake
		const float halfPi = glm::pi<float>() * 0.5f;
		const float totalPitch = m_Pitch + m_PitchDelta;
		const float pitchRatio = glm::abs(totalPitch) / halfPi; // 0.0 to 1.0
		const float dampingFactor = glm::mix(0.6f, 0.9f, pitchRatio); // Less damping near poles

		//adaptive damping for smooth camera
		m_YawDelta *= dampingFactor;
		m_PitchDelta *= dampingFactor;
		m_PositionDelta *= 0.8f;
	}

	void EditorCamera::Focus(const glm::vec3& focusPoint)
	{
		m_FocalPoint = focusPoint;
		m_CameraMode = CameraMode::FLYCAM;
		if (m_Distance > m_MinFocusDistance)
		{
			m_Distance -= m_Distance - m_MinFocusDistance;
			m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
		}
		m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
		UpdateCameraView();
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		const float x = glm::min(float(m_ViewportRight - m_ViewportLeft) / 1000.0f, 2.4f); // max = 2.4f
		const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		const float y = glm::min(float(m_ViewportBottom - m_ViewportTop) / 1000.0f, 2.4f); // max = 2.4f
		const float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 1.0f; // Just a base multiplier, real scaling happens in GetMouseDelta
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = glm::max(distance, 0.0f);
		float speed = distance * distance;
		speed = glm::min(speed, 50.0f); // max speed = 50
		return speed;
	}

	void EditorCamera::OnEvent(Event& event)
	{
		if (event.GetEventType() == EventType::MouseScrolled)
		{
			MouseScrolledEvent& scrollEvent = static_cast<MouseScrolledEvent&>(event);
			OnMouseScroll(scrollEvent);
			event.Handled = true;
		}
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		if (Input::IsMouseButtonDown(MouseButton::Right))
		{
			m_NormalSpeed += e.GetYOffset() * 0.3f * m_NormalSpeed;
			m_NormalSpeed = std::clamp(m_NormalSpeed, MIN_SPEED, MAX_SPEED);
		}
		else
		{
			MouseZoom(e.GetYOffset() * 0.1f);
			UpdateCameraView();
		}

		return true;
	}

	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		float modifier = 1.0f;
		if (Input::IsKeyDown(KeyCode::LeftControl))
			modifier = 0.5f;
		if (Input::IsKeyDown(KeyCode::LeftShift))
			modifier = 2.0f;

		float panSpeed = m_Distance * 1.0f * modifier;

		m_FocalPoint -= GetRightDirection() * delta.x * panSpeed;
		m_FocalPoint += GetUpDirection() * delta.y * panSpeed;
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta)
	{
		const float yawSign = GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
		m_YawDelta += yawSign * delta.x * RotationSpeed();
		m_PitchDelta += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		float zoomRatio = 1.0f + (-delta * m_ZoomSensitivity * 0.5f);

		if (Input::IsKeyDown(KeyCode::LeftControl))
		{
			float ratioChange = zoomRatio - 1.0f;
			zoomRatio = 1.0f + (ratioChange * 0.4f);
		}
		if (Input::IsKeyDown(KeyCode::LeftShift))
		{
			float ratioChange = zoomRatio - 1.0f;
			zoomRatio = 1.0f + (ratioChange * 2.5f);
		}

		float oldDistance = m_Distance;
		float newDistance = m_Distance * zoomRatio;

		newDistance = glm::clamp(newDistance, MIN_DISTANCE, MAX_DISTANCE);
		m_Distance = newDistance;

		const glm::vec3 forwardDir = GetForwardDirection();
		m_Position = m_FocalPoint - forwardDir * m_Distance;
	}

	glm::vec3 EditorCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.f, 0.f, 0.f));
	}

	glm::vec3 EditorCamera::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance + m_PositionDelta;
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch - m_PitchDelta, -m_Yaw - m_YawDelta, 0.0f));
	}

	void EditorCamera::NormalizeAngles()
	{
		// Normalize yaw to [-π, π] range
		while (m_Yaw > glm::pi<float>())
			m_Yaw -= 2.0f * glm::pi<float>();
		while (m_Yaw < -glm::pi<float>())
			m_Yaw += 2.0f * glm::pi<float>();

		// Improved pitch clamping with delta handling
		const float halfPi = glm::pi<float>() * 0.5f;
		const float pitchLimit = halfPi - 0.02f; // Slightly larger buffer
		const float totalPitch = m_Pitch + m_PitchDelta;

		if (totalPitch > pitchLimit)
		{
			m_Pitch = pitchLimit;
			m_PitchDelta = 0.0f; // Stop delta accumulation
		}
		else if (totalPitch < -pitchLimit)
		{
			m_Pitch = -pitchLimit;
			m_PitchDelta = 0.0f; // Stop delta accumulation
		}
	}

	glm::vec3 EditorCamera::GetHorizontalForwardDirection() const
	{
		// Get current yaw rotation only (ignore pitch)
		glm::quat yawOnlyRotation = glm::quat(glm::vec3(0.0f, -m_Yaw - m_YawDelta, 0.0f));
		return glm::rotate(yawOnlyRotation, glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::GetHorizontalRightDirection() const
	{
		// Get current yaw rotation only (ignore pitch)
		glm::quat yawOnlyRotation = glm::quat(glm::vec3(0.0f, -m_Yaw - m_YawDelta, 0.0f));
		return glm::rotate(yawOnlyRotation, glm::vec3(1.0f, 0.0f, 0.0f));
	}

}