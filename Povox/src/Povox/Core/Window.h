#pragma once
#include "Povox/Core/Core.h"
#include "Povox/Events/Event.h"


namespace Povox {

	
	struct WindowSpecification
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;

		struct WindowState
		{
			bool IsInitialized = false;
			bool VSync = false;
		} State;

		WindowSpecification(const std::string& title = "Povox Engine", uint32_t width = 1600, uint32_t height = 900)
			: Title(title), Width(width), Height(height) 
		{
		}
	};
	class VulkanSwapchain;
	// interface representing a desktop system based window
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual bool Init() = 0;
		virtual void Close() = 0;

		virtual void OnUpdate() = 0;
		virtual void PollEvents() = 0;
		virtual void OnResize(uint32_t width, uint32_t height) = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual Ref<VulkanSwapchain> GetSwapchain() = 0;
		virtual const WindowSpecification& GetSpecification() const = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& eventcallback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Scope<Window> Create(const WindowSpecification& specs = WindowSpecification());
	};
}
