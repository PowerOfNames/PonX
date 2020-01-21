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
		virtual void OnImGuiRender() override;

		void Begin();
		void End();

	private:
		float m_Time = 0.0f;

	};
}