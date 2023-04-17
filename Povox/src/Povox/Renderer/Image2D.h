#pragma once
#include "Utilities.h"


namespace Povox {

	enum class ImageFormat
	{
		None = 0,

		//Color
		RGBA8,
		RGB8,
		RG8,

		RED_INTEGER,
		RED_FLOAT,

		//Depth
		DEPTH24STENCIL8,

		//Defaults
		Depth = DEPTH24STENCIL8
	};

	namespace Utils {

		static bool IsDepthFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::DEPTH24STENCIL8: return true;
			}
			return false;
		}
	}

	enum class ImageUsage
	{
		COLOR_ATTACHMENT,
		DEPTH_ATTACHMENT,
		INPUT_ATTACHMENT,

		COPY_SRC,
		COPY_DST,

		STORAGE,

		SAMPLED
	};

	enum class ImageTiling
	{
		OPTIMAL,
		LINEAR
	};

	struct ImageUsages
	{
		ImageUsages() = default;
		ImageUsages(std::initializer_list<ImageUsage> usages)
			:Usages(usages) {}

		std::vector<ImageUsage> Usages;

		bool ContainsUsage(ImageUsage usage)
		{
			return std::find(Usages.begin(), Usages.end(), usage) != Usages.end();
		}
	};

	struct ImageSpecification
	{
		uint32_t Width = 0, Height = 0, ChannelCount = 4;
		ImageFormat Format = ImageFormat::None;
		MemoryUtils::MemoryUsage Memory = MemoryUtils::MemoryUsage::UNDEFINED;
		ImageUsages Usages;
		ImageTiling Tiling = ImageTiling::LINEAR; //check whether this is supported or not upon startup end set it then globally
		uint32_t MipLevels = 1;

		bool DedicatedSampler = false;
	};


	class Image2D
	{
	public:
		virtual ~Image2D() = default;

		virtual void Destroy() = 0;
		virtual const ImageSpecification& GetSpecification() const = 0;
		
		virtual void SetData(void* data) = 0;

		virtual int ReadPixel(int posX, int posY) = 0;

		virtual void* GetDescriptorSet() = 0;

		static Ref<Image2D> Create(const ImageSpecification& spec);
		static Ref<Image2D> Create(uint32_t width, uint32_t height, uint32_t channels = 4);
	};
}
