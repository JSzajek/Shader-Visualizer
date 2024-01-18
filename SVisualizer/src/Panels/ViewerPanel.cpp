#include "svis_pch.h"
#include "ViewerPanel.h"

#include <imgui.h>
#include <imgui_internal.h>

ViewerPanel::ViewerPanel(uint32_t* colorTextureId)
	: m_colorTextureID(colorTextureId),
	m_size(0), m_focused(false), m_hovered(false), m_sizeChanged(false)
{
	m_initialMousePosition = { 0 };
	m_orthoFocalPoint = { 0, 0 };

	m_orthoSize = 500.0f;
	m_panModifier = 100.0f;
	m_zoomModifier = 10.0f;
	m_panningFlag = 0x0000;
	m_isMousePressed = false;
	m_mouseFirstPressed = false;

	m_camera = Elysium::CreateShared<Elysium::OrthographicCamera>();

	UpdateCameraView();
}

void ViewerPanel::OnImGuiRender()
{
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResizeFlagsMask_;
	ImGui::SetNextWindowClass(&window_class);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (!ImGui::Begin("Viewer Panel", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
		ImGui::End();

	ImGui::Columns(1);

	const ImVec2 panelSize = ImGui::GetContentRegionAvail();
	ImGui::Image(reinterpret_cast<void*>(static_cast<uint64_t>(*m_colorTextureID)), panelSize, ImVec2(0, 1), ImVec2(1, 0));

	if ((panelSize.x > 0 && panelSize.y > 0) && (m_size.x != panelSize.x || m_size.y != panelSize.y))
	{
		m_size.x = static_cast<uint32_t>(panelSize.x);
		m_size.y = static_cast<uint32_t>(panelSize.y);

		m_sizeChanged = true;

		UpdateCameraProjection();
	}

	m_focused = ImGui::IsWindowFocused();
	m_hovered = ImGui::IsWindowHovered();

	ImGui::End();
	ImGui::PopStyleVar();
}

void ViewerPanel::OnEvent(Elysium::Event& _event)
{
	Elysium::EventDispatcher dispatcher(_event);

	dispatcher.Dispatch<Elysium::MouseMovedEvent>(BIND_EVENT_FN(ViewerPanel::OnMouseMove));
	dispatcher.Dispatch<Elysium::MouseButtonPressedEvent>(BIND_EVENT_FN(ViewerPanel::OnMouseButtonPressed));
	dispatcher.Dispatch<Elysium::MouseButtonReleasedEvent>(BIND_EVENT_FN(ViewerPanel::OnMouseButtonReleased));
	dispatcher.Dispatch<Elysium::MouseScrolledEvent>(BIND_EVENT_FN(ViewerPanel::OnMouseScroll));

	dispatcher.Dispatch<Elysium::KeyPressedEvent>(BIND_EVENT_FN(ViewerPanel::OnKeyPressed));
	dispatcher.Dispatch<Elysium::KeyReleasedEvent>(BIND_EVENT_FN(ViewerPanel::OnKeyReleased));
}

void ViewerPanel::FocusOnRect(const Elysium::Math::Vec2& center, const Elysium::Math::Vec2& rectSize)
{
	const float maxScale = std::max(rectSize.x, rectSize.y) + 15 /* Buffer Constant */;

	m_orthoFocalPoint = center;
	m_orthoSize = maxScale;

	UpdateCameraView();
	UpdateCameraProjection();
}

bool ViewerPanel::OnMouseMove(Elysium::MouseMovedEvent& _event)
{
	if (m_focused && m_isMousePressed)
	{
		const Elysium::Math::Vec2 mouse = Elysium::Math::Vec2(_event.GetX(), _event.GetY());
		if (m_mouseFirstPressed)
		{
			m_initialMousePosition = mouse;
			m_mouseFirstPressed = false;
		}

		const Elysium::Math::Vec2 delta = mouse - m_initialMousePosition;
		m_initialMousePosition = mouse;

		const Elysium::Math::Vec2 norm = delta * m_panModifier * static_cast<float>(Elysium::Time::DeltaTime());

		m_orthoFocalPoint += Elysium::Math::Vec2::Left * norm.x;
		m_orthoFocalPoint += Elysium::Math::Vec2::Down * norm.y;

		UpdateCameraView();
	}
	return false;
}

bool ViewerPanel::OnMouseScroll(Elysium::MouseScrolledEvent& _event)
{
	const float delta = _event.GetYOffset();
	m_orthoSize -= delta * m_zoomModifier;

	if (m_orthoSize < 10.0f)
		m_orthoSize = 10.0f;

	UpdateCameraProjection();

	return false;
}

bool ViewerPanel::OnMouseButtonPressed(Elysium::MouseButtonPressedEvent& _event)
{
	m_isMousePressed = true;
	m_mouseFirstPressed = true;

	// Steal focus if hovered and click is detected
	if (m_hovered)
		ImGui::SetWindowFocus("Viewer Panel");

	return false;
}

bool ViewerPanel::OnMouseButtonReleased(Elysium::MouseButtonReleasedEvent& _event)
{
	m_isMousePressed = false;

	return false;
}

bool ViewerPanel::OnKeyPressed(Elysium::KeyPressedEvent& _event)
{
	switch (_event.GetKeyCode())
	{
	case Elysium::Key::LeftShift:
		BIT_SET(m_panningFlag, LEFTSHIFT);
		break;
	case Elysium::Key::LeftControl:
		BIT_SET(m_panningFlag, LEFTCTRL);
		break;
	}
	return false;
}

bool ViewerPanel::OnKeyReleased(Elysium::KeyReleasedEvent& _event)
{
	switch (_event.GetKeyCode())
	{
	case Elysium::Key::LeftShift:
		BIT_CLEAR(m_panningFlag, LEFTSHIFT);
		break;
	case Elysium::Key::LeftControl:
		BIT_CLEAR(m_panningFlag, LEFTCTRL);
		break;
	}
	return false;
}

void ViewerPanel::UpdateCameraProjection()
{
	const float aspectRatio = m_size.x / (float)m_size.y;

	m_camera->SetProperties(aspectRatio, m_orthoSize);

	Elysium::CameraData& cameraRef = Elysium::CoreUniformBuffers::GetCameraDataRef();
	cameraRef.m_viewport = Elysium::Math::Vec4((float)m_size.x, (float)m_size.y, 0, 0);

	cameraRef.m_orthoProjectionMatrix = m_camera->GetProjection();
}

void ViewerPanel::UpdateCameraView()
{
	m_camera->SetPosition(m_orthoFocalPoint);

	Elysium::CameraData& cameraRef = Elysium::CoreUniformBuffers::GetCameraDataRef();
	cameraRef.m_orthoViewMatrix = m_camera->GetView();
}
