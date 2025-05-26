#pragma once

#include "Zenith/Core/Base.hpp"

#include <functional>
#include <string>
#include <ostream>
#include <type_traits>
#include <typeindex>

namespace Zenith {

	enum class EventType : uint8_t
	{
		None = 0,
		WindowClose, WindowMinimize, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseButtonDown, MouseMoved, MouseScrolled
	};

	enum EventCategory : uint8_t
	{
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4),
	};

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

	class Event
	{
	public:
		virtual ~Event() = default;
		bool Handled = false;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;

		virtual std::string ToString() const { return GetName(); }

		ZN_FORCE_INLINE bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}
	};

	using EventCallbackFn = std::function<bool(Event&)>;

	class EventBus {
	public:
		template<typename T, typename = std::enable_if_t<std::is_base_of_v<Event, T>>>
		void Listen(const std::function<bool(T&)>& callback)
		{
			auto& listeners = m_Listeners[typeid(T)];
			listeners.emplace_back([callback](Event& e) -> bool
			{
				if (e.GetEventType() == T::GetStaticType())
				{
					return callback(static_cast<T&>(e));
				}
				return false;
			});
		}

		void Dispatch(Event& event)
		{
			auto it = m_Listeners.find(std::type_index(typeid(event)));
			if (it != m_Listeners.end()) {
				for (auto& listener : it->second) {
					if (event.Handled) break;
					event.Handled = listener(event);
				}
			}
		}

	private:
		std::unordered_map<std::type_index, std::vector<EventCallbackFn>> m_Listeners;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}

}
