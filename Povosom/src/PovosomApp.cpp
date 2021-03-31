#include <Povox.h>
#include <Povox/Core/EntryPoint.h>

#include "Povosom2D.h"


namespace Povox {

	class Povosom : public Application
	{
	public:
		Povosom()
		{
			PushLayer(new Povosom2D());
		}

		~Povosom()
		{
		}
	};

	Application* CreateApplication()
	{
		return new Povosom();
	}

}
