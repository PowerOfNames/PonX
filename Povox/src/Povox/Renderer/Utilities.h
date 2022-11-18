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
	namespace ShaderUtils {

		enum class ShaderDataType
		{
			None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
		};

		static uint32_t ShaderDataTypeSize(ShaderDataType type)
		{
			switch (type)
			{
				case ShaderDataType::Float:		return 4;
				case ShaderDataType::Float2:	return 4 * 2;
				case ShaderDataType::Float3:	return 4 * 3;
				case ShaderDataType::Float4:	return 4 * 4;
				case ShaderDataType::Mat3:		return 4 * 3 * 3;
				case ShaderDataType::Mat4:		return 4 * 4 * 4;
				case ShaderDataType::Int:		return 4;
				case ShaderDataType::Int2:		return 4 * 2;
				case ShaderDataType::Int3:		return 4 * 3;
				case ShaderDataType::Int4:		return 4 * 4;
				case ShaderDataType::Bool:		return 1;
			}

			PX_CORE_ASSERT(false, "No such ShaderDataType defined!");
			return 0;
		}
	}
}
