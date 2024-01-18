#pragma once

#include "Elysium.h"
#include "Elysium/Scene/Entity.h"

class ViewerPanel;

class SVisLayer : public Elysium::Layer
{
private:
	enum ModifierKeys : uint8_t
	{
		LeftShift	= 1,
		RightShift	= 2,
		LeftCtrl	= 3,
		RightCtrl	= 4,
	};
public:
	SVisLayer();
	virtual ~SVisLayer() override;
public:
	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate() override;
	void OnImGuiRender() override;
	void OnEvent(Elysium::Event& _event) override;
private:
	bool OnWindowResize(Elysium::WindowResizeEvent& _event);
	bool OnKeyPressed(Elysium::KeyPressedEvent& _event);
	bool OnKeyReleased(Elysium::KeyReleasedEvent& _event);
private:
	//void Recenter();
	//void OpenGroupExport();
	//void OpenFileDialog();
	//void SaveFileDialog();
	//void OpenExampleFile();
	//void OpenFile(const std::string& filepath);
	//void SaveCurrentImage(const std::string& outputFilepath);
	//void CalculateOutputDimensions();
private:
	Elysium::Unique<ViewerPanel> m_viewerPanel;

	Elysium::Shared<Elysium::Texture2D> m_activeTexture;

	Elysium::Shared<Elysium::Scene> m_scene;

	Elysium::Shared<Elysium::FrameBuffer> m_fbo;
#if 0
	Elysium::Shared<Elysium::FrameBuffer> m_normalsFBO;
	Elysium::Shared<Elysium::Shader> m_normalsRotatorShader;
	Elysium::Shared<Elysium::Shader> m_spriteShader;

	Elysium::Shared<Elysium::Shader> m_lineShader;

	Elysium::Entity m_sprite;

	std::string m_currentFilePath;
	bool m_isExampleFile;
#endif

	uint32_t m_outputTextureId;

	//Elysium::Math::iVec2 m_outputSize;
	//Elysium::Math::iVec2 m_outputOffset;

	short m_modifierKeyFlag;
};
