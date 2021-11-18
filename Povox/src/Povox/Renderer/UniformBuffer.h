#pragma once

#include "Povox/Core/Core.h"

namespace Povox {

	class Uniformbuffer
	{
	public:
		virtual ~Uniformbuffer() = default;

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		static Ref<Uniformbuffer> Create(uint32_t size, uint32_t binding);
	};
}
