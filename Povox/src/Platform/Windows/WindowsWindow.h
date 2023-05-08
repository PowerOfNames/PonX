#pragma once

#include "Povox/Core/Window.h"
#include "Povox/Renderer/GraphicsContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"

#include <GLFW/glfw3.h>

namespace Povox {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowSpecification& specs);
		virtual ~WindowsWindow() = default;

		virtual bool Init() override;
		virtual void Close() override;

		// Gets called every frame
		virtual void OnUpdate() override;
		virtual void PollEvents() override;

		virtual void OnResize(uint32_t width, uint32_t height) override;

		virtual inline uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual inline uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual inline Ref<VulkanSwapchain> GetSwapchain() override { return m_Swapchain; } //investigate, if this creates any leaks
		virtual inline const WindowSpecification& GetSpecification() const override { return m_Specification; }


		// Window attibutes
		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		virtual void SetVSync(bool enabled) override;
		virtual bool IsVSync() const override;

		virtual inline void* GetNativeWindow() const override { return m_Window; }

	private:
		GLFWwindow* m_Window = nullptr;
		WindowSpecification m_Specification{};

		Ref<GraphicsContext> m_Context = nullptr; // move into renderer?
		Ref<VulkanSwapchain> m_Swapchain = nullptr;

		struct WindowData
		{
			std::string* Title = nullptr;
			int* Width = nullptr, *Height = nullptr;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
		bool m_GLFW_NO_API = false;
	};
}
