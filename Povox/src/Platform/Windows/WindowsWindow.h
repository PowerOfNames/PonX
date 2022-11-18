#pragma once

#include "Povox/Core/Window.h"
#include "Povox/Renderer/GraphicsContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"

#include <GLFW/glfw3.h>

namespace Povox {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		// Gets called every frame
		void OnUpdate() override;

		void OnResize(uint32_t width, uint32_t height) override;

		inline unsigned int GetWidth() const override { return m_Data.Width; }
		inline unsigned int GetHeight() const override { return m_Data.Width; }

		inline Ref<VulkanSwapchain> GetSwapchain() override { return m_Swapchain; } //investigate, if this creates any leaks

		// Window attibutes
		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		inline virtual void* GetNativeWindow() const override { return m_Window; }


	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window = nullptr;
		Ref<GraphicsContext> m_Context = nullptr; // move into renderer?
		Ref<VulkanSwapchain> m_Swapchain = nullptr;

		struct WindowData
		{
			std::string Title = "No Title";
			int Width = 0, Height = 0;
			bool VSync = true;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
		bool m_GLFW_NO_API = false;
	};
}
