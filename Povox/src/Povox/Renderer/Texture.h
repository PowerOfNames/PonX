#pragma once

#include "Povox/Core/Core.h"

#include <string>

namespace Povox {

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetData(void* data) = 0;


		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		virtual ~Texture2D() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetData(void* data) = 0;


		virtual bool operator==(const Texture& other) const = 0;

		static Ref<Texture2D> Create(uint32_t width, uint32_t height, uint32_t channels = 4);
		static Ref<Texture2D> Create(const std::string& path);
	};

}
