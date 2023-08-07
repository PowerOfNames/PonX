#pragma once
#include "Povox/Core/Core.h"
#include "Povox/Core/UUID.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>


namespace Povox {

	// Maybe temp
	struct SciParticle
	{
		//Base -> Position in 3D and Radius
		glm::vec4 SpatialXYZR = glm::vec4(1.0f);

		//Properties -> Just color for now
		glm::vec3 Color = glm::vec3(1.0f);

		//Optional
		uint64_t UID = Povox::UUID();
	};
	
	// TODO: create Particle from layout. PArticle is just an array of byte data. 
	//       Layout is description of the byte array -> contains the offsets to the set parameters in bytes
	struct SciParticleLayout
	{
		SciParticle BaseParicle{};



	};

	


	struct SciParticleSetProperties
	{
		uint64_t ParticleCount = 0;
	};

	class SciParticleSet
	{
	public:
		SciParticleSet(const std::string& debugName = "ParticleDataSet_Debug");
		~SciParticleSet();

		bool LoadSet(const std::string& path);

		void SetLayout(const SciParticleLayout& layout) { m_Layout = layout; }
		SciParticleLayout GetLayout() { return m_Layout; }
		const SciParticleLayout& GetLayout() const { return m_Layout; }

		/**
		 * Adds a particle of layout to the set. Layout must match the layout of this set.
		 * Returns the uuid of the new particle.
		 */
		uint64_t AddParticle(const SciParticleLayout& particle);

		/**
		 * Tries to remove the particle of uuid form the set.
		 */
		void RemoveParticle(long uuid);

		inline const SciParticleSetProperties& GetProperties() const { return m_Properties; }

		static Povox::Ref<SciParticleSet> CreateCustomSet(uint64_t particleCount, const SciParticleLayout& layout, long rngSeed, const std::string& debugName = "CustomParticleSet_Debug");

	private:
		SciParticleSetProperties m_Properties{};
		SciParticleLayout m_Layout{};

		//char m_Data[];

		std::string m_DebugName = "ParticleDataSet_Debug";
	};
}

