#pragma once

#ifdef PX_PLATFORM_WINDOWS

extern Povox::Application* Povox::CreateApplication();

int main(int argc, char** argv)
{
	Povox::Log::Init();
	PX_CORE_INFO("Welcome to Povox - A first attempt of a Voxel based Engine by PowerOfNames!");

	auto app = Povox::CreateApplication();
	app->Run();

	delete app;
}

#endif


