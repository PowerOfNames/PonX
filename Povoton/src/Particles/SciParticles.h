#pragma once
#include "Povox.h"



#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>


namespace Povox {

	struct SciParticleSetSpecification
	{
		const uint64_t MaxParticleCount = 1000;
		BufferLayout ParticleLayout;

		bool RandomGeneration = false;
		bool GPUSimulationActive = false;

		std::string DebugName = "SciParticleSet";
	};

	class SciParticleSet
	{
	public:
		SciParticleSet(const SciParticleSetSpecification& specs);
		~SciParticleSet();

		// if GPU simulation is active, this swaps the buffer offsets, such that the attached Compute Pipeline can procede
		void OnUpdate(uint32_t deltaTime);

		//bool LoadSet(const std::string& path);

		inline void* GetDataBuffer() { return (void*)m_Data; }
		inline const void* GetDataBuffer() const { return m_Data; }

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


		uint8_t* m_Data;
		uint32_t m_SSBOOffset;

	};
}

