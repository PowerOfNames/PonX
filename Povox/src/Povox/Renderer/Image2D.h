#pragma once



namespace Povox {

	enum class ImageFormat
	{
		None = 0,

		//Color
		RGBA8,
		RED_INTEGER,

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

	enum class MemoryUsage
	{
		Unknown = 0,

		GPU_ONLY = 1,
		CPU_ONLY = 2,
		UPLOAD	 = 3,
		DOWNLOAD = 4,

		CPU_COPY = 5,
		CPU_TO_GPU = UPLOAD,
		GPU_TO_CPU = DOWNLOAD
	};

	enum class ImageUsage
	{
		COLOR_ATTACHMENT,
		DEPTH_ATTACHMENT,
		INPUT_ATTACHMENT,

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
	};

	struct ImageSpecification
	{
		uint32_t Width = 0, Height = 0;
		ImageFormat Format = ImageFormat::None;
		MemoryUsage Memory = MemoryUsage::Unknown;
		ImageUsages Usages;
		ImageTiling Tiling = ImageTiling::LINEAR; //check wheather this is supported or not upon startup end set it then globally
		uint32_t MipLevels = 1;

		bool CopySrc = false;
		bool CopyDst = false;
	};

	class Image2D
	{
	public:
		virtual ~Image2D() = 0;

		virtual void Destroy() = 0;
		virtual const ImageSpecification& GetSpecification() const = 0;

		static Ref<Image2D> Create(const ImageSpecification& spec);
	};
}
