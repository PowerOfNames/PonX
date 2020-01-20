#pragma once

#include "Povox/Core/Core.h"
#include "Povox/Events/Event.h"

namespace Povox {

	class POVOX_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string GetDebugName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;

	};
}
