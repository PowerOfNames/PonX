#include "pxpch.h"
#include "Povox/Scene/Entity.h"


namespace Povox {

	Entity::Entity(entt::entity handle, Scene* scene)
		:m_EntityHandle(handle), m_Scene(scene)
	{
	}
}