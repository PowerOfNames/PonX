#pragma once

#include "Povox/Core/Core.h"

namespace Povox {

	// At the moment, the event system is a blocking event system, which means, that upon receiving a event, it must immediately be handled.
	// Later on the line of developement, it would be better, to buffer events in some kind of event bus and handle the buffer in the 'event' phase
	// of onUpdate

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved, FramebufferResize,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4)
	};

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::##type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

	class POVOX_API Event
	{
		friend class EventDispatcher;

	public:
		virtual ~Event() = default;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		inline bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}

	public: 
		bool Handled = false;
	};

	class EventDispatcher
	{
		template<typename T>
		using EventFN = std::function<bool(T&)>;
	public:
		EventDispatcher(Event& e)
			: m_Event(e) 
		{
		}

		// Typename F will be deduced by the compiler
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			// Checks which type the current event we are trying to dispatch is, whether or not is matches T
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled = func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}
}
