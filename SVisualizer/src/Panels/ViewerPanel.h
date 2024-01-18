#pragma once

#include "Elysium.h"

#include "Elysium/Scene/2DComponents.h"

#define LEFTSHIFT	1
#define LEFTCTRL	2

class ViewerPanel
{
public:
	ViewerPanel(uint32_t* colorTextureId);
public:
	void OnImGuiRender();
	void OnEvent(Elysium::Event& _event);

	inline const Elysium::Math::iVec2 GetSize() const { return m_size; }

	inline bool IsFocused() const { return m_focused; }
	inline bool IsHovered() const { return m_hovered; }
	inline bool HasViewSizeChange() 
	{ 
		if (m_sizeChanged) 
			m_sizeChanged = false; 
		return true; 
	}

	inline Elysium::Shared<Elysium::OrthographicCamera> GetCamera() const { return m_camera; }

	void FocusOnRect(const Elysium::Math::Vec2& center, const Elysium::Math::Vec2& rectSize);
private:
	bool OnMouseMove(Elysium::MouseMovedEvent& _event);
	bool OnMouseScroll(Elysium::MouseScrolledEvent& _event);
	bool OnMouseButtonPressed(Elysium::MouseButtonPressedEvent& _event);
	bool OnMouseButtonReleased(Elysium::MouseButtonReleasedEvent& _event);
	bool OnKeyPressed(Elysium::KeyPressedEvent& _event);
	bool OnKeyReleased(Elysium::KeyReleasedEvent& _event);

	void UpdateCameraProjection();
	void UpdateCameraView();
private:
	uint32_t* m_colorTextureID;

	Elysium::Math::iVec2 m_size;
	Elysium::Math::Vec2 m_initialMousePosition;

	bool m_focused;
	bool m_hovered;
	bool m_sizeChanged;

	Elysium::Math::Vec2 m_orthoFocalPoint;
	float m_orthoSize;
	float m_panModifier;
	float m_zoomModifier;

	short m_panningFlag;

	bool m_mouseFirstPressed;
	bool m_isMousePressed;

	Elysium::Shared<Elysium::OrthographicCamera>  m_camera;
};
