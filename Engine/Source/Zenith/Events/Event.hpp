#pragma once

#include "Zenith/Core/Base.hpp"

#include <functional>
#include <string>
#include <ostream>
#include <queue>
#include <type_traits>
#include <typeindex>

namespace Zenith {

	enum class EventType : uint8_t
	{
		None = 0,
		WindowClose, WindowMinimize, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		EditorExitPlayMode, AssetReloaded,
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
		EventCategoryEditor = BIT(5)
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

		void StopPropagation() { m_PropagationStopped = true; }
		bool IsPropagationStopped() const { return m_PropagationStopped; }
	private:
		bool m_PropagationStopped = false;
	};

	using EventCallbackFn = std::function<bool(Event&)>;

	class EventBus {
	public:
		using ListenerID = uint64_t;

	private:
		struct Listener
		{
			ListenerID ID;
			int Priority;
			EventCallbackFn Callback;
			std::function<bool(const Event&)> Filter;
		};

		std::unordered_map<std::type_index, std::vector<Listener>> m_Listeners;
		std::queue<std::unique_ptr<Event>> m_EventQueue;
		ListenerID m_NextListenerID = 1;

	public:
		template<typename T, typename = std::enable_if_t<std::is_base_of_v<Event, T>>>
		ListenerID Listen(const std::function<bool(T&)>& callback, int priority = 0, std::function<bool(const T&)> filter = [](const T&) { return true; })
		{
			auto& vec = m_Listeners[typeid(T)];
			ListenerID id = m_NextListenerID++;
			vec.push_back({
				id,
				priority,
				[callback](Event& e) -> bool {
					if (e.GetEventType() == T::GetStaticType()) {
						return callback(static_cast<T&>(e));
					}
					return false;
				},
				[filter](const Event& e) -> bool {
					return filter(static_cast<const T&>(e));
				}
			});

			std::sort(vec.begin(), vec.end(), [](const Listener& a, const Listener& b) {
				return a.Priority > b.Priority;
			});

			return id;
		}

		bool RemoveListener(ListenerID id)
		{
			for (auto& [type, vec] : m_Listeners) {
				auto it = std::remove_if(vec.begin(), vec.end(),
					[id](const Listener& l) { return l.ID == id; });
				if (it != vec.end()) {
					vec.erase(it, vec.end());
					return true;
				}
			}
			return false;
		}

		void Dispatch(Event& event)
		{
			auto it = m_Listeners.find(std::type_index(typeid(event)));
			if (it == m_Listeners.end()) {
				return;
			}

			for (const auto& listener : it->second) {
				if (event.Handled || event.IsPropagationStopped()) {
					break;
				}

				if (!listener.Filter(event)) {
					continue;
				}

				bool handled = listener.Callback(event);
				if (handled) {
					event.Handled = true;
				}
			}
		}

		void QueueEvent(std::unique_ptr<Event> event)
		{
			m_EventQueue.push(std::move(event));
		}

		void DispatchQueued()
		{
			while (!m_EventQueue.empty())
			{
				auto event = std::move(m_EventQueue.front());
				m_EventQueue.pop();
				Dispatch(*event);
			}
		}
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}

}
