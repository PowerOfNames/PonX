#pragma once

#include "Povox/Core/Core.h"
#include "Povox/Events/Event.h"
#include "Povox/Core/Timestep.h"

namespace Povox {

	class POVOX_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep deltatime) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string GetDebugName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;

	};
}
