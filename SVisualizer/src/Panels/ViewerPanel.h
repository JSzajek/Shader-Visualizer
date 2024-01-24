#pragma once

#include "Elysium.h"

#include "Elysium/Scene/2DComponents.h"

struct ShaderPackage;

class ViewerPanel
{
public:
	ViewerPanel(ShaderPackage* package);
public:
	void OnUpdate();
	void DrawTo(const Elysium::Shared<Elysium::Shader>& shader);

	inline bool IsFocused() const { return m_focused; }
	inline bool IsHovered() const { return m_hovered; }
public:
	void OnImGuiRender();
	void OnEvent(Elysium::Event& _event);
private:
	bool OnMouseScroll(Elysium::MouseScrolledEvent& _event);

	void UpdateCameraProjection();
	void UpdateCameraView();
	void FocusCamera();

	void SnapShot();
private:
	ShaderPackage* m_package;

	Elysium::Math::iVec2 m_size;
	Elysium::Math::iVec2 m_outputSize;
	bool m_outputSizeChanged;

	Elysium::Shared<Elysium::Scene> m_scene;
	Elysium::Entity m_sprite;

	Elysium::Shared<Elysium::FrameBuffer> m_hdrfbo;
	std::array<Elysium::Shared<Elysium::FrameBuffer>, 2> m_bloomFbos;
	Elysium::Shared<Elysium::FrameBuffer> m_fbo;
	Elysium::Shared<Elysium::FrameBuffer> m_shaderfbo;

	Elysium::Shared<Elysium::Shader> m_blurShader;
	Elysium::Shared<Elysium::Shader> m_bloomShader;
	Elysium::Shared<Elysium::Shader> m_spriteShader;

	Elysium::Shared<Elysium::OrthographicCamera>  m_camera;

	float m_orthoSize;
	float m_zoomModifier;

	bool m_focused;
	bool m_hovered;

	float m_currentTime;
	float m_prevGamma;
	float m_prevExposure;

	bool m_playing;
	bool m_settingsVisible;

	// Debug
	enum class DrawPass : uint8_t
	{
		None,
		BrightPass,
		BlurPass,

		Count
	};
	const char* m_drawPassStrs[3] = { "None", "Bright Pixels", "Blurring" };
	DrawPass m_debugPass;
	Elysium::Shared<Elysium::Shader> m_debugShader;
};
