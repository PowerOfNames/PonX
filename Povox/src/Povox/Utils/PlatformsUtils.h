#pragma once

#include <string>

namespace Povox {

	class FileDialog
	{
	public:
		// These return empty strings if canceled
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};

}