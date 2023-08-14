#pragma once



namespace Povox {


	class ShaderResourceSystem
	{
	public:
		virtual ~ShaderResourceSystem() = default;

		virtual bool Init() = 0;
		virtual void OnUpdate() = 0;
		virtual void Shutdown() = 0;

	};


}