#pragma once

#ifdef PX_PLATFORM_WINDOWS

extern Povox::Application* Povox::CreateApplication();

int main(int argc, char** argv)
{
	Povox::Log::Init();

	PX_PROFILE_BEGIN_SESSION("Startup", "PovoxProfile-Startup.json");
	auto app = Povox::CreateApplication();
	PX_PROFILE_END_SESSION();

	PX_PROFILE_BEGIN_SESSION("Running", "PovoxProfile-Running.json");
	app->Run();
	PX_PROFILE_END_SESSION();

	PX_PROFILE_BEGIN_SESSION("Shutdown", "PovoxProfile-Shutdown.json");
	delete app;
	PX_PROFILE_END_SESSION();
}

#endif


