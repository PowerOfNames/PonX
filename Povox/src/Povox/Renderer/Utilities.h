#pragma once
#include "Povox/Core/Log.h"

namespace Povox {
	namespace MemoryUtils {

		enum class MemoryUsage
		{
			UNDEFINED = 0,

			GPU_ONLY = 1,
			CPU_ONLY = 2,
			UPLOAD = 3,
			DOWNLOAD = 4,

			CPU_COPY = 5,
			CPU_TO_GPU = UPLOAD,
			GPU_TO_CPU = DOWNLOAD
		};
	}
}
