#pragma once

#include "Elysium.h"
#include "Elysium/Scene/Entity.h"

class ViewerPanel;
class ShaderEditorPanel;

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
	bool OnKeyPressed(Elysium::KeyPressedEvent& _event);
	bool OnKeyReleased(Elysium::KeyReleasedEvent& _event);
private:
	Elysium::Unique<ViewerPanel> m_viewerPanel;
	Elysium::Unique<ShaderEditorPanel> m_editorPanel;

	Elysium::Shared<Elysium::Shader> m_defaultShader;

	short m_modifierKeyFlag;
};
