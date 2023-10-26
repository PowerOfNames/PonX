#pragma once
#include "Povox.h"



#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>


namespace Povox {

	struct SciParticleSetSpecification
	{
		const uint64_t MaxParticleCount = 10000;
		BufferLayout ParticleLayout;

		bool RandomGeneration = false;
		std::string DebugName = "SciParticleSet";
	};

	class SciParticleSet
	{
	public:
		SciParticleSet(const SciParticleSetSpecification& specs);
		~SciParticleSet();

		void OnUpdate(uint32_t deltaTime);

		//bool LoadSet(const std::string& path);

		inline Povox::Ref<Povox::StorageBuffer> GetDataBuffer() { return m_Data; }
		inline const Povox::Ref<Povox::StorageBuffer>& GetDataBuffer() const { return m_Data; }

		inline BufferLayout GetLayout() { return m_Specification.ParticleLayout; }
		inline const BufferLayout& GetLayout() const { return m_Specification.ParticleLayout; }

		inline uint64_t GetMaxParticleCount() { return m_Specification.MaxParticleCount; }
		inline uint64_t GetParticleCount() { return m_ParticleCount; }
		inline uint32_t GetMaxSize() { return m_Specification.MaxParticleCount * m_Specification.ParticleLayout.GetStride(); }
		inline uint32_t GetSize() { return m_Size; }

		
		inline const SciParticleSetSpecification& GetSpecifications() const { return m_Specification; }

	private:
		SciParticleSetSpecification m_Specification{};

		uint64_t m_ParticleCount = 0;
		uint32_t m_Size = 0;


		Povox::Ref<Povox::StorageBuffer> m_Data = nullptr;

	};
}

