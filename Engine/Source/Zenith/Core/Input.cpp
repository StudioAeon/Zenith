#include "znpch.hpp"
#include "Zenith/Core/Input.hpp"

#include "Window.hpp"

#include "Zenith/Core/Application.hpp"
#include "Zenith/ImGui/ImGuiCore.hpp"

namespace Zenith {

	SDL_Window* Input::s_ApplicationWindow = nullptr;

	void Input::Update()
	{
		// Detect controllers on first run
		static bool hasDetectedControllers = false;
		if (!hasDetectedControllers && SDL_WasInit(SDL_INIT_JOYSTICK))
		{
			int numJoysticks = 0;
			SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);

			if (numJoysticks > 0)
			{
				for (int i = 0; i < numJoysticks; i++)
				{
					SDL_JoystickID instanceId = joysticks[i];

					if (s_Controllers.find(instanceId) != s_Controllers.end())
					{
						continue;
					}

					if (SDL_IsGamepad(instanceId))
					{
						SDL_Gamepad* gamepad = SDL_OpenGamepad(instanceId);
						if (gamepad)
						{
							Controller& controller = s_Controllers[instanceId];
							controller.ID = instanceId;
							controller.GamepadHandle = gamepad;
							controller.Name = SDL_GetGamepadName(gamepad) ? SDL_GetGamepadName(gamepad) : "Unknown Gamepad";

							for (int j = 0; j < SDL_GAMEPAD_AXIS_COUNT; j++)
								controller.DeadZones[j] = 0.1f;
						}
					}
				}
			}

			if (joysticks)
				SDL_free(joysticks);

			hasDetectedControllers = true;
		}

		if (!SDL_WasInit(SDL_INIT_JOYSTICK))
		{
			return;
		}

		int numKeys = 0;
		const bool* keyboardState = SDL_GetKeyboardState(&numKeys);
		s_KeyboardState = keyboardState;

		float mouseXFloat, mouseYFloat;
		s_MouseState = SDL_GetMouseState(&mouseXFloat, &mouseYFloat);
		s_MouseX = static_cast<int>(mouseXFloat);
		s_MouseY = static_cast<int>(mouseYFloat);

		for (auto it = s_Controllers.begin(); it != s_Controllers.end(); )
		{
			int id = it->first;
			Controller& controller = it->second;

			bool isDisconnected = !controller.GamepadHandle ||
								 !SDL_GamepadConnected(controller.GamepadHandle);

			if (isDisconnected)
			{
				if (controller.GamepadHandle)
				{
					SDL_CloseGamepad(controller.GamepadHandle);
					controller.GamepadHandle = nullptr;
				}
				it = s_Controllers.erase(it);
			}
			else
				it++;
		}

		for (auto& [id, controller] : s_Controllers)
		{
			if (!controller.GamepadHandle)
				continue;

			for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
			{
				bool isPressed = SDL_GetGamepadButton(controller.GamepadHandle, static_cast<SDL_GamepadButton>(i));

				if (isPressed && !controller.ButtonDown[i])
					controller.ButtonStates[i].State = KeyState::Pressed;
				else if (!isPressed && controller.ButtonDown[i])
					controller.ButtonStates[i].State = KeyState::Released;

				controller.ButtonDown[i] = isPressed;
			}

			for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++)
			{
				float axisValue = SDL_GetGamepadAxis(controller.GamepadHandle, static_cast<SDL_GamepadAxis>(i)) / 32767.0f;
				controller.AxisStates[i] = abs(axisValue) > controller.DeadZones[i] ? axisValue : 0.0f;
			}
		}
	}

	void Input::ProcessEvent(const SDL_Event& event)
	{
		switch (event.type)
		{
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			ProcessKeyboardEvent(event);
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		case SDL_EVENT_MOUSE_MOTION:
			ProcessMouseEvent(event);
			break;
		case SDL_EVENT_JOYSTICK_ADDED:
		case SDL_EVENT_JOYSTICK_REMOVED:
			ProcessGamepadEvent(event);
			break;
		case SDL_EVENT_GAMEPAD_ADDED:
		case SDL_EVENT_GAMEPAD_REMOVED:
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			ProcessGamepadEvent(event);
			break;
		default:
			if (event.type >= 0x650 && event.type <= 0x6FF)
			{
				ZN_CORE_WARN("Unhandled controller event: {} (0x{:X})", event.type, event.type);
			}
			break;
		}
	}

	void Input::ProcessKeyboardEvent(const SDL_Event& event)
	{
		KeyCode keyCode = static_cast<KeyCode>(event.key.scancode);
		
		if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
		{
			UpdateKeyState(keyCode, KeyState::Pressed);
		}
		else if (event.type == SDL_EVENT_KEY_UP)
		{
			UpdateKeyState(keyCode, KeyState::Released);
		}
	}

	void Input::ProcessMouseEvent(const SDL_Event& event)
	{
		switch (event.type)
		{
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		{
			MouseButton button = static_cast<MouseButton>(event.button.button - 1);
			UpdateButtonState(button, KeyState::Pressed);
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			MouseButton button = static_cast<MouseButton>(event.button.button - 1);
			UpdateButtonState(button, KeyState::Released);
			break;
		}
		case SDL_EVENT_MOUSE_MOTION:
			s_MouseX = static_cast<int>(event.motion.x);
			s_MouseY = static_cast<int>(event.motion.y);
			break;
		}
	}

	void Input::ProcessGamepadEvent(const SDL_Event& event)
	{
		switch (event.type)
		{
		case SDL_EVENT_JOYSTICK_ADDED:
		{
			SDL_JoystickID instanceId = event.jdevice.which;
			if (SDL_IsGamepad(instanceId))
			{
				// Check if we already have this controller
				if (s_Controllers.find(instanceId) != s_Controllers.end())
				{
					break;
				}

				SDL_Gamepad* gamepad = SDL_OpenGamepad(instanceId);
				if (gamepad)
				{
					Controller& controller = s_Controllers[instanceId];
					controller.ID = instanceId;
					controller.GamepadHandle = gamepad;
					controller.Name = SDL_GetGamepadName(gamepad) ? SDL_GetGamepadName(gamepad) : "Unknown Gamepad";

					for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++)
						controller.DeadZones[i] = 0.1f;
				}
				else
				{
					ZN_CORE_ERROR("Failed to open gamepad {}: {}", instanceId, SDL_GetError());
				}
			}
			break;
		}
		case SDL_EVENT_JOYSTICK_REMOVED:
		{
			SDL_JoystickID instanceId = event.jdevice.which;
			auto it = s_Controllers.find(instanceId);
			if (it != s_Controllers.end())
			{
				if (it->second.GamepadHandle)
					SDL_CloseGamepad(it->second.GamepadHandle);
				s_Controllers.erase(it);
			}
			else
			{
				ZN_CORE_WARN("Tried to remove controller {} but it wasn't found", instanceId);
			}
			break;
		}
		case SDL_EVENT_GAMEPAD_ADDED:
		{
			SDL_JoystickID instanceId = event.gdevice.which;
			if (s_Controllers.find(instanceId) != s_Controllers.end())
			{
				break;
			}

			SDL_Gamepad* gamepad = SDL_OpenGamepad(instanceId);
			if (gamepad)
			{
				Controller& controller = s_Controllers[instanceId];
				controller.ID = instanceId;
				controller.GamepadHandle = gamepad;
				controller.Name = SDL_GetGamepadName(gamepad) ? SDL_GetGamepadName(gamepad) : "Unknown Gamepad";

				for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++)
					controller.DeadZones[i] = 0.1f;
			}
			else
			{
				ZN_CORE_ERROR("Failed to open gamepad {}: {}", instanceId, SDL_GetError());
			}
			break;
		}
		case SDL_EVENT_GAMEPAD_REMOVED:
		{
			SDL_JoystickID instanceId = event.gdevice.which;
			auto it = s_Controllers.find(instanceId);
			if (it != s_Controllers.end())
			{
				if (it->second.GamepadHandle)
					SDL_CloseGamepad(it->second.GamepadHandle);
				s_Controllers.erase(it);
			}
			else
			{
				ZN_CORE_WARN("Tried to remove controller {} but it wasn't found", instanceId);
			}
			break;
		}
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		{
			SDL_JoystickID instanceId = event.gbutton.which;
			int button = event.gbutton.button;
			if (s_Controllers.find(instanceId) != s_Controllers.end())
				UpdateControllerButtonState(instanceId, button, KeyState::Pressed);
			break;
		}
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
		{
			SDL_JoystickID instanceId = event.gbutton.which;
			int button = event.gbutton.button;
			if (s_Controllers.find(instanceId) != s_Controllers.end())
				UpdateControllerButtonState(instanceId, button, KeyState::Released);
			break;
		}
		}
	}

	bool Input::IsKeyPressed(KeyCode key)
	{
		return s_KeyData.find(key) != s_KeyData.end() && s_KeyData[key].State == KeyState::Pressed;
	}

	bool Input::IsKeyHeld(KeyCode key)
	{
		return s_KeyData.find(key) != s_KeyData.end() && s_KeyData[key].State == KeyState::Held;
	}

	bool Input::IsKeyDown(KeyCode keycode)
	{
		bool enableImGui = ImGui::GetCurrentContext() != nullptr;

		if (!enableImGui || !ImGui::GetCurrentContext())
		{
			if (!s_KeyboardState)
				return false;
			SDL_Scancode scancode = static_cast<SDL_Scancode>(keycode);
			return s_KeyboardState[scancode];
		}

		if (s_ApplicationWindow)
		{
			SDL_Window* currentFocus = SDL_GetKeyboardFocus();
			if (currentFocus == s_ApplicationWindow)
			{
				if (!s_KeyboardState)
					return false;
				SDL_Scancode scancode = static_cast<SDL_Scancode>(keycode);
				return s_KeyboardState[scancode];
			}
		}

		if (!s_KeyboardState)
			return false;
		SDL_Scancode scancode = static_cast<SDL_Scancode>(keycode);
		return s_KeyboardState[scancode];
	}

	bool Input::IsKeyReleased(KeyCode key)
	{
		return s_KeyData.find(key) != s_KeyData.end() && s_KeyData[key].State == KeyState::Released;
	}

	bool Input::IsMouseButtonPressed(MouseButton button)
	{
		return s_MouseData.find(button) != s_MouseData.end() && s_MouseData[button].State == KeyState::Pressed;
	}

	bool Input::IsMouseButtonHeld(MouseButton button)
	{
		return s_MouseData.find(button) != s_MouseData.end() && s_MouseData[button].State == KeyState::Held;
	}

	bool Input::IsMouseButtonDown(MouseButton button)
	{
		bool enableImGui = ImGui::GetCurrentContext() != nullptr;

		if (!enableImGui || !ImGui::GetCurrentContext())
		{
			int sdlButton = static_cast<int>(button) + 1;
			return (s_MouseState & SDL_BUTTON_MASK(sdlButton)) != 0;
		}

		if (s_ApplicationWindow)
		{
			SDL_Window* mouseFocus = SDL_GetMouseFocus();
			if (mouseFocus == s_ApplicationWindow)
			{
				int sdlButton = static_cast<int>(button) + 1;
				return (s_MouseState & SDL_BUTTON_MASK(sdlButton)) != 0;
			}
		}

		int sdlButton = static_cast<int>(button) + 1;
		return (s_MouseState & SDL_BUTTON_MASK(sdlButton)) != 0;
	}

	bool Input::IsMouseButtonReleased(MouseButton button)
	{
		return s_MouseData.find(button) != s_MouseData.end() && s_MouseData[button].State == KeyState::Released;
	}

	float Input::GetMouseX()
	{
		return static_cast<float>(s_MouseX);
	}

	float Input::GetMouseY()
	{
		return static_cast<float>(s_MouseY);
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		return { static_cast<float>(s_MouseX), static_cast<float>(s_MouseY) };
	}

	void Input::SetApplicationWindow(SDL_Window* window)
	{
		s_ApplicationWindow = window;
	}

	SDL_Window* Input::GetApplicationWindow()
	{
		return s_ApplicationWindow;
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		SDL_Window* sdlWindow = s_ApplicationWindow;

		if (!sdlWindow) {

			sdlWindow = SDL_GetMouseFocus();
			if (!sdlWindow) {
				sdlWindow = SDL_GetKeyboardFocus();
			}
		}

		if (!sdlWindow)
		{
			ZN_CORE_WARN("No active SDL window found for cursor mode change");
			return;
		}

		switch (mode)
		{
			case CursorMode::Normal:
				SDL_SetWindowRelativeMouseMode(sdlWindow, false);
				SDL_ShowCursor();
				s_CursorHidden = false;
				s_CursorLocked = false;
				break;
			case CursorMode::Hidden:
				SDL_SetWindowRelativeMouseMode(sdlWindow, false);
				SDL_HideCursor();
				s_CursorHidden = true;
				s_CursorLocked = false;
				break;
			case CursorMode::Locked:
				SDL_SetWindowRelativeMouseMode(sdlWindow, true);
				SDL_HideCursor();
				s_CursorHidden = true;
				s_CursorLocked = true;
				break;
		}

		if (ImGui::GetCurrentContext()) {
			IMGUI::SetInputEnabled(mode == CursorMode::Normal);
		}
	}

	CursorMode Input::GetCursorMode()
	{
		if (s_CursorLocked)
			return CursorMode::Locked;
		else if (s_CursorHidden)
			return CursorMode::Hidden;
		else
			return CursorMode::Normal;
	}

	bool Input::IsControllerPresent(int id)
	{
		return s_Controllers.find(id) != s_Controllers.end();
	}

	std::vector<int> Input::GetConnectedControllerIDs()
	{
		std::vector<int> ids;
		ids.reserve(s_Controllers.size());
		for (auto [id, controller] : s_Controllers)
			ids.emplace_back(id);

		return ids;
	}

	const Controller* Input::GetController(int id)
	{
		if (!Input::IsControllerPresent(id))
			return nullptr;

		return &s_Controllers.at(id);
	}

	std::string_view Input::GetControllerName(int id)
	{
		if (!Input::IsControllerPresent(id))
			return {};

		return s_Controllers.at(id).Name;
	}

	bool Input::IsControllerButtonPressed(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return false;

		auto& controller = s_Controllers.at(controllerID);
		return controller.ButtonStates.find(button) != controller.ButtonStates.end() && controller.ButtonStates[button].State == KeyState::Pressed;
	}

	bool Input::IsControllerButtonHeld(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return false;

		auto& controller = s_Controllers.at(controllerID);
		return controller.ButtonStates.find(button) != controller.ButtonStates.end() && controller.ButtonStates[button].State == KeyState::Held;
	}

	bool Input::IsControllerButtonDown(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return false;

		const Controller& controller = s_Controllers.at(controllerID);
		if (!controller.GamepadHandle)
			return false;

		return SDL_GetGamepadButton(controller.GamepadHandle, static_cast<SDL_GamepadButton>(button));
	}

	bool Input::IsControllerButtonReleased(int controllerID, int button)
	{
		if (!Input::IsControllerPresent(controllerID))
			return true;

		auto& controller = s_Controllers.at(controllerID);
		return controller.ButtonStates.find(button) != controller.ButtonStates.end() && controller.ButtonStates[button].State == KeyState::Released;
	}

	float Input::GetControllerAxis(int controllerID, int axis)
	{
		if (!Input::IsControllerPresent(controllerID))
			return 0.0f;

		const Controller& controller = s_Controllers.at(controllerID);
		if (controller.AxisStates.find(axis) == controller.AxisStates.end())
			return 0.0f;

		return controller.AxisStates.at(axis);
	}

	uint8_t Input::GetControllerHat(int controllerID, int hat)
	{
		if (!Input::IsControllerPresent(controllerID))
			return 0;

		const Controller& controller = s_Controllers.at(controllerID);
		if (!controller.GamepadHandle)
			return 0;

		// SDL3 gamepads don't have hats in the same way as joysticks
		// This would need to be handled differently if hat support is needed
		return 0;
	}

	float Input::GetControllerDeadzone(int controllerID, int axis)
	{
		if (!Input::IsControllerPresent(controllerID))
			return 0.0f;

		const Controller& controller = s_Controllers.at(controllerID);
		if (controller.DeadZones.find(axis) == controller.DeadZones.end())
			return 0.1f; // Default deadzone

		return controller.DeadZones.at(axis);
	}

	void Input::SetControllerDeadzone(int controllerID, int axis, float deadzone)
	{
		if (!Input::IsControllerPresent(controllerID))
			return;

		Controller& controller = s_Controllers.at(controllerID);
		controller.DeadZones[axis] = deadzone;
	}

	void Input::TransitionPressedKeys()
	{
		for (const auto& [key, keyData] : s_KeyData)
		{
			if (keyData.State == KeyState::Pressed)
				UpdateKeyState(key, KeyState::Held);
		}
	}

	void Input::TransitionPressedButtons()
	{
		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Pressed)
				UpdateButtonState(button, KeyState::Held);
		}

		for (const auto& [id, controller] : s_Controllers)
		{
			for (const auto& [button, buttonStates] : controller.ButtonStates)
			{
				if (buttonStates.State == KeyState::Pressed)
					UpdateControllerButtonState(id, button, KeyState::Held);
			}
		}
	}

	void Input::UpdateKeyState(KeyCode key, KeyState newState)
	{
		auto& keyData = s_KeyData[key];
		keyData.Key = key;
		keyData.OldState = keyData.State;
		keyData.State = newState;
	}

	void Input::UpdateButtonState(MouseButton button, KeyState newState)
	{
		auto& mouseData = s_MouseData[button];
		mouseData.Button = button;
		mouseData.OldState = mouseData.State;
		mouseData.State = newState;
	}

	void Input::UpdateControllerButtonState(int controllerID, int button, KeyState newState)
	{
		if (s_Controllers.find(controllerID) == s_Controllers.end())
			return;

		auto& controllerButtonData = s_Controllers.at(controllerID).ButtonStates[button];
		controllerButtonData.Button = button;
		controllerButtonData.OldState = controllerButtonData.State;
		controllerButtonData.State = newState;
	}

	void Input::ClearReleasedKeys()
	{
		for (const auto& [key, keyData] : s_KeyData)
		{
			if (keyData.State == KeyState::Released)
				UpdateKeyState(key, KeyState::None);
		}

		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Released)
				UpdateButtonState(button, KeyState::None);
		}

		for (const auto& [id, controller] : s_Controllers)
		{
			for (const auto& [button, buttonStates] : controller.ButtonStates)
			{
				if (buttonStates.State == KeyState::Released)
					UpdateControllerButtonState(id, button, KeyState::None);
			}
		}
	}

	void Input::Shutdown()
	{
		if (!s_Controllers.empty())
		{
			for (auto& [id, controller] : s_Controllers)
			{
				if (controller.GamepadHandle)
				{
					SDL_CloseGamepad(controller.GamepadHandle);
					controller.GamepadHandle = nullptr;
				}
			}

			s_Controllers.clear();
		}

		// Clear other input states
		s_KeyData.clear();
		s_MouseData.clear();
		s_KeyboardState = nullptr;
		s_MouseState = 0;
		s_MouseX = 0;
		s_MouseY = 0;
		s_CursorHidden = false;
		s_CursorLocked = false;
	}

}