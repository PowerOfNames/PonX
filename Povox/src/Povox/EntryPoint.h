#pragma once

#ifdef PX_PLATFORM_WINDOWS

extern Povox::Application* Povox::CreateApplication();

int main(int argc, char** argv)
{
	Povox::Log::Init();
	PX_CORE_ERROR("Something went wrong!");
	PX_CRITICAL("Damn");
	int a = 12;
	PX_INFO("Variable: {0}, in {1} at Line: {2}", a, __FILE__, __LINE__);

	auto app = Povox::CreateApplication();
	app->Run();

	delete app;
}

#endif


