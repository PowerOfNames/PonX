#pragma once

#ifdef PX_PLATFORM_WINDOWS

extern Povox::Application* Povox::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Povox::CreateApplication();
	app->Run();

	delete app;
}

#endif


