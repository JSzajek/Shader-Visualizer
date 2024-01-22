#include "svis_pch.h"

#include "SVisLayer.h"

#include "Panels/ViewerPanel.h"
#include "Panels/ShaderEditorPanel.h"

#include "Elysium/Factories/ShaderFactory.h"

#include <imgui_internal.h>

SVisLayer::SVisLayer()
	: m_viewerPanel(nullptr),
	m_editorPanel(nullptr),
	m_modifierKeyFlag(0)
{
}

SVisLayer::~SVisLayer()
{
}

void SVisLayer::OnAttach()
{
	m_editorPanel = Elysium::CreateUnique<ShaderEditorPanel>();
	m_viewerPanel = Elysium::CreateUnique<ViewerPanel>();
}

void SVisLayer::OnDetach()
{
	m_editorPanel = nullptr;
	m_viewerPanel = nullptr;
}

void SVisLayer::OnUpdate()
{
	m_viewerPanel->OnUpdate();
	m_editorPanel->OnUpdate();

	// TODO:: Move into appropriate pass?
	Elysium::CoreUniformBuffers::UploadDirtyData();

	Elysium::Shared<Elysium::Shader> currentShader = nullptr;
	m_editorPanel->GetCurrentShader(currentShader);
	m_viewerPanel->DrawTo(currentShader);
}

void SVisLayer::OnImGuiRender()
{
	static bool opt_fullscreen = true;
	static bool dockspaceOpen = true;
	static bool viewportOpen = true;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoCloseButton;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}
	else
	{
		dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
	{
		window_flags |= ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse;
	}

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();

	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	m_viewerPanel->OnImGuiRender();
	m_editorPanel->OnImGuiRender();

	ImGui::End();

#if 0
	static bool showDemo = true;
	ImGui::ShowDemoWindow(&showDemo);
#endif
}

void SVisLayer::OnEvent(Elysium::Event& _event)
{
	Elysium::EventDispatcher dispatcher(_event);
	dispatcher.Dispatch<Elysium::KeyPressedEvent>(BIND_EVENT_FN(SVisLayer::OnKeyPressed));
	dispatcher.Dispatch<Elysium::KeyReleasedEvent>(BIND_EVENT_FN(SVisLayer::OnKeyReleased));

	if (m_viewerPanel->IsHovered())
		m_viewerPanel->OnEvent(_event);
}

bool SVisLayer::OnKeyPressed(Elysium::KeyPressedEvent& _event)
{
	switch (_event.GetKeyCode())
	{
		case Elysium::Key::LeftShift:
			BIT_SET(m_modifierKeyFlag, ModifierKeys::LeftShift);
			break;
		case Elysium::Key::RightShift:
			BIT_SET(m_modifierKeyFlag, ModifierKeys::RightShift);
			break;
		case Elysium::Key::LeftControl:
			BIT_SET(m_modifierKeyFlag, ModifierKeys::LeftCtrl);
			break;
		case Elysium::Key::RightControl:
			BIT_SET(m_modifierKeyFlag, ModifierKeys::RightCtrl);
			break;
		case Elysium::Key::O:
		{
			if (BIT_CHECK(m_modifierKeyFlag, ModifierKeys::LeftCtrl) || BIT_CHECK(m_modifierKeyFlag, ModifierKeys::RightCtrl))
			{
				m_editorPanel->OpenFile();
				return true;
			}
			break;
		}
		case Elysium::Key::N:
		{
			if (BIT_CHECK(m_modifierKeyFlag, ModifierKeys::LeftCtrl) || BIT_CHECK(m_modifierKeyFlag, ModifierKeys::RightCtrl))
			{
				m_editorPanel->NewFile();
				return true;
			}
			break;
		}
		case Elysium::Key::S:
		{
			if (BIT_CHECK(m_modifierKeyFlag, ModifierKeys::LeftCtrl) || BIT_CHECK(m_modifierKeyFlag, ModifierKeys::RightCtrl))
			{
				m_editorPanel->SaveFile();
				return true;
			}
			break;
		}
		case Elysium::Key::F5:
		{
			m_editorPanel->Compile();
			return true;
		}
	}
	return false;
}

bool SVisLayer::OnKeyReleased(Elysium::KeyReleasedEvent& _event)
{
	switch (_event.GetKeyCode())
	{
		case Elysium::Key::LeftShift:
			BIT_CLEAR(m_modifierKeyFlag, ModifierKeys::LeftShift);
			break;
		case Elysium::Key::RightShift:
			BIT_CLEAR(m_modifierKeyFlag, ModifierKeys::RightShift);
			break;
		case Elysium::Key::LeftControl:
			BIT_CLEAR(m_modifierKeyFlag, ModifierKeys::LeftCtrl);
			break;
		case Elysium::Key::RightControl:
			BIT_CLEAR(m_modifierKeyFlag, ModifierKeys::RightCtrl);
			break;
	}
	return false;
}
