#include "svis_pch.h"

#include <Elysium.h>
#include <Elysium/Core/EntryPoint.h>

#include "Layers/SVisLayer.h"

class SVisualizerApp : public Elysium::Application
{
public:
	SVisualizerApp()
		: Application("Shader Visualizer", 1024, 600)
	{
		PushLayer(new SVisLayer());
	}

	~SVisualizerApp()
	{
	}
};

Elysium::Application* Elysium::CreateApplication()
{
	return new SVisualizerApp();
}
