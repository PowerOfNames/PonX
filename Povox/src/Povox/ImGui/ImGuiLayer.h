#pragma once
#include "Povox/Core/Layer.h"
#include "Povox/Events/ApplicationEvent.h"
#include "Povox/Events/MouseEvent.h"
#include "Povox/Events/KeyEvent.h"

namespace Povox {

	class POVOX_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& event);

		virtual void OnImGuiRender() override;

		void BlockEvents(bool state) { m_BlockEvents = state; }

		void Begin();
		void End();

		void SetDarkThemeColors();
	private:
		float m_Time = 0.0f;
		bool m_BlockEvents = true;
	};
}